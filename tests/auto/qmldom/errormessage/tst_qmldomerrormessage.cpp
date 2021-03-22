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
