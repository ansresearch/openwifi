#ifndef _FIXEDPTC_H_
#define _FIXEDPTC_H_


/*
 * fixedptc.h is a 32-bit or 64-bit fixed point numeric library.
 *
 * The symbol FIXEDPT_BITS, if defined before this library header file
 * is included, determines the number of bits in the data type (its "width").
 * The default width is 32-bit (FIXEDPT_BITS=32) and it can be used
 * on any recent C99 compiler. The 64-bit precision (FIXEDPT_BITS=64) is
 * available on compilers which implement 128-bit "long long" types. This
 * precision has been tested on GCC 4.2+.
 *
 * The FIXEDPT_WBITS symbols governs how many bits are dedicated to the
 * "whole" part of the number (to the left of the decimal point). The larger
 * this width is, the larger the numbers which can be stored in the fixedpt
 * number. The rest of the bits (available in the FIXEDPT_FBITS symbol) are
 * dedicated to the fraction part of the number (to the right of the decimal
 * point).
 *
 * Since the number of bits in both cases is relatively low, many complex
 * functions (more complex than div & mul) take a large hit on the precision
 * of the end result because errors in precision accumulate.
 * This loss of precision can be lessened by increasing the number of
 * bits dedicated to the fraction part, but at the loss of range.
 *
 * Adventurous users might utilize this library to build two data types:
 * one which has the range, and one which has the precision, and carefully
 * convert between them (including adding two number of each type to produce
 * a simulated type with a larger range and precision).
 *
 * The ideas and algorithms have been cherry-picked from a large number
 * of previous implementations available on the Internet.
 * Tim Hartrick has contributed cleanup and 64-bit support patches.
 *
 * == Special notes for the 32-bit precision ==
 * Signed 32-bit fixed point numeric library for the 24.8 format.
 * The specific limits are -8388608.999... to 8388607.999... and the
 * most precise number is 0.00390625. In practice, you should not count
 * on working with numbers larger than a million or to the precision
 * of more than 2 decimal places. Make peace with the fact that PI
 * is 3.14 here. :)
 */

/*
 * Copyright (c) 2010-2012 Ivan Voras <ivoras@freebsd.org>
 * Copyright (c) 2012 Tim Hartrick <tim@edgecast.com>
 * Copyright (c) 2020 Damian Wrobel <dwrobel@ertelnet.rybnik.pl>
 * Copyright (c) 2020 D3 Engineering, LLC.  Some code by
 *   Christopher White <cwhite@d3engineering.com>
 Copyright (c) 2022 ANS research group Univ. Brescia 
 * Lorenzo Ghiro removed part of the previously available code
 * and modified the implementation of the exp function <lorenzo.ghiro@unibs.it>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define FIXEDPT_VCSID "$Id$"

/* === Single-file vs. separate compilation ============================= */

#if defined(FIXEDPTC_IMPLEMENTATION) && defined(FIXEDPTC_EXTERN)
/* Not supported - can't combine extern and static versions of the
 * same functions in the same C file. */
#error "Please define at most one of FIXEDPTC_IMPLEMENTATION and FIXEDPTC_EXTERN"

#elif !defined(FIXEDPTC_IMPLEMENTATION) && !defined(FIXEDPTC_EXTERN)
/* single-file mode with inlines --- default for backward compatibility. */
#undef FIXEDPTC__PROTOTYPES
#define FIXEDPTC_IMPLEMENTATION
#define FIXEDPTC__PROTO static inline
#define FIXEDPTC__NO__EXPORT

#elif defined(FIXEDPTC_IMPLEMENTATION)
/* Implementation only */
#undef FIXEDPTC__PROTOTYPES
#define FIXEDPTC__PROTO

#else /* defined(FIXEDPTC_EXTERN) */
/* Interface only */
#define FIXEDPTC__PROTOTYPES
#define FIXEDPTC__PROTO extern

#endif

/* === Custom exporting ================================================= */

/* If there's not already an EXPORT_SYMBOL, or if we're not exporting, create
 * our own FIXEDPTC__EXPORT_SYMBOL as a nop so we don't have to conditionally
 * compile the export on every function.  In addition, define a symbol that
 * will tell us to undef our nop before leaving this file.  This should help
 * avoid collisions.
 */

#if defined(FIXEDPTC__NO__EXPORT) || !defined(EXPORT_SYMBOL)
#define FIXEDPTC__CLEANUP__EXPORT_SYMBOL
#define FIXEDPTC__STRCAT(a,b) FIXEDPTC__STRCAT2(a,___,b)
#define FIXEDPTC__STRCAT2(a,b,c) a ## b ## c

/* Use a typedef for the nop because (1) it swallows a trailing semicolon;
 * (2) it doesn't trigger "unused function" warnings; and (3) it doesn't
 * create any symbol-table entries.
 */
