// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef UNICODE_H
#define UNICODE_H

#include <QtCore/private/qunicodetables_p.h>
#include <QtCore/qchar.h>

typedef unsigned char LChar;
typedef unsigned short UChar;
typedef int32_t UChar32;

namespace Unicode {
    // u_tolower applies only Simple_Lowercase_Mapping. This is in contrast to QChar::toLower.
    inline UChar32 u_tolower(UChar32 ch) {
        if (ch > QChar::LastValidCodePoint)
            return ch;
        const auto fold = QUnicodeTables::properties(char32_t(ch))->cases[QUnicodeTables::LowerCase];
        return fold.special ? ch : (ch + fold.diff);
    }

    // u_toupper applies only Simple_Uppercase_Mapping. This is in contrast to QChar::toUpper.
    inline UChar32 u_toupper(UChar32 ch) {
        if (ch > QChar::LastValidCodePoint)
            return ch;
        const auto fold = QUnicodeTables::properties(char32_t(ch))->cases[QUnicodeTables::UpperCase];
        return fold.special ? ch : (ch + fold.diff);
    }
}

using Unicode::u_toupper;
using Unicode::u_tolower;

#define U16_IS_LEAD(ch) QChar::isHighSurrogate((ch))
#define U16_IS_TRAIL(ch) QChar::isLowSurrogate((ch))
#define U16_GET_SUPPLEMENTARY(lead, trail) static_cast<UChar32>(QChar::surrogateToUcs4((lead), (trail)))
#define U_IS_BMP(ch) ((ch) < 0x10000)
#define U16_LENGTH(c) ((uint32_t)(c)<=0xffff ? 1 : 2)
#define UCHAR_MAX_VALUE 0x10ffff

#define U_MASK(category) (1u << (category))
#define U_GET_GC_MASK(c) U_MASK(QChar::category((c)))
#define U_GC_L_MASK (U_GC_LU_MASK|U_GC_LL_MASK|U_GC_LT_MASK|U_GC_LM_MASK|U_GC_LO_MASK)
#define U_GC_LU_MASK U_MASK(QChar::Letter_Uppercase)
#define U_GC_LL_MASK U_MASK(QChar::Letter_Lowercase)
#define U_GC_LT_MASK U_MASK(QChar::Letter_Titlecase)
#define U_GC_LM_MASK U_MASK(QChar::Letter_Modifier)
#define U_GC_LO_MASK U_MASK(QChar::Letter_Other)
#define U_GC_MN_MASK U_MASK(QChar::Mark_NonSpacing)
#define U_GC_MC_MASK U_MASK(QChar::Mark_SpacingCombining)
#define U_GC_ND_MASK U_MASK(QChar::Number_DecimalDigit)
#define U_GC_PC_MASK U_MASK(QChar::Punctuation_Connector)

#endif // UNICODE_H
