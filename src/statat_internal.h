#ifndef SOLCOMPAT_STATAT_INTERNAL_H
#define SOLCOMPAT_STATAT_INTERNAL_H

typedef int (*solcompat_stat_fn)(const char *, void *, int);
__attribute__((visibility("hidden")))
int solcompat_statat_with(int, const char *, void *, int, solcompat_stat_fn);

#endif
