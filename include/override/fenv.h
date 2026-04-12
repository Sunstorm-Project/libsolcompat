/*
 * libsolcompat -- <fenv.h> for Solaris 7
 *
 * Solaris 7 has no <fenv.h> (C99). This provides the full C99
 * floating-point environment interface, implemented via the SPARC FSR.
 *
 * Part of libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_FENV_H
#define _SOLCOMPAT_OVERRIDE_FENV_H

/*
 * __sun guard: the FE_* exception flag values and fenv_t/fexcept_t
 * typedefs are SPARC FSR-specific.  glibc on x86_64 defines these
 * with different values and types (typedef unsigned long fenv_t);
 * redefining them here when the override path leaks into x86 compiles
 * produces warnings or errors depending on GCC's diagnostic level.
 *
 * Solaris 7 has no <fenv.h> at all, so #include_next would fail there
 * (which is why there isn't one).  On any host that DOES have a
 * <fenv.h> (glibc, musl, macOS), using that system's definitions is
 * the right answer — this override only meaningfully applies on
 * Solaris.
 */
#ifdef __sun

#ifdef __cplusplus
extern "C" {
#endif

/* Exception flags (SPARC FSR accrued exception bit positions) */
#define FE_INVALID    0x10
#define FE_DIVBYZERO  0x02
#define FE_OVERFLOW   0x08
#define FE_UNDERFLOW  0x04
#define FE_INEXACT    0x01
#define FE_ALL_EXCEPT (FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW | FE_INEXACT)

/* Rounding directions (SPARC FSR rd field values) */
#define FE_TONEAREST  0
#define FE_TOWARDZERO 1
#define FE_UPWARD     2
#define FE_DOWNWARD   3

/* Types */
typedef unsigned int fenv_t;
typedef unsigned int fexcept_t;

/* Default environment */
extern const fenv_t *_fe_dfl_env_p;
#define FE_DFL_ENV (_fe_dfl_env_p)

/* Exception handling */
int feclearexcept(int);
int feraiseexcept(int);
int fetestexcept(int);
int fegetexceptflag(fexcept_t *, int);
int fesetexceptflag(const fexcept_t *, int);

/* Rounding */
int fegetround(void);
int fesetround(int);

/* Environment */
int fegetenv(fenv_t *);
int fesetenv(const fenv_t *);
int feholdexcept(fenv_t *);
int feupdateenv(const fenv_t *);

#ifdef __cplusplus
}
#endif

#else /* !__sun — pass through to host's real fenv.h */
#include_next <fenv.h>
#endif

#endif /* _SOLCOMPAT_OVERRIDE_FENV_H */
