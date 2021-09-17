/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "tst_qmltc.h"

// Generated headers:
#include "ResolvedNameConflict.h"
#include "helloworld.h"

// Qt:
#include <QtCore/qstring.h>
#include <QtCore/qurl.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>

tst_qmltc::tst_qmltc()
{
#if defined(QMLTC_TESTS_DISABLE_CACHE) && QMLTC_TESTS_DISABLE_CACHE
    qputenv("QML_DISABLE_DISK_CACHE", "1");
#else
    qputenv("QML_DISABLE_DISK_CACHE", "0");
#endif
}

void tst_qmltc::initTestCase()
{
    const auto status = isCacheDisabled() ? u"DISABLED" : u"ENABLED";
    qInfo() << u"Disk cache is" << status;

    // Note: just check whether the QML code is valid. QQmlComponent is good for
    // it. also, we can use qrc to make sure the file is in the resource system.
    QUrl urls[] = {
        QUrl("qrc:/QmltcTests/data/NameConflict.qml"),
        QUrl("qrc:/QmltcTests/data/HelloWorld.qml"),
    };

    QQmlEngine e;
    QQmlComponent component(&e);
    for (const auto &url : urls) {
        component.loadUrl(url);
        QVERIFY2(!component.isError(), qPrintable(u"Bad QML file. "_qs + component.errorString()));
    }
}

void tst_qmltc::qmlNameConflictResolution()
{
    // we can include user-renamed files
}

void tst_qmltc::helloWorld()
{
    QSKIP("Nothing is supported yet.");
}

QTEST_MAIN(tst_qmltc)
