#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H
#include <stddef.h>
typedef struct { const char *id,*source,*optimization,*flags; } TTVersion;
size_t tt_version_count(void);
const TTVersion *tt_version_at(size_t index);
const TTVersion *tt_version_find(const char *id);
/* Resolve explicit paths or test20/dev/stress aliases relative to project cwd. */
int tt_resolve_dataset(const char *name,char *out,size_t capacity,int *short_window,int *long_window,char *error,size_t error_capacity);
#endif
