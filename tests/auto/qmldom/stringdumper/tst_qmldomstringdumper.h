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
#ifndef TST_QMLDOMSTRINGDUMPER_H
#define TST_QMLDOMSTRINGDUMPER_H
#include <QtQmlDom/private/qqmldomstringdumper_p.h>

#include <QtTest/QtTest>
#include <QTextStream>
#include <QDebug>

#include <limits>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

class TestStringDumper: public QObject
{
    Q_OBJECT
private slots:
    void testDumperToString() {
        QCOMPARE(dumperToString(u"bla"), QStringLiteral(u"bla"));
        QCOMPARE(dumperToString([](Sink s) { s(u"bla"); s(u"bla"); }), QStringLiteral(u"blabla"));
    }

    void testSinkInt() {
        QCOMPARE(dumperToString([](Sink s) { sinkInt(s, 1); }), QStringLiteral(u"1"));
        QCOMPARE(dumperToString([](Sink s) { sinkInt(s, 0); }), QStringLiteral(u"0"));
        QCOMPARE(dumperToString([](Sink s) { sinkInt(s, -1); }), QStringLiteral(u"-1"));
        QCOMPARE(dumperToString([](Sink s) { sinkInt(s, std::numeric_limits<qint32>::max()); }), QStringLiteral(u"2147483647"));
        QCOMPARE(dumperToString([](Sink s) { sinkInt(s, std::numeric_limits<qint32>::min()); }), QStringLiteral(u"-2147483648"));
        QCOMPARE(dumperToString([](Sink s) { sinkInt(s, std::numeric_limits<quint32>::max()); }), QStringLiteral(u"4294967295"));
        QCOMPARE(dumperToString([](Sink s) { sinkInt(s, std::numeric_limits<qint64>::min()); }), QStringLiteral(u"-9223372036854775808"));
        QCOMPARE(dumperToString([](Sink s) { sinkInt(s, std::numeric_limits<qint64>::max()); }), QStringLiteral(u"9223372036854775807"));
        QCOMPARE(dumperToString([](Sink s) { sinkInt(s, std::numeric_limits<quint64>::max()); }), QStringLiteral(u"18446744073709551615"));
    }

    void testSinkEscaped() {
        QCOMPARE(dumperToString([](Sink s) { sinkEscaped(s, u""); }), QStringLiteral(u"\"\""));
        QStringView s1 = u"bla:\"bla\\\""; // uR"(bla:"bla\")";
        QStringView s1Escaped = u"\"bla:\\\"bla\\\\\\\"\"";
        QCOMPARE(dumperToString([s1](Sink s) { sinkEscaped(s, s1); }), s1Escaped);
        QCOMPARE(dumperToString([](Sink s) { sinkEscaped(s, u"", EscapeOptions::NoOuterQuotes); }), QStringLiteral(u""));
        QStringView s2 = u"bla:\"bla\\\"\n";
        QStringView s2Escaped = u"bla:\\\"bla\\\\\\\"\\n"; // uR"(bla:\"bla\\\"\n)"
        QCOMPARE(dumperToString([s2](Sink s) { sinkEscaped(s, s2, EscapeOptions::NoOuterQuotes); }), s2Escaped);
    }

    void testDevNull() {
        sinkEscaped(devNull, u"");
        sinkEscaped(devNull, u"bla:\"bla\\\"\n");
    }

    void testSinkIndent() {
        QCOMPARE(dumperToString([](Sink s) { sinkIndent(s, 0); }), QStringLiteral(u""));
        QCOMPARE(dumperToString([](Sink s) { sinkIndent(s, 1); }), QStringLiteral(u" "));
        QCOMPARE(dumperToString([](Sink s) { sinkIndent(s, 10); }), QStringLiteral(u"          "));
        QCOMPARE(dumperToString([](Sink s) { sinkIndent(s, 40); }), QStringLiteral(u"          ").repeated(4));
    }


    void testSinkNewline(){
        QCOMPARE(dumperToString([](Sink s) { sinkNewline(s); }), QStringLiteral(u"\n"));
        QCOMPARE(dumperToString([](Sink s) { sinkNewline(s, 0); }), QStringLiteral(u"\n"));
        QCOMPARE(dumperToString([](Sink s) { sinkNewline(s, 1); }), QStringLiteral(u"\n "));
        QCOMPARE(dumperToString([](Sink s) { sinkNewline(s, 10); }), QStringLiteral(u"\n          "));
        QCOMPARE(dumperToString([](Sink s) { sinkNewline(s, 40); }), QStringLiteral(u"\n") + QStringLiteral(u"          ").repeated(4));
    }

    void testDumpErrorLevel(){
        QCOMPARE(dumperToString([](Sink s) { dumpErrorLevel(s, ErrorLevel::Error); }), QStringLiteral(u"Error"));
        QCOMPARE(dumperToString([](Sink s) { dumpErrorLevel(s, ErrorLevel::Warning); }), QStringLiteral(u"Warning"));
    }
};

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif
