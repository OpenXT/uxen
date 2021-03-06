/** @file
 * Shared folders: Mappings header.
 */

/*
 * Copyright (C) 2006-2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___MAPPINGS_H
#define ___MAPPINGS_H

#include "shfl.h"
#include <VBox/shflsvc.h>

#define QUOTA_INVALID 0xFFFFFFFFFFFFFFFF

typedef struct
{
    wchar_t        *pszFolderName;       /**< directory at the host to share with the guest */
    PSHFLSTRING pMapName;             /**< share name for the guest */
    wchar_t     *file_suffix;         /* file suffix for shared files, or NULL */
    uint32_t    cMappings;            /**< number of mappings */
    bool        fValid;               /**< mapping entry is used/valid */
    bool        fHostCaseSensitive;   /**< host file name space is case-sensitive */
    bool        fGuestCaseSensitive;  /**< guest file name space is case-sensitive */
    bool        fWritable;            /**< folder is writable for the guest */
    bool        fAutoMount;           /**< folder will be auto-mounted by the guest */
    bool        fSymlinksCreate;      /**< guest is able to create symlinks */
    uint64_t    opts;                 /**< other opt flags */
    uint64_t    quota_max, quota_cur; /**< folder quota in bytes */
} MAPPING;
/** Pointer to a MAPPING structure. */
typedef MAPPING *PMAPPING;

void vbsfMappingInit(void);

bool vbsfMappingQuery(uint32_t iMapping, PMAPPING *pMapping);

int vbsfMappingsAdd(PSHFLSTRING pFolderName, PSHFLSTRING pMapName, PSHFLSTRING pFileSuffix,
                    bool fWritable, bool fAutoMount, bool fCreateSymlinks,
                    uint64_t opts, uint64_t quota);
int vbsfMappingsRemove(PSHFLSTRING pMapName);

int vbsfMappingsQuery(PSHFLCLIENTDATA pClient, PSHFLMAPPING pMappings, uint32_t *pcMappings);
int vbsfMappingsQueryName(PSHFLCLIENTDATA pClient, SHFLROOT root, SHFLSTRING *pString);
int vbsfMappingsQueryFileSuffix(PSHFLCLIENTDATA pClient, SHFLROOT root, wchar_t **suffix);
int vbsfMappingsQueryWritable(PSHFLCLIENTDATA pClient, SHFLROOT root, bool *fWritable);
int vbsfMappingsQueryAutoMount(PSHFLCLIENTDATA pClient, SHFLROOT root, bool *fAutoMount);
int vbsfMappingsQuerySymlinksCreate(PSHFLCLIENTDATA pClient, SHFLROOT root, bool *fSymlinksCreate);
int vbsfMappingsQueryCrypt(PSHFLCLIENTDATA pClient, SHFLROOT root, wchar_t *path, int *crypt_mode);
int vbsfMappingsQueryQuota(PSHFLCLIENTDATA pClient, SHFLROOT root,
                           uint64_t *quota_max, uint64_t *quota_cur);
int vbsfMappingsUpdateQuota(PSHFLCLIENTDATA pClient, SHFLROOT root, uint64_t quota_cur);
int vbsfMapFolder(PSHFLCLIENTDATA pClient, PSHFLSTRING pszMapName, RTUTF16 delimiter,
                  bool fCaseSensitive, SHFLROOT *pRoot);
int vbsfUnmapFolder(PSHFLCLIENTDATA pClient, SHFLROOT root);

const wchar_t* vbsfMappingsQueryHostRoot(SHFLROOT root);
bool vbsfIsGuestMappingCaseSensitive(SHFLROOT root);
bool vbsfIsHostMappingCaseSensitive(SHFLROOT root);

int vbsfMappingLoaded(const PMAPPING pLoadedMapping, SHFLROOT root);
PMAPPING vbsfMappingGetByRoot(SHFLROOT root);
PMAPPING vbsfMappingGetByName(PRTUTF16 pwszName, SHFLROOT *pRoot);

#endif /* !___MAPPINGS_H */

