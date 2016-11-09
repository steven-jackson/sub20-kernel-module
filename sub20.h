/*
 * Copyright 2016 Steven Jackson <sj@oscode.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __MFD_SUB20_H
#define __MFD_SUB20_H

#include <linux/kernel.h>
#include <linux/platform_device.h>

/* packet codes */
#define SUB20_GPIO_CONFIG	0x60
#define SUB20_GPIO_READ		0x61
#define SUB20_GPIO_WRITE	0x62
#define SUB20_UART_CONFIG	0x68

struct sub20_gpio_request {
	__le32 mask;
	__le32 value;
} __packed;

/* ucsra bits */
#define SUB20_UART_RX_ENABLE	0x80
#define SUB20_UART_TX_ENABLE	0x40
#define SUB20_UART_CHAR_5	0
#define SUB20_UART_CHAR_6	2
#define SUB20_UART_CHAR_7	4
#define SUB20_UART_CHAR_8	6
#define SUB20_UART_CHAR_9	7
#define SUB20_UART_PARITY_NONE	0
#define SUB20_UART_PARITY_EVEN	0x20
#define SUB20_UART_PARITY_ODD	0x30
#define SUB20_UART_STOP_1	0
#define SUB20_UART_STOP_2	8

struct sub20_uart_config {
	u8 ucsra;
	__le16 baud;
} __packed;

#define SUB20_PACKET_SIZE		256
#define SUB20_GPIO_RESPONSE_SIZE	4

struct sub20_packet {
	/* this is set by sub20's usb_transaction */
	u8 packet_size;
	u8 code;
	/* set this to the size of the used struct in the union */
	u8 size;
	union {
		struct sub20_gpio_request gpio_request;
		__le32 gpio_response;
		struct sub20_uart_config uart_config;
		/* TODO: check this */
		uint8_t padding[SUB20_PACKET_SIZE - 1];
	} u;
} __packed;

enum sub20_devices {
	SUB20_LCD,
	SUB20_IR,
	SUB20_ADC,
	SUB20_I2C,
	SUB20_UART,
	SUB20_MDC,
	SUB20_MDIO,
	SUB20_PWM,

	SUB20_DEVICE_COUNT
};

struct sub20 {
	struct usb_device *usb_dev;
	struct platform_device pdev;
	struct device *dev;
	struct mutex lock;
	struct gpio_chip gc;
	u32 gpio_config_cache;
	u32 reserved_gpios;

	int (*usb_transaction)(struct sub20 *board,
			       struct sub20_packet *request,
			       struct sub20_packet *response);
};

#endif
