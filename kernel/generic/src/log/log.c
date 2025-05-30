/*
 * Copyright (c) 2013 Martin Sucha
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#include <abi/log.h>
#include <arch.h>
#include <atomic.h>
#include <console/console.h>
#include <ddi/ddi.h>
#include <ddi/irq.h>
#include <errno.h>
#include <ipc/event.h>
#include <ipc/irq.h>
#include <log.h>
#include <panic.h>
#include <print.h>
#include <printf_core.h>
#include <stdarg.h>
#include <stdlib.h>
#include <str.h>
#include <synch/spinlock.h>
#include <syscall/copy.h>
#include <sysinfo/sysinfo.h>
#include <typedefs.h>

#define LOG_PAGES    8
#define LOG_LENGTH   (LOG_PAGES * PAGE_SIZE)
#define LOG_ENTRY_HEADER_LENGTH (sizeof(size_t) + sizeof(uint32_t))

/** Cyclic buffer holding the data for kernel log */
static uint8_t log_buffer[LOG_LENGTH] __attribute__((aligned(PAGE_SIZE)));

/** Kernel log initialized */
static atomic_bool log_inited = false;

/** Position in the cyclic buffer where the first log entry starts */
static size_t log_start = 0;

/** Sum of length of all log entries currently stored in the cyclic buffer */
static size_t log_used = 0;

/** Log spinlock */
static IRQ_SPINLOCK_INITIALIZE(log_lock);

/** Overall count of logged messages, which may overflow as needed */
static uint32_t log_counter = 0;

/** Starting position of the entry currently being written to the log */
static size_t log_current_start = 0;

/** Length (including header) of the entry currently being written to the log */
static size_t log_current_len = 0;

/** Start of the next entry to be handed to uspace starting from log_start */
static size_t next_for_uspace = 0;

static void log_update(void *);

/** Initialize kernel logging facility
 *
 */
void log_init(void)
{
	event_set_unmask_callback(EVENT_KLOG, log_update);
	atomic_store(&log_inited, true);
}

static size_t log_copy_from(uint8_t *data, size_t pos, size_t len)
{
	for (size_t i = 0; i < len; i++, pos = (pos + 1) % LOG_LENGTH) {
		data[i] = log_buffer[pos];
	}
	return pos;
}

static size_t log_copy_to(const uint8_t *data, size_t pos, size_t len)
{
	for (size_t i = 0; i < len; i++, pos = (pos + 1) % LOG_LENGTH) {
		log_buffer[pos] = data[i];
	}
	return pos;
}

/** Append data to the currently open log entry.
 *
 * This function requires that the log_lock is acquired by the caller.
 */
static void log_append(const uint8_t *data, size_t len)
{
	/* Cap the length so that the entry entirely fits into the buffer */
	if (len > LOG_LENGTH - log_current_len) {
		len = LOG_LENGTH - log_current_len;
	}

	if (len == 0)
		return;

	size_t log_free = LOG_LENGTH - log_used - log_current_len;

	/* Discard older entries to make space, if necessary */
	while (len > log_free) {
		size_t entry_len;
		log_copy_from((uint8_t *) &entry_len, log_start, sizeof(size_t));
		log_start = (log_start + entry_len) % LOG_LENGTH;
		log_used -= entry_len;
		log_free += entry_len;
		next_for_uspace -= entry_len;
	}

	size_t pos = (log_current_start + log_current_len) % LOG_LENGTH;
	log_copy_to(data, pos, len);
	log_current_len += len;
}

/** Begin writing an entry to the log.
 *
 * This acquires the log and output buffer locks, so only calls to log_* functions should
 * be used until calling log_end.
 */
void log_begin(log_facility_t fac, log_level_t level)
{
	console_lock();
	irq_spinlock_lock(&log_lock, true);
	irq_spinlock_lock(&kio_lock, true);

	log_current_start = (log_start + log_used) % LOG_LENGTH;
	log_current_len = 0;

	/* Write header of the log entry, the length will be written in log_end() */
	log_append((uint8_t *) &log_current_len, sizeof(size_t));
	log_append((uint8_t *) &log_counter, sizeof(uint32_t));
	uint32_t fac32 = fac;
	uint32_t lvl32 = level;
	log_append((uint8_t *) &fac32, sizeof(uint32_t));
	log_append((uint8_t *) &lvl32, sizeof(uint32_t));

	log_counter++;
}

