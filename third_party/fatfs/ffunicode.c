/*
 * NOVA's compact FatFs LFN conversion backend.
 *
 * NOVA uses CP437 for the FatFs OEM interface. ASCII long filenames are
 * supported case-insensitively. Bytes/code points outside ASCII are rejected
 * rather than silently mis-converted; a full FatFs ffunicode.c table can
 * replace this file when multilingual filename support is added.
 */
#include "ff.h"

WCHAR ff_oem2uni(WCHAR oem, WORD cp) {
    if (cp != 437u || oem >= 0x80u) return 0u;
    return oem;
}

WCHAR ff_uni2oem(DWORD uni, WORD cp) {
    if (cp != 437u || uni >= 0x80u) return 0u;
    return (WCHAR)uni;
}

DWORD ff_wtoupper(DWORD uni) {
    if (uni >= (DWORD)'a' && uni <= (DWORD)'z') {
        return uni - ((DWORD)'a' - (DWORD)'A');
    }
    return uni;
}
