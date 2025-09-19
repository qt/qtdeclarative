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
#include <QtGui/qpa/qplatformtheme.h>

class tst_QQuickStyle : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickStyle();

private slots:
    void cleanup();
    void configurationFile_data();
    void configurationFile();
    void commandLineArgument();
    void environmentVariables();
    void lookup();
    void qGuiApplicationPaletteChangesArePropagatedToControls();
    void defaultPaletteIsUpdatedWhenChangingColorScheme();

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

    QQuickStylePrivate::reset();
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

void tst_QQuickStyle::qGuiApplicationPaletteChangesArePropagatedToControls()
{
    QVERIFY(!QQuickTheme::instance());
    QQuickStyle::setStyle("Fusion");
    QCOMPARE(QQuickStyle::name(), QStringLiteral("Fusion"));
    QQmlEngine engine;
    engine.addImportPath(dataDirectory());
    QQmlComponent component(&engine);
    component.setData("import QtQuick; import QtQuick.Controls; ApplicationWindow { Label { anchors.centerIn: parent; } }", QUrl());

    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    auto *label = object->findChild<QQuickLabel *>();
    QVERIFY(label);
    auto *labelPrivate = QQuickLabelPrivate::get(label);

    // The fusion style causes QQuickTheme::palette() to use the platform themes palette directly.
    QVERIFY(QQuickTheme::instance()->usePlatformPalette());

    QPalette qGuiAppPalette = QGuiApplication::palette();
    const QPalette* qPlatformThemePalette = QGuiApplicationPrivate::platformTheme()->palette(QPlatformTheme::LabelPalette);

    // The initial QGuiApplication::palette() is usually built from the platform theme's system palette.
    // The platform theme can have multiple other "role" palettes, including a LabelPalette, which is what QQuickTheme will query for.
    // Since the initial QGuiApplication::palette() typically has a resolveMask of 0, it will not be prioritized over the defaultPalette.
    // Hence we expect the label's palette and default palette to both be equal to the platform theme's LabelPalette,
    // which differs from the system palette on macOS. However, if the platform theme returns nullptr for LabelPalette (which happens when the platform theme is "offscreen"),
    // we fallback to a gray palette that is in sync with QGuiApplication::palette().
    const QColor expectedInitialWindowTextColor = qPlatformThemePalette ? qPlatformThemePalette->windowText().color() : qGuiAppPalette.windowText().color();
    QCOMPARE(labelPrivate->palette()->windowText(), expectedInitialWindowTextColor);
    QCOMPARE(labelPrivate->defaultPalette().windowText().color(), expectedInitialWindowTextColor);

    qGuiAppPalette.setColor(QPalette::WindowText, QColorConstants::Magenta);
    QGuiApplication::setPalette(qGuiAppPalette);

    QTRY_COMPARE(labelPrivate->palette()->active()->windowText(), QColorConstants::Magenta);
    QCOMPARE(qGuiAppPalette.windowText().color(), QColorConstants::Magenta);
    // When the QQuickTheme uses the platform palette, QQuickTheme::palette(Scope scope) will take the palette directly from
    // QGuiApplicationPrivate::platformTheme(), skipping any potential changes made to QGuiApplication::palette().
    // We'll thus have a mismatch between the Label's palette (which is updated and propagated from QQuickWindow::palette)
    // and default palette (which is taken from the platform theme's palette).
    QCOMPARE_NE(labelPrivate->defaultPalette().windowText().color(), QColorConstants::Magenta);
    QCOMPARE(labelPrivate->defaultPalette().windowText().color(), expectedInitialWindowTextColor);
}

void tst_QQuickStyle::defaultPaletteIsUpdatedWhenChangingColorScheme()
{
    QVERIFY(!bool(QQuickTheme::instance()));
    QQuickStyle::setStyle("Basic");
    QCOMPARE(QQuickStyle::name(), QStringLiteral("Basic"));
    QQmlEngine engine;
    engine.addImportPath(dataDirectory());
    QQmlComponent component(&engine);
    component.setData("import QtQuick; import QtQuick.Controls; Label { }", QUrl());

    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    auto *labelPrivate = QQuickLabelPrivate::get(qobject_cast<QQuickLabel *>(object.data()));
    QColor oldLabelWindowText = labelPrivate->defaultPalette().windowText().color();
    QColor oldLabelWindow = labelPrivate->defaultPalette().window().color();

    QPalette oldSystemPalette = QQuickTheme::palette(QQuickTheme::System);

    auto oldColorScheme = QGuiApplication::styleHints()->colorScheme();
    auto guard = qScopeGuard([](){
        QGuiApplication::styleHints()->unsetColorScheme();
    });

    if (oldColorScheme == Qt::ColorScheme::Dark)
        QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    else if (oldColorScheme == Qt::ColorScheme::Light)
        QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    else
        QSKIP(qPrintable(QLatin1String("Skipping test. "
            "The %1 platform theme likely lacks a proper colorScheme() and requestColorScheme(Qt::ColorScheme) implementation.").arg(qGuiApp->platformName())));

    // Colors should be updated
    QTRY_COMPARE_NE(QQuickTheme::palette(QQuickTheme::System), oldSystemPalette);

    QCOMPARE_NE(labelPrivate->defaultPalette().windowText().color(), oldLabelWindowText);
    QCOMPARE_NE(labelPrivate->defaultPalette().window().color(), oldLabelWindow);

    if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Light) {
        QCOMPARE_GT(labelPrivate->defaultPalette().window().color().lightnessF(), 0.5);
        QCOMPARE_LT(labelPrivate->defaultPalette().windowText().color().lightnessF(), 0.5);
    } else if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
        QCOMPARE_LT(labelPrivate->defaultPalette().window().color().lightnessF(), 0.5);
        QCOMPARE_GT(labelPrivate->defaultPalette().windowText().color().lightnessF(), 0.5);
    }
}

QTEST_MAIN(tst_QQuickStyle)

#include "tst_qquickstyle.moc"
