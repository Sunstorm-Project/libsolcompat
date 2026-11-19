/*
 * solcompat/snprintf.h — C99-conformant snprintf/vsnprintf
 *
 * Solaris 7's snprintf/vsnprintf return -1 on truncation instead of
 * the C99-required count of characters that would have been written.
 * This breaks the universal pattern: n = snprintf(NULL, 0, fmt, ...)
 *
 * These replacements provide correct C99 return-value semantics.
 */
#ifndef SOLCOMPAT_SNPRINTF_H
#define SOLCOMPAT_SNPRINTF_H

#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/* All formatted-output entry points share the C99-compatible engine. */
#ifndef SOLCOMPAT_PRINTF_FORMAT
# if defined(__clang__)
#  define SOLCOMPAT_PRINTF_FORMAT(fmt, args) __attribute__((format(printf, fmt, args)))
# elif defined(__GNUC__)
#  define SOLCOMPAT_PRINTF_FORMAT(fmt, args) __attribute__((format(gnu_printf, fmt, args)))
# else
#  define SOLCOMPAT_PRINTF_FORMAT(fmt, args)
# endif
#endif
int solcompat_printf(const char *fmt, ...) SOLCOMPAT_PRINTF_FORMAT(1, 2);
int solcompat_vprintf(const char *fmt, va_list ap) SOLCOMPAT_PRINTF_FORMAT(1, 0);
int solcompat_fprintf(FILE *stream, const char *fmt, ...) SOLCOMPAT_PRINTF_FORMAT(2, 3);
int solcompat_vfprintf(FILE *stream, const char *fmt, va_list ap) SOLCOMPAT_PRINTF_FORMAT(2, 0);
int solcompat_sprintf(char *str, const char *fmt, ...) SOLCOMPAT_PRINTF_FORMAT(2, 3);
int solcompat_vsprintf(char *str, const char *fmt, va_list ap) SOLCOMPAT_PRINTF_FORMAT(2, 0);

int solcompat_snprintf(char *str, size_t size, const char *fmt, ...) SOLCOMPAT_PRINTF_FORMAT(3, 4);
int solcompat_vsnprintf(char *str, size_t size, const char *fmt, va_list ap) SOLCOMPAT_PRINTF_FORMAT(3, 0);

#ifdef __cplusplus
}
#endif

/*
 * In C: macro-redirect snprintf → solcompat_snprintf so all calls
 * get C99 return values automatically.
 *
 * In C++: declare snprintf/vsnprintf as real functions in the global
 * namespace. This lets std::snprintf work via <cstdio>. At link time,
 * -lsolcompat before -lc ensures the solcompat version is used.
 * We can't use macros because #define snprintf breaks std::snprintf.
 */
#ifdef __cplusplus
extern "C" int snprintf(char *str, size_t size, const char *fmt, ...);
extern "C" int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
#else

#define printf solcompat_printf
#define vprintf solcompat_vprintf
#define fprintf solcompat_fprintf
#define vfprintf solcompat_vfprintf
#define sprintf solcompat_sprintf
#define vsprintf solcompat_vsprintf
#define snprintf  solcompat_snprintf
#define vsnprintf solcompat_vsnprintf
#endif

#endif /* SOLCOMPAT_SNPRINTF_H */
