#!/usr/bin/env python3
"""SocketCAN/vcan integration tests for the project CANopen wire contract."""
from __future__ import annotations

import os
import subprocess
import sys
import time
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HOST = ROOT / "tests" / "host"
sys.path.insert(0, str(HOST))
from can_socket import Frame, open_socket, receive, send  # noqa: E402

NODE_ID = 0x0A
DEVICE = ROOT / "middleware" / "canopen" / "examples" / "canopen_vcan_device.py"


def wait_for(sock, arbitration_id: int, timeout: float = 1.0) -> Frame:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        frame = receive(sock, max(0.0, deadline - time.monotonic()))
        if frame is not None and frame.arbitration_id == arbitration_id:
            return frame
    raise AssertionError(f"timed out waiting for CAN ID 0x{arbitration_id:03X}")


class VcanCanopenTest(unittest.TestCase):
    def setUp(self) -> None:
        self.interface = os.environ.get("CAN_PORT_IFACE", "vcan0")
        self.socket = open_socket(self.interface)
        self.device = subprocess.Popen(
            [sys.executable, str(DEVICE), "--interface", self.interface, "--node-id", hex(NODE_ID), "--emcy-on-start"],
            cwd=ROOT,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
        )
        # Device creation must not race the first client request.
        time.sleep(0.05)

    def tearDown(self) -> None:
        self.device.terminate()
        try:
            self.device.wait(timeout=2)
        except subprocess.TimeoutExpired:
            self.device.kill()
            self.device.wait(timeout=2)
        if self.device.returncode not in (0, -15):
            stderr = self.device.stderr.read() if self.device.stderr else ""
            self.fail(f"vcan CANopen device exited unexpectedly: {stderr}")
        self.socket.close()

    def sdo_upload(self, index: int, subindex: int = 0) -> bytes:
        send(self.socket, 0x600 + NODE_ID, bytes((0x40, index & 0xFF, index >> 8, subindex, 0, 0, 0, 0)))
        response = wait_for(self.socket, 0x580 + NODE_ID)
        self.assertNotEqual(response.data[0], 0x80, response.data.hex())
        command = response.data[0]
        if command & 0x02:
            length = 4 - ((command >> 2) & 0x03)
            return response.data[4 : 4 + length]
        self.assertEqual(command, 0x41, "expected segmented upload initialization with size")
        expected_length = int.from_bytes(response.data[4:8], "little")
        received = bytearray()
        toggle = 0
        while True:
            send(self.socket, 0x600 + NODE_ID, bytes((0x60 | toggle, 0, 0, 0, 0, 0, 0, 0)))
            segment = wait_for(self.socket, 0x580 + NODE_ID)
            self.assertEqual(segment.data[0] & 0x10, toggle)
            length = 7 - ((segment.data[0] >> 1) & 0x07)
            received.extend(segment.data[1 : 1 + length])
            if segment.data[0] & 0x01:
                break
            toggle ^= 0x10
        self.assertEqual(len(received), expected_length)
        return bytes(received)

    def sdo_download_u32(self, index: int, value: int, subindex: int = 0) -> None:
        send(
            self.socket,
            0x600 + NODE_ID,
            bytes((0x23, index & 0xFF, index >> 8, subindex)) + value.to_bytes(4, "little"),
        )
        response = wait_for(self.socket, 0x580 + NODE_ID)
        self.assertEqual(response.data[0], 0x60, response.data.hex())

    def test_sdo_expedited_read_identity_1018(self) -> None:
        value = self.sdo_upload(0x1018, 1)
        self.assertEqual(int.from_bytes(value, "little"), 0x12345678)

    def test_sdo_expedited_download_and_readback(self) -> None:
        self.sdo_download_u32(0x2000, 0x11223344)
        self.assertEqual(self.sdo_upload(0x2000), bytes.fromhex("44332211"))

    def test_sdo_segmented_upload(self) -> None:
        value = self.sdo_upload(0x2001)
        self.assertEqual(value, b"CANopenNode SocketCAN segmented regression payload.")

    def test_nmt_heartbeat_and_pdo_exchange(self) -> None:
        # See a pre-operational heartbeat, then enter operational state.
        heartbeat = wait_for(self.socket, 0x700 + NODE_ID)
        self.assertIn(heartbeat.data, (b"\x00", b"\x7f"))
        send(self.socket, 0x000, bytes((0x01, NODE_ID)))
        while True:
            heartbeat = wait_for(self.socket, 0x700 + NODE_ID)
            if heartbeat.data == b"\x05":
                break
        send(self.socket, 0x200 + NODE_ID, b"\x34\x12")
        send(self.socket, 0x080, b"")
        tpdo = wait_for(self.socket, 0x180 + NODE_ID)
        self.assertEqual(tpdo.data, b"\x34\x12")

    def test_nmt_standard_state_machine_transitions(self) -> None:
        self.assertIn(wait_for(self.socket, 0x700 + NODE_ID).data, (b"\x00", b"\x7f"))
        transitions = ((0x01, 0x05), (0x02, 0x04), (0x80, 0x7F))
        for command, expected_state in transitions:
            send(self.socket, 0x000, bytes((command, NODE_ID)))
            heartbeat = wait_for(self.socket, 0x700 + NODE_ID)
            self.assertEqual(heartbeat.data, bytes((expected_state,)))

        # Reset-node emits the CANopen boot-up heartbeat and returns to pre-op.
        send(self.socket, 0x000, bytes((0x81, NODE_ID)))
        bootup = wait_for(self.socket, 0x700 + NODE_ID)
        self.assertEqual(bootup.data, b"\x00")
        send(self.socket, 0x000, bytes((0x01, 0x00)))
        self.assertEqual(wait_for(self.socket, 0x700 + NODE_ID).data, b"\x05")

    def test_malformed_nmt_frame_does_not_change_state(self) -> None:
        self.assertIn(wait_for(self.socket, 0x700 + NODE_ID).data, (b"\x00", b"\x7f"))
        send(self.socket, 0x000, b"\x01")
        deadline = time.monotonic() + 0.2
        while time.monotonic() < deadline:
            frame = receive(self.socket, max(0.0, deadline - time.monotonic()))
            if frame is not None and frame.arbitration_id == 0x700 + NODE_ID:
                self.assertEqual(frame.data, b"\x7f")
                return
        self.fail("malformed NMT frame produced no state heartbeat")

    def test_sync_is_state_gated(self) -> None:
        self.assertIn(wait_for(self.socket, 0x700 + NODE_ID).data, (b"\x00", b"\x7f"))
        send(self.socket, 0x000, bytes((0x02, NODE_ID)))
        self.assertEqual(wait_for(self.socket, 0x700 + NODE_ID).data, b"\x04")
        send(self.socket, 0x080, b"")
        deadline = time.monotonic() + 0.05
        while time.monotonic() < deadline:
            frame = receive(self.socket, max(0.0, deadline - time.monotonic()))
            self.assertFalse(frame is not None and frame.arbitration_id == 0x180 + NODE_ID)

    def test_sdo_invalid_access_returns_canonical_abort(self) -> None:
        send(self.socket, 0x600 + NODE_ID, bytes((0x40, 0x99, 0x99, 0, 0, 0, 0, 0)))
        response = wait_for(self.socket, 0x580 + NODE_ID)
        self.assertEqual(response.data, bytes((0x80, 0x99, 0x99, 0)) + (0x06020000).to_bytes(4, "little"))

        send(self.socket, 0x600 + NODE_ID, bytes((0x23, 0x18, 0x10, 1, 0, 0, 0, 0)))
        response = wait_for(self.socket, 0x580 + NODE_ID)
        self.assertEqual(response.data, bytes((0x80, 0x18, 0x10, 1)) + (0x06010002).to_bytes(4, "little"))

    def test_dynamic_pdo_mapping_record_is_sdo_writable(self) -> None:
        send(self.socket, 0x600 + NODE_ID, bytes((0x2F, 0x00, 0x16, 0x00, 0x01, 0, 0, 0)))
        response = wait_for(self.socket, 0x580 + NODE_ID)
        self.assertEqual(response.data[0], 0x60)
        self.assertEqual(self.sdo_upload(0x1600, 0), b"\x01")

    def test_lss_fastscan_response(self) -> None:
        send(self.socket, 0x7E5, bytes((0x51, 0, 0, 0, 0, 0, 0, 0)))
        response = wait_for(self.socket, 0x7E4)
        self.assertEqual(response.data[0], 0x4F)

    def test_emcy_is_observable(self) -> None:
        emergency = wait_for(self.socket, 0x080 + NODE_ID)
        self.assertEqual(emergency.data[:3], bytes((0x10, 0x23, 0x10)))

    def test_sdo_block_transfer_is_enabled_in_firmware_configuration(self) -> None:
        # Wire-level block-transfer interoperability remains a hardware-target
        # test because this host harness deliberately models only the CI frame
        # contracts. This protects the build-time dependency configuration.
        configuration = (ROOT / "App" / "Inc" / "CO_driver_custom.h").read_text(encoding="utf-8")
        self.assertIn("CO_CONFIG_CRC16", configuration)
        self.assertIn("CO_CONFIG_SDO_SRV_BUFFER_SIZE 1024U", configuration)
        self.assertIn("CO_CONFIG_SDO_CLI_BUFFER_SIZE 1024U", configuration)


if __name__ == "__main__":
    unittest.main(verbosity=2)
