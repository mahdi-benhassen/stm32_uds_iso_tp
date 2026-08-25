# UDS HIL Testing

`tests/hardware/run_uds_stm32f767_acceptance.py` is a safety-gated SocketCAN runner. It uses an independent CAN interface and defaults to non-destructive checks. It does not configure the interface, alter termination, erase Flash, or reset the ECU unless the operator explicitly supplies the corresponding option.

The acceptance inventory covers UDS session control, ECU reset, ReadDTCInformation, ReadDataByIdentifier, SecurityAccess, CommunicationControl policy, RoutineControl policy, RequestDownload policy, TransferData sequencing, RequestTransferExit policy, and TesterPresent. The underlying ISO-TP runner covers SF, FF, CF, FC, BS, STmin, sequence errors, timeout, and overflow behavior. The CANopen coexistence campaign must exercise NMT, heartbeat, SDO, PDO, and EMCY concurrently with UDS traffic.

For every request, capture timestamp, CAN ID, DLC, data, response time, NRC or positive response, test name, and verdict. The runner exports machine-readable JSON, CSV, and a human-readable report when the corresponding output paths are supplied. These files are evidence containers; they do not turn a dry run into physical HIL evidence.

A real HIL campaign must record firmware SHA, board serial and revision, MCU package, clock, CAN bitrate and sample point, transceiver, termination, bus load, supply, temperature, operator, tool versions, and recovery result. Host dry runs and fake-HAL tests are not substitutes for this evidence.