#define FIXEDPTC__EXPORT_SYMBOL(x) typedef int FIXEDPTC__STRCAT(x,__LINE__)

#else

/* EXPORT_SYMBOL is defined, and we are exporting */
#define FIXEDPTC__EXPORT_SYMBOL(x) EXPORT_SYMBOL(x)

#endif

/* === Bit sizes ======================================================== */

#ifndef FIXEDPT_BITS
#define FIXEDPT_BITS    32
#endif

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#if FIXEDPT_BITS == 32
typedef int32_t fixedpt;
typedef int64_t fixedptd;
typedef uint32_t fixedptu;
typedef uint64_t fixedptud;
#elif FIXEDPT_BITS == 64
typedef int64_t fixedpt;
typedef __int128_t fixedptd;
typedef uint64_t fixedptu;
typedef __uint128_t fixedptud;
#else
#error "FIXEDPT_BITS must be equal to 32 or 64"
#endif

#ifndef FIXEDPT_WBITS
#define FIXEDPT_WBITS   14
#endif

#ifndef MAX_DEC
#define MAX_DEC   5
#endif

#if FIXEDPT_WBITS >= FIXEDPT_BITS
#error "FIXEDPT_WBITS must be less than or equal to FIXEDPT_BITS"
#endif

#define FIXEDPT_FBITS   (FIXEDPT_BITS - FIXEDPT_WBITS)
#define FIXEDPT_FMASK   (((fixedpt)1 << FIXEDPT_FBITS) - 1)

/* === Interface ======================================================== */

#define fixedpt_rconst(R) ((fixedpt)((R) * FIXEDPT_ONE + ((R) >= 0 ? 0.5 : -0.5)))
#define fixedpt_fromint(I) ((fixedptd)(I) << FIXEDPT_FBITS)
#define fixedpt_toint(F) ((F) >> FIXEDPT_FBITS)
#define fixedpt_add(A,B) ((A) + (B))
#define fixedpt_sub(A,B) ((A) - (B))
#define fixedpt_xmul(A,B)                       \
    ((fixedpt)(((fixedptd)(A) * (fixedptd)(B)) >> FIXEDPT_FBITS))
#define fixedpt_fracpart(A) ((fixedpt)(A) & FIXEDPT_FMASK)

#define FIXEDPT_ZERO fixedpt_fromint(0)
#define FIXEDPT_ONE ((fixedpt)((fixedpt)1 << FIXEDPT_FBITS))
#define FIXEDPT_ONE_HALF (FIXEDPT_ONE >> 1)
#define FIXEDPT_TWO (FIXEDPT_ONE + FIXEDPT_ONE)
#define FIXEDPT_PI  fixedpt_rconst(3.14159265358979323846)
#define FIXEDPT_TWO_PI  fixedpt_rconst(2 * 3.14159265358979323846)
#define FIXEDPT_E   fixedpt_rconst(2.7182818284590452354)

#define fixedpt_abs(A) ((A) < 0 ? -(A) : (A))

/* fixedpt is meant to be usable in environments without floating point support
 * (e.g. microcontrollers, kernels), so we can't use floating point types directly.
 * Putting them only in macros will effectively make them optional. */
#define fixedpt_tofloat(T) ((float) ((T)*((float)(1)/(float)(1L << FIXEDPT_FBITS))))

/* Prototypes of the functions below */

#ifdef FIXEDPTC__PROTOTYPES

FIXEDPTC__PROTO fixedpt fixedpt_mul(fixedpt A, fixedpt B);
FIXEDPTC__PROTO void fixedpt_str(fixedpt A, char *str, int max_dec);
FIXEDPTC__PROTO fixedpt fixedpt_exp(fixedpt fp);


#endif /* FIXEDPTC__PROTOTYPES */

/* === Implementation =================================================== */

#ifdef FIXEDPTC_IMPLEMENTATION

/* Multiplies two fixedpt numbers, returns the result. */
FIXEDPTC__PROTO fixedpt
fixedpt_mul(fixedpt A, fixedpt B)
{
    return (((fixedptd)A * (fixedptd)B) >> FIXEDPT_FBITS);
}
FIXEDPTC__EXPORT_SYMBOL(fixedpt_mul);

/**
 * Convert the given fixedpt number to a decimal string.
 * The max_dec argument specifies how many decimal digits to the right
 * of the decimal point to generate. If set to -1, the "default" number
 * of decimal digits will be used (2 for 32-bit fixedpt width, 10 for
 * 64-bit fixedpt width); If set to -2, "all" of the digits will
 * be returned, meaning there will be invalid, bogus digits outside the
 * specified precisions.
 */
