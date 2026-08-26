# Modular UDS Service Backends

The generic dispatcher now exposes explicit service-group backends instead of a catch-all application callback. The groups share one bounded handler signature, but each group has a separate field and a separate service-family mapping. The library owns UDS framing, service attributes, session/address checks, response bounds, and NRC translation. The application owns device-specific memory, identifiers, transfer policy, timing, event, authentication, and secured-data behavior.

| Group | Service identifiers | Backend field | Current contract |
|---|---:|---|---|
| Memory | `0x23`, `0x3D` | `UdsMemoryServiceBackend` | Application validates address, size, permissions, and persistence. |
| DID extension | `0x24`, `0x2C` | `UdsDidServiceBackend` | Application owns scaling records and dynamic DID storage. |
| Transfer extension | `0x35`, `0x38` | `UdsTransferServiceBackend` | Application owns upload/file semantics and bounded transfer state. |
| Timing | `0x83` | `UdsTimingServiceBackend` | Application owns accepted timing profiles and reconfiguration. |
| Periodic/events | `0x2A`, `0x86` | `UdsPeriodicEventServiceBackend` | Application owns bounded scheduling, event queues, and emission. |
| Link control | `0x87` | `UdsLinkControlServiceBackend` | Application owns the controller/channel transition. |
| Authentication | `0x29` | `UdsAuthenticationServiceBackend` | Application owns credentials, certificates, and authorization policy. |
| Secured data | `0x84` | `UdsSecuredDataServiceBackend` | Application owns cryptographic framing and replay/freshness policy. |

A service identifier may be recognized by the generic attribute table while still returning `0x11` when its backend is absent. This is intentional: protocol routing and backend availability are separate facts. A configured backend is called with caller-supplied request and response buffers; it must validate service-specific lengths and keep response writes within the supplied capacity. The generic dispatcher rejects a reported response length that exceeds that capacity and translates callback errors through the existing NRC mapping.

This layer does not pretend to implement device behavior that the repository cannot provide. It does not allocate, block, sleep, access HAL/CMSIS/registers, or create unbounded queues. Periodic and event services remain application-owned because their timing source, queue limits, cancellation policy, and transmit-completion guarantees depend on the target integration. Likewise, memory access, authentication, and secured-data processing remain disabled until an application supplies and reviews an appropriate backend.

The contract test verifies all service-group mappings and the absent-backend result. The DTC backend is intentionally separate in `uds_dtc.h`: `0x19` capabilities and `0x14` clear operations have diagnostic-record-specific contracts rather than being folded into the generic service-group selector.
