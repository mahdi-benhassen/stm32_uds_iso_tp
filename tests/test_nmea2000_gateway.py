from middleware.gateway.nmea2000_gateway import (
    BatteryGateway,
    ExtendedCanFrame,
    Nmea2000Error,
    PGN_BATTERY_STATUS_1,
    encode_battery_status_1,
    make_j1939_id,
    parse_j1939_id,
)


def test_extended_id_round_trip():
    can_id = make_j1939_id(6, PGN_BATTERY_STATUS_1, 42)
    parsed = parse_j1939_id(can_id)
    assert parsed.priority == 6
    assert parsed.pgn == PGN_BATTERY_STATUS_1
    assert parsed.source == 42
    assert parsed.destination is None


def test_battery_status_encoding():
    frame = encode_battery_status_1(42, 0, 48.0, -12.3, 298.15)
    assert isinstance(frame, ExtendedCanFrame)
    parsed = parse_j1939_id(frame.can_id)
    assert parsed.pgn == PGN_BATTERY_STATUS_1
    assert frame.data[0] == 0
    assert int.from_bytes(frame.data[1:3], "little") == 4800
    assert int.from_bytes(frame.data[3:5], "little", signed=True) == -123
    assert int.from_bytes(frame.data[5:7], "little", signed=True) == 29815
    assert frame.data[7] == 0xFF


def test_gateway_mapping_and_bounds():
    gateway = BatteryGateway(42)
    assert gateway.map_canopen_value(0x6060, 48000) == (PGN_BATTERY_STATUS_1, 1, 48000)
    assert gateway.map_canopen_value(0x6070, 120) == (PGN_BATTERY_STATUS_1, 2, 120)
    assert gateway.map_canopen_value(0x6000, 1) is None
    try:
        gateway.battery_status(100000.0, 0.0, 298.0)
    except Nmea2000Error:
        pass
    else:
        raise AssertionError("out-of-range voltage was accepted")


def test_invalid_standard_id_rejected():
    try:
        ExtendedCanFrame(0x700, b"\x00")
    except Nmea2000Error:
        pass
    else:
        raise AssertionError("standard CAN identifier was accepted by the NMEA 2000 frame boundary")
