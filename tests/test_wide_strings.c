#include <wchar.h>
#include <stdio.h>
#include <locale.h>
#include <dlfcn.h>
int main(void)
{
    wchar_t dst[6] = L"ABCD";
    if (wcslcpy(dst,L"pqrstu",4) != 6 || wcscmp(dst,L"pqr") || dst[4] ||
        wcslcat(dst,L"12",5) != 5 || wcscmp(dst,L"pqr1") ||
        wcslcat(dst,L"z",2) != 3 || wcscmp(dst,L"pqr1") ||
        wcpcpy(dst,L"XY") != dst+2 ||
        wcpncpy(dst,L"q",5) != dst+1 || dst[1] || dst[4] ||
        wcpncpy(dst,L"abcd",2) != dst+2 || dst[0]!=L'a' || dst[1]!=L'b' ||
        wcslcpy(dst,L"zero",0)!=4 || dst[0]!=L'a' ||
        wcscasecmp(L"Solaris",L"soLARIS") || wcsncasecmp(L"tea",L"TEst",2) ||
        wcscasecmp(L"a",L"B")>=0 || wcsncasecmp(L"a",L"B",0))
        return 1;
    void *libc = dlopen("libc.so.1", RTLD_LAZY);
    int (*native_wctob)(wint_t) = libc ? dlsym(libc, "wctob") : NULL;
    if (!native_wctob) return 2;
    const char *locales[] = {"C", "en_US.UTF-8", "en_US", NULL};
    for (int i = 0; locales[i]; ++i) {
        if (!setlocale(LC_CTYPE, locales[i])) continue;
        for (wint_t c = 0; c <= 0x200; ++c)
            if (wctob(c) != native_wctob(c)) {
                fprintf(stderr, "%s wctob(%lu): %d, native %d\n", locales[i],
                        (unsigned long)c, wctob(c), native_wctob(c));
                return 3;
            }
        if (wctob(WEOF) != native_wctob(WEOF)) return 4;
    }
    dlclose(libc);
    puts("PASS wide copy, append, padding, end pointers and case comparisons");
    return 0;
}
