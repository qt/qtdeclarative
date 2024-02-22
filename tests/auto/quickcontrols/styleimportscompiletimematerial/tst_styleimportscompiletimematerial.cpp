// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qregularexpression.h>
#include <QtTest/qtest.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/private/qqmlmetatype_p.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/private/qquickstyle_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>

using namespace QQuickVisualTestUtils;
using namespace QQuickControlsTestUtils;

class tst_StyleImportsCompileTimeMaterial : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_StyleImportsCompileTimeMaterial();

private slots:
    void importMaterialStyleWithoutControls();
};

tst_StyleImportsCompileTimeMaterial::tst_StyleImportsCompileTimeMaterial()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_StyleImportsCompileTimeMaterial::importMaterialStyleWithoutControls()
{
    QQuickControlsApplicationHelper helper(this, QLatin1String("importMaterialStyleWithoutControls.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());

    auto button = helper.window->property("button").value<QQuickButton*>();
    QVERIFY(button);
    // The Material style sets a size 14 font for Button.
    QCOMPARE(button->font().pixelSize(), 14);
    const QTypeRevision latestControlsRevision = QQmlMetaType::latestModuleVersion(QLatin1String("QtQuick.Controls"));
    QVERIFY2(!latestControlsRevision.isValid(), "QtQuick.Controls should not be imported when using compile-time style selection");
}

QTEST_MAIN(tst_StyleImportsCompileTimeMaterial)

#include "tst_styleimportscompiletimematerial.moc"
