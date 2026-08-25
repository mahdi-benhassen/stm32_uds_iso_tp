#!/usr/bin/env bash
# SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
set -euo pipefail

INTERFACE="${1:-vcan0}"
if [[ ! "$INTERFACE" =~ ^[A-Za-z0-9_.-]{1,15}$ ]]; then
    echo "Invalid vcan interface name: $INTERFACE" >&2
    exit 64
fi
if ! command -v ip >/dev/null 2>&1; then
    echo "iproute2 is required; install the 'ip' command before creating vcan." >&2
    exit 69
fi
if ip link show "$INTERFACE" >/dev/null 2>&1; then
    ip link set "$INTERFACE" up
    echo "$INTERFACE is already available."
    exit 0
fi

if command -v modprobe >/dev/null 2>&1; then
    modprobe vcan 2>/dev/null || true
fi
ip link add dev "$INTERFACE" type vcan
ip link set "$INTERFACE" up
echo "Created $INTERFACE."
