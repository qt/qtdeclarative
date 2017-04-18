/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <QtTest/qtest.h>
#include "../shared/visualtestutil.h"

#include <QtGui/qpalette.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p.h>

using namespace QQuickVisualTestUtil;

class tst_palette : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void palette_data();
    void palette();

    void inheritance_data();
    void inheritance();
};

void tst_palette::palette_data()
{
    QTest::addColumn<QString>("testFile");
    QTest::addColumn<QPalette>("expectedPalette");

    QTest::newRow("Control") << "palette-control-default.qml" << QPalette();
    QTest::newRow("AppWindow") << "palette-appwindow-default.qml" << QPalette();
    QTest::newRow("Popup") << "palette-popup-default.qml" << QPalette();

    QPalette customPalette;
    customPalette.setColor(QPalette::AlternateBase, QColor("aqua"));
    customPalette.setColor(QPalette::Base, QColor("azure"));
    customPalette.setColor(QPalette::BrightText, QColor("beige"));
    customPalette.setColor(QPalette::Button, QColor("bisque"));
    customPalette.setColor(QPalette::ButtonText, QColor("chocolate"));
    customPalette.setColor(QPalette::Dark, QColor("coral"));
    customPalette.setColor(QPalette::Highlight, QColor("crimson"));
    customPalette.setColor(QPalette::HighlightedText, QColor("fuchsia"));
    customPalette.setColor(QPalette::Light, QColor("gold"));
    customPalette.setColor(QPalette::Link, QColor("indigo"));
    customPalette.setColor(QPalette::LinkVisited, QColor("ivory"));
    customPalette.setColor(QPalette::Mid, QColor("khaki"));
    customPalette.setColor(QPalette::Midlight, QColor("lavender"));
    customPalette.setColor(QPalette::Shadow, QColor("linen"));
    customPalette.setColor(QPalette::Text, QColor("moccasin"));
    customPalette.setColor(QPalette::ToolTipBase, QColor("navy"));
    customPalette.setColor(QPalette::ToolTipText, QColor("orchid"));
    customPalette.setColor(QPalette::Window, QColor("plum"));
    customPalette.setColor(QPalette::WindowText, QColor("salmon"));

    QTest::newRow("Control:custom") << "palette-control-custom.qml" << customPalette;
    QTest::newRow("AppWindow:custom") << "palette-appwindow-custom.qml" << customPalette;
    QTest::newRow("Popup:custom") << "palette-popup-custom.qml" << customPalette;
}

void tst_palette::palette()
{
    QFETCH(QString, testFile);
    QFETCH(QPalette, expectedPalette);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl(testFile));

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    QVariant var = object->property("palette");
    QVERIFY(var.isValid());

    QPalette actualPalette = var.value<QPalette>();
    QCOMPARE(actualPalette, expectedPalette);
}

void tst_palette::inheritance_data()
{
    QTest::addColumn<QString>("testFile");

    QTest::newRow("Control") << "inheritance-control.qml";
    QTest::newRow("Popup") << "inheritance-popup.qml";
}

void tst_palette::inheritance()
{
    QFETCH(QString, testFile);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl(testFile));

    QScopedPointer<QQuickApplicationWindow> window(qobject_cast<QQuickApplicationWindow *>(component.create()));
    QVERIFY2(!window.isNull(), qPrintable(component.errorString()));

    QObject *control = window->property("control").value<QObject *>();
    QObject *child = window->property("child").value<QObject *>();
    QObject *grandChild = window->property("grandChild").value<QObject *>();
    QVERIFY(control && child && grandChild);

    QCOMPARE(window->palette(), QPalette());

    QCOMPARE(control->property("palette").value<QPalette>(), QPalette());
    QCOMPARE(child->property("palette").value<QPalette>(), QPalette());
    QCOMPARE(grandChild->property("palette").value<QPalette>(), QPalette());

    QPalette childPalette;
    childPalette.setColor(QPalette::Base, Qt::red);
    childPalette.setColor(QPalette::Text, Qt::green);
    childPalette.setColor(QPalette::Button, Qt::blue);
    child->setProperty("palette", childPalette);
    QCOMPARE(child->property("palette").value<QPalette>(), childPalette);
    QCOMPARE(grandChild->property("palette").value<QPalette>(), childPalette);

    QPalette grandChildPalette(childPalette);
    grandChildPalette.setColor(QPalette::Base, Qt::cyan);
    grandChildPalette.setColor(QPalette::Mid, Qt::magenta);
    grandChild->setProperty("palette", grandChildPalette);
    QCOMPARE(child->property("palette").value<QPalette>(), childPalette);
    QCOMPARE(grandChild->property("palette").value<QPalette>(), grandChildPalette);

    QPalette windowPalette;
    windowPalette.setColor(QPalette::Window, Qt::gray);
    window->setPalette(windowPalette);
    QCOMPARE(window->palette(), windowPalette);
    QCOMPARE(control->property("palette").value<QPalette>(), windowPalette);

    childPalette.setColor(QPalette::Window, Qt::gray);
    QCOMPARE(child->property("palette").value<QPalette>(), childPalette);

    grandChildPalette.setColor(QPalette::Window, Qt::gray);
    QCOMPARE(grandChild->property("palette").value<QPalette>(), grandChildPalette);

    child->setProperty("palette", QVariant());
    QCOMPARE(child->property("palette").value<QPalette>(), windowPalette);
    QCOMPARE(grandChild->property("palette").value<QPalette>(), grandChildPalette);

    grandChild->setProperty("palette", QVariant());
    QCOMPARE(grandChild->property("palette").value<QPalette>(), windowPalette);
}

QTEST_MAIN(tst_palette)

#include "tst_palette.moc"
