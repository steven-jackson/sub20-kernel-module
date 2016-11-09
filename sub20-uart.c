/*
 * Dimax SUB-20 UART Driver
 *
 * Copyright 2016 Steven Jackson <sj@oscode.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>

static int sub20_uart_probe(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver sub20_uart_driver = {
	.probe = sub20_uart_probe,
	.driver = {
		.name = "sub20-uart",
	},
};

module_platform_driver(sub20_uart_driver);
