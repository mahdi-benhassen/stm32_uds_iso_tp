from middleware.diagnostics.uds_isotp import (
    CanFrame,
    IsoTpError,
    IsoTpReceiver,
    UdsIsoTpNode,
    UdsServer,
    isotp_encode,
)


def test_single_frame_round_trip():
    frames = isotp_encode(0x700, b"\x10\x01")
    receiver = IsoTpReceiver()
    flow, payload = receiver.feed(frames[0])
    assert flow == []
    assert payload == b"\x10\x01"


def test_multi_frame_round_trip_and_flow_control():
    payload = bytes(range(20))
    frames = isotp_encode(0x700, payload)
    receiver = IsoTpReceiver()
    flow, completed = receiver.feed(frames[0])
    assert len(flow) == 1
    assert flow[0].data == b"\x30\x00\x00"
    assert completed is None
    for frame in frames[1:-1]:
        flow, completed = receiver.feed(frame)
        assert flow == []
        assert completed is None
    _, completed = receiver.feed(frames[-1])
    assert completed == payload


def test_sequence_mismatch_is_rejected():
    receiver = IsoTpReceiver()
    receiver.feed(CanFrame(0x700, bytes((0x10, 0x08)) + b"abcdef"))
    try:
        receiver.feed(CanFrame(0x700, b"\x22gh"))
    except IsoTpError as exc:
        assert "sequence" in str(exc)
    else:
        raise AssertionError("sequence mismatch was accepted")


def test_uds_services_and_negative_responses():
    values = {0xF190: b"F767REF"}
    writes = {}

    def read_did(did):
        return values.get(did)

    def write_did(did, value):
        if did != 0xF187:
            return False
        writes[did] = value
        return True

    server = UdsServer(read_did, write_did)
    assert server.handle(b"\x10\x03")[:2] == b"\x50\x03"
    assert server.handle(b"\x22\xF1\x90") == b"\x62\xF1\x90F767REF"
    assert server.handle(b"\x2E\xF1\x87abc") == b"\x6E\xF1\x87"
    assert writes[0xF187] == b"abc"
    assert server.handle(b"\x22\x12\x34") == b"\x7F\x22\x31"
    assert server.handle(b"\x99") == b"\x7F\x99\x11"
    assert server.handle(b"\x11\x01") == b"\x51\x01"
    assert server.reset_requested


def test_node_filters_request_id_and_returns_isotp_response():
    server = UdsServer(lambda did: b"ok" if did == 0x1000 else None, lambda _did, _v: False)
    node = UdsIsoTpNode(0x600, 0x580, server)
    assert node.receive(CanFrame(0x601, b"\x03\x22\x10\x00")) == []
    response = node.receive(CanFrame(0x600, b"\x03\x22\x10\x00"))
    assert len(response) == 1
    assert response[0].can_id == 0x580
    assert response[0].data == b"\x05\x62\x10\x00ok"
