/**
 * @file tte_printf_override.h
 *
 * @brief Override tonc's tte_printf (which is #defined to iprintf) with a
 *        vsnprintf + tte_write implementation.
 *
 * WHY: libtonc defines `tte_printf` as `iprintf` (tonc_tte.h). iprintf is a
 * REAL printf variant that writes through the newlib FILE layer
 * (_vfiprintf_r -> stdout FILE -> __sbprintf -> _fflush_r -> __swrite ->
 * _write_r -> devoptab indirect call). On GBA that chain is fragile (and in
 * this project the devoptab/handle area gets corrupted at runtime, so any
 * FILE output jumps to a garbage address -> hard freeze, e.g. the deck
 * select screen). Text rendering only needs memory formatting + tte render,
 * never stdio.
 *
 * USAGE: include AFTER <tonc.h> in any translation unit that calls
 * tte_printf. The #undef removes tonc's macro, then we re-define it to our
 * own function.
 */
#ifndef TTE_PRINTF_OVERRIDE_H
#define TTE_PRINTF_OVERRIDE_H

#include <stdarg.h>
#include <string.h>
#include <tonc_tte.h>

// Must not overflow: longest call is a #{P:...} tag + a <=80-char desc.
#define GBA_TTE_PRINTF_BUF_SIZE 256

static inline int gba_tte_printf(const char* fmt, ...)
{
    char buf[GBA_TTE_PRINTF_BUF_SIZE];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    tte_write(buf);
    return 0;
}

// Replace tonc's `tte_printf == iprintf` macro with our FILE-free version.
#ifdef tte_printf
#undef tte_printf
#endif
#define tte_printf gba_tte_printf

#endif // TTE_PRINTF_OVERRIDE_H
