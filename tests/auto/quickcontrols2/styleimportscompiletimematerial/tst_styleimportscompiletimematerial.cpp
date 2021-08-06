/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
