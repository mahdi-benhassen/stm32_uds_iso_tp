from test_nmea2000_gateway import (
    test_battery_status_encoding,
    test_extended_id_round_trip,
    test_gateway_mapping_and_bounds,
    test_invalid_standard_id_rejected,
)


def main():
    tests = [
        test_extended_id_round_trip,
        test_battery_status_encoding,
        test_gateway_mapping_and_bounds,
        test_invalid_standard_id_rejected,
    ]
    for test in tests:
        test()
        print(f"{test.__name__}: PASS")
    print(f"NMEA 2000 gateway contract: {len(tests)} tests passed")


if __name__ == "__main__":
    main()
