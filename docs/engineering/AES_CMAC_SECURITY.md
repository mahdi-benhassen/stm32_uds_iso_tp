# Reusable AES-CMAC-128 Security Utility

The repository now provides a dependency-free AES-128 and AES-CMAC-128 implementation under `library/crypto/`. `aes_cmac_128()` accepts a 16-byte key, an arbitrary-length message, and writes a 16-byte tag without dynamic allocation, heap use, delays, or mutable global state. The implementation is suitable for a bounded embedded callback, subject to the application enforcing its own execution-time and message-length policy.

## SecurityAccess boundary

`uds_security_cmac_derive_key()` is an application-side helper for deriving a 16-byte key from a caller-owned 16-byte master key and 16-byte seed. The generic UDS `0x27` service remains callback-driven: it does not select a cryptographic algorithm, embed a production secret, or retain key material. An application may call the helper from its `security_key` callback and compare the result with `uds_security_cmac_constant_time_equal()`.

The caller owns all key and seed storage and controls its lifetime. Keys must be provisioned, protected, erased, and rotated according to the product security design. No production key, certificate, provisioning format, or authorization policy is present in this repository. The helper does not provide freshness, replay protection, authorization, secure storage, side-channel resistance for the complete product, or a complete AUTOSAR/ISO 21434 security policy.

## Data contract

| Item | Contract |
|---|---|
| AES key | Exactly 16 bytes, AES-128 |
| CMAC tag | Exactly 16 bytes, in the standard CMAC byte order |
| Seed helper input | Exactly 16 bytes |
| Seed helper output | `CMAC(master_key, seed)` as a 16-byte derived key |
| Allocation | None; caller supplies buffers |
| Generic UDS coupling | None; integration occurs through application callbacks |
| Test evidence | RFC 4493 empty, 16-byte, 40-byte, and 64-byte known-answer tests plus the seed helper vector |

The test suite validates the AES-CMAC primitive against RFC 4493 known-answer vectors. These tests establish algorithmic correctness for the covered vectors; they do not constitute a product security assessment or hardware side-channel evaluation.
