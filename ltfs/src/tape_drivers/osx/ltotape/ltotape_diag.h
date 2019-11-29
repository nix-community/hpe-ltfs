/************************************************************************************
**
**  Hewlett Packard LTFS backend for LTO and DAT tape drives
**
** FILE:            ltotape_diag.c
**
** CONTENTS:        Definitions and further header files for LTO diagnostic routines
**
** (C) Copyright 2015-2018 Hewlett Packard Enterprise Development LP
** (c) Copyright 2010, 2011 Quantum Corporation
**
** This program is free software; you can redistribute it and/or modify it
**  under the terms of version 2.1 of the GNU Lesser General Public License
**  as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, but 
**  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
**  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
**  License for more details.
**
** You should have received a copy of the GNU General Public License along
**  with this program; if not, write to:
**    Free Software Foundation, Inc.
**    51 Franklin Street, Fifth Floor
**    Boston, MA 02110-1301, USA.
**
**   26 April 2010
**
*************************************************************************************
**
** Copyright (C) 2012 OSR Open Systems Resources, Inc.
** 
************************************************************************************* 
*/

#ifndef __ltotape_diag_h
#define __ltotape_diag_h

#ifdef HPE_mingw_BUILD
/* 
 * OSR
 *
 * Our environment needs some fixups before ibm_tape can be
 * included
 */
# include "libltfs/arch/win/win_util.h"

# ifndef ushort
#  define ushort unsigned short
# endif
# ifndef uint
#  define uint unsigned int
# endif
# ifndef daddr_t
#  define daddr_t long
# endif
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "libltfs/tape_ops.h"
#include "libltfs/ltfslogging.h"
#include "libltfs/ltfs.h"

#ifndef HPE_mingw_BUILD
# define MAX_PATH 256  /* Maximum path length; Windows already defines this */
# ifdef __APPLE__
#  include "../../linux/ibmtape/IBM_tape.h"
# else
#  include "../ibmtape/IBM_tape.h" /* for some common definitions */
# endif
#endif

#include "ltotape_timeout.h"

/*
 *  Definitions
 */
#ifdef HPE_BUILD
# define HPLTFS_COPYRIGHT   "Portions (C) Copyright 2015, 2016 Hewlett Packard Enterprise Development LP"
#elif defined QUANTUM_BUILD
# define QTMLTFS_COPYRIGHT   "Portions copyright (c) 2010-2011 Quantum Corporation"
#endif

#define KB   (1024)
#define MB   (KB * 1024)
#define GB   (MB * 1024)

#define SNAPSHOT_LENGTH        (256 * KB)  /* Max log size we'll handle                      */
#define MAX_SNAPSHOT_RETRIES   10 /* wait up to 10s for the snapshot to become available -   */
                                  /*  see ltotape_read_snapshot() in ltotape_diag.c          */
#define MAX_RETAINED_SNAPSHOTS 10 /* Keep up to ten snapshots (older files will be deleted)  */

#define LINUX_LOGFILE_DIR      "/var/log"
#define MACOS_LOGFILE_DIR      "/var/tmp/ltfs"	/* This is used for dumping support tickets */

#define LTOTAPE_TIMESTAMP_TYPE_OFFSET    10
#define LTOTAPE_TIMESTAMP_OFFSET         12
#define LTOTAPE_LIBSN_OFFSET             52
#define LTOTAPE_LIBSN_LENGTH             32

#define LTOTAPE_CLEAR_ERRHIST_NEXUS      0xFF
#define LTOTAPE_RBMODE_ERRHIST           0x1C

#define SENDDIAG_BUF_LEN                 8
#define DIAG_TRIGGER_DUMP                0x60  /* actually 0x160 but we assume the 1 !  */
#define DIAG_TRIGGER_MINIDUMP            0x63  /* actually 0x163 but as above..         */

#define DUMP_HEADER_SIZE                 4
#define DUMP_TRANSFER_SIZE               (512 * KB)
#define MINI_DUMP_HEADER_SIZE            256
#define MINI_DUMP_TRANSFER_SIZE          (256 * KB)

/*
 * Function prototype for (public) function:
 */
int   ltotape_log_snapshot (void *device, int minidump);
char* ltotape_get_default_snapshotdir (void);
char* ltotape_set_snapshotdir (char* newdir);

#endif // __ltotape_diag_h
