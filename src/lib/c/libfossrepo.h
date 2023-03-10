/*
 librep: A set of functions for accessing the file repository.

 SPDX-FileCopyrightText: © 2007-2011 Hewlett-Packard Development Company, L.P.

 SPDX-License-Identifier: LGPL-2.1-only
*/
/**
Repository config files:
/etc/fossology/
RepPath.conf # contains path to the mounted directory
# If it does not exist, then "." is used.
Depth.conf	# contains a number for the current depth
Hosts.conf	# list of hosts and hex ranges (used to find host with data)

The layout looks like:
host1/	# Each host has a directory. This can be mounted or local
host1/type/	# Type describes the repository (e.g., gold, file, license)
host1/type/00/	# Directories are lowercase octets (00 to ff)
host1/type/00/00/	# Directories are lowercase octets (00 to ff)
host1/type/00/00/sha1.md5.len	# Files are lowercase octets and digits
host2/	# Each host has a directory. This can be mounted or local
host2/type/	# Type describes the repository (e.g., gold, file, license)
host2/type/00/	# Directories are lowercase octets (00 to ff)
host2/type/00/00/	# Directories are lowercase octets (00 to ff)
**/

#ifndef LIBFOSSREPO_H
#define LIBFOSSREPO_H

#include <fossconfig.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <utime.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <grp.h>

#ifndef FOSSREPO_CONF
#define FOSSREPO_CONF "/srv/fossology/repository"
#endif
#ifndef FOSSGROUP
#define FOSSGROUP "fossology"
#endif


/* General Repository usage */
int fo_RepOpen();
/* call before using any other function */
void fo_RepClose();
/* call after using all other functions */
int fo_RepOpenFull(fo_conf* config);
/* agents should call fo_RepOpen() */
char* fo_RepValidate(fo_conf* config); /* checks the repo config */

/* Get info -- caller must free() returned string. */
char* fo_RepGetRepPath();
/* path to mounted repository */
char* fo_RepGetHost(char* Type, char* Filename);
char* fo_RepMkPath(const char* Type, char* Filename);

/* Not intended for external use */
int _RepMkDirs(char* Filename);

/* Sanity checks */
int fo_RepExist(char* Type, char* Filename);
int fo_RepExist2(char* Type, char* Filename);
int fo_RepHostExist(char* Type, char* Host);

/* Removal */
int fo_RepRemove(char* Type, char* Filename);

/* Replacements for fopen/fclose */
FILE* fo_RepFread(char* Type, char* Filename);
FILE* fo_RepFwrite(char* Type, char* Filename);
int fo_RepFclose(FILE* F);
int fo_RepImport(char* Source, char* Type, char* Filename, int HardLink);

/** Replacements for mmap */
struct RepMmapStruct
{
    int FileHandle;       ///< Handle from open()
    unsigned char* Mmap;  ///< Memory pointer from mmap
    uint32_t MmapSize;    ///< Size of file mmap
    uint32_t _MmapSize;   ///< Real size of mmap (set to page boundary)
};
typedef struct RepMmapStruct RepMmapStruct;
void fo_RepMunmap(RepMmapStruct* M);
RepMmapStruct* fo_RepMmap(char* Type, char* Filename);
RepMmapStruct* fo_RepMmapFile(char* FullFilename);

#endif
