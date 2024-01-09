// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
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
