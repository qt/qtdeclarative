// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/private/qquickstyle_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtGui/private/qguiapplication_p.h>

class tst_QQuickStyle : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickStyle();

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

tst_QQuickStyle::tst_QQuickStyle()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

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
#ifdef QT_BUILD_INTERNAL
    QCOMPARE(QQuickLabelPrivate::get(label)->palette()->windowText(), Qt::red);
#endif
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