/** Finish writing an entry to the log.
 *
 * This releases the log and output buffer locks.
 */
void log_end(void)
{
	/* Set the length in the header to correct value */
	log_copy_to((uint8_t *) &log_current_len, log_current_start, sizeof(size_t));
	log_used += log_current_len;

	kio_push_bytes("\n", 1);
	irq_spinlock_unlock(&kio_lock, true);
	irq_spinlock_unlock(&log_lock, true);

	/* This has to be called after we released the locks above */
	kio_flush();
	kio_update(NULL);
	log_update(NULL);
	console_unlock();
}

static void log_update(void *event)
{
	if (!atomic_load(&log_inited))
		return;

	irq_spinlock_lock(&log_lock, true);
	if (next_for_uspace < log_used)
		event_notify_0(EVENT_KLOG, true);
	irq_spinlock_unlock(&log_lock, true);
}

static int log_printf_str_write(const char *str, size_t size, void *data)
{
	kio_push_bytes(str, size);
	log_append((const uint8_t *)str, size);
	return EOK;
}

/** Append a message to the currently being written entry.
 *
 * Requires that an entry has been started using log_begin()
 */
int log_vprintf(const char *fmt, va_list args)
{
	printf_spec_t ps = {
		log_printf_str_write,
		NULL
	};

	return printf_core(fmt, &ps, args);
}

/** Append a message to the currently being written entry.
 *
 * Requires that an entry has been started using log_begin()
 */
int log_printf(const char *fmt, ...)
{
	int ret;
	va_list args;

	va_start(args, fmt);
	ret = log_vprintf(fmt, args);
	va_end(args);

	return ret;
}

/** Log a message to the kernel log.
 *
 * This atomically appends a log entry.
 * The resulting message should not contain a trailing newline, as the log
 * entries are explicitly delimited when stored in the log.
 */
int log(log_facility_t fac, log_level_t level, const char *fmt, ...)
{
	int ret;
	va_list args;

	log_begin(fac, level);

	va_start(args, fmt);
	ret = log_vprintf(fmt, args);
	va_end(args);

	log_end();

	return ret;
}

/** Control of the log from uspace
 *
 */
sys_errno_t sys_klog(sysarg_t operation, uspace_addr_t buf, size_t size,
    sysarg_t level, uspace_ptr_size_t uspace_nread)
{
	char *data;
	errno_t rc;

	if (size > PAGE_SIZE)
		return (sys_errno_t) ELIMIT;

	switch (operation) {
	case KLOG_WRITE:
		data = (char *) malloc(size + 1);
		if (!data)
			return (sys_errno_t) ENOMEM;

		rc = copy_from_uspace(data, buf, size);
		if (rc) {
			free(data);
			return (sys_errno_t) rc;
		}
		data[size] = 0;

		if (level >= LVL_LIMIT)
			level = LVL_NOTE;

		log(LF_USPACE, level, "%s", data);

		free(data);
		return EOK;
	case KLOG_READ:
		data = (char *) malloc(size);
		if (!data)
			return (sys_errno_t) ENOMEM;

		size_t entry_len = 0;
		size_t copied = 0;

		rc = EOK;

		irq_spinlock_lock(&log_lock, true);

		while (next_for_uspace < log_used) {
			size_t pos = (log_start + next_for_uspace) % LOG_LENGTH;
			log_copy_from((uint8_t *) &entry_len, pos, sizeof(size_t));

			if (entry_len > PAGE_SIZE) {
				/*
				 * Since we limit data transfer
				 * to uspace to a maximum of PAGE_SIZE
				 * bytes, skip any entries larger
				 * than this limit to prevent
				 * userspace being stuck trying to
				 * read them.
				 */
				next_for_uspace += entry_len;
				continue;
			}

			if (size < copied + entry_len) {
				if (copied == 0)
					rc = EOVERFLOW;
				break;
			}

			log_copy_from((uint8_t *) (data + copied), pos, entry_len);
			copied += entry_len;
			next_for_uspace += entry_len;
		}

		irq_spinlock_unlock(&log_lock, true);

		if (rc != EOK) {
			free(data);
			return (sys_errno_t) rc;
		}

		rc = copy_to_uspace(buf, data, size);

		free(data);

		if (rc != EOK)
			return (sys_errno_t) rc;

		return copy_to_uspace(uspace_nread, &copied, sizeof(copied));
		return EOK;
	default:
		return (sys_errno_t) ENOTSUP;
	}
}

/** @}
 */
