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
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/private/qquickstyle_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include "../shared/util.h"

class tst_QQuickStyle : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void cleanup();
    void lookup();
    void configurationFile_data();
    void configurationFile();
    void commandLineArgument();
    void environmentVariables();

private:
    Q_REQUIRED_RESULT bool loadControls();
    void unloadControls();
};

void tst_QQuickStyle::cleanup()
{
    unloadControls();

    QGuiApplicationPrivate::styleOverride.clear();
    qunsetenv("QT_QUICK_CONTROLS_STYLE");
    qunsetenv("QT_QUICK_CONTROLS_FALLBACK_STYLE");
    qunsetenv("QT_QUICK_CONTROLS_CONF");
}

bool tst_QQuickStyle::loadControls()
{
    QQmlEngine engine;
    engine.addImportPath(dataDirectory());
    QQmlComponent component(&engine);
    component.setData("import QtQuick; import QtQuick.Controls; Control { }", QUrl());

    QScopedPointer<QObject> object(component.create());
    if (object.isNull()) {
        qWarning() << component.errorString();
        return false;
    }
    return true;
}

void tst_QQuickStyle::unloadControls()
{
    qmlClearTypeRegistrations();
}

void tst_QQuickStyle::lookup()
{
    QQuickStyle::setStyle("Material");
    QCOMPARE(QQuickStyle::name(), QString("Material"));

    QVERIFY(loadControls());

    // The font size for editors in the (default) Normal variant is 16.
    // If this is wrong, the style plugin may not have been loaded.
    QCOMPARE(QQuickTheme::instance()->font(QQuickTheme::TextArea).pixelSize(), 16);

    QCOMPARE(QQuickStyle::name(), QString("Material"));
}

void tst_QQuickStyle::configurationFile_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("expectedStyle");

    QTest::newRow("Basic") << "basic.conf" << "Basic";
    QTest::newRow("Fusion") << "fusion.conf" << "Fusion";
    QTest::newRow("Imagine") << "imagine.conf" << "Imagine";
    QTest::newRow("Material") << "material.conf" << "Material";
    QTest::newRow("Universal") << "universal.conf" << "Universal";
    QTest::newRow("Custom") << "custom.conf" << "Custom";
}

void tst_QQuickStyle::configurationFile()
{
    QFETCH(QString, fileName);
    QFETCH(QString, expectedStyle);

    qputenv("QT_QUICK_CONTROLS_CONF", testFile(fileName).toLocal8Bit());

    // Load a control. The import causes the configuration file to be read.
    QQmlEngine engine;
    engine.addImportPath(":/data");
    QQmlComponent labelComponent(&engine);
    labelComponent.setData("import QtQuick; import QtQuick.Controls; Label {}", QUrl());

    QScopedPointer<QObject> object(labelComponent.create());
    QVERIFY2(!object.isNull(), qPrintable(labelComponent.errorString()));

    QCOMPARE(QQuickStyle::name(), expectedStyle);
    QVERIFY(!QQuickStylePrivate::isUsingDefaultStyle());

    // Test that fonts and palettes specified in configuration files are respected.
    QQuickLabel *label = qobject_cast<QQuickLabel *>(object.data());
    QVERIFY(label);
    // Make it small so that there's less possibility for the default/system
    // pixel size to match it and give us false positives.
    QCOMPARE(label->font().pixelSize(), 3);
    QCOMPARE(QQuickLabelPrivate::get(label)->palette()->windowText(), Qt::red);
}

void tst_QQuickStyle::commandLineArgument()
{
    QGuiApplicationPrivate::styleOverride = "CmdLineArgStyle";

    QVERIFY(loadControls());

    QCOMPARE(QQuickStyle::name(), QString("CmdLineArgStyle"));
}

void tst_QQuickStyle::environmentVariables()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "EnvVarStyle");
    qputenv("QT_QUICK_CONTROLS_FALLBACK_STYLE", "EnvVarFallbackStyle");
    QTest::ignoreMessage(QtWarningMsg, "QT_QUICK_CONTROLS_FALLBACK_STYLE: the specified fallback style" \
        " \"EnvVarFallbackStyle\" is not one of the built-in Qt Quick Controls 2 styles");
    QCOMPARE(QQuickStyle::name(), QString("EnvVarStyle"));
    QCOMPARE(QQuickStylePrivate::fallbackStyle(), QString());
}

QTEST_MAIN(tst_QQuickStyle)

#include "tst_qquickstyle.moc"
