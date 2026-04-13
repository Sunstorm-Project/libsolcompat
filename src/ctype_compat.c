/*
 * ctype_compat.c -- C99/POSIX ctype functions missing from Solaris 7
 *
 * isblank() — C99
 * iswblank() — POSIX.1-2001 wide-char predicate
 * *_l() locale variants — POSIX.1-2008
 *
 * Solaris 7 has no per-thread locale support, so the _l variants
 * simply ignore the locale argument and call the base function.
 *
 * Part of libsolcompat
 */

#include <ctype.h>
#include <wchar.h>
#include <wctype.h>

/* isblank() -- C99 character classification function
 * Returns non-zero if c is a blank character (space or horizontal tab).
 */
int isblank(int c)
{
    return (c == ' ' || c == '\t');
}

/* iswblank() -- POSIX.1-2001 wide-char blank predicate.
 * Solaris 7's <wctype.h> predates POSIX 2001 and ships only the
 * original C90 set (iswspace, iswalpha, ...). gnulib modules like
 * wordwrap.c emit a bare iswblank reference and the link fails.
 */
int iswblank(wint_t wc)
{
    return (wc == (wint_t)L' ' || wc == (wint_t)L'\t');
}

/*
 * POSIX.1-2008 locale-aware character classification.
 * Solaris 7 has no per-thread locale — these call the global variants.
 */
int isalnum_l(int character, locale_t locale)  { (void)locale; return isalnum(character); }
int isalpha_l(int character, locale_t locale)  { (void)locale; return isalpha(character); }
int isblank_l(int character, locale_t locale)  { (void)locale; return isblank(character); }
int iscntrl_l(int character, locale_t locale)  { (void)locale; return iscntrl(character); }
int isdigit_l(int character, locale_t locale)  { (void)locale; return isdigit(character); }
int isgraph_l(int character, locale_t locale)  { (void)locale; return isgraph(character); }
int islower_l(int character, locale_t locale)  { (void)locale; return islower(character); }
int isprint_l(int character, locale_t locale)  { (void)locale; return isprint(character); }
int ispunct_l(int character, locale_t locale)  { (void)locale; return ispunct(character); }
int isspace_l(int character, locale_t locale)  { (void)locale; return isspace(character); }
int isupper_l(int character, locale_t locale)  { (void)locale; return isupper(character); }
int isxdigit_l(int character, locale_t locale) { (void)locale; return isxdigit(character); }
int tolower_l(int character, locale_t locale)  { (void)locale; return tolower(character); }
int toupper_l(int character, locale_t locale)  { (void)locale; return toupper(character); }
