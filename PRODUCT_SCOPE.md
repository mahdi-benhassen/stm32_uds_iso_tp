# Product Scope

## Purpose

This repository is an engineering reference for an independent ISO 15765-2 ISO-TP transport and ISO 14229 UDS protocol stack. It provides fixed-storage protocol state machines, generic CAN/CAN-FD frame callbacks, STM32 adapter contracts, a minimal STM32F767 bxCAN application, host tests, HIL inventory tooling, and build/analysis infrastructure. It is not a finished vehicle ECU, bootloader, safety product, or production update system.

## Supported areas

| Area | Repository behavior | Boundary |
|---|---|---|
| Classical CAN | 8-byte SF/FF/CF/FC framing and bounded reassembly | Physical transceiver, wiring, timing margin, and board behavior require HIL |
| CAN FD | Valid CAN-FD data lengths through 64 bytes, Single-Frame escape, extended First-Frame length above 4,095 bytes, and BRS metadata | A concrete FDCAN board project and physical CAN-FD campaign are not included |
| UDS | Callback-based service dispatch, bounded request/response storage, DID registry, SecurityAccess provider seam, and download-policy seam | Application policy, keys, Flash behavior, reset handoff, and authentication remain product-owned |
| STM32F767 | Generated HAL/CMSIS target with bxCAN and diagnostic IDs `0x7E0`/`0x7E8` | The F767 target is Classical CAN only because bxCAN is not CAN FD |
| FDCAN-capable STM32 | Public adapter contract carrying actual data length and BRS | Vendor-generated clocks, GPIO, message RAM, filters, linker files, and board HIL must be supplied |
| Host validation | CTest, sanitizers, coverage instrumentation, formatting, clang-tidy, cppcheck, architecture checks, and HIL dry runs | Software evidence does not prove electrical interoperability or formal conformance |

## Explicit exclusions

The repository does not contain or require CANopen, an object dictionary, network-management services, profile-specific device behavior, gateway protocols, or unrelated industrial protocol stacks. It also does not claim secure boot, authenticated firmware activation, rollback/anti-rollback, production cryptography, manufacturing qualification, EMC qualification, or formal ISO certification.

## Evidence rule

A host test, sanitizer run, coverage report, cross-build, or adapter-contract test is software evidence only. A product release must additionally record the exact MCU and board, HAL and compiler revisions, transceiver, nominal/data bit rates, message-RAM configuration, filters, termination, wiring, captured traces, timing measurements, environmental conditions, security review, and applicable conformance evidence.

## Scope-change rule

A new addressing format, UDS service, transport feature, target family, security provider, or Flash policy requires an API review, implementation change, negative-test matrix, documentation update, and corresponding CI or hardware evidence. It must not be introduced through an unrelated build flag or by reintroducing a protocol dependency.
