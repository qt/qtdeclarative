/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/
#ifndef UNICODE_H
#define UNICODE_H

#include <QChar>

typedef unsigned char LChar;
typedef unsigned short UChar;
typedef int32_t UChar32;

namespace Unicode {
    inline UChar toLower(UChar ch) {
        return QChar::toLower(ch);
    }

    inline UChar toUpper(UChar ch) {
        return QChar::toUpper(ch);
    }
    inline UChar32 u_tolower(UChar32 ch) {
        return QChar::toLower(ch);
    }
    inline UChar32 u_toupper(UChar32 ch) {
        return QChar::toUpper(ch);
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
