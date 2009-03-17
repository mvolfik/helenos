/*
 * Copyright (c) 2009 Jakub Jermar
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

/** @addtogroup sparc64mm
 * @{
 */
/** @file
 */

#include <arch/mm/page.h>
#include <arch/mm/tlb.h>
#include <genarch/mm/page_ht.h>
#include <mm/frame.h>
#include <arch/mm/frame.h>
#include <bitops.h>
#include <debug.h>
#include <align.h>
#include <config.h>

/** Perform sparc64 specific initialization of paging. */
void page_arch_init(void)
{
	if (config.cpu_active == 1)
		page_mapping_operations = &ht_mapping_operations;
}

/** Map memory-mapped device into virtual memory.
 *
 * We are currently using identity mapping for mapping device registers.
 *
 * @param physaddr	Physical address of the page where the device is
 * 			located.
 * @param size		Size of the device's registers. This argument is
 * 			ignored.
 *
 * @return		Virtual address of the page where the device is mapped.
 */
uintptr_t hw_map(uintptr_t physaddr, size_t size)
{
	return PA2KA(physaddr);
}

/** @}
 */
