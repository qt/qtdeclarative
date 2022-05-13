// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
