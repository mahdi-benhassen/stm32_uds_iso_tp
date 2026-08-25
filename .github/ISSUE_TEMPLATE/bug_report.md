---
name: Bug report
about: Report a reproducible firmware, build, protocol, or documentation problem
title: "[bug] "
labels: bug
assignees: ""
---

## Summary

Describe the problem in one paragraph.

## Environment

- Commit or release:
- MCU and exact board/package:
- Toolchain and version:
- STM32CubeF7 revision:
- CAN bitrate and node-ID:
- Build personality:

## Reproduction

List the exact commands, CAN frames, hardware setup, or test inputs needed to reproduce the issue.

## Expected behavior

Describe the expected result, timing, state, or frame sequence.

## Observed behavior

Describe the actual result and include relevant logs, traces, assertions, or reset information. Remove credentials and private data.

## Validation

- [ ] I ran the deterministic host tests.
- [ ] I ran the affected firmware personality build.
- [ ] I attached a minimal trace or log when the issue involves CAN or timing.
- [ ] I confirmed this is not a duplicate issue.
