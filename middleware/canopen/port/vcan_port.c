/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
/* SocketCAN implementation of middleware/canopen/port/can_port.h. */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include "can_port.h"

#ifdef CAN_PORT_SOCKETCAN

#include <errno.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

static int s_socket = -1;
static can_port_rx_callback_t s_rx_callback;

static const char *
can_port_interface(void) {
    const char *value = getenv("CAN_PORT_IFACE");
    return (value != NULL && value[0] != '\0') ? value : "vcan0";
}

int
can_port_init(uint32_t bitrate) {
    struct ifreq ifr = {0};
    struct sockaddr_can address = {0};
    const char *interface_name = can_port_interface();
    int enable = 1;

    (void)bitrate; /* vcan has no bit-rate controller. */
    if (s_socket >= 0) {
        return -EALREADY;
    }
    if (strlen(interface_name) >= sizeof(ifr.ifr_name)) {
        return -ENAMETOOLONG;
    }

    s_socket = socket(PF_CAN, SOCK_RAW | SOCK_CLOEXEC, CAN_RAW);
    if (s_socket < 0) {
        return -errno;
    }
    (void)setsockopt(s_socket, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &enable, sizeof(enable));

    (void)strncpy(ifr.ifr_name, interface_name, sizeof(ifr.ifr_name) - 1U);
    if (ioctl(s_socket, SIOCGIFINDEX, &ifr) < 0) {
        int result = -errno;
        can_port_deinit();
        return result;
    }

    address.can_family = AF_CAN;
    address.can_ifindex = ifr.ifr_ifindex;
    if (bind(s_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        int result = -errno;
        can_port_deinit();
        return result;
    }
    return 0;
}

int
can_port_send(uint32_t id, uint8_t *data, uint8_t len) {
    struct can_frame frame = {0};
    ssize_t sent;

    if (s_socket < 0) {
        return -ENODEV;
    }
    if (data == NULL || len > CAN_PORT_MAX_DLC || id > 0x7FFU) {
        return -EINVAL;
    }

    frame.can_id = id;
    frame.len = len;
    memcpy(frame.data, data, len);
    sent = write(s_socket, &frame, sizeof(frame));
    return sent == (ssize_t)sizeof(frame) ? 0 : (sent < 0 ? -errno : -EIO);
}

void
can_port_register_rx(can_port_rx_callback_t cb) {
    s_rx_callback = cb;
}

int
can_port_poll(uint32_t timeout_ms) {
    struct pollfd descriptor = {.fd = s_socket, .events = POLLIN, .revents = 0};
    struct can_frame frame = {0};
    int ready;
    ssize_t received;

    if (s_socket < 0) {
        return -ENODEV;
    }
    ready = poll(&descriptor, 1U, timeout_ms > (uint32_t)INT32_MAX ? INT32_MAX : (int)timeout_ms);
    if (ready == 0) {
        return 0;
    }
    if (ready < 0) {
        return errno == EINTR ? 0 : -errno;
    }
    if ((descriptor.revents & POLLIN) == 0) {
        return -EIO;
    }
    received = read(s_socket, &frame, sizeof(frame));
    if (received != (ssize_t)sizeof(frame)) {
        return received < 0 ? -errno : -EIO;
    }
    if ((frame.can_id & (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_ERR_FLAG)) != 0U || frame.len > CAN_PORT_MAX_DLC) {
        return -EPROTO;
    }
    if (s_rx_callback != NULL) {
        s_rx_callback(frame.can_id & CAN_SFF_MASK, frame.data, frame.len);
    }
    return 1;
}

void
can_port_deinit(void) {
    if (s_socket >= 0) {
        (void)close(s_socket);
    }
    s_socket = -1;
    s_rx_callback = NULL;
}

#else

#error "vcan_port.c requires CAN_PORT_SOCKETCAN"

#endif
