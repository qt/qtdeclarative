// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "tst_qmldomerrormessage.h"
#include <QtQmlDom/private/qqmldomerrormessage_p.h>

#include <QtTest/QtTest>
#include <QTextStream>
#include <QDebug>

#include <limits>


QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

static ErrorGroups myErrors(){
    static ErrorGroups res = {{NewErrorGroup("StaticAnalysis"), NewErrorGroup("FancyDetector")}};
    return res;
}

constexpr const char *myError0 = "my.company.error0";

void registerMyError() {
    ErrorMessage::msg(myError0, myErrors().warning(u"Error number 0"));
}

static auto myError1 = ErrorMessage::msg("my.company.error1", myErrors().warning(u"Error number 1"));
static auto myError2 = ErrorMessage::msg("my.company.error2", myErrors().error(u"Error number 2 on %1"));

void TestErrorMessage::testError()
{
    registerMyError();
    auto err0 = ErrorMessage::load(myError0);
    QCOMPARE(err0.errorId, QLatin1String(myError0));
    QCOMPARE(err0.message, dumperToString(u"Error number 0"));
    QCOMPARE(err0.level, ErrorLevel::Warning);
    auto err1 = ErrorMessage::load(QLatin1String("my.company.error1"));
    QCOMPARE(err1.errorId, myError1);
    QCOMPARE(err1.message, dumperToString(u"Error number 1"));
    QCOMPARE(err1.level, ErrorLevel::Warning);
    auto err1bis = ErrorMessage::load("my.company.error1");
    QCOMPARE(err1bis.errorId, myError1);
    QCOMPARE(err1bis.message, dumperToString(u"Error number 1"));
    QCOMPARE(err1bis.level, ErrorLevel::Warning);
    auto err2 = ErrorMessage::load(myError2, QLatin1String("extra info"));
    QCOMPARE(err2.errorId, myError2);
    QCOMPARE(err2.message, dumperToString(u"Error number 2 on extra info"));
    QCOMPARE(err2.level, ErrorLevel::Error);
}

}
}
QT_END_NAMESPACE

#ifndef NO_QTEST_MAIN
QTEST_MAIN(QQmlJS::Dom::TestErrorMessage)
#endif
