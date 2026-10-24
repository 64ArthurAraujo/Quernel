#pragma once
#include <quernel/config.h>

int printk(const char *fmt, ...);

/* Dummy printk to toggle the visibility of logs */
#define no_printk(fmt, ...) ()

#if QUER_INFORMATIONAL
/**
 * pr_info - Print an info-level message
 * @fmt: format string
 * @...: arguments for the format string
 * 
 */
#define pr_info(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define pr_info(fmt, ...) no_printk(fmt, ##__VA_ARGS__)
#endif

#if QUER_DEBUG
/**
 * pr_debug - Print a debug-level message
 * @fmt: format string
 * @...: arguments for the format string
 *
 */
#define pr_debug(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...) no_printk(fmt, ##__VA_ARGS__)
#endif

#if QUER_ERR
/**
 * pr_err - Print an error-level message
 * @fmt: format string
 * @...: arguments for the format string
 */
#define pr_err(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define pr_err(fmt, ...) no_printk(fmt, ##__VA_ARGS__)
#endif