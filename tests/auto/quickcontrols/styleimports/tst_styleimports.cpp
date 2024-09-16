// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qregularexpression.h>
#include <QtGui/qpalette.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtTest/qtest.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/private/qquickstyle_p.h>
#include <QtQuickControls2Impl/private/qquickiconlabel_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControlsTestUtils/private/qtest_quickcontrols_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>

using namespace QQuickControlsTestUtils;

class tst_StyleImports : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_StyleImports();

private slots:
    void initTestCase() override;

    void cleanup();

    void select_data();
    void select();

    void platformSelectors();
    void customStyleSelector();

    void fallbackStyleShouldNotOverwriteTheme_data();
    void fallbackStyleShouldNotOverwriteTheme();

    void fallbackStyleThemeRespected_data();
    void fallbackStyleThemeRespected();

    void attachedTypesAvailable_data();
    void attachedTypesAvailable();
};

tst_StyleImports::tst_StyleImports()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_StyleImports::initTestCase()
{
    QQmlDataTest::initTestCase();
}

void tst_StyleImports::cleanup()
{
    qmlClearTypeRegistrations();
}

void tst_StyleImports::select_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("style");
    QTest::addColumn<QString>("fallback");
    QTest::addColumn<QString>("expected");

    // Action.qml exists in the FileSystemStyle style and the Basic style.
    QTest::newRow("control=Action,style=basic,fallback=empty") << "Action.qml" << "Basic" << "" << "Basic";
    QTest::newRow("control=Action,style=fs,fallback=empty") << "Action.qml" << "FileSystemStyle" << "" << "FileSystemStyle";
    QTest::newRow("control=Action,style=qrc,fallback=empty") << "Action.qml" << "ResourceStyle" << "" << "Basic";
    QTest::newRow("control=Action,style=nosuch,fallback=empty") << "Action.qml" << "NoSuchStyle" << "" << "Basic";
    QTest::newRow("control=Action,style=import,fallback=empty") << "Action.qml" << "StyleThatImportsFusion" << "" << "StyleThatImportsFusion";

    QTest::newRow("control=Action,style=basic,fallback=mat") << "Action.qml" << "Basic" << "Material" << "";
    QTest::newRow("control=Action,style=fs,fallback=mat") << "Action.qml" << "FileSystemStyle" << "Material" << "FileSystemStyle";
    QTest::newRow("control=Action,style=qrc,fallback=mat") << "Action.qml" << "ResourceStyle" << "Material" << "Basic";
    QTest::newRow("control=Action,style=nosuch,fallback=mat") << "Action.qml" << "NoSuchStyle" << "Material" << "Basic";
    QTest::newRow("control=Action,style=import,fallback=mat") << "Action.qml" << "StyleThatImportsFusion" << "Material" << "StyleThatImportsFusion";

    // Amongst the styles we're testing here, ScrollView.qml only exists in the Basic style.
    QTest::newRow("control=ScrollView,style=basic,fallback=empty") << "ScrollView.qml" << "Basic" << "" << "Basic";
    QTest::newRow("control=ScrollView,style=fs,fallback=empty") << "ScrollView.qml" << "FileSystemStyle" << "" << "Basic";
    QTest::newRow("control=ScrollView,style=qrc,fallback=empty") << "ScrollView.qml" << "ResourceStyle" << "" << "Basic";
    QTest::newRow("control=ScrollView,style=nosuch,fallback=empty") << "ScrollView.qml" << "NoSuchStyle" << "" << "Basic";
    QTest::newRow("control=ScrollView,style=import,fallback=empty") << "ScrollView.qml" << "StyleThatImportsFusion" << "" << "Fusion";

    QTest::newRow("control=ScrollView,style=basic,fallback=mat") << "ScrollView.qml" << "Basic" << "Material" << "Basic";
    QTest::newRow("control=ScrollView,style=fs,fallback=mat") << "ScrollView.qml" << "FileSystemStyle" << "Material" << "Material";
    QTest::newRow("control=ScrollView,style=qrc,fallback=mat") << "ScrollView.qml" << "ResourceStyle" << "Material" << "Material";
    QTest::newRow("control=ScrollView,style=nosuch,fallback=mat") << "ScrollView.qml" << "NoSuchStyle" << "Material" << "Basic";
    QTest::newRow("control=ScrollView,style=import,fallback=mat") << "ScrollView.qml" << "StyleThatImportsFusion" << "Material" << "Material";

    // Label.qml exists in the FileSystemStyle, Basic and Material styles.
    QTest::newRow("control=Label,style=basic,fallback=empty") << "Label.qml" << "Basic" << "" << "Basic";
    QTest::newRow("control=Label,style=fs,fallback=empty") << "Label.qml" << "FileSystemStyle" << "" << "FileSystemStyle";
    QTest::newRow("control=Label,style=qrc,fallback=empty") << "Label.qml" << "ResourceStyle" << "" << "Basic";
    QTest::newRow("control=Label,style=nosuch,fallback=empty") << "Label.qml" << "NoSuchStyle" << "" << "Basic";
    QTest::newRow("control=Label,style=import,fallback=empty") << "Label.qml" << "StyleThatImportsFusion" << "" << "Fusion";

    QTest::newRow("control=Label,style=basic,fallback=mat") << "Label.qml" << "Basic" << "Material" << "Basic";
    QTest::newRow("control=Label,style=fs,fallback=mat") << "Label.qml" << "FileSystemStyle" << "Material" << "FileSystemStyle";
    QTest::newRow("control=Label,style=qrc,fallback=mat") << "Label.qml" << "ResourceStyle" << "Material" << "Material";
    QTest::newRow("control=Label,style=nosuch,fallback=mat") << "Label.qml" << "NoSuchStyle" << "Material" << "Basic";
    QTest::newRow("control=Label,style=import,fallback=mat") << "Label.qml" << "StyleThatImportsFusion" << "Material" << "Material";

    // Button.qml exists in all styles including the fs and qrc styles
    QTest::newRow("control=Button,style=basic,fallback=empty") << "Button.qml" << "Basic" << "" << "Basic";
    QTest::newRow("control=Button,style=fs,fallback=empty") << "Button.qml" << "FileSystemStyle" << "" << "FileSystemStyle";
    QTest::newRow("control=Button,style=qrc,fallback=empty") << "Button.qml" << "ResourceStyle" << "" << "ResourceStyle";
    QTest::newRow("control=Button,style=nosuch,fallback=empty") << "Button.qml" << "NoSuchStyle" << "" << "Basic";
    QTest::newRow("control=Button,style=import,fallback=empty") << "Button.qml" << "StyleThatImportsFusion" << "" << "Fusion";

    QTest::newRow("control=Button,style=basic,fallback=mat") << "Button.qml" << "Basic" << "Material" << "Basic";
    QTest::newRow("control=Button,style=fs,fallback=mat") << "Button.qml" << "FileSystemStyle" << "Material" << "FileSystemStyle";
    QTest::newRow("control=Button,style=qrc,fallback=mat") << "Button.qml" << "ResourceStyle" << "Material" << "ResourceStyle";
    QTest::newRow("control=Button,style=nosuch,fallback=mat") << "Button.qml" << "NoSuchStyle" << "Material" << "Basic";
    QTest::newRow("control=Button,style=import,fallback=mat") << "Button.qml" << "StyleThatImportsFusion" << "Material" << "Material";
}

