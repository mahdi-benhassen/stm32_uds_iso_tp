# v1 Security Threat Model and Release Gate

## Scope decision

The current reference product does not claim secure boot, signed field updates, or a production gateway security architecture. Those mechanisms must not be implied by the CANopen stack integration. They become implementation requirements only if the product requires field firmware updates or exposed gateway services.

## Threat areas

| Area | Review questions | Required decision/evidence |
|---|---|---|
| CAN attack surface | Can an unauthenticated node issue NMT, SDO, LSS, PDO, or EMCY traffic? | Approved threat model and mitigation/risk acceptance |
| SDO abuse | Are write-capable objects bounded, rate-limited, and safe under malformed/rapid requests? | Negative tests and product policy |
| LSS abuse | Can node-ID/bitrate changes be restricted during production operation? | Commissioning and lock policy |
| Malformed frames | Do invalid DLC, COB-ID, mapping, and sequence inputs remain fail-closed? | Host and physical robustness evidence |
| Debug port | What is the development-to-production debug lifecycle? | Approved programming/debug policy |
| RDP and option bytes | Which STM32 protection level and option bytes are used? | Signed manufacturing record |
| Secrets | Are any credentials, keys, or calibration secrets present? | Secret inventory and custody decision |
| Manufacturing | Who provisions identity/configuration and verifies security settings? | Production traveler and audit record |
| Vulnerability handling | How are reports received, triaged, fixed, and disclosed? | Maintainer/security contact and process |

## Release gate

Before a production label, the security owner must sign the threat model, debug/RDP/option-byte policy, manufacturing security record, and vulnerability-reporting process. If secure update is required, add a separate design for an immutable trust anchor, signed image format, key custody and rotation, anti-rollback, recovery image, failed-update recovery, and audit logging. Until those approvals exist, the product claim remains a software-validated reference and not a secure-update product.
