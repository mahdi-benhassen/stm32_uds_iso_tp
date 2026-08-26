# UDS Service Matrix

This matrix separates generic protocol behavior from application backend availability and validation evidence. **Target Build** means the STM32F767 ARM GCC firmware build unless otherwise stated; it does not mean Keil Arm Compiler 6. **HIL** is marked only when physical evidence exists.

| SID | Service | Parser | Validation | Backend | Reference Backend | Unit Test | Integration Test | Target Build | HIL | Production Status |
|---:|---|---|---|---|---|---|---|---|---|---|
| `0x10` | DiagnosticSessionControl | Implemented | Length, subfunction, session transition | Not required for core | None | Pass | Endpoint coverage | Pass ARM GCC | Not executed | IMPLEMENTED |
| `0x11` | ECUReset | Implemented | Five types, suppress bit, deferred completion | Platform executor required | C092 hard/soft policy only | Pass | Endpoint ordering coverage | Pass ARM GCC | Not executed | PROTOCOL ONLY |
| `0x14` | ClearDiagnosticInformation | Implemented | Exact length, group range delegated | `UdsClearDtcFn` required | DTC fixture clear-all | Pass | DTC clear/read sequence | Pass ARM GCC | Not executed | BACKEND REQUIRED |
| `0x19` | ReadDTCInformation | Partial generic parser | Subfunction/length/capability | `UdsDtcBackend` required | Deterministic three-record fixture | Pass | Fixture filter/snapshot/extended/clear sequence | Pass ARM GCC | Not executed | BACKEND REQUIRED |
| `0x22` | ReadDataByIdentifier | Implemented | DID and response bounds delegated | DID read callback required | Test DID callback | Pass | Endpoint DID/multi-frame coverage | Pass ARM GCC | Not executed | IMPLEMENTED |
| `0x23` | ReadMemoryByAddress | Backend-pass-through parser | Common session gate and optional memory preflight; address/size semantics backend-owned | Memory backend required | None | Selector/preflight only | Not complete | Source-integrated | Not executed | PROTOCOL ONLY |
| `0x24` | ReadScalingDataByIdentifier | Backend-pass-through parser | Common session/address gate | DID extension backend required | None | Selector only | Not complete | Source-integrated | Not executed | PROTOCOL ONLY |
| `0x27` | SecurityAccess | Implemented | Level, sequence, seed lifetime, attempts, lockout | Seed/key callbacks required | Reference provider and CMAC helper | Pass | Session/lockout/endpoint coverage | Pass ARM GCC | Not executed | BACKEND REQUIRED |
| `0x28` | CommunicationControl | Implemented | Length, subfunction, callback result, suppress bit | Communication callback required | Test callback | Pass | Endpoint callback coverage | Pass ARM GCC | Not executed | BACKEND REQUIRED |
| `0x29` | Authentication | Backend-pass-through parser | Common session/address gate only | Authentication state machine required | None | Selector only | Not complete | Source-integrated | Not executed | PROTOCOL ONLY |
| `0x2A` | ReadDataByPeriodicIdentifier | Backend-pass-through parser | Common gate and nonzero queue bound | Bounded scheduler required | None | Selector/preflight only | Not complete | Source-integrated | Not executed | PROTOCOL ONLY |
| `0x2C` | DynamicallyDefineDataIdentifier | Backend-pass-through parser | Common session/address gate only | DID dynamic registry required | None | Selector only | Not complete | Source-integrated | Not executed | PROTOCOL ONLY |
| `0x2E` | WriteDataByIdentifier | Backend-pass-through parser | Common session/address gate only | DID write descriptors required | None | Selector only | Not complete | Source-integrated | Not executed | PROTOCOL ONLY |
| `0x31` | RoutineControl | Implemented | Length, routine ID, response bounds | Routine callback required | Test callback | Pass | Endpoint coverage | Pass ARM GCC | Not executed | BACKEND REQUIRED |
| `0x34` | RequestDownload | Implemented | Address/length encoding, block bound, session | Download callback required | Test callback | Pass | ISO-TP multi-frame coverage | Pass ARM GCC | Not executed | BACKEND REQUIRED |
| `0x35` | RequestUpload | Backend-pass-through parser | Common programming/physical gate only | Transfer backend required | None | Selector only | Not complete | Source-integrated | Not executed | PROTOCOL ONLY |
| `0x36` | TransferData | Implemented for download | Active transfer, block counter, size, callback | Transfer callback required | Test callback | Pass | Multi-frame sequence coverage | Pass ARM GCC | Not executed | BACKEND REQUIRED |
| `0x37` | RequestTransferExit | Implemented for download | Active transfer and response bounds | Transfer-exit callback required | Test callback | Pass | Transfer sequence coverage | Pass ARM GCC | Not executed | BACKEND REQUIRED |
| `0x38` | RequestFileTransfer | Backend-pass-through parser | Common programming/physical gate only | File-transfer backend required | None | Selector only | Not complete | Source-integrated | Not executed | PROTOCOL ONLY |
| `0x3D` | WriteMemoryByAddress | Backend-pass-through parser | Common programming/physical gate and optional memory preflight | Memory backend required | None | Selector/preflight only | Not complete | Source-integrated | Not executed | PROTOCOL ONLY |
| `0x3E` | TesterPresent | Implemented | Exact length, subfunction, suppress bit | Not required | None | Pass | 1,000-request regression | Pass ARM GCC | Not executed | IMPLEMENTED |
| `0x83` | AccessTimingParameter | Backend-pass-through parser | Common physical/session gate | Timing backend required; bounds backend-owned | Test timing callback | Selector/route pass | Not complete | Source-integrated | Not executed | PROTOCOL ONLY |
| `0x84` | SecuredDataTransmission | Backend-pass-through parser | Common physical/session gate only | Secured-data semantics required | None | Selector only | Not complete | Source-integrated | Not executed | PROTOCOL ONLY |
| `0x85` | ControlDTCSetting | Implemented | Length, subfunction, suppress bit | DTC-setting callback required | Test callback | Pass | Endpoint callback coverage | Pass ARM GCC | Not executed | BACKEND REQUIRED |
| `0x86` | ResponseOnEvent | Backend-pass-through parser | Common physical/session gate and nonzero queue bound | Bounded event registry required | None | Selector/preflight only | Not complete | Source-integrated | Not executed | PROTOCOL ONLY |
| `0x87` | LinkControl | Backend-pass-through parser | Common physical/session gate only | Physical link-control backend required | None | Selector only | Not complete | Source-integrated | Not executed | PROTOCOL ONLY |

## Status definitions

**IMPLEMENTED** means generic protocol behavior is implemented and host/integration tests cover the stated boundary. **BACKEND REQUIRED** means generic handling exists but application callbacks or data policy are necessary for useful operation. **PROTOCOL ONLY** means the current implementation provides routing and/or an integration contract but not the service-specific parser/state machine requested by the review prompt. **IMPLEMENTED + HIL VERIFIED** is intentionally unused because no physical HIL evidence exists.

## References

[1]: https://uds.readthedocs.io/en/latest/pages/user_guide/message_translation.html#readdtcinformation "py-uds ReadDTCInformation service translation documentation"
