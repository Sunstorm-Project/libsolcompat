/*
 * override/poll.h — POSIX poll extensions for Solaris 7
 *
 * Solaris 7's poll.h may lack ppoll().  This wrapper pulls in the
 * real header, then adds the missing API.
 *
 * Part of libsolcompat — https://github.com/firefly128/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_POLL_H
#define _SOLCOMPAT_OVERRIDE_POLL_H

/* Pull in the real Solaris 7 /usr/include/poll.h */
#include_next <poll.h>

#ifdef __sun
#include <solcompat/poll_ext.h>
#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_POLL_H */