void tst_StyleImports::select()
{
    QFETCH(QString, file);
    QFETCH(QString, style);
    QFETCH(QString, fallback);
    QFETCH(QString, expected);

    // In Qt 5, there were several accepted forms for style names.
    // In Qt 6, the only accepted form is the base name of the style directory.
    const bool invalidStyleName = style.contains(QLatin1Char('/'));
    if (invalidStyleName)
        QTest::ignoreMessage(QtWarningMsg,
            "Style names must not contain paths; see the \"Definition of a Style\" documentation for more information");
    QQuickStyle::setStyle(style);
    QQuickStyle::setFallbackStyle(fallback);

    QQmlEngine engine;
    engine.addImportPath(QLatin1String(":/"));
//    engine.addImportPath(directory());
    engine.addImportPath(dataDirectory() + QLatin1String("/styles"));
    QQmlComponent component(&engine);
    const QString controlName = file.mid(0, file.indexOf(QLatin1Char('.')));
    component.setData(QString::fromLatin1("import QtQuick; import QtQuick.Controls; %1 { }").arg(controlName).toUtf8(), QUrl());

    const bool nonExistentStyle = style == QLatin1String("NoSuchStyle");
    if (nonExistentStyle)
        QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Component is not ready");
    QScopedPointer<QObject> object(component.create());
    if (nonExistentStyle) {
        QVERIFY(object.isNull());
        return;
    }

    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    if (!QQuickStylePrivate::builtInStyles().contains(expected)) {
        // We're expecting a custom style.
        QCOMPARE(object->objectName(), expected);
    } else {
        const QUrl expectedUrl(
                    QLatin1String("qrc:///qt-project.org/imports/QtQuick/Controls/%1/%2")
                    .arg(expected, file));
        QQmlComponent c2(&engine, expectedUrl);
        QVERIFY2(c2.isReady(), qPrintable(c2.errorString()));
        QScopedPointer<QObject> o2(c2.create());
        QCOMPARE(object->metaObject(), o2->metaObject());
    }
}

