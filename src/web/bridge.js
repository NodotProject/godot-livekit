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
      collected.register(obj, id, obj);
    }
    return id;
  }

  // Makes id refer to obj from now on instead of to the placeholder it was created for.
  function promote(id, placeholder, obj) {
    collected.unregister(placeholder);
    objects.set(id, new WeakRef(obj));
    collected.register(obj, id);
    // Any id obj already has (e.g. from an event) keeps working too.
    if (!objectIds.has(obj)) {
      objectIds.set(obj, id);
    }
  }

  function objectOf(id) {
    const ref = objects.get(id);
    return ref ? ref.deref() : undefined;
  }

  // livekit-client only names remote tracks on their publications.
  const trackNames = new WeakMap();

  function trackInfo(track, pub) {
    if (pub && pub.trackName) {
      trackNames.set(track, pub.trackName);
    }
    return {
      id: idOf(track),
      sid: track.sid || '',
      name: track.name || trackNames.get(track) || '',
      kind: KIND[track.kind] || 0,
      source: SOURCE[track.source] || 0,
      muted: !!track.isMuted,
      stream_state: STREAM_STATE[track.streamState] || 0,
    };
  }

  function publicationInfo(pub) {
    if (pub.pending) {
      return {
        id: idOf(pub),
        sid: '',
        name: pub.track.name || '',
        kind: KIND[pub.track.kind] || 0,
        source: SOURCE[pub.track.source] || 0,
        muted: !!pub.track.isMuted,
        mime_type: '',
        simulcasted: false,
        subscribed: false,
        track: trackInfo(pub.track),
      };
    }
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
      track: pub.track ? trackInfo(pub.track, pub) : null,
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

  // Identifies a specific participant (not just an identity, which a later participant may reuse)
  // and snapshots its state, since it may have left by the time C++ handles an event.
  function participantRef(room, p) {
    return p ? { id: idOf(p), local: p === room.localParticipant, info: participantInfo(p) } : null;
  }

  // Returns the room's local participant if participantId still refers to it and the connection
  // hasn't ended. (It may be reconnecting, in which case livekit-client queues the operation.)
  function localParticipantOf(id, participantId) {
    const r = roomOf(id);
    const live = r.room && r.connected && r.state !== STATE_DISCONNECTED;
    const lp = live ? r.room.localParticipant : null;
    if (!lp || idOf(lp) !== participantId) {
      throw new Error('participant is no longer connected');
    }
    return lp;
  }

  // Like localParticipantOf, for a remote participant still in the room.
  function remoteParticipantOf(id, participantId) {
    const r = roomOf(id);
    const p = objectOf(participantId);
    const live = r.room && r.connected && r.state !== STATE_DISCONNECTED;
    if (!live || !p || r.room.remoteParticipants.get(p.identity) !== p) {
      throw new Error('participant is no longer connected');
    }
    return p;
  }

  // Reports what happened to C++, which only sees events between frames.
  function push(r, event) {
    (r.connected ? r.events : r.pending).push(event);
  }

  function listen(r) {
    const room = r.room;
    const E = LK.RoomEvent;
    const who = (p) => ({ participant: participantRef(room, p) });
    room.on(E.Disconnected, () => {
      r.state = STATE_DISCONNECTED;
      push(r, { type: 'disconnected' });
    });
    // livekit-client reports a signaling-only resume separately from a full reconnect, and a resume
    // can escalate into one. Native reports both as a single reconnect.
    const onReconnecting = () => {
      if (!r.autoReconnect) {
        // Matches the native binding: without auto-reconnect, a lost connection is a disconnect.
        r.state = STATE_DISCONNECTED;
        push(r, { type: 'disconnected' });
        room.removeAllListeners();
        room.disconnect();
        return;
      }
      if (r.state === STATE_RECONNECTING) {
        return;
      }
      r.state = STATE_RECONNECTING;
      push(r, { type: 'reconnecting' });
    };
    room.on(E.SignalReconnecting, onReconnecting);
    room.on(E.Reconnecting, onReconnecting);
    room.on(E.Reconnected, () => {
      r.state = STATE_CONNECTED;
      push(r, { type: 'reconnected' });
    });
    room.on(E.ParticipantConnected, (p) => {
      r.names.set(p.identity, p.name || '');
      push(r, Object.assign({ type: 'participant_connected' }, who(p)));
    });
    room.on(E.ParticipantDisconnected, (p) => push(r, Object.assign({ type: 'participant_disconnected' }, who(p))));
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
      push(r, Object.assign({ type, track: trackInfo(track, pub), publication: publicationInfo(pub) }, who(p)));
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
          r.events.push({
            type: 'connected',
            local: participantRef(room, room.localParticipant),
            remotes: Array.from(room.remoteParticipants.values()).map((p) => participantRef(room, p)),
          });
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

    participant_info(participantId) {
      const p = objectOf(participantId);
      return p ? participantInfo(p) : null;
    },

    participant_publications(participantId) {
      const p = objectOf(participantId);
      return p ? Array.from(p.trackPublications.values()).map(publicationInfo) : [];
    },

    local_set_metadata(id, participantId, metadata) {
      localParticipantOf(id, participantId).setMetadata(metadata).catch(warn('set_metadata'));
    },

    local_set_name(id, participantId, name) {
      localParticipantOf(id, participantId).setName(name).catch(warn('set_name'));
    },

    local_set_attributes(id, participantId, attributes) {
      localParticipantOf(id, participantId).setAttributes(attributes).catch(warn('set_attributes'));
    },

    local_publish_data(id, participantId, b64, reliable, destinations, topic) {
      const options = { reliable: !!reliable };
      if (destinations.length) {
        options.destinationIdentities = destinations;
      }
      if (topic) {
        options.topic = topic;
      }
      localParticipantOf(id, participantId).publishData(fromBase64(b64), options).catch(warn('publish_data'));
    },

    local_unpublish_track(id, participantId, sid) {
      const lp = localParticipantOf(id, participantId);
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


  // --- Audio ---
  //
  // Audio moves between Godot and livekit-client through AudioWorklets on one AudioContext:
  // captured frames are posted to a "source" worklet whose output feeds a publishable
  // MediaStreamTrack, and a "sink" worklet copies remote tracks' audio back for Godot to poll.

  const WORKLETS = `
    class GodotLiveKitSource extends AudioWorkletProcessor {
      constructor(options) {
        super();
        this.channels = options.processorOptions.channels;
        // Frames beyond this are dropped (oldest first) to bound latency; 0 means unbounded.
        this.capacity = options.processorOptions.capacity;
        this.chunks = [];
        this.offset = 0;
        this.queued = 0;
        // Frames played, dropped or cleared so far, which the main thread compares against the
        // frames it has sent to know what's still queued.
        this.removed = 0;
        this.blocks = 0;
        this.stopped = false;
        this.port.onmessage = (e) => {
          if (e.data === 'stop') {
            this.stopped = true;
            return;
          }
          if (e.data === 'clear') {
            this.removed += this.queued;
            this.chunks = [];
            this.offset = 0;
            this.queued = 0;
            return;
          }
          this.chunks.push(e.data);
          this.queued += e.data.length / this.channels;
          while (this.capacity && this.queued > this.capacity) {
            const excess = this.queued - this.capacity;
            const remaining = (this.chunks[0].length - this.offset) / this.channels;
            if (remaining > excess) {
              this.offset += excess * this.channels;
              this.queued -= excess;
              this.removed += excess;
            } else {
              this.chunks.shift();
              this.offset = 0;
              this.queued -= remaining;
              this.removed += remaining;
            }
          }
        };
      }
      process(inputs, outputs) {
        // A processor that returns true keeps running even once its node is disconnected.
        if (this.stopped) return false;
        const out = outputs[0];
        const frames = out[0].length;
        for (let i = 0; i < frames; i++) {
          if (!this.chunks.length) {
            for (let c = 0; c < out.length; c++) out[c].fill(0, i);
            break;
          }
          const chunk = this.chunks[0];
          for (let c = 0; c < out.length; c++) {
            out[c][i] = chunk[this.offset + Math.min(c, this.channels - 1)];
          }
          this.offset += this.channels;
          this.queued--;
          this.removed++;
          if (this.offset >= chunk.length) {
            this.chunks.shift();
            this.offset = 0;
          }
        }
        if (++this.blocks % 8 === 0) {
          this.port.postMessage(this.removed);
        }
        return true;
      }
    }
    class GodotLiveKitSink extends AudioWorkletProcessor {
      constructor() {
        super();
        this.frames = 480;
        this.buffer = new Float32Array(this.frames * 2);
        this.n = 0;
        this.stopped = false;
        this.port.onmessage = (e) => (this.stopped = e.data === 'stop');
      }
      process(inputs) {
        if (this.stopped) return false;
        const input = inputs[0];
        if (!input || !input.length) return true;
        const left = input[0];
        const right = input[1] || input[0];
        for (let i = 0; i < left.length; i++) {
          this.buffer[this.n * 2] = left[i];
          this.buffer[this.n * 2 + 1] = right[i];
          if (++this.n === this.frames) {
            this.port.postMessage({ channels: input.length, data: this.buffer }, [this.buffer.buffer]);
            this.buffer = new Float32Array(this.frames * 2);
            this.n = 0;
          }
        }
        return true;
      }
    }
    registerProcessor('godot-livekit-source', GodotLiveKitSource);
    registerProcessor('godot-livekit-sink', GodotLiveKitSink);
  `;

  // Remote audio buffered for Godot beyond this is dropped, matching the native binding.
  const MAX_STREAM_SECONDS = 5;
  // Direct-capture sources (queue_size_ms of 0) keep at most this much audio queued.
  const DIRECT_CAPTURE_SECONDS = 0.2;

  let audioContext = null;
  let workletsReady = null;
  const sources = new Map();
  const streams = new Map();
  // Placeholder publications are held until livekit-client finishes publishing.
  const pendingPublications = new Set();
  // Source channel counts by track. livekit-client otherwise decides whether to publish stereo
  // from the track's reported settings, which don't reliably reflect a Web Audio destination's.
  const sourceChannels = new WeakMap();

  function context() {
    if (!audioContext) {
      audioContext = new AudioContext({ sampleRate: 48000, latencyHint: 'interactive' });
      const url = URL.createObjectURL(new Blob([WORKLETS], { type: 'application/javascript' }));
      workletsReady = audioContext.audioWorklet.addModule(url);
      // Browsers only allow audio to start after a user gesture.
      const resume = () => {
        if (audioContext.state === 'suspended') {
          audioContext.resume().catch(() => {});
        }
      };
      for (const type of ['pointerdown', 'keydown', 'touchend']) {
        window.addEventListener(type, resume, { capture: true, passive: true });
      }
      resume();
    }
    return audioContext;
  }

  // Streaming linear resampler for interleaved audio.
  function makeResampler(fromRate, toRate, channels) {
    const ratio = fromRate / toRate;
    let position = 0;
    let previous = new Float32Array(channels);
    return (input) => {
      if (fromRate === toRate) {
        return input;
      }
      const frames = input.length / channels;
      const output = [];
      // position is relative to input frame 0; frame -1 is the previous chunk's last frame.
      while (position < frames - 1) {
        const i = Math.floor(position);
        const t = position - i;
        for (let c = 0; c < channels; c++) {
          const a = i < 0 ? previous[c] : input[i * channels + c];
          const b = input[(i + 1) * channels + c];
          output.push(a + (b - a) * t);
        }
        position += ratio;
      }
      position -= frames;
      if (frames > 0) {
        previous = input.slice((frames - 1) * channels, frames * channels);
      }
      return Float32Array.from(output);
    };
  }

  // Called directly (not through `call`) with samples copied out of the wasm heap.
  function sourceCapture(id, samples, sampleRate, channels) {
    const s = sources.get(id);
    if (!s) {
      return;
    }
    if (!s.resampler || s.inputRate !== sampleRate || s.inputChannels !== channels) {
      s.inputRate = sampleRate;
      s.inputChannels = channels;
      s.resampler = makeResampler(sampleRate, context().sampleRate, channels);
    }
    let data = s.resampler(samples);
    if (channels !== s.channels) {
      const frames = data.length / channels;
      const converted = new Float32Array(frames * s.channels);
      for (let f = 0; f < frames; f++) {
        for (let c = 0; c < s.channels; c++) {
          converted[f * s.channels + c] = data[f * channels + Math.min(c, channels - 1)];
        }
      }
      data = converted;
    }
    // Resampling down can leave nothing from a very short capture.
    if (!data.length) {
      return;
    }
    s.sent += data.length / s.channels;
    if (s.node) {
      s.node.port.postMessage(data, [data.buffer]);
    } else {
      s.pending.push(data);
    }
  }

  // Copies up to maxFrames of interleaved stereo audio into heap (a Float32Array) at offset.
  function streamRead(id, heap, offset, maxFrames) {
    const st = streams.get(id);
    if (!st) {
      return 0;
    }
    let frames = 0;
    while (frames < maxFrames && st.chunks.length) {
      const chunk = st.chunks[0];
      const n = Math.min(maxFrames - frames, (chunk.length - st.offset) / 2);
      heap.set(chunk.subarray(st.offset, st.offset + n * 2), offset + frames * 2);
      frames += n;
      st.offset += n * 2;
      st.queued -= n;
      if (st.offset >= chunk.length) {
        st.chunks.shift();
        st.offset = 0;
      }
    }
    return frames;
  }

  function attachStream(st, track) {
    if (st.closed || st.track === track || !track.mediaStreamTrack) {
      return;
    }
    detachStream(st);
    st.track = track;
    const ctx = context();
    const media = new MediaStream([track.mediaStreamTrack]);
    // Chrome only feeds remote WebRTC audio to Web Audio while a media element plays it.
    st.element = new Audio();
    st.element.muted = true;
    st.element.srcObject = media;
    st.element.play().catch(() => {});
    st.input = ctx.createMediaStreamSource(media);
    workletsReady.then(() => {
      if (st.track !== track) {
        return;
      }
      st.node = new AudioWorkletNode(ctx, 'godot-livekit-sink', {
        numberOfOutputs: 1,
        outputChannelCount: [1],
        channelCountMode: 'max',
      });
      st.node.port.onmessage = (e) => {
        st.channels = e.data.channels;
        st.chunks.push(e.data.data);
        st.queued += e.data.data.length / 2;
        while (st.queued > ctx.sampleRate * MAX_STREAM_SECONDS && st.chunks.length > 1) {
          st.queued -= (st.chunks[0].length - st.offset) / 2;
          st.chunks.shift();
          st.offset = 0;
        }
      };
      // Worklets are only processed while connected toward the destination.
      st.silence = ctx.createGain();
      st.silence.gain.value = 0;
      st.input.connect(st.node);
      st.node.connect(st.silence);
      st.silence.connect(ctx.destination);
    });
  }

  function detachStream(st) {
    if (st.node) {
      stopWorklet(st.node);
    }
    for (const node of [st.input, st.silence]) {
      if (node) {
        node.disconnect();
      }
    }
    if (st.element) {
      st.element.srcObject = null;
    }
    st.track = st.input = st.node = st.silence = st.element = null;
  }

  function stopWorklet(node) {
    node.port.onmessage = null;
    node.port.postMessage('stop');
    node.port.close();
    node.disconnect();
  }

  function newStream() {
    const id = nextId++;
    const st = { chunks: [], offset: 0, queued: 0, channels: 1, closed: false };
    streams.set(id, st);
    return [id, st];
  }

  Object.assign(api, {
    audio_sample_rate() {
      return context().sampleRate;
    },

    source_create(sampleRate, channels, queueMs) {
      const ctx = context();
      const id = nextId++;
      const s = {
        channels: Math.max(1, Math.min(2, channels)),
        pending: [],
        node: null,
        // Frames sent toward the worklet, and how many of them it has reported removing. The
        // queue is their difference, so it's current even before the worklet's next report.
        sent: 0,
        removed: 0,
        capacity: queueMs > 0 ? 0 : ctx.sampleRate * DIRECT_CAPTURE_SECONDS,
        destination: ctx.createMediaStreamDestination(),
      };
      // Destinations default to stereo, which would upmix a mono source.
      s.destination.channelCount = s.channels;
      sources.set(id, s);
      workletsReady.then(() => {
        if (sources.get(id) !== s) {
          return;
        }
        s.node = new AudioWorkletNode(ctx, 'godot-livekit-source', {
          numberOfInputs: 0,
          outputChannelCount: [s.channels],
          processorOptions: {
            channels: s.channels,
            capacity: s.capacity,
          },
        });
        s.node.port.onmessage = (e) => (s.removed = Math.max(s.removed, e.data));
        s.node.connect(s.destination);
        for (const data of s.pending) {
          s.node.port.postMessage(data, [data.buffer]);
        }
        s.pending = [];
      });
      return id;
    },

    source_destroy(id) {
      const s = sources.get(id);
      if (s) {
        if (s.node) {
          stopWorklet(s.node);
        }
        sources.delete(id);
      }
    },

    source_clear(id) {
      const s = sources.get(id);
      if (s) {
        for (const data of s.pending) {
          s.sent -= data.length / s.channels;
        }
        s.pending = [];
        // The worklet's later reports will include the cleared frames.
        s.removed = s.sent;
        if (s.node) {
          s.node.port.postMessage('clear');
        }
      }
    },

    source_queued_duration(id) {
      const s = sources.get(id);
      if (!s) {
        return 0;
      }
      // The worklet drops frames beyond capacity as they arrive but reports it later.
      const queued = s.capacity ? Math.min(s.sent - s.removed, s.capacity) : s.sent - s.removed;
      return queued / context().sampleRate;
    },

    local_audio_track_create(sourceId, name) {
      const s = sources.get(sourceId);
      if (!s) {
        throw new Error('invalid audio source');
      }
      // A clone per track, so that muting or unpublishing (which stops the track) one track
      // doesn't affect the source's other tracks.
      const mediaTrack = s.destination.stream.getAudioTracks()[0].clone();
      const track = new LK.LocalAudioTrack(mediaTrack, undefined, true, context());
      track.name = name || '';
      track.source = LK.Track.Source.Microphone;
      sourceChannels.set(track, s.channels);
      return trackInfo(track);
    },

    track_set_muted(trackId, muted) {
      const track = objectOf(trackId);
      if (track && typeof track.mute === 'function') {
        (muted ? track.mute() : track.unmute()).catch(warn(muted ? 'mute' : 'unmute'));
      }
    },

    local_publish_track(id, participantId, trackId, options) {
      const lp = localParticipantOf(id, participantId);
      const track = objectOf(trackId);
      if (!track) {
        throw new Error('invalid track');
      }
      const sourceNames = ['unknown', 'camera', 'microphone', 'screen_share', 'screen_share_audio'];
      const publishOptions = { name: track.name || undefined };
      if ('source' in options) {
        publishOptions.source = sourceNames[options.source] || 'unknown';
        track.source = publishOptions.source;
      }
      if (sourceChannels.has(track)) {
        publishOptions.forceStereo = sourceChannels.get(track) === 2;
      }
      for (const key of ['dtx', 'red', 'simulcast']) {
        if (key in options) {
          publishOptions[key] = !!options[key];
        }
      }
      // livekit-client publishes asynchronously, so a placeholder stands in until it's done.
      const placeholder = { pending: true, track };
      pendingPublications.add(placeholder);
      const pubId = idOf(placeholder);
      lp.publishTrack(track, publishOptions)
        .then((pub) => {
          promote(pubId, placeholder, pub);
          const source = videoCaptureTracks.get(track.mediaStreamTrack);
          if (source) {
            resendVideoFrames(source, track.mediaStreamTrack);
          }
        })
        .catch(warn('publish_track'))
        .finally(() => pendingPublications.delete(placeholder));
      return publicationInfo(placeholder);
    },

    stream_from_track(trackId) {
      const track = objectOf(trackId);
      if (!track || track.kind !== 'audio') {
        throw new Error('invalid track');
      }
      const [id, st] = newStream();
      attachStream(st, track);
      return id;
    },

    stream_from_participant(roomId, participantId, source) {
      const participant = remoteParticipantOf(roomId, participantId);
      const sourceName = ['unknown', 'camera', 'microphone', 'screen_share', 'screen_share_audio'][source];
      const [id, st] = newStream();
      const matches = (track) => track && track.kind === 'audio' && track.source === sourceName;
      for (const pub of participant.trackPublications.values()) {
        if (matches(pub.track)) {
          attachStream(st, pub.track);
        }
      }
      // Like the native binding, the stream follows the participant's track for that source.
      st.onSubscribed = (track) => matches(track) && attachStream(st, track);
      participant.on(LK.ParticipantEvent.TrackSubscribed, st.onSubscribed);
      st.participant = participant;
      return id;
    },

    stream_info(id) {
      const st = streams.get(id);
      return { sample_rate: context().sampleRate, num_channels: st ? st.channels : 1 };
    },

    stream_close(id) {
      const st = streams.get(id);
      if (st) {
        st.closed = true;
        detachStream(st);
        if (st.participant) {
          st.participant.off(LK.ParticipantEvent.TrackSubscribed, st.onSubscribed);
        }
        streams.delete(id);
      }
    },
  });

  // --- Video ---
  //
  // Captured frames are drawn onto a canvas whose captureStream() tracks are published. Remote
  // tracks play in hidden video elements, and polling reads their newest frame through a canvas.

  const videoSources = new Map();
  const videoStreams = new Map();
  // Local video tracks' canvas capture tracks, mapped to their sources.
  const videoCaptureTracks = new WeakMap();
  const RESEND_INTERVAL_MS = 200;
  const RESEND_DURATION_MS = 3000;

  // Called directly (not through `call`) with rgba viewing the wasm heap.
  function videoSourceCapture(id, rgba, width, height, rotation) {
    const s = videoSources.get(id);
    if (!s) {
      return;
    }
    const image = new ImageData(new Uint8ClampedArray(rgba.buffer, rgba.byteOffset, rgba.length), width, height);
    const quarterTurn = rotation === 90 || rotation === 270;
    resize(s.canvas, quarterTurn ? height : width, quarterTurn ? width : height);
    if (rotation === 90 || rotation === 180 || rotation === 270) {
      // Browsers can't send rotation as metadata the way native WebRTC does, so the pixels turn.
      resize(s.scratch, width, height);
      s.scratch.getContext('2d').putImageData(image, 0, 0);
      s.ctx.save();
      s.ctx.translate(s.canvas.width / 2, s.canvas.height / 2);
      s.ctx.rotate((rotation * Math.PI) / 180);
      s.ctx.drawImage(s.scratch, -width / 2, -height / 2);
      s.ctx.restore();
    } else {
      s.ctx.putImageData(image, 0, 0);
    }
    s.captures++;
    for (const track of s.tracks) {
      if (track.readyState === 'ended') {
        s.tracks.delete(track);
      } else {
        track.requestFrame();
      }
    }
  }

  // The server only announces a track once its media arrives, and a canvas track only sends a
  // frame when one is requested after the canvas changes. So until the game captures a frame, a
  // newly published track resends the source's current one.
  function resendVideoFrames(s, track) {
    const captures = s.captures;
    const started = Date.now();
    const timer = setInterval(() => {
      if (s.captures !== captures || track.readyState === 'ended' || Date.now() - started > RESEND_DURATION_MS) {
        clearInterval(timer);
        return;
      }
      // Drawing the canvas onto itself counts as a change without altering it.
      s.ctx.drawImage(s.canvas, 0, 0);
      track.requestFrame();
    }, RESEND_INTERVAL_MS);
  }

  function resize(canvas, width, height) {
    if (canvas.width !== width || canvas.height !== height) {
      canvas.width = width;
      canvas.height = height;
    }
  }

  // Captures a stream's newest frame if it hasn't been read yet, returning its [width, height].
  function videoStreamPrepare(id) {
    const st = videoStreams.get(id);
    const el = st && st.element;
    // HAVE_CURRENT_DATA: a frame can be drawn.
    if (!el || el.readyState < 2 || !el.videoWidth) {
      return null;
    }
    if (st.frameCallbacks) {
      if (!st.fresh) {
        return null;
      }
    } else if (el.currentTime === st.lastTime) {
      return null;
    }
    st.fresh = false;
    st.lastTime = el.currentTime;
    const width = el.videoWidth;
    const height = el.videoHeight;
    resize(st.canvas, width, height);
    st.ctx.drawImage(el, 0, 0, width, height);
    st.pixels = st.ctx.getImageData(0, 0, width, height).data;
    return [width, height];
  }

  // Copies the frame from videoStreamPrepare into heap (a Uint8Array) at offset.
  function videoStreamRead(id, heap, offset) {
    const st = videoStreams.get(id);
    if (st && st.pixels) {
      heap.set(st.pixels, offset);
      st.pixels = null;
    }
  }

  function attachVideo(st, track) {
    if (st.closed || st.track === track || !track.mediaStreamTrack) {
      return;
    }
    detachVideo(st);
    st.track = track;
    const el = document.createElement('video');
    el.muted = true;
    el.playsInline = true;
    // In the document but out of sight, since browsers needn't decode video that isn't.
    Object.assign(el.style, {
      position: 'fixed',
      left: '0',
      top: '0',
      width: '1px',
      height: '1px',
      opacity: '0',
      pointerEvents: 'none',
    });
    document.body.appendChild(el);
    el.srcObject = new MediaStream([track.mediaStreamTrack]);
    el.play().catch(() => {});
    st.element = el;
    st.fresh = false;
    st.lastTime = -1;
    st.frameCallbacks = typeof el.requestVideoFrameCallback === 'function';
    if (st.frameCallbacks) {
      const onFrame = () => {
        if (st.element === el) {
          st.fresh = true;
          el.requestVideoFrameCallback(onFrame);
        }
      };
      el.requestVideoFrameCallback(onFrame);
    }
  }

  function detachVideo(st) {
    if (st.element) {
      st.element.srcObject = null;
      st.element.remove();
    }
    st.track = st.element = st.pixels = null;
  }

  function newVideoStream() {
    const id = nextId++;
    const canvas = document.createElement('canvas');
    const st = { canvas, ctx: canvas.getContext('2d', { willReadFrequently: true }), closed: false };
    videoStreams.set(id, st);
    return [id, st];
  }

  Object.assign(api, {
    video_source_create(width, height) {
      const id = nextId++;
      const canvas = document.createElement('canvas');
      canvas.width = Math.max(1, width);
      canvas.height = Math.max(1, height);
      const ctx = canvas.getContext('2d');
      // Black (rather than transparent) until the first capture, as a new native source sends.
      ctx.fillStyle = 'black';
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      videoSources.set(id, {
        canvas,
        ctx,
        scratch: document.createElement('canvas'),
        tracks: new Set(),
        captures: 0,
      });
      return id;
    },

    video_source_destroy(id) {
      videoSources.delete(id);
    },

    local_video_track_create(sourceId, name) {
      const s = videoSources.get(sourceId);
      if (!s) {
        throw new Error('invalid video source');
      }
      // A capture track per local track, so that each can be muted or stopped on its own.
      const mediaTrack = s.canvas.captureStream(0).getVideoTracks()[0];
      s.tracks.add(mediaTrack);
      videoCaptureTracks.set(mediaTrack, s);
      const track = new LK.LocalVideoTrack(mediaTrack, undefined, true);
      track.name = name || '';
      track.source = LK.Track.Source.Camera;
      return trackInfo(track);
    },

    video_stream_from_track(trackId) {
      const track = objectOf(trackId);
      if (!track || track.kind !== 'video') {
        throw new Error('invalid track');
      }
      const [id, st] = newVideoStream();
      attachVideo(st, track);
      return id;
    },

    video_stream_from_participant(roomId, participantId, source) {
      const participant = remoteParticipantOf(roomId, participantId);
      const sourceName = ['unknown', 'camera', 'microphone', 'screen_share', 'screen_share_audio'][source];
      const [id, st] = newVideoStream();
      const matches = (track) => track && track.kind === 'video' && track.source === sourceName;
      for (const pub of participant.trackPublications.values()) {
        if (matches(pub.track)) {
          attachVideo(st, pub.track);
        }
      }
      // Like the native binding, the stream follows the participant's track for that source.
      st.onSubscribed = (track) => matches(track) && attachVideo(st, track);
      participant.on(LK.ParticipantEvent.TrackSubscribed, st.onSubscribed);
      st.participant = participant;
      return id;
    },

    video_stream_close(id) {
      const st = videoStreams.get(id);
      if (st) {
        st.closed = true;
        detachVideo(st);
        if (st.participant) {
          st.participant.off(LK.ParticipantEvent.TrackSubscribed, st.onSubscribed);
        }
        videoStreams.delete(id);
      }
    },
  });

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
    sourceCapture,
    streamRead,
    videoSourceCapture,
    videoStreamPrepare,
    videoStreamRead,
  };
})();
