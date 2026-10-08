/* SPDX-License-Identifier: MPL-2.0 */
#ifndef CLI_SDK_ALLOCATION_GUARD_H
#define CLI_SDK_ALLOCATION_GUARD_H
#include <stdlib.h>
void *cli_sdk_malloc(size_t);
void *cli_sdk_calloc(size_t, size_t);
void *cli_sdk_realloc(void *, size_t);
void cli_sdk_free(void *);
#define malloc cli_sdk_malloc
#define calloc cli_sdk_calloc
#define realloc cli_sdk_realloc
#define free cli_sdk_free
#endif
