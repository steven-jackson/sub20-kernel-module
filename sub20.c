/*
 * Dimax SUB-20 Driver
 *
 * Copyright 2016 Steven Jackson <sj@oscode.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mfd/core.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/usb.h>

#include "sub20.h"

#define SUB20_VID		0x4d8
#define SUB20_PID		0xffc3
/* TODO: is it possible to get these at run time? */
#define SUB20_OUT_EP		1
#define SUB20_IN_EP		0x82
#define SUB20_USB_TIMEOUT_MS	100

#define SUB20_GPIO_COUNT		32

/* alternative gpio functions */
#define SUB20_GPIO_LCD_D0		BIT(0)
#define SUB20_GPIO_LCD_D1		BIT(1)
#define SUB20_GPIO_LCD_D2		BIT(2)
#define SUB20_GPIO_LCD_D3		BIT(3)
#define SUB20_GPIO_IR_TX		BIT(4)
#define SUB20_GPIO_LCD_EN		BIT(6)
#define SUB20_GPIO_LCD_RS		BIT(7)
#define SUB20_GPIO_I2C_SCL		BIT(8)
#define SUB20_GPIO_I2C_SDA		BIT(9)
#define SUB20_GPIO_RS232_RXD_TTL	BIT(10)
#define SUB20_GPIO_RS232_TXD_TTL	BIT(11)
#define SUB20_GPIO_MDC			BIT(14)
#define SUB20_GPIO_MDIO			BIT(15)
#define SUB20_GPIO_ADC0			BIT(16)
#define SUB20_GPIO_ADC1			BIT(17)
#define SUB20_GPIO_ADC2			BIT(18)
#define SUB20_GPIO_ADC3			BIT(19)
#define SUB20_GPIO_ADC4			BIT(20)
#define SUB20_GPIO_ADC5			BIT(21)
#define SUB20_GPIO_ADC6			BIT(22)
#define SUB20_GPIO_ADC7			BIT(23)
#define SUB20_GPIO_PWM0			BIT(24)
#define SUB20_GPIO_PWM1			BIT(25)
#define SUB20_GPIO_PWM2			BIT(26)
#define SUB20_GPIO_PWM3			BIT(27)
#define SUB20_GPIO_PWM4			BIT(28)
#define SUB20_GPIO_PWM5			BIT(29)
#define SUB20_GPIO_PWM6			BIT(30)
#define SUB20_GPIO_PWM7			BIT(31)

/* TODO: use the alternative functions definitions */
#define SUB20_LCD_GPIOS		0x000000cf
#define SUB20_IR_GPIOS		0x00000010
#define SUB20_ADC_GPIOS		0x00ff0000
#define SUB20_I2C_GPIOS		0x00000300
#define SUB20_UART_GPIOS	0x00000c00
#define SUB20_MDC_GPIOS		0x00004000
#define SUB20_MDIO_GPIOS	0x00008000
#define SUB20_PWM_GPIOS		0xff000000

static int sub20_transaction(struct sub20 *board,
			     struct sub20_packet *request,
			     struct sub20_packet *response)
{
	int ret, actual;

	/*
	 * TODO:
	 * - do I sanitize input?
	 * - asynchronous out
	 * - allow NULL response, but this will require looking up the
	 * - response size and providing a preallocated buffer
	 * - does usb_sndbulkpipe / usbrcv need to be called each time?
	 */

	/*
	 * packet_size is set to total size - 1, so + 2 accounts for the code
	 * and size fields
	 */
	request->packet_size = request->size + 2;

	/*
	 * packet_size is set to at least 64, + 3 presumably accounts for
	 * packet_size, code and size
	 */
	response->packet_size = max(response->size + 3, 64);

	mutex_lock(&board->lock);

	ret = usb_bulk_msg(board->usb_dev,
			   usb_sndbulkpipe(board->usb_dev, SUB20_OUT_EP),
			   request, request->packet_size + 1, &actual,
			   SUB20_USB_TIMEOUT_MS);

	if (ret) {
		dev_err(board->dev, "failed to send packet 0x%0x: %d\n",
			request->code, ret);
		goto exit;
	}

	ret = usb_bulk_msg(board->usb_dev,
			   usb_rcvbulkpipe(board->usb_dev, SUB20_IN_EP),
			   response, response->packet_size, &actual,
			   SUB20_USB_TIMEOUT_MS);

