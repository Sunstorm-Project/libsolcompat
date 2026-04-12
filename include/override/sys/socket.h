/*
 * override/sys/socket.h — POSIX socket extensions for Solaris 7
 *
 * Solaris 7's sys/socket.h:
 * - Lacks AF_INET6 and sockaddr_storage
 * - Uses msg_accrights instead of msg_control without _XPG4_2
 * - Lacks CMSG_LEN and CMSG_SPACE
 *
 * We define _XPG4_2 to get the POSIX struct msghdr with msg_control,
 * msg_controllen, and msg_flags instead of the old BSD msg_accrights.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */
#ifndef _SOLCOMPAT_OVERRIDE_SYS_SOCKET_H
#define _SOLCOMPAT_OVERRIDE_SYS_SOCKET_H

#ifdef __sun
/* Enable POSIX msghdr with msg_control/msg_controllen */
#ifndef _XPG4_2
#define _XPG4_2
#define _SOLCOMPAT_DEFINED_XPG4_2
#endif
#endif /* __sun */

/* Pull in the real system header */
#include_next <sys/socket.h>

#ifdef __sun
#ifdef _SOLCOMPAT_DEFINED_XPG4_2
#undef _XPG4_2
#undef _SOLCOMPAT_DEFINED_XPG4_2
#endif

#include <solcompat/network.h>

/*
 * POSIX.1-2001 CMSG_LEN and CMSG_SPACE macros.
 * Solaris 7 has CMSG_DATA/CMSG_FIRSTHDR/CMSG_NXTHDR but lacks these two.
 */
#ifndef CMSG_LEN
#define CMSG_LEN(data_length) \
    (_CMSG_HDR_ALIGN(sizeof(struct cmsghdr)) + (data_length))
#endif

#ifndef CMSG_SPACE
#define CMSG_SPACE(data_length) \
    (_CMSG_HDR_ALIGN(sizeof(struct cmsghdr) + (data_length)))
#endif
#endif /* __sun */

#endif /* _SOLCOMPAT_OVERRIDE_SYS_SOCKET_H */
