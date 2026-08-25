# Authoritative UDS / ISO-TP library

The authoritative standalone implementation is contained in `include/uds_iso_tp/` and `src/`. It is the only implementation built by `library/CMakeLists.txt` and the only implementation covered by the standalone CI workflow. It is independent of CANopenNode and supports Classical CAN and CAN FD through injected frame callbacks.

The `compat/legacy_diagnostics/` directory is a temporary migration area for the inherited CubeMX firmware’s old ABI and host contracts. It is not part of the standalone library target, is not a source for new features, and must not be used as the reference implementation. It remains only until the inherited application wrapper is migrated to the endpoint API.
