/*
 * sys/soundcard.h — OSS sound interface for Solaris 7
 *
 * Solaris 7's /dev/audio and /dev/dsp speak a subset of the OSS
 * (Open Sound System) ABI. SDL2, openal-soft, and ScummVM include
 * <sys/soundcard.h> unconditionally when configure detects an OSS
 * path. This header provides the SNDCTL_* ioctl constants and the
 * struct audio_buf_info type, which lets the OSS backends compile.
 * Runtime behavior depends on the Solaris driver's subset support.
 *
 * Part of libsolcompat — https://github.com/Sunstorm-Project/libsolcompat
 */

#ifndef _SOLCOMPAT_SYS_SOUNDCARD_H
#define _SOLCOMPAT_SYS_SOUNDCARD_H

#include <sys/types.h>
#include <sys/ioctl.h>

/* Audio format codes */
#define AFMT_QUERY      0x00000000
#define AFMT_MU_LAW     0x00000001
#define AFMT_A_LAW      0x00000002
#define AFMT_IMA_ADPCM  0x00000004
#define AFMT_U8         0x00000008
#define AFMT_S16_LE     0x00000010
#define AFMT_S16_BE     0x00000020
#define AFMT_S8         0x00000040
#define AFMT_U16_LE     0x00000080
#define AFMT_U16_BE     0x00000100
#define AFMT_MPEG       0x00000200

#if defined(__sparc) || defined(__BIG_ENDIAN__)
#define AFMT_S16_NE     AFMT_S16_BE
#define AFMT_U16_NE     AFMT_U16_BE
#else
#define AFMT_S16_NE     AFMT_S16_LE
#define AFMT_U16_NE     AFMT_U16_LE
#endif

/* Core ioctls used by SDL2 / openal-soft / ScummVM */
#define SNDCTL_DSP_RESET        _IO('P', 0)
#define SNDCTL_DSP_SYNC         _IO('P', 1)
#define SNDCTL_DSP_SPEED        _IOWR('P', 2, int)
#define SNDCTL_DSP_STEREO       _IOWR('P', 3, int)
#define SNDCTL_DSP_GETBLKSIZE   _IOWR('P', 4, int)
#define SNDCTL_DSP_SETFMT       _IOWR('P', 5, int)
#define SNDCTL_DSP_CHANNELS     _IOWR('P', 6, int)
#define SNDCTL_DSP_POST         _IO('P', 8)
#define SNDCTL_DSP_SUBDIVIDE    _IOWR('P', 9, int)
#define SNDCTL_DSP_SETFRAGMENT  _IOWR('P', 10, int)
#define SNDCTL_DSP_GETFMTS      _IOR('P', 11, int)
#define SNDCTL_DSP_GETOSPACE    _IOR('P', 12, struct audio_buf_info)
#define SNDCTL_DSP_GETISPACE    _IOR('P', 13, struct audio_buf_info)
#define SNDCTL_DSP_NONBLOCK     _IO('P', 14)
#define SNDCTL_DSP_GETCAPS      _IOR('P', 15, int)
#define SNDCTL_DSP_GETODELAY    _IOR('P', 23, int)
#define SNDCTL_DSP_GETIPTR      _IOR('P', 17, count_info)
#define SNDCTL_DSP_GETOPTR      _IOR('P', 18, count_info)

/* DSP capability bits */
#define DSP_CAP_REVISION        0x000000ff
#define DSP_CAP_DUPLEX          0x00000100
#define DSP_CAP_REALTIME        0x00000200
#define DSP_CAP_BATCH           0x00000400
#define DSP_CAP_COPROC          0x00000800
#define DSP_CAP_TRIGGER         0x00001000
#define DSP_CAP_MMAP            0x00002000

struct audio_buf_info {
    int fragments;      /* number of available fragments */
    int fragstotal;     /* total allocated fragments */
    int fragsize;       /* fragment size in bytes */
    int bytes;          /* available bytes */
};

typedef struct count_info {
    int bytes;
    int blocks;
    int ptr;
} count_info;

#endif /* _SOLCOMPAT_SYS_SOUNDCARD_H */