	if (ret) {
		dev_err(board->dev, "failed to receive packet for 0x%x: %d\n",
			request->code, ret);
		goto exit;
	}

exit:
	mutex_unlock(&board->lock);
	return ret;
}

/* this can work on all gpios at once, so value is per gpio */
static int sub20_gpio_config(struct gpio_chip *chip, u32 value,
			     u32 mask)
{
	int ret;
	struct sub20 *board = container_of(chip, struct sub20, gc);
	struct sub20_packet response = {
		.size = SUB20_GPIO_RESPONSE_SIZE,
	};
	struct sub20_packet request = {
		.code = SUB20_GPIO_CONFIG,
		.size = sizeof(struct sub20_gpio_request),
		.u.gpio_request = {
			.mask = cpu_to_le32(mask),
			.value = cpu_to_le32(value),
		},
	};

	ret = sub20_transaction(board, &request, &response);
	if (ret)
		return ret;

	board->gpio_config_cache = le32_to_cpu(response.u.gpio_response);

	return 0;
}

static int sub20_gpio_get_direction(struct gpio_chip *chip, unsigned int offset)
{
	struct sub20 *board = container_of(chip, struct sub20, gc);

	if (board->gpio_config_cache & (1u << offset))
		return GPIOF_DIR_OUT;

	return GPIOF_DIR_IN;
}

static int sub20_gpio_direction_input(struct gpio_chip *chip,
				      unsigned int offset)
{
	return sub20_gpio_config(chip, 0, 1u << offset);
}

static int sub20_gpio_direction_output(struct gpio_chip *chip,
				       unsigned int offset, int value)
{
	const u32 mask = 1u << offset;
	int ret;

	ret = sub20_gpio_config(chip, mask, mask);
	if (ret)
		return ret;

	chip->set(chip, offset, value);

	return 0;
}

static int sub20_gpio_get(struct gpio_chip *chip, unsigned int offset)
{
	int ret;
	const u32 mask = 1u << offset;
	struct sub20 *board = container_of(chip, struct sub20, gc);
	struct sub20_packet response = {
		.size = SUB20_GPIO_RESPONSE_SIZE,
	};
	struct sub20_packet request = {
		.code = SUB20_GPIO_READ,
		.size = sizeof(struct sub20_gpio_request),
		.u.gpio_request = {
			.mask = cpu_to_le32(mask),
		},
	};

	ret = sub20_transaction(board, &request, &response);
	if (ret)
		return ret;

	return !!(le32_to_cpu(response.u.gpio_response) & mask);
}

static void sub20_gpio_set(struct gpio_chip *chip, unsigned int offset,
			   int value)
{
	struct sub20 *board = container_of(chip, struct sub20, gc);
	struct sub20_packet response = {
		.size = SUB20_GPIO_RESPONSE_SIZE,
	};
	struct sub20_packet request = {
		.code = SUB20_GPIO_WRITE,
		.size = sizeof(struct sub20_gpio_request),
		.u.gpio_request = {
			.mask = cpu_to_le32(1u << offset),
			.value = cpu_to_le32(value << offset),
		},
	};

	sub20_transaction(board, &request, &response);
}

/**
 * Attempt to reserve or free the gpio for internal use.
 *
 * Returns 0 on success.
 */
static int sub20_reserve_gpio(struct sub20 *board, unsigned int offset,
			      bool reserve)
{
	int ret;

	if (!!(BIT(offset) & board->reserved_gpios) == reserve)
		return 0;

	if (reserve) {
		ret = devm_gpio_request(board->dev, offset, "reserved");
		if (ret)
			return ret;
	} else
		devm_gpio_free(board->dev, offset);

	board->reserved_gpios ^= BIT(offset);

	return 0;
}

static int sub20_reserve_gpios(struct sub20 *board, u32 mask, u32 value)
{
	int offset;
	bool set, err, ret = 0;
	u32 modified_gpios;

	for (offset = 0; offset < SUB20_GPIO_COUNT; ++offset) {
		set = !!(value & BIT(offset));
		err = sub20_reserve_gpio(board, offset, set);
		if (err)
			ret = err;
	}

	/* roll back modified gpios on failure */
	if (ret) {
		for (offset = 0; offset < SUB20_GPIO_COUNT; ++offset) {
			if (modified_gpios & BIT(offset))
				sub20_reserve_gpio(board, offset, false);
		}
	}

	return ret;
}

