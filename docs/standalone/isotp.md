# ISO-TP Classical CAN and CAN FD

The standalone transport supports the ISO-TP network-layer model over both Classical CAN and CAN FD. The selected `IsoTpConfig` is explicit; the frame metadata records whether the emitted frame is CAN FD and whether bit-rate switching is requested.

| Property | Classical CAN profile | CAN-FD profile |
|---|---:|---:|
| Data length | 0–8 bytes | 0–64 bytes using valid CAN-FD DLC lengths |
| Normal SF payload | 1–7 bytes | 1–7 bytes |
| CAN-FD SF escape | Not used | `PCI=0`, next byte is `SF_DL`, up to `TX_DL-2` bytes |
| Normal FF length | 12-bit, up to 4,095 | 12-bit, up to 4,095 |
| Extended FF length | Not used by the F767 profile | `FF_DL=0` followed by a 32-bit length |
| CF payload | Up to 7 bytes | Up to `TX_DL-1` bytes |
| BRS | Not available | Explicit `bit_rate_switch` metadata |

The supported CAN-FD data lengths are 8, 12, 16, 20, 24, 32, 48, and 64 bytes. Short CAN-FD Single Frames are padded to the next valid data length. Multi-frame CAN-FD frames use the configured `tx_dl`; reception accepts valid CAN-FD lengths and can also accept Classical CAN frames when the application policy permits mixed links.

For a First Frame longer than 4,095 bytes, the low FF length nibble and the following length byte are zero, and the next four bytes carry the 32-bit big-endian payload length. The default library limit is 16,384 bytes, but `ISOTP_MAX_PAYLOAD` is compile-time configurable. A length above the bound is rejected with `ISOTP_ERR_OVERFLOW` before copying payload bytes.

The receiver sends Flow Control with configured block size and STmin. A transmitter accepts CTS, bounded WAIT, and OVERFLOW. The maximum WAIT count is fixed by `ISOTP_DEFAULT_MAX_WAIT_FRAMES` or the per-link configuration; repeated WAIT frames cannot extend a transfer indefinitely. Sequence mismatches and inter-frame deadline expiry reset the transfer deterministically.

The implementation intentionally excludes address-extension formats, mixed addressing, broadcast/functional policy, and every optional ISO-TP network-layer feature. Those require an explicit API and test matrix rather than being inferred from a CAN-FD DLC value.

## References

[1]: https://www.iso.org/standard/66574.html "ISO 15765-2:2016 — Road vehicles — Diagnostic communication over CAN — Part 2"
[2]: https://github.com/SimonCahill/isotp-c "Reference ISO-TP C implementation documenting Classical CAN and CAN-FD profiles"
