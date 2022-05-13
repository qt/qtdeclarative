// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qregularexpression.h>
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

    QTest::newRow("control=Action,style=basic,fallback=mat") << "Action.qml" << "Basic" << "Material" << "";
    QTest::newRow("control=Action,style=fs,fallback=mat") << "Action.qml" << "FileSystemStyle" << "Material" << "FileSystemStyle";
    QTest::newRow("control=Action,style=qrc,fallback=mat") << "Action.qml" << "ResourceStyle" << "Material" << "Basic";
    QTest::newRow("control=Action,style=nosuch,fallback=mat") << "Action.qml" << "NoSuchStyle" << "Material" << "Basic";

    // Amongst the styles we're testing here, ScrollView.qml only exists in the Basic style.
    QTest::newRow("control=ScrollView,style=basic,fallback=empty") << "ScrollView.qml" << "Basic" << "" << "Basic";
    QTest::newRow("control=ScrollView,style=fs,fallback=empty") << "ScrollView.qml" << "FileSystemStyle" << "" << "Basic";
    QTest::newRow("control=ScrollView,style=qrc,fallback=empty") << "ScrollView.qml" << "ResourceStyle" << "" << "Basic";
    QTest::newRow("control=ScrollView,style=nosuch,fallback=empty") << "ScrollView.qml" << "NoSuchStyle" << "" << "Basic";

    QTest::newRow("control=ScrollView,style=basic,fallback=mat") << "ScrollView.qml" << "Basic" << "Material" << "Basic";
    QTest::newRow("control=ScrollView,style=fs,fallback=mat") << "ScrollView.qml" << "FileSystemStyle" << "Material" << "Basic";
    QTest::newRow("control=ScrollView,style=qrc,fallback=mat") << "ScrollView.qml" << "ResourceStyle" << "Material" << "Basic";
    QTest::newRow("control=ScrollView,style=nosuch,fallback=mat") << "ScrollView.qml" << "NoSuchStyle" << "Material" << "Basic";

    // Label.qml exists in the FileSystemStyle, Basic and Material styles.
    QTest::newRow("control=Label,style=basic,fallback=empty") << "Label.qml" << "Basic" << "" << "Basic";
    QTest::newRow("control=Label,style=fs,fallback=empty") << "Label.qml" << "FileSystemStyle" << "" << "FileSystemStyle";
    QTest::newRow("control=Label,style=qrc,fallback=empty") << "Label.qml" << "ResourceStyle" << "" << "Basic";
    QTest::newRow("control=Label,style=nosuch,fallback=empty") << "Label.qml" << "NoSuchStyle" << "" << "Basic";

    QTest::newRow("control=Label,style=basic,fallback=mat") << "Label.qml" << "Basic" << "Material" << "Basic";
    QTest::newRow("control=Label,style=fs,fallback=mat") << "Label.qml" << "FileSystemStyle" << "Material" << "FileSystemStyle";
    QTest::newRow("control=Label,style=qrc,fallback=mat") << "Label.qml" << "ResourceStyle" << "Material" << "Basic";
    QTest::newRow("control=Label,style=nosuch,fallback=mat") << "Label.qml" << "NoSuchStyle" << "Material" << "Basic";

    // Button.qml exists in all styles including the fs and qrc styles
    QTest::newRow("control=Button,style=basic,fallback=empty") << "Button.qml" << "Basic" << "" << "Basic";
    QTest::newRow("control=Button,style=fs,fallback=empty") << "Button.qml" << "FileSystemStyle" << "" << "FileSystemStyle";
    QTest::newRow("control=Button,style=qrc,fallback=empty") << "Button.qml" << "ResourceStyle" << "" << "ResourceStyle";
    QTest::newRow("control=Button,style=nosuch,fallback=empty") << "Button.qml" << "NoSuchStyle" << "" << "Basic";

    QTest::newRow("control=Button,style=basic,fallback=mat") << "Button.qml" << "Basic" << "Material" << "Basic";
    QTest::newRow("control=Button,style=fs,fallback=mat") << "Button.qml" << "FileSystemStyle" << "Material" << "FileSystemStyle";
    QTest::newRow("control=Button,style=qrc,fallback=mat") << "Button.qml" << "ResourceStyle" << "Material" << "ResourceStyle";
    QTest::newRow("control=Button,style=nosuch,fallback=mat") << "Button.qml" << "NoSuchStyle" << "Material" << "Basic";
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

    // TODO: test built-in styles below too
    // We can't check for the attached style object since that API is in a plugin,
    // and it's not possible to use e.g. the baseUrl of the QQmlContext
    // nor the metaObject to test it either.

    if (!QQuickStylePrivate::builtInStyles().contains(expected)) {
        // We're expecting a custom style.
        QCOMPARE(object->objectName(), expected);
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

void tst_StyleImports::fallbackStyleShouldNotOverwriteTheme_data()
{
    QTest::addColumn<QString>("style");
    QTest::addColumn<QString>("fallbackStyle");
    QTest::addColumn<QColor>("expectedContentItemColor");

    QTest::addRow("style=Fusion,fallbackStyle=Material")
        << QString::fromLatin1("Fusion") << QString::fromLatin1("Material") << QColor::fromRgb(0x252525);
    QTest::addRow("style=ResourceStyle,fallbackStyle=Material")
        << QString::fromLatin1("ResourceStyle") << QString::fromLatin1("Material") << QColor("salmon");
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

QTEST_MAIN(tst_StyleImports)

#include "tst_styleimports.moc"
