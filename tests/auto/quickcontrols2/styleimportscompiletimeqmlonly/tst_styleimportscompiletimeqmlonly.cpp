// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qregularexpression.h>
#include <QtTest/qtest.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/private/qquickstyle_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>

using namespace QQuickControlsTestUtils;

class tst_StyleImportsCompileTimeQmlOnly : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_StyleImportsCompileTimeQmlOnly();

private slots:
    void importQmlOnlyStyleWithoutControls();
};

tst_StyleImportsCompileTimeQmlOnly::tst_StyleImportsCompileTimeQmlOnly()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_StyleImportsCompileTimeQmlOnly::importQmlOnlyStyleWithoutControls()
{
    QQuickControlsApplicationHelper helper(this,
        QLatin1String("importQmlOnlyStyleWithoutControls.qml"), {}, QStringList() << dataDirectory());
    QVERIFY2(helper.ready, helper.failureMessage());

    auto button = helper.window->property("button").value<QQuickButton*>();
    QVERIFY(button);
    QCOMPARE(button->font().pixelSize(), 8);
}

QTEST_MAIN(tst_StyleImportsCompileTimeQmlOnly)

#include "tst_styleimportscompiletimeqmlonly.moc"