// Tests that the various platforms are available as selectors.
void tst_StyleImports::platformSelectors()
{
    QQuickStyle::setStyle(QLatin1String("PlatformStyle"));

    QQmlApplicationEngine engine;
    engine.addImportPath(dataDirectory() + QLatin1String("/styles"));
    engine.load(testFileUrl("applicationWindowWithButton.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    QQuickWindow *window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    QVERIFY(window);

    QObject *button = window->property("button").value<QObject*>();
    QVERIFY(button);

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QCOMPARE(button->objectName(), "PlatformStyle/+linux");
#elif defined(Q_OS_MACOS)
    QCOMPARE(button->objectName(), "PlatformStyle/+macos");
#elif defined(Q_OS_WIN)
    QCOMPARE(button->objectName(), "PlatformStyle/+windows");
#else
    QCOMPARE(button->objectName(), "PlatformStyle/Button.qml");
#endif
}

// Tests that a file selector is added for custom styles.
// Note that this is different to the regular QML import mechanism
// that results in e.g. FileSystemStyle/Button.qml being found;
// it allows non-template (Controls), custom user types to be
// picked up via selectors.
void tst_StyleImports::customStyleSelector()
{
    QQuickStyle::setStyle(QLatin1String("FileSystemStyle"));

    QQmlApplicationEngine engine;
    engine.addImportPath(dataDirectory() + QLatin1String("/styles"));
    engine.load(testFileUrl("customStyleSelector.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    QQuickWindow *window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    QVERIFY(window);

    QObject *customComponent = window->property("customComponent").value<QObject*>();
    QVERIFY(customComponent);
    QCOMPARE(customComponent->objectName(), "+FileSystemStyle/CustomComponent.qml");
}

QT_BEGIN_NAMESPACE
extern QPalette qt_fusionPalette();
QT_END_NAMESPACE

void tst_StyleImports::fallbackStyleShouldNotOverwriteTheme_data()
{
    QTest::addColumn<QString>("style");
    QTest::addColumn<QString>("fallbackStyle");
    QTest::addColumn<QColor>("expectedContentItemColor");

    QTest::addRow("style=ResourceStyle,fallbackStyle=Material")
        << QString::fromLatin1("ResourceStyle")
        << QString::fromLatin1("Material") << QColor("salmon");

    // A button's contentItem property will reflect the inactive button color,
    // when the button is not shown.
    // => read that color from the platform theme's ButtonPalette.
    // => fall back to the SystemPalette, if no ButtonPalette is available
    // => skip data row, if no platform theme is found
    QPlatformTheme *platformTheme = QGuiApplicationPrivate::platformTheme();
    if (!platformTheme) {
        qWarning() << "No platform theme available from QGuiApplicationPrivate::platformTheme()";
        return;
    }

    const QPalette *buttonPalette = platformTheme->palette(QPlatformTheme::ButtonPalette);

    if (!buttonPalette) {
        qWarning() << "No ButtonPalette found. Falling back to SystemPalette.";
        buttonPalette = platformTheme->palette(QPlatformTheme::SystemPalette);
    }

    QTest::addRow("style=Fusion,fallbackStyle=Material")
        << QString::fromLatin1("Fusion") << QString::fromLatin1("Material")
        << buttonPalette->color(QPalette::Inactive, QPalette::ButtonText);
}

void tst_StyleImports::fallbackStyleShouldNotOverwriteTheme()
{
    QFETCH(QString, style);
    QFETCH(QString, fallbackStyle);
    QFETCH(QColor, expectedContentItemColor);

    QQuickStyle::setStyle(style);
    QQuickStyle::setFallbackStyle(fallbackStyle);

    QQmlApplicationEngine engine;
    engine.addImportPath(QLatin1String(":/"));
    engine.addImportPath(dataDirectory());
    engine.load(testFileUrl("applicationWindowWithButton.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    QQuickWindow *window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    QVERIFY(window);

    QObject *button = window->property("button").value<QObject*>();
    QVERIFY(button);

    QQuickIconLabel *contentItem = button->property("contentItem").value<QQuickIconLabel*>();
    QVERIFY(contentItem);

    // For example: the Fusion style provides Button.qml, so the Button's text color
    // should be that of QPalette::ButtonText from QQuickFusionTheme.
    QCOMPARE(contentItem->color(), expectedContentItemColor);
}

enum FallbackMethod {
    QmlDirImport,
    EnvVar
};

void tst_StyleImports::fallbackStyleThemeRespected_data()
{
    QTest::addColumn<QString>("qmlFilePath");
    QTest::addColumn<QString>("runtimeStyle");
    QTest::addColumn<FallbackMethod>("fallbackMethod");
    QTest::addColumn<Qt::ColorScheme>("colorScheme");
    QTest::addColumn<QColor>("expectedButtonTextColor");
    QTest::addColumn<QColor>("expectedWindowColor");

    // Taken from qquickmaterialstyle.cpp.
    static const QRgb materialBackgroundColorLight = 0xFFFFFBFE;
    static const QRgb materialBackgroundColorDark = 0xFF1C1B1F;

    // Notes:
    // - FileSystemStyle has blue button text.
    // - StyleThatImportsMaterial has red button text.
    // - All rows result in Material being the fallback.

    QTest::newRow("import controls, env var fallback, light") << "applicationWindowWithButton.qml"
        << "FileSystemStyle" << EnvVar << Qt::ColorScheme::Light
        << QColor::fromRgb(0x0000ff) << QColor::fromRgba(materialBackgroundColorLight);
    QTest::newRow("import controls, env var fallback, dark") << "applicationWindowWithButton.qml"
        << "FileSystemStyle" << EnvVar << Qt::ColorScheme::Dark
        << QColor::fromRgb(0x0000ff) << QColor::fromRgba(materialBackgroundColorDark);

    QTest::newRow("import style, qmldir fallback, light") << "importStyleWithQmlDirFallback.qml"
        << "" << QmlDirImport << Qt::ColorScheme::Light
        << QColor::fromRgb(0xff0000) << QColor::fromRgba(materialBackgroundColorLight);
    QTest::newRow("import style, qmldir fallback, dark") << "importStyleWithQmlDirFallback.qml"
        << "" << QmlDirImport << Qt::ColorScheme::Dark
        << QColor::fromRgb(0xff0000) << QColor::fromRgba(materialBackgroundColorDark);
}

// Tests that a fallback style's (the Material style, in this case) theme settings
// are respected for both run-time and compile-time style selection.
void tst_StyleImports::fallbackStyleThemeRespected()
{
    QFETCH(QString, qmlFilePath);
    QFETCH(QString, runtimeStyle);
    QFETCH(FallbackMethod, fallbackMethod);
    QFETCH(Qt::ColorScheme, colorScheme);
    QFETCH(QColor, expectedButtonTextColor);
    QFETCH(QColor, expectedWindowColor);

    const char *materialThemeEnvVarName = "QT_QUICK_CONTROLS_MATERIAL_THEME";
    const QString originalMaterialTheme = qgetenv(materialThemeEnvVarName);
    qputenv(materialThemeEnvVarName, colorScheme == Qt::ColorScheme::Light ? "Light" : "Dark");

    // Only set this if it's not empty, because setting an empty style
    // will still cause it be resolved and we end up using the platform default.
    if (!runtimeStyle.isEmpty())
        QQuickStyle::setStyle(runtimeStyle);

    const char *fallbackStyleEnvVarName = "QT_QUICK_CONTROLS_FALLBACK_STYLE";
    const QString originalFallbackStyle = qgetenv(fallbackStyleEnvVarName);
    if (fallbackMethod == EnvVar)
        qputenv(fallbackStyleEnvVarName, "Material");

    auto cleanup = qScopeGuard([&]() {
        qputenv(materialThemeEnvVarName, qPrintable(originalMaterialTheme));
        qputenv(fallbackStyleEnvVarName, qPrintable(originalFallbackStyle));
    });

    QQuickControlsApplicationHelper helper(this, qmlFilePath, {},
        QStringList() << dataDirectory() + QLatin1String("/styles"));
    QVERIFY2(helper.ready, helper.failureMessage());

    auto button = helper.window->property("button").value<QQuickButton*>();
    QVERIFY(button);
    // contentItem should be a label with "salmon" text color.
    QCOMPARE(button->contentItem()->property("color").value<QColor>(), expectedButtonTextColor);
    QCOMPARE(helper.appWindow->color(), expectedWindowColor);

    // If using run-time style selection, check that QQuickStyle reports the correct values.
    // QQuickStyle is not supported when using compile-time style selection.
    if (!runtimeStyle.isEmpty()) {
        QCOMPARE(QQuickStyle::name(), runtimeStyle);
        QCOMPARE(QQuickStylePrivate::fallbackStyle(), "Material");
    }
}

void tst_StyleImports::attachedTypesAvailable_data()
{
    QTest::addColumn<QString>("import");

    // QtQuick.Controls import.
    QTest::newRow("Controls") << "";

    const QStringList styles = testStyles();
    for (const QString &styleImport : styles)
        QTest::newRow(qPrintable(styleImport)) << styleImport;
}

void tst_StyleImports::attachedTypesAvailable()
{
    QFETCH(QString, import);

    // If it's QtQuick.Controls, don't prepend anything.
    if (!import.isEmpty())
        import.prepend(QLatin1Char('.'));

    // Should not warn about missing types.
    QTest::failOnWarning(QRegularExpression(".?"));

    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData(QString::fromLatin1(R"(
        import QtQuick
        import QtQuick.Controls%1

        Item {
            SplitView {
                handle: Rectangle {
                    opacity: SplitHandle.hovered || SplitHandle.pressed ? 1.0 : 0.0
                }
                Item {} // Need these to ensure the handle is actually created, otherwise we won't get warnings.
                Item {}
            }

            Dialog {
                anchors.centerIn: Overlay.overlay
            }
        }
    )").arg(import).toLatin1(), QUrl());
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
}

QTEST_MAIN(tst_StyleImports)

#include "tst_styleimports.moc"
