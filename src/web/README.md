# Web build

Web exports can't load the native LiveKit SDK, so the web build implements the same classes on top of [livekit-client](https://github.com/livekit/client-sdk-js) (2.22.3, embedded in the extension). The C++ in this directory registers the classes and calls `bridge.js`, which drives livekit-client and queues its events for the frame callback to emit as signals.

The web build is meant to behave like the native one, and the shared tests in `test/` run against both. This page lists where it can't, with the reasons. The [README's web section](../../README.md#web) covers the export settings.

## General

- **Single-threaded only.** The web build is single-threaded, like Godot's default web templates, so enable **Extensions Support** in the Web export preset and leave **Thread Support** off.
- **Events arrive between frames.** The browser delivers LiveKit events while the game isn't running, so wait for them by yielding (e.g. `await get_tree().process_frame`). A blocking loop like `OS.delay_msec()` never sees them.
- **Tested in Chrome.** The automated tests run in headless Chrome. Firefox and Safari aren't covered by them yet.
- **Some failures are logged later.** Operations that the browser completes asynchronously (e.g. publishing a track or data, setting metadata, or changing encryption keys) report failures as browser console warnings once they fail, rather than with `push_error()` when called.

## Unavailable

- **`LiveKitAudioProcessingModule`** isn't registered, so `ClassDB.class_exists()` reports it as missing. Browsers don't expose WebRTC's audio processing, but they already apply echo cancellation, noise suppression and gain control to microphone audio captured through `getUserMedia()`, which Godot's web microphone input uses.

## Rooms and participants

- **Participants belong to their connection.** A participant object kept from an earlier connection reports "participant is no longer connected" when used, rather than acting on a later connection's participant. (Natively, the SDK has freed those participants.)
- **Publishing is asynchronous.** The publication `publish_track()` returns starts with an empty sid, which fills in once `local_track_published` fires. From then on, `get_track_publications()` includes it.

## Audio

- **Audio waits for a user gesture.** Audio runs on the browser's `AudioContext`, which browsers only start after a click, key press or touch. It resumes on the first one.
- **Frames are resampled.** `LiveKitAudioSource` frames are resampled to the `AudioContext`'s rate, so they needn't be exactly 10ms long, as they must be in native direct mode.
- **Direct mode is capped.** With `queue_size_ms` of 0, a source buffers at most 200ms, to keep latency low. Natively, direct mode doesn't cap the buffer.

## Video

- **Rotation is applied to the pixels.** Browsers can't send rotation as metadata, so `capture_frame()` rotates the image instead. Receivers then see it upright, which they don't natively, since the native binding ignores rotation metadata on receipt.
- **Timestamps come from the browser,** not `capture_frame()`'s `timestamp_us`.
- **New tracks repeat their frame.** The server only announces a track once media arrives, and a canvas track only sends a frame when its canvas changes. So until the game captures its next frame, a newly published track resends its source's current frame every 200ms, for up to 3 seconds.
- **Received frames can lag a poll.** Where the browser can convert a `VideoFrame` to RGBA, frames are converted asynchronously with `VideoFrame.copyTo()`, so each one reaches `frame_received` one poll after it arrives. Other browsers, and rotated or flipped frames, go through a canvas instead, which blocks the main thread for longer at large resolutions.

## Screen capture

`LiveKitScreenCapture` uses the browser's `getDisplayMedia()`, and browsers only capture what the user picks:

- **`start()` asks the user** what to share. Browsers may only allow that after a user gesture, so call it from an input event handler. Frames arrive once the user has chosen, and if the user declines, the capture doesn't start (a warning is logged).
- **Monitors and windows can't be listed or chosen.** `get_monitors()` and `get_windows()` return empty arrays, and `create_for_monitor()` and `create_for_window()` fail.
- **`check_permissions()` reports `PERMISSION_WARNING`**, since nothing is captured until the user agrees, or `PERMISSION_ERROR` in browsers without `getDisplayMedia()`, such as mobile ones.
- **`screenshot()` returns the latest frame** of a started capture, and fails before the first one.
- **The user can stop sharing** through the browser's own controls, which stops the capture.

## End-to-end encryption

Media and data are encrypted by livekit-client's encryption worker, which is embedded in the extension. Keys are derived and ratcheted the same way as in the native SDK. These differ:

- **Ratcheting is asynchronous.** `ratchet_shared_key()` and `ratchet_key()` return an empty array, and the new key is available from `get_shared_key()` or `get_key()` once it's ready.
- **Keys are shared or per-participant, not both.** Setting a shared key requires connecting with `LiveKitE2eeOptions.shared_key`, and per-participant keys require connecting without it. Mixing them is an error.
- **`set_key_index()` with a shared key switches every participant's key index,** since livekit-client keeps one index for the shared key.
- **`set_key_index()` fails if no key has been set at that index,** rather than switching to a missing key, since livekit-client switches index by setting that index's key again.
- **Unsupported options fail the connection:** `ENCRYPTION_CUSTOM`, a `ratchet_salt` that isn't valid UTF-8 (livekit-client takes the salt as a string), or a browser without encryption support. With `ENCRYPTION_NONE`, `get_e2ee_manager()` returns null.
- **`e2ee_state_changed` only reports what the worker does:** failures (3 for a failed decryption, 4 for a missing key, 6 for an internal error) and automatic ratchets of per-participant keys (5), never 0 (new) or 1 (OK).
- **`get_frame_cryptors()` returns snapshots.** Each cryptor reflects its track's settings when it was returned, plus changes made through it.
- **`set_enabled(false)` republishes tracks,** as livekit-client does to change whether they're encrypted.
- **Sends wait for encryption.** The connection is only reported once the worker confirms encryption is on. `publish_data()`, `perform_rpc()` and `publish_track()` wait for key and encryption changes requested before them, then fail rather than send unencrypted if encryption is on but couldn't start, if the local participant's frame cryptor is disabled, or (for data) if the local participant has no key.
- **A shared key's automatic ratchets aren't visible.** The worker only reports ratchets of per-participant keys, so `get_shared_key()` keeps returning the key from before a receiver ratcheted it.
- **With `failure_tolerance` of 0 or more,** a shared or local key that's exceeded it at an index that isn't current only recovers once `set_key_index()` switches to that index. (Remote participants' keys, and the current index, recover when they're set again.)
- **Data-only receivers don't follow a sender's ratchet.** A receiver that gets data, but no media, from a sender can't decrypt that sender's data after it ratchets its key. That's a livekit-client bug ([livekit/client-sdk-js#2187](https://github.com/livekit/client-sdk-js/issues/2187)). Media frames do ratchet the receiver's key, after which data decrypts too. Otherwise, set the ratcheted key on the receiver.