static int sub20_uart_config(struct sub20 *board, u8 config, u16 baud)
{
	int ret;
	u32 gpio_mask, gpio_value = 0;
	struct sub20_packet response;
	struct sub20_packet request = {
		.code = SUB20_UART_CONFIG,
		.size = sizeof(struct sub20_uart_config),
		.u.uart_config = {
			.ucsra = config,
			.baud = cpu_to_le16(baud),
		},
	};

	gpio_mask = SUB20_GPIO_RS232_RXD_TTL | SUB20_GPIO_RS232_TXD_TTL;

	if (config & SUB20_UART_RX_ENABLE)
		gpio_value |= SUB20_GPIO_RS232_RXD_TTL;

	if (config & SUB20_UART_TX_ENABLE)
		gpio_value |= SUB20_GPIO_RS232_TXD_TTL;

	ret = sub20_reserve_gpios(board, gpio_mask, gpio_value);
	if (ret)
		return ret;

	/* TODO: cache the config */
	ret = sub20_transaction(board, &request, &response);
	if (ret)
		return ret;

	return 0;
}

static int sub20_uart_enable(struct sub20 *board)
{
	/* TODO: update rather than overwrite */
	return sub20_uart_config(board, SUB20_UART_RX_ENABLE, 0);
}

static int sub20_uart_disable(struct sub20 *board)
{
	/* TODO: update rather than overwrite */
	return sub20_uart_config(board, 0, 0);
}

static const struct usb_device_id sub20_table[] = {
	{ USB_DEVICE(SUB20_VID, SUB20_PID) },
	{ }
};
MODULE_DEVICE_TABLE(usb, sub20_table);

static const struct mfd_cell sub20_devs[] = {
	{
		.name = "sub20-uart",
	},
};

static int sub20_probe(struct usb_interface *interface,
		       const struct usb_device_id *id)
{
	struct sub20 *board;
	int ret;

	board = kzalloc(sizeof(*board), GFP_KERNEL);
	if (board == NULL)
		return -ENOMEM;

	mutex_init(&board->lock);

	board->dev = &interface->dev;
	board->usb_dev = usb_get_dev(interface_to_usbdev(interface));
	board->usb_transaction = sub20_transaction;

	sub20_uart_disable(board);

	usb_set_intfdata(interface, board);

	board->gc.label = "SUB-20 GPIO";
	board->gc.owner = THIS_MODULE;
	board->gc.get_direction = sub20_gpio_get_direction;
	board->gc.direction_input = sub20_gpio_direction_input;
	board->gc.direction_output = sub20_gpio_direction_output;
	board->gc.get = sub20_gpio_get;
	board->gc.set = sub20_gpio_set;
	board->gc.base = -1;
	board->gc.ngpio = SUB20_GPIO_COUNT;
	board->gc.can_sleep = true;

	ret = gpiochip_add(&board->gc);
	if (ret < 0) {
		dev_err(board->dev, "failed to add the gpio chip");
		return ret;
	}

	/*
	 * populate the config cache in case the device hasn't been reset since
	 * last use
	 */
	ret = sub20_gpio_config(&board->gc, 0, 0);
	if (ret)
		/* TODO: does disconnect get called on fail probe? */
		return 0;


	ret = mfd_add_hotplug_devices(board->dev, sub20_devs,
				      ARRAY_SIZE(sub20_devs));
	if (ret != 0) {
		dev_err(board->dev, "failed to add mfd devices to core");
		goto error;
	}

	ret = sub20_uart_config(board, 0, 0);
	if (ret)
		dev_warn(board->dev, "failed to disable uart");

	return 0;

error:
	if (board) {
		usb_put_dev(board->usb_dev);
		kfree(board);
	}

	return ret;
}

static void sub20_disconnect(struct usb_interface *interface)
{
	struct sub20 *board = usb_get_intfdata(interface);

	mfd_remove_devices(&interface->dev);
	gpiochip_remove(&board->gc);
	usb_set_intfdata(interface, NULL);
	usb_put_dev(board->usb_dev);
	kfree(board);
}

static struct usb_driver sub20_driver = {
	.name		= "sub20",
	.probe		= sub20_probe,
	.disconnect	= sub20_disconnect,
	.id_table	= sub20_table,
};
module_usb_driver(sub20_driver);

MODULE_DESCRIPTION("Dimax SUB-20 board mfd");
MODULE_AUTHOR("Steven Jackson <sj@oscode.net>");
MODULE_LICENSE("GPL");
