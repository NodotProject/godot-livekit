extends GutTest
## Tests LiveKitAudioProcessingModule.create() factory and processing.

const SAMPLE_RATE := 48000
const FRAME := 480  # 10ms at 48kHz


func _tone(frames: int) -> PackedFloat32Array:
	var data := PackedFloat32Array()
	data.resize(frames * FRAME)
	for i in data.size():
		data[i] = 0.5 * sin(i * 0.05) * sin(i * 0.0007)
	return data


func test_create_apm():
	var apm = LiveKitAudioProcessingModule.create({"echo_cancellation": true, "noise_suppression": true})
	assert_not_null(apm, "AudioProcessingModule.create should return non-null")


func test_apm_is_ref_counted():
	var apm = LiveKitAudioProcessingModule.create()
	assert_true(apm is RefCounted, "AudioProcessingModule should be RefCounted")


func test_process_stream_returns_frame():
	var apm = LiveKitAudioProcessingModule.create({"noise_suppression": true})
	var out: PackedFloat32Array = apm.process_stream(_tone(1), SAMPLE_RATE, 1)
	assert_eq(out.size(), FRAME, "process_stream should return a full 10ms frame")


func test_process_stream_accepts_multiple_frames():
	var batched = LiveKitAudioProcessingModule.create({"echo_cancellation": true, "noise_suppression": true})
	var framewise = LiveKitAudioProcessingModule.create({"echo_cancellation": true, "noise_suppression": true})
	var far := _tone(3)
	var near := _tone(3)
	assert_true(batched.process_reverse_stream(far, SAMPLE_RATE, 1), "reverse stream should accept 30ms")
	var batched_out: PackedFloat32Array = batched.process_stream(near, SAMPLE_RATE, 1)
	var framewise_out := PackedFloat32Array()
	for f in 3:
		framewise.process_reverse_stream(far.slice(f * FRAME, (f + 1) * FRAME), SAMPLE_RATE, 1)
	for f in 3:
		framewise_out.append_array(framewise.process_stream(near.slice(f * FRAME, (f + 1) * FRAME), SAMPLE_RATE, 1))
	assert_eq(batched_out, framewise_out, "30ms of audio should process like three 10ms frames")


func test_process_stream_rejects_partial_frames():
	var apm = LiveKitAudioProcessingModule.create()
	var data := PackedFloat32Array()
	data.resize(FRAME + 100)
	# The native APM aborts the process on partial frames, so they must be rejected up front.
	var out: PackedFloat32Array = apm.process_stream(data, SAMPLE_RATE, 1)
	assert_eq(out.size(), 0, "process_stream should reject audio that isn't a multiple of 10ms")
	assert_false(apm.process_reverse_stream(data, SAMPLE_RATE, 1),
		"process_reverse_stream should reject audio that isn't a multiple of 10ms")
	assert_push_error(2)


func test_echo_cancellation_removes_echo():
	var apm = LiveKitAudioProcessingModule.create({"echo_cancellation": true})
	var delay_frames := 4
	var far := _tone(300)
	var energy_in := 0.0
	var energy_out := 0.0
	for f in range(300):
		apm.process_reverse_stream(far.slice(f * FRAME, (f + 1) * FRAME), SAMPLE_RATE, 1)
		apm.set_stream_delay_ms(delay_frames * 10)
		var near := PackedFloat32Array()
		near.resize(FRAME)
		if f >= delay_frames:
			for i in FRAME:
				near[i] = 0.25 * far[(f - delay_frames) * FRAME + i]
		var out: PackedFloat32Array = apm.process_stream(near, SAMPLE_RATE, 1)
		# Measure after the canceller has had time to converge.
		if f >= 200:
			for i in FRAME:
				energy_in += near[i] * near[i]
				energy_out += out[i] * out[i]
	assert_lt(energy_out, energy_in * 0.01, "echo should be attenuated by at least 20dB")


func test_capture_frame_rejects_invalid_frame_without_crashing():
	var source = LiveKitAudioSource.create(48000, 1, 0)
	var data := PackedFloat32Array()
	data.resize(1920)
	# Direct capture requires 10ms frames; the native error must not terminate the process.
	source.capture_frame(data, 48000, 1, 1920)
	assert_push_error(1)
