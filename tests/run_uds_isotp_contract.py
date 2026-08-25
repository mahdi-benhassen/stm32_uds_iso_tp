from test_uds_isotp import (
    test_multi_frame_round_trip_and_flow_control,
    test_node_filters_request_id_and_returns_isotp_response,
    test_sequence_mismatch_is_rejected,
    test_single_frame_round_trip,
    test_uds_services_and_negative_responses,
)


def main():
    tests = [
        test_single_frame_round_trip,
        test_multi_frame_round_trip_and_flow_control,
        test_sequence_mismatch_is_rejected,
        test_uds_services_and_negative_responses,
        test_node_filters_request_id_and_returns_isotp_response,
    ]
    for test in tests:
        test()
        print(f"{test.__name__}: PASS")
    print(f"UDS/ISO-TP contract: {len(tests)} tests passed")


if __name__ == "__main__":
    main()
