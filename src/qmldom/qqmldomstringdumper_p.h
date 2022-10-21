/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**/
#ifndef DUMPER_H
#define DUMPER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qqmldom_global.h"
#include "qqmldomconstants_p.h"
#include "qqmldomfunctionref_p.h"

#include <QtCore/QString>
#include <QtCore/QStringView>
#include <QtCore/QDebug>

#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

using Sink = function_ref<void(QStringView)>;
using SinkF = std::function<void(QStringView)>;
using DumperFunction = std::function<void(Sink)>;

class Dumper{
public:
    DumperFunction dumper;
private:
    // We want to avoid the limit of one user conversion:
    // after doing (* -> QStringView) we cannot have QStringView -> Dumper, as it
    // would be the second user defined conversion.
    // For a similar reason we have a template to accept function_ref<void(Sink)> .
    // The end result is that void f(Dumper) can be called nicely, and avoid overloads:
    // f(u"bla"), f(QLatin1String("bla")), f(QString()), f([](Sink s){...}),...
    template <typename T>
    using if_compatible_dumper = typename
    std::enable_if<std::is_convertible<T, DumperFunction>::value, bool>::type;

    template<typename T>
    using if_string_view_convertible = typename
    std::enable_if<std::is_convertible_v<T, QStringView>, bool>::type;

public:
    Dumper(QStringView s):
        dumper([s](Sink sink){ sink(s); }) {}

    Dumper(std::nullptr_t): Dumper(QStringView(nullptr)) {}

    template <typename Stringy, if_string_view_convertible<Stringy> = true>
    Dumper(Stringy string):
        Dumper(QStringView(string)) {}

    template <typename U, if_compatible_dumper<U> = true>
    Dumper(U f): dumper(f) {}

    void operator()(Sink s) { dumper(s); }
};

template <typename T>
void sinkInt(Sink s, T i) {
    const int BUFSIZE = 42; // safe up to 128 bits
    QChar buf[BUFSIZE];
    int ibuf = BUFSIZE;
    buf[--ibuf] = QChar(0);
    bool neg = false;
    if (i < 0)
        neg=true;
    int digit = i % 10;
    i = i / 10;
    if constexpr (std::is_signed_v<T>) {
        if (neg) { // we change the sign here because -numeric_limits<T>::min() == numeric_limits<T>::min()
            i = -i;
            digit = - digit;
        }
    }
    buf[--ibuf] = QChar::fromLatin1('0' + digit);
    while (i > 0 && ibuf > 0) {
        digit = i % 10;
        buf[--ibuf] = QChar::fromLatin1('0' + digit);
        i = i / 10;
    }
    if (neg && ibuf > 0)
        buf[--ibuf] = QChar::fromLatin1('-');
    s(QStringView(&buf[ibuf], BUFSIZE - ibuf -1));
}

QMLDOM_EXPORT QString dumperToString(Dumper writer);

QMLDOM_EXPORT void sinkEscaped(Sink sink, QStringView s,
                               EscapeOptions options = EscapeOptions::OuterQuotes);

inline void devNull(QStringView) {}

QMLDOM_EXPORT void sinkIndent(Sink s, int indent);

QMLDOM_EXPORT void sinkNewline(Sink s, int indent = 0);

QMLDOM_EXPORT void dumpErrorLevel(Sink s, ErrorLevel level);

QMLDOM_EXPORT void dumperToQDebug(Dumper dumper, QDebug debug);

QMLDOM_EXPORT void dumperToQDebug(Dumper dumper, ErrorLevel level = ErrorLevel::Debug);

QMLDOM_EXPORT QDebug operator<<(QDebug d, Dumper dumper);

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE

#endif // DUMPER_H