FIXEDPTC__PROTO void
fixedpt_str(fixedpt A, char *str, int max_dec)
{
    int ndec = 0, slen = 0;
    char tmp[12] = {0};
    fixedptud fr;
    fixedptu ip;
    const fixedptud one = (fixedptud)1 << FIXEDPT_BITS;
    const fixedptud mask = one - 1;


    if (max_dec == -1)
#if FIXEDPT_BITS == 32
#if FIXEDPT_WBITS > 16
        max_dec = 2;
#else
        max_dec = 4;
#endif
#elif FIXEDPT_BITS == 64
        max_dec = 10;
#else
#error Invalid width
#endif
    else if (max_dec == -2)
        max_dec = 15;

    if (A < 0) {
        str[slen++] = '-';
        A *= -1;
    }

    ip = (fixedptu) fixedpt_toint(A);
    do {
        tmp[ndec++] = '0' + ip % 10;
        ip /= 10;
    } while (ip != 0);

    while (ndec > 0)
        str[slen++] = tmp[--ndec];
    str[slen++] = '.';

    fr = (fixedpt_fracpart(A) << FIXEDPT_WBITS) & mask;
    do {
        fr = (fr & mask) * 10;

        str[slen++] = '0' + (fr >> FIXEDPT_BITS) % 10;
        ndec++;
    } while (fr != 0 && ndec < max_dec);

    if (ndec > 1 && str[slen - 1] == '0')
        str[slen - 1] = '\0'; // cut off trailing 0
    else
        str[slen] = '\0';
}
FIXEDPTC__EXPORT_SYMBOL(fixedpt_str);


#ifndef MAXORDER
#define MAXORDER    6 //cant be greater than 6, max Taylor term is x^MAXORDER
#endif
 
FIXEDPTC__PROTO fixedpt
fixedpt_exp(fixedpt x)
{
    /*
    Taylor Mc-Laurin
    e^x = 1 + x + x^2/2 + x^3/6 + x^4/24 + x^5/120 ... + x^n/(n!) + o(x^n)
    */
    int i;
    fixedpt tmp, res;

    fixedpt polis[MAXORDER+1];
    fixedpt factors[MAXORDER+1];
    polis[0] = 1;
    polis[1] = x;

    factors[0] = fixedpt_rconst(1.0);
    factors[1] = fixedpt_rconst(1.0);
    factors[2] = fixedpt_rconst(0.5);
    factors[3] = fixedpt_rconst(0.16666666666666666);
    factors[4] = fixedpt_rconst(0.04166666666666666);
    factors[5] = fixedpt_rconst(0.008333333333333333);
    factors[6] = fixedpt_rconst(0.001388888888888889);

    for (i = 2; i < MAXORDER+1; i++) {
        polis[i] = fixedpt_mul(polis[i - 1], x);

        //NB: add only terms which are not in overflow!
        // --> careful checks of base and exp sign...
        if ((i % 2 == 0) && (polis[i] < 0)) {
            //even exp but term became negative... means overflow!
            // make this term zero so won't count in summation
            polis[i] = FIXEDPT_ZERO;
        }
        else if ((i % 2 == 1) && (x < 0) && (polis[i] > 0)) {
            //Here we started from a negative number raised to an odd exp,
            // should stay negative but became positive... overflow!
            polis[i] = FIXEDPT_ZERO;
        }
        else if ((i % 2 == 1) && (x > 0) && (polis[i] < 0)) {
            // Reverse case for base positive but result negative
            polis[i] = FIXEDPT_ZERO;
        }
    }

    res = FIXEDPT_ONE + x;
    
    for (i = 2; i < MAXORDER+1; i++) {
        tmp = fixedpt_mul(polis[i], factors[i]);
        //Check multiplication kept correct sign
        if (polis[i] > 0 && tmp < 0) {
            //Wrong! for polis > 0, tmp should remain positive 
            tmp = FIXEDPT_ZERO;
        } else if (polis[i] < 0 && tmp > 0) {
            //Wrong! for polis < 0, tmp should remain negative 
            tmp = FIXEDPT_ZERO;
        }
        //Here tmp kept correct sign after multiplication :)
        // or has been reset to ZERO, we can add safely
        res = res + tmp;
    }
    return res;
}
FIXEDPTC__EXPORT_SYMBOL(fixedpt_exp);


#endif /* FIXEDPTC_IMPLEMENTATION */

/* === Custom exporting - cleanup ======================================= */

#ifdef FIXEDPTC__CLEANUP__EXPORT_SYMBOL
#undef FIXEDPTC__CLEANUP__EXPORT_SYMBOL
#undef FIXEDPTC__STRCAT
#undef FIXEDPTC__STRCAT2
#endif /* FIXEDPTC__CLEANUP__EXPORT_SYMBOL */

/* === Cleanup ========================================================== */

/* Undef the internal symbols we defined above, just in case. */
#undef FIXEDPTC__PROTOTYPES
#undef FIXEDPTC__PROTO
#undef FIXEDPTC__NO__EXPORT
#undef FIXEDPTC__EXPORT_SYMBOL

#endif /* _FIXEDPTC_H_ */