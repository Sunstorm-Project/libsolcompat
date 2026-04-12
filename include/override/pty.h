/*
 * override/pty.h — PTY functions for Solaris 7
 *
 * Solaris 7 does not ship <pty.h> (added in later releases).
 * Modern software (screen, openssh, Python, etc.) includes this
 * header directly for openpty/forkpty/login_tty.
 *
 * Note: no #include_next since the real file doesn't exist.
 *
 * Part of libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_PTY_H
#define _SOLCOMPAT_OVERRIDE_PTY_H

#ifdef __sun
#include <solcompat/pty.h>
#else
#include_next <pty.h>
#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_PTY_H */
