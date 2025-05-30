/*
 * Copyright (c) 2025 Jiri Svoboda
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

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_VOL_H
#define LIBDEVICE_VOL_H

#include <async.h>
#include <errno.h>
#include <loc.h>
#include <stdint.h>
#include <types/label.h>
#include <types/vol.h>

extern errno_t vol_create(vol_t **);
extern void vol_destroy(vol_t *);
extern errno_t vol_get_parts(vol_t *, service_id_t **, size_t *);
extern errno_t vol_part_add(vol_t *, service_id_t);
extern errno_t vol_part_info(vol_t *, service_id_t, vol_part_info_t *);
extern errno_t vol_part_eject(vol_t *, service_id_t, vol_eject_flags_t);
extern errno_t vol_part_empty(vol_t *, service_id_t);
extern errno_t vol_part_insert(vol_t *, service_id_t);
extern errno_t vol_part_insert_by_path(vol_t *, const char *);
extern errno_t vol_part_get_lsupp(vol_t *, vol_fstype_t, vol_label_supp_t *);
extern errno_t vol_part_mkfs(vol_t *, service_id_t, vol_fstype_t, const char *,
    const char *);
extern errno_t vol_part_set_mountp(vol_t *, service_id_t, const char *);
extern errno_t vol_get_volumes(vol_t *, volume_id_t **, size_t *);
extern errno_t vol_info(vol_t *, volume_id_t, vol_info_t *);
extern errno_t vol_fstype_format(vol_fstype_t, char **);
extern errno_t vol_pcnt_fs_format(vol_part_cnt_t, vol_fstype_t, char **);
extern errno_t vol_mountp_validate(const char *);
extern errno_t vol_part_by_mp(vol_t *, const char *, service_id_t *);

#endif

/** @}
 */
