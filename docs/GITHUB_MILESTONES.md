# GitHub Milestones Guide & Release Engineering Practices

This document defines how GitHub Milestones are structured, tracked, and automated within the `stm32_uds_iso_tp` project to satisfy ISO 14229-1 (UDS), ISO 15765-2 (ISO-TP), and ASPICE engineering traceability standards.

---

## 1. Overview: What is a GitHub Milestone?

In GitHub, a **Milestone** is a project management and release-tracking mechanism that groups related Issues and Pull Requests toward a specific delivery target, due date, or strategic objective.

Milestones allow embedded firmware teams to:
1. **Track Release Completeness**: Monitor overall progress with an automated visual burndown indicator (Open vs. Closed issues).
2. **Enforce ASPICE / ISO 26262 Traceability**: Ensure that every feature, bug fix, or driver port maps directly to a versioned release gate.
3. **Automate Changelog & Release Notes Generation**: Automatically collect and categorize all closed deliverables under a target release.

---

## 2. Three Milestone Archetypes Implemented in This Repo

We have selected and implemented **3 representative milestone examples** directly in the GitHub repository:

| Milestone | Title | Type | Status | Progress | Key Target |
|:---:|---|:---:|:---:|:---:|---|
| **#1** | `v1.1.0 - Modular UDS Diagnostic Services & Core Stack` | **Version Release** | **CLOSED** | 100% (26/26) | ISO 14229-1 Service modularization, session jump FSM, DTC engine |
| **#2** | `v1.2.0 - Multi-MCU Hardware Abstraction & CAN-FD Transport` | **Platform Porting** | **CLOSED** | 100% (12/12) | STM32F1/F7/C0 HAL abstraction, 64-byte CAN-FD, RX ISR ring buffer |
| **#3** | `v2.0.0 - Automotive Cybersecurity & FOTA Bootloader Architecture` | **Strategic Epic** | **OPEN** | Active Roadmap | AES-CMAC128 (0x27), Auth (0x29), NVM wear-leveling, Dual-Bank FOTA |

---

