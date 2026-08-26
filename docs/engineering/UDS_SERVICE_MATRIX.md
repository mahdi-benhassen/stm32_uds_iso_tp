# UDS Service Matrix

This matrix distinguishes what the generic library can parse and route from what requires an application backend and what requires target hardware evidence. “Backend contract” means the API boundary exists and the dispatcher enforces bounded invocation; it does not mean a device implementation is supplied.

| SID | Service | Generic protocol path | Backend/configuration status | Hardware/HIL status |
|---:|---|---|---|---|
| `0x10` | DiagnosticSessionControl | Implemented with session transitions and timing state | No device backend required for core behavior | Not run on target |
| `0x11` | ECUReset | Implemented for reset types `0x01`–`0x05`, deferred completion | C092 policy supports `0x01` and `0x03`; application owns execution | HIL reset not run |
| `0x14` | ClearDiagnosticInformation | Implemented length check and positive/NRC path | `UdsClearDtcFn` required; no storage bundled | Not run on target |
| `0x19` | ReadDTCInformation | Subfunction/length/capability validation and legacy-compatible dispatch | `UdsDtcBackend` required for structured reporting; no records bundled | Not run on target |
| `0x22` | ReadDataByIdentifier | Implemented through DID callback/registry | Application DID data required | Not run on target |
| `0x23` | ReadMemoryByAddress | Modular backend route | Memory backend and secure-region preflight required | Not run on target |
| `0x24` | ReadScalingDataByIdentifier | Modular backend route | DID-extension backend required | Not run on target |
| `0x27` | SecurityAccess | Implemented callback-driven seed/key state and lockout | AES-CMAC helper available; algorithm and secret remain application-owned | Not run on target |
| `0x28` | CommunicationControl | Implemented through callback | Application controls communications | Not run on target |
| `0x29` | Authentication | Modular backend route | Authentication backend required | Not run on target |
| `0x2A` | ReadDataByPeriodicIdentifier | Modular backend route | Bounded periodic/event backend required; no queue is hidden in library | Not run on target |
| `0x2C` | DynamicallyDefineDataIdentifier | Modular backend route | DID-extension backend required | Not run on target |
| `0x2E` | WriteDataByIdentifier | Modular backend route | Existing DID write/data policy must be supplied by application integration | Not run on target |
| `0x31` | RoutineControl | Implemented through routine callback | Application routine registry required | Not run on target |
| `0x34` | RequestDownload | Implemented with bounded download state | Application validates address/length and supplies block policy | Not run on target |
| `0x35` | RequestUpload | Modular backend route | Transfer-extension backend required | Not run on target |
| `0x36` | TransferData | Implemented for active download state | Application transfer callback required | Not run on target |
| `0x37` | RequestTransferExit | Implemented for active download state | Application exit callback required | Not run on target |
| `0x38` | RequestFileTransfer | Modular backend route | Transfer-extension backend required | Not run on target |
| `0x3D` | WriteMemoryByAddress | Modular backend route | Memory backend and secure-region preflight required | Not run on target |
| `0x3E` | TesterPresent | Implemented, including suppress-positive-response | No device backend required for core response | Not run on target |
| `0x83` | AccessTimingParameter | Modular backend route | Timing backend required | Not run on target |
| `0x84` | SecuredDataTransmission | Modular backend route | Secured-data backend and product cryptographic policy required | Not run on target |
| `0x85` | ControlDTCSetting | Implemented through callback | Application DTC-setting policy required | Not run on target |
| `0x86` | ResponseOnEvent | Modular backend route | Bounded periodic/event backend required | Not run on target |
| `0x87` | LinkControl | Modular backend route | Link-control backend required | Not run on target |

The project intentionally does not claim that “all 27 services” are device-complete merely because their identifiers are recognized. The generic architecture supplies stable, bounded integration seams for the service families whose semantics depend on the ECU product. Unsupported or unconfigured groups return a negative response rather than silently pretending to operate.
