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

The receiver sends Flow Control with configured block size and STmin. The transmitter exposes four explicit states: `IDLE`, `WAIT_FIRST_FLOW_CONTROL`, `SEND_CONSECUTIVE`, and `WAIT_BLOCK_FLOW_CONTROL`. CTS is accepted only in either FC-wait state, stores BS/STmin, and enters `SEND_CONSECUTIVE`; a non-zero BS limits the next CF block and BS zero means no block limit. WAIT preserves the current FC-wait state and increments a bounded counter. OVERFLOW aborts immediately. The maximum WAIT count is fixed by `ISOTP_DEFAULT_MAX_WAIT_FRAMES` or the per-link configuration; repeated WAIT frames cannot extend a transfer indefinitely.

Flow Control validation checks the expected request-side CAN ID, profile-valid frame DLC, minimum FC length, FC PCI type, and STmin encoding. Classical CAN uses a three-byte FC frame; CAN FD requires a valid CAN-FD DLC. STmin values `0x00–0x7F` and `0xF1–0xF9` are accepted, while reserved values are rejected. Malformed FC frames do not advance the state; invalid flow status, invalid STmin, and WAIT exhaustion reset the transmitter with `ISOTP_ERR_FLOW_CONTROL`, while an explicit peer OVERFLOW resets it with `ISOTP_ERR_FLOW_OVERFLOW`. Sequence mismatches and inter-frame deadline expiry reset the transfer deterministically.

## ISO-TP TX Flow Control Block Size

The FC byte sequence `30 03 00` means **CTS**, `BS=3`, and `STmin=0`. After transmitting the First Frame, the transmitter accepts this FC, sets its current block limit to three Consecutive Frames, and resets the per-block counter. The resulting trace is:

```text
FF
CF SN=1
CF SN=2
CF SN=3
(wait for FC; no fourth CF is emitted)
FC ...
CF SN=4
CF SN=5
CF SN=6
(wait for FC; no seventh CF is emitted)
```

`BS=0` means that no block-size boundary is imposed, so all remaining CFs are sent subject only to STmin, timeout, and completion rules. For `BS>0`, each successfully emitted CF increments the current block counter. When the counter reaches the received BS and data remains, the TX state becomes `WAIT_BLOCK_FLOW_CONTROL`. The First Frame and Flow Control frames are not counted as CFs, and a CF is counted only when `isotp_tx_next()` returns `ISOTP_TX_FRAME_READY`.

Each newly accepted CTS replaces the previous BS and resets the block counter to zero. Therefore, a peer can change BS between blocks without resetting the CF sequence number. A new CTS whose BS exceeds the remaining CF count does not create an unnecessary FC wait: the transfer completes after its final CF. The counter is also not consumed by WAIT frames; WAIT extends the bounded FC deadline while preserving the current block progress, and a subsequent CTS starts the next block with a reset counter. OVERFLOW aborts the active transfer immediately and emits no further CFs.

BS and STmin are independent controls. BS limits the number of CFs in one block; STmin limits the minimum elapsed time between CFs, including when BS is zero. The millisecond host clock models the `0xF1–0xF9` sub-millisecond encodings at one-millisecond resolution, so target-timer and analyzer measurements are required for sub-millisecond physical validation.

The dedicated regression coverage is in `library/tests/isotp/test_isotp.c`: `test_tx_fc_bs_zero`, `test_tx_fc_bs_one`, `test_tx_fc_bs_three`, `test_tx_fc_bs_larger_than_remaining`, `test_tx_fc_bs_changes_between_blocks`, `test_tx_fc_wait_does_not_consume_bs`, `test_tx_fc_cts_resets_bs_counter`, `test_tx_bs_preserves_sequence_number`, and `test_tx_bs_respects_stmin`. These tests assert emitted CF sequence numbers and FC-boundary states rather than checking completion alone.

The implementation intentionally excludes address-extension formats, mixed addressing, broadcast/functional policy, and every optional ISO-TP network-layer feature. Those require an explicit API and test matrix rather than being inferred from a CAN-FD DLC value.

## References

[1]: https://www.iso.org/standard/66574.html "ISO 15765-2:2016 — Road vehicles — Diagnostic communication over CAN — Part 2"
[2]: https://github.com/SimonCahill/isotp-c "Reference ISO-TP C implementation documenting Classical CAN and CAN-FD profiles"