### Example 1: Semantic Version Release Milestone
- **Milestone #1**: `v1.1.0 - Modular UDS Diagnostic Services & Core Stack`
- **Live URL**: [https://github.com/mahdi-benhassen/stm32_uds_iso_tp/milestone/1](https://github.com/mahdi-benhassen/stm32_uds_iso_tp/milestone/1)
- **Archetype Purpose**: Time-boxed release tracking all software features, bug fixes, and protocol compliance issues required for a minor version tag.
- **Scope & Deliverables**:
  - Modular service dispatcher architecture: `svc_diag_session.c`, `svc_read_did.c`, `svc_write_did.c`, `svc_clear_dtc.c`, `svc_tester_present.c`, `svc_ecu_reset.c`, `svc_io_control.c`, `svc_dyn_did.c`, `svc_periodic_did.c`, `svc_link_ctrl.c`.
  - ISO 14229-1 DiagnosticSessionControl (0x10) jump state machine with S3 server timer supervision.
  - ReadDTCInformation (0x19) sub-functions 0x01, 0x02, 0x03, 0x04, 0x06, 0x14 (Fault Detection Counter), and 0x15 (Permanent Status).
  - ClearDiagnosticInformation (0x14) across all memory blocks and default session enablement.
  - Rate-limited superloop integration API (`uds_server_poll_ms`).
- **Associated Issues / PRs**:
  - PRs: [#152](https://github.com/mahdi-benhassen/stm32_uds_iso_tp/pull/152), [#151](https://github.com/mahdi-benhassen/stm32_uds_iso_tp/pull/151), [#150](https://github.com/mahdi-benhassen/stm32_uds_iso_tp/pull/150), [#149](https://github.com/mahdi-benhassen/stm32_uds_iso_tp/pull/149)
  - Issues: #148, #146, #145, #143, #139, #138, #136, #134, #133, #131, #130, #129, #128, #127, #126, #125, #121, #119, #118, #115, #61, #55.

---

### Example 2: Platform & Hardware Porting Milestone
- **Milestone #2**: `v1.2.0 - Multi-MCU Hardware Abstraction & CAN-FD Transport`
- **Live URL**: [https://github.com/mahdi-benhassen/stm32_uds_iso_tp/milestone/2](https://github.com/mahdi-benhassen/stm32_uds_iso_tp/milestone/2)
- **Archetype Purpose**: Subsystem and cross-hardware validation milestone focusing on driver decoupling, target MCU ports, and bus transport scalability.
- **Scope & Deliverables**:
  - Generic CAN and CAN-FD hardware abstraction layer (HAL) decoupling protocol logic from register-level drivers.
  - Target family support:
    - **STM32F767ZI**: Dual bxCAN controller with hardware filter banks.
    - **STM32F103xB/xC**: Classic bxCAN integration with optimized RAM footprint.
    - **STM32C092 / STM32G0**: Bosch M_CAN / FDCAN peripheral driver.
  - ISO 15765-2 CAN-FD enhancements: 64-byte payload frames, extended 29-bit CAN IDs, and non-blocking RX ISR ring buffer queues.
- **Associated Issues / PRs**:
  - Issues: #53, #57, #70, #83, #85, #90, #91, #94, #105, #123, #137, #147.

---

### Example 3: Strategic Architectural & Cybersecurity Epic Milestone
- **Milestone #3**: `v2.0.0 - Automotive Cybersecurity & FOTA Bootloader Architecture`
- **Live URL**: [https://github.com/mahdi-benhassen/stm32_uds_iso_tp/milestone/3](https://github.com/mahdi-benhassen/stm32_uds_iso_tp/milestone/3)
- **Archetype Purpose**: Forward-looking roadmap and epic milestone tracking major architectural advancements, automotive security standards (ISO 21434), and production bootloader integration.
- **Scope & Deliverables**:
  - **SecurityAccess (0x27)**: Level 1 (4-byte PRNG seed/key) and Level 2 (16-byte AES-CMAC-128 cryptographic challenge-response) with attempt lockout counters.
  - **Authentication (0x29)**: Asymmetric certificate exchange, digital signature verification, and proof of ownership.
  - **NVM Flash Balancing Engine**: Wear-leveling algorithm for DTC history and dynamic DID persistence across Flash pages.
  - **Dual-Bank Bootloader**:
    - Single-bank bootloader fallback with vector table relocation.
    - Dual-bank A/B slot swap with CRC32 / SHA-256 integrity checks and automatic rollback protection.
  - **Transfer FSM**: `uds_transfer_fsm.c` non-blocking Flash erase/write state machine for Services 0x34, 0x36, 0x37.
- **Associated Issues / PRs**:
  - Open Roadmap: [#153 (Service 0x29 Certificate Exchange)](https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/153), [#154 (Dual-Bank A/B Bootloader Swap)](https://github.com/mahdi-benhassen/stm32_uds_iso_tp/issues/154).
  - Completed Milestones Components: #67, #68, #69, #74, #76, #79, #84, #88, #92, #113, #120, #124, #132, #135, #140, #141, #144.

---

## 3. How to Use & Manage Milestones

### Method A: Using the Repository CLI (`tools/github_milestone_manager.py`)

This repository provides an automated milestone manager script:

```bash
# 1. List all milestones and their progress bar
python tools/github_milestone_manager.py list

# 2. View a detailed report of issues in a milestone
python tools/github_milestone_manager.py report --milestone 3

# 3. Generate a Markdown Release Changelog directly from a milestone
python tools/github_milestone_manager.py changelog --milestone 1

# 4. Check if a Pull Request is assigned to a milestone (used in CI)
python tools/github_milestone_manager.py check-pr --pr 152

# 5. Assign an issue or PR to a milestone
python tools/github_milestone_manager.py assign --milestone 3 --issue 153
```

### Method B: Using GitHub Web UI
1. Navigate to **Issues** or **Pull Requests** > **Milestones**.
2. Click **New Milestone** to create a release target with a name, due date, and detailed description.
3. On any Issue or Pull Request page, use the right sidebar **Milestone** dropdown to link the item.

### Method C: Using GitHub CLI (`gh`)
```bash
# View milestones
gh milestone list

# Create a new milestone
gh milestone create --title "v2.1.0 - Automotive Ethernet DoIP" --description "ISO 13400-2 Diagnostics over IP"

# Assign an issue to a milestone
gh issue edit 153 --milestone "v2.0.0 - Automotive Cybersecurity & FOTA Bootloader Architecture"
```

---

## 4. Continuous Integration & Quality Gate

Every Pull Request must be associated with a target milestone before it can be merged into `main`.

- **Workflow**: [`.github/workflows/milestone-checker.yml`](../.github/workflows/milestone-checker.yml)
- **PR Template**: [`.github/pull_request_template.md`](../.github/pull_request_template.md) enforces specifying the target milestone.

When a milestone reaches **100% completion** and all qualification tests pass (Unit tests, Sanitizers, STM32 cross-compilation, and HIL loopback), the release engineer closes the milestone and tags the release.
