// JavaScript side of the web implementation of godot-livekit, built on livekit-client (evaluated
// just before this file, as window.LivekitClient). The C++ classes call `GodotLiveKit.call()` with
// JSON arguments and drain each room's event queue once per frame.
(function () {
  'use strict';
  if (window.GodotLiveKit) {
    return;
  }
  const LK = window.LivekitClient;

  // Values match the native binding's enums.
  const KIND = { audio: 1, video: 2 };
  const SOURCE = { camera: 1, microphone: 2, screen_share: 3, screen_share_audio: 4 };
  const STREAM_STATE = { active: 1, paused: 2 };
  const CONNECTION_QUALITY = { poor: 0, good: 1, excellent: 2, lost: 3 };
  const DATA_KIND_LOSSY = 0;
  const DATA_KIND_RELIABLE = 1;
  const STATE_DISCONNECTED = 0;
  const STATE_CONNECTED = 1;
  const STATE_RECONNECTING = 2;

  let nextId = 1;
  const rooms = new Map();
  // Tracks and publications are referenced from C++ by id. References are weak so that objects
  // livekit-client has dropped (e.g. after a room ends) can be collected.
  const objects = new Map();
  const objectIds = new WeakMap();
  const collected = new FinalizationRegistry((id) => objects.delete(id));

  function idOf(obj) {
    if (!obj) {
      return 0;
    }
    let id = objectIds.get(obj);
    if (!id) {
      id = nextId++;
      objectIds.set(obj, id);
      objects.set(id, new WeakRef(obj));
      collected.register(obj, id);
    }
    return id;
  }

  function objectOf(id) {
    const ref = objects.get(id);
    return ref ? ref.deref() : undefined;
  }

  function trackInfo(track) {
    return {
      id: idOf(track),
      sid: track.sid || '',
      name: track.name || '',
      kind: KIND[track.kind] || 0,
      source: SOURCE[track.source] || 0,
      muted: !!track.isMuted,
      stream_state: STREAM_STATE[track.streamState] || 0,
    };
  }

  function publicationInfo(pub) {
    return {
      id: idOf(pub),
      sid: pub.trackSid || '',
      name: pub.trackName || '',
      kind: KIND[pub.kind] || 0,
      source: SOURCE[pub.source] || 0,
      muted: !!pub.isMuted,
      mime_type: pub.mimeType || '',
      simulcasted: !!pub.simulcasted,
      subscribed: !!pub.isSubscribed,
      track: pub.track ? trackInfo(pub.track) : null,
    };
  }

  function participantInfo(p) {
    return {
      sid: p.sid || '',
      name: p.name || '',
      identity: p.identity || '',
      metadata: p.metadata || '',
      attributes: Object.assign({}, p.attributes || {}),
      kind: typeof p.kind === 'number' ? p.kind : 0,
    };
  }

  function toBase64(bytes) {
    let s = '';
    for (let i = 0; i < bytes.length; i += 0x8000) {
      s += String.fromCharCode.apply(null, bytes.subarray(i, i + 0x8000));
    }
    return btoa(s);
  }

  function fromBase64(b64) {
    const s = atob(b64);
    const bytes = new Uint8Array(s.length);
    for (let i = 0; i < s.length; i++) {
      bytes[i] = s.charCodeAt(i);
    }
    return bytes;
  }

  function warn(what) {
    return (e) => console.warn(`godot-livekit: ${what} failed:`, e);
  }

  function roomOf(id) {
    const r = rooms.get(id);
    if (!r) {
      throw new Error(`unknown room ${id}`);
    }
    return r;
  }

  function participantOf(r, identity, isLocal) {
    if (!r.room) {
      return null;
    }
    if (isLocal) {
      return r.room.localParticipant;
    }
    return r.room.getParticipantByIdentity(identity) || null;
  }

  // Reports what happened to C++, which only sees events between frames.
  function push(r, event) {
    (r.connected ? r.events : r.pending).push(event);
  }

  function listen(r) {
    const room = r.room;
    const E = LK.RoomEvent;
    const who = (p) => ({ identity: p ? p.identity : '', local: !!(p && p === room.localParticipant) });
    room.on(E.Disconnected, () => {
      r.state = STATE_DISCONNECTED;
      push(r, { type: 'disconnected' });
    });
    room.on(E.Reconnecting, () => {
      if (!r.autoReconnect) {
        // Matches the native binding: without auto-reconnect, a lost connection is a disconnect.
        r.state = STATE_DISCONNECTED;
        push(r, { type: 'disconnected' });
        room.removeAllListeners();
        room.disconnect();
        return;
      }
      r.state = STATE_RECONNECTING;
      push(r, { type: 'reconnecting' });
    });
    room.on(E.Reconnected, () => {
      r.state = STATE_CONNECTED;
      push(r, { type: 'reconnected' });
    });
    room.on(E.ParticipantConnected, (p) => {
      r.names.set(p.identity, p.name || '');
      push(r, { type: 'participant_connected', identity: p.identity });
    });
    room.on(E.ParticipantDisconnected, (p) => push(r, { type: 'participant_disconnected', identity: p.identity }));
    room.on(E.RoomMetadataChanged, (metadata) => {
      push(r, { type: 'room_metadata_changed', old: r.metadata, new: metadata || '' });
      r.metadata = metadata || '';
    });
    room.on(E.ConnectionQualityChanged, (quality, p) => {
      if (quality in CONNECTION_QUALITY) {
        push(r, Object.assign({ type: 'connection_quality_changed', quality: CONNECTION_QUALITY[quality] }, who(p)));
      }
    });
    room.on(E.ParticipantMetadataChanged, (previous, p) =>
      push(r, Object.assign({ type: 'participant_metadata_changed', old: previous || '', new: p.metadata || '' }, who(p))),
    );
    room.on(E.ParticipantNameChanged, (name, p) => {
      push(r, Object.assign({ type: 'participant_name_changed', old: r.names.get(p.identity) || '', new: name || '' }, who(p)));
      r.names.set(p.identity, name || '');
    });
    room.on(E.ParticipantAttributesChanged, (changed, p) =>
      push(r, Object.assign({ type: 'participant_attributes_changed', changed: Object.assign({}, changed) }, who(p))),
    );
    const pubEvent = (type) => (pub, p) => push(r, Object.assign({ type, publication: publicationInfo(pub) }, who(p)));
    room.on(E.TrackPublished, pubEvent('track_published'));
    room.on(E.TrackUnpublished, pubEvent('track_unpublished'));
    room.on(E.TrackMuted, pubEvent('track_muted'));
    room.on(E.TrackUnmuted, pubEvent('track_unmuted'));
    room.on(E.LocalTrackPublished, pubEvent('local_track_published'));
    room.on(E.LocalTrackUnpublished, pubEvent('local_track_unpublished'));
    const trackEvent = (type) => (track, pub, p) =>
      push(r, Object.assign({ type, track: trackInfo(track), publication: publicationInfo(pub) }, who(p)));
    room.on(E.TrackSubscribed, trackEvent('track_subscribed'));
    room.on(E.TrackUnsubscribed, trackEvent('track_unsubscribed'));
    room.on(E.DataReceived, (payload, p, kind, topic) =>
      push(
        r,
        Object.assign(
          {
            type: 'data_received',
            data: toBase64(payload),
            kind: kind === LK.DataPacket_Kind.LOSSY ? DATA_KIND_LOSSY : DATA_KIND_RELIABLE,
            topic: topic || '',
          },
          who(p),
        ),
      ),
    );
  }

  function teardown(r) {
    clearTimeout(r.timeout);
    if (r.room) {
      // Client-initiated disconnects aren't reported, matching the native binding.
      r.room.removeAllListeners();
      r.room.disconnect().catch(() => {});
      r.room = null;
    }
    r.state = STATE_DISCONNECTED;
    r.connected = false;
    r.events = [];
    r.pending = [];
  }

  const api = {
    room_create() {
      const id = nextId++;
      rooms.set(id, { room: null, state: STATE_DISCONNECTED, events: [], pending: [], connected: false, names: new Map() });
      return id;
    },

    room_destroy(id) {
      const r = rooms.get(id);
      if (r) {
        teardown(r);
        rooms.delete(id);
      }
    },

    room_connect(id, url, token, options) {
      const r = roomOf(id);
      teardown(r);
      r.autoReconnect = options.auto_reconnect !== false;
      r.metadata = '';
      r.sid = '';
      r.names = new Map();
      const room = new LK.Room({ dynacast: !!options.dynacast, adaptiveStream: false });
      r.room = room;
      listen(r);
      const timeoutSec = typeof options.connect_timeout === 'number' ? options.connect_timeout : 15.0;
      if (timeoutSec > 0) {
        r.timeout = setTimeout(() => {
          if (r.room === room && !r.connected) {
            teardown(r);
            // Formatted like the native binding (Godot's String::num drops a trailing ".0").
            const seconds = String(Number(timeoutSec.toFixed(1)));
            r.events.push({ type: 'connection_failed', error: `connection timed out after ${seconds} seconds` });
          }
        }, timeoutSec * 1000);
      }
      room
        .connect(url, token, { autoSubscribe: options.auto_subscribe !== false })
        .then(() => {
          if (r.room !== room) {
            return;
          }
          clearTimeout(r.timeout);
          r.metadata = room.metadata || '';
          r.names.set(room.localParticipant.identity, room.localParticipant.name || '');
          for (const p of room.remoteParticipants.values()) {
            r.names.set(p.identity, p.name || '');
          }
          room.getSid().then((sid) => (r.sid = sid)).catch(() => {});
          r.state = STATE_CONNECTED;
          r.connected = true;
          // 'connected' comes first, as in the native binding.
          r.events.push({ type: 'connected' });
          r.events.push(...r.pending);
          r.pending = [];
        })
        .catch((e) => {
          if (r.room !== room) {
            return;
          }
          teardown(r);
          r.events.push({ type: 'connection_failed', error: String((e && e.message) || e) });
        });
    },

    room_disconnect(id) {
      teardown(roomOf(id));
    },

    room_take_events(id) {
      const r = rooms.get(id);
      if (!r || r.events.length === 0) {
        return [];
      }
      const events = r.events;
      r.events = [];
      return events;
    },

    room_info(id) {
      const r = roomOf(id);
      const connected = r.connected && r.state !== STATE_DISCONNECTED;
      return {
        state: r.state,
        sid: connected ? r.sid : '',
        name: connected && r.room ? r.room.name || '' : '',
        metadata: connected && r.room ? r.room.metadata || '' : '',
      };
    },

    room_remote_identities(id) {
      const r = roomOf(id);
      return r.connected && r.room ? Array.from(r.room.remoteParticipants.keys()) : [];
    },

    participant_info(id, identity, isLocal) {
      const p = participantOf(roomOf(id), identity, isLocal);
      return p ? participantInfo(p) : null;
    },

    participant_publications(id, identity, isLocal) {
      const p = participantOf(roomOf(id), identity, isLocal);
      return p ? Array.from(p.trackPublications.values()).map(publicationInfo) : [];
    },

    local_set_metadata(id, metadata) {
      roomOf(id).room.localParticipant.setMetadata(metadata).catch(warn('set_metadata'));
    },

    local_set_name(id, name) {
      roomOf(id).room.localParticipant.setName(name).catch(warn('set_name'));
    },

    local_set_attributes(id, attributes) {
      roomOf(id).room.localParticipant.setAttributes(attributes).catch(warn('set_attributes'));
    },

    local_publish_data(id, b64, reliable, destinations, topic) {
      const options = { reliable: !!reliable };
      if (destinations.length) {
        options.destinationIdentities = destinations;
      }
      if (topic) {
        options.topic = topic;
      }
      roomOf(id).room.localParticipant.publishData(fromBase64(b64), options).catch(warn('publish_data'));
    },

    local_unpublish_track(id, sid) {
      const lp = roomOf(id).room.localParticipant;
      const pub = lp.trackPublications.get(sid);
      if (pub && pub.track) {
        lp.unpublishTrack(pub.track).catch(warn('unpublish_track'));
      }
    },

    publication_info(pubId) {
      const pub = objectOf(pubId);
      return pub ? publicationInfo(pub) : null;
    },

    publication_set_subscribed(pubId, subscribed) {
      const pub = objectOf(pubId);
      if (pub && typeof pub.setSubscribed === 'function') {
        pub.setSubscribed(!!subscribed);
      }
    },

    track_info(trackId) {
      const track = objectOf(trackId);
      return track ? trackInfo(track) : null;
    },
  };

  window.GodotLiveKit = {
    // Returns JSON for C++. Errors are reported as {"__error": message} rather than thrown into wasm.
    call(name, argsJson) {
      try {
        const result = api[name].apply(null, JSON.parse(argsJson));
        return result === undefined ? 'null' : JSON.stringify(result);
      } catch (e) {
        return JSON.stringify({ __error: `${name}: ${(e && e.message) || e}` });
      }
    },
    api,
    idOf,
    objectOf,
  };
})();
