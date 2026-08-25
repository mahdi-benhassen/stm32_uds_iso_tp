# Security Policy

## Scope

This repository is a reference firmware and development baseline. It is not a secure-boot implementation, a field-update system, a safety certificate, or a complete CAN network security solution. The default gateway is disabled and the default board hooks keep application outputs in a safe state.

Security issues include vulnerabilities that can affect code execution, memory safety, CAN message handling, unauthorized configuration, diagnostic access, gateway authorization, debug access, build provenance, or release artifact integrity.

## Supported versions

Security fixes are applied to the current `main` branch. Consumers should pin a reviewed commit or release tag and maintain their own product backport policy.

| Version | Support status |
|---|---|
| `main` | Supported for active development |
| Tagged releases | Supported according to the release notes |
| Unmodified historical commits | Not actively supported |

## Reporting a vulnerability

Do not disclose an unpatched vulnerability in a public issue. Report it privately through the repository owner’s GitHub security contact or the private communication channel configured for this repository. Include:

- A concise description and affected commit or release.
- The affected personality, board assumptions, and build configuration.
- Reproduction steps or a minimal proof of concept that does not damage hardware.
- The expected and observed behavior.
- Any proposed mitigation and whether the issue is publicly known.

The maintainer should acknowledge receipt, assess severity and affected versions, coordinate a fix, and publish release notes after a mitigation is available. Do not include credentials, private keys, customer data, or uncontrolled exploit payloads in the report.

## Security boundaries

The following controls are required before deploying a product derived from this reference:

- Replace the reference vendor identity, product code, revision, and serial number.
- Restrict LSS, SDO writes, diagnostics, and commissioning access to a controlled physical or authenticated boundary.
- Keep the CiA 309 gateway disabled unless a product-specific authentication, authorization, rate-limit, audit, and session policy has been reviewed.
- Define a secure-boot and signed-update architecture if firmware updates are supported.
- Lock or control SWD/JTAG access according to the manufacturing and service threat model.
- Validate CAN bus-off recovery, malformed-frame handling, watchdog behavior, power-loss behavior, and safe outputs on the target board.
- Track third-party revisions and license notices through `THIRD_PARTY.md` and the build manifest.

The CAN bus provides no confidentiality or built-in authorization. Physical access, a compromised node, or an exposed diagnostic connector can allow message injection, configuration changes, denial of service, or unauthorized state transitions unless the product adds appropriate controls.

## Release response

A security release should identify the affected commit range, severity, configuration gate, mitigation, validation evidence, and any required firmware or hardware update. Retain the exact firmware artifact hash and dependency manifest used for the release.
