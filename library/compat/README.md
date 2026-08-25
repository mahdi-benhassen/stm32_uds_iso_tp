# Compatibility migration area

`legacy_diagnostics/` contains the inherited CubeMX runtime ABI and its original host-test sources after migration out of the former top-level diagnostics directory. These files are retained temporarily so the copied firmware snapshot and its existing application wrapper remain buildable while the wrapper is migrated.

This directory is **not authoritative** and is excluded from the standalone `library/CMakeLists.txt` target and standalone CI. New ISO-TP/UDS fixes belong only in `library/include/uds_iso_tp/` and `library/src/`. The compatibility tree can be deleted after the inherited application is switched to the standalone endpoint and its original CI/test paths are migrated.
