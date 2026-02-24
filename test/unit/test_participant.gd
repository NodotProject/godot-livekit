extends GutTest

func test_create_participant():
    var p = LiveKitParticipant.new()
    assert_not_null(p, "Participant should be created")
    assert_eq(p.get_sid(), "", "SID should be empty for unbound participant")

func test_create_local_participant():
    var lp = LiveKitLocalParticipant.new()
    assert_not_null(lp, "LocalParticipant should be created")
    assert_true(lp is LiveKitParticipant, "LocalParticipant should inherit Participant")

func test_create_remote_participant():
    var rp = LiveKitRemoteParticipant.new()
    assert_not_null(rp, "RemoteParticipant should be created")
    assert_true(rp is LiveKitParticipant, "RemoteParticipant should inherit Participant")
