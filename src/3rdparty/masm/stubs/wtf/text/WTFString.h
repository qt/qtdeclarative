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
#ifndef WTFSTRING_H
#define WTFSTRING_H

#include <QString>
#include <wtf/ASCIICType.h>
#include <wtf/unicode/Unicode.h>
#include <memory>

namespace WTF {

class PrintStream;

class String : public QString
{
public:
    String() = default;
    String(const QString& s) : QString(s) {}
    bool is8Bit() const { return false; }
    const unsigned char *characters8() const { return 0; }
    const UChar *characters16() const { return reinterpret_cast<const UChar*>(constData()); }

    template <typename T>
    const T* characters() const;

    bool operator!() const { return isEmpty(); }

    void dump(PrintStream &) const {}
};

template <>
inline const unsigned char* String::characters<unsigned char>() const { return characters8(); }
template <>
inline const UChar* String::characters<UChar>() const { return characters16(); }

}

// Don't import WTF::String into the global namespace to avoid conflicts with QQmlJS::VM::String
namespace JSC {
    using WTF::String;
}

#define WTFMove(value) std::move(value)

#endif // WTFSTRING_H
