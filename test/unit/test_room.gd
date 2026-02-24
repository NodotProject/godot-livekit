extends GutTest

func test_create_room():
    var room = LiveKitRoom.new()
    assert_not_null(room, "Room should be created")
    
func test_local_participant_initially_null():
    var room = LiveKitRoom.new()
    assert_null(room.get_local_participant(), "Local participant should be null before connection")

func test_remote_participants_initially_empty():
    var room = LiveKitRoom.new()
    var remotes = room.get_remote_participants()
    assert_eq(remotes.size(), 0, "Remote participants should be empty before connection")
