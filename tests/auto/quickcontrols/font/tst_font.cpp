// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/qtest.h>

#include <QtGui/qfont.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p_p.h>
#include <QtQuickControls2/qquickstyle.h>

class tst_font : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_font();

private slots:
    void systemFont();

    void font_data();
    void font();

    void inheritance_data();
    void inheritance();

    void defaultFont_data();
    void defaultFont();

    void listView_data();
    void listView();

    void resolve();
};

static QFont testFont()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick; import QtQuick.Controls; Text { }", QUrl());

    QScopedPointer<QObject> object(component.create());
    Q_ASSERT_X(!object.isNull(), "testFont", qPrintable(component.errorString()));

    QVariant var = object->property("font");
    Q_ASSERT_X(var.isValid(), "testFont", var.typeName());
    return var.value<QFont>();
}

tst_font::tst_font()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    QQuickStyle::setStyle("Basic");
}

void tst_font::systemFont()
{
    QSKIP("QTBUG-70063: qmlClearTypeRegistrations() call causes crash");

    const QFont *originalSystemFont = QGuiApplicationPrivate::platformTheme()->font(QPlatformTheme::SystemFont);
    if (!originalSystemFont)
        QSKIP("Cannot test the system font on a minimal platform");

    const QFont fontBefore = testFont();
    QCOMPARE(fontBefore, *originalSystemFont);

    qmlClearTypeRegistrations();
    delete QGuiApplicationPrivate::app_font;
    QGuiApplicationPrivate::app_font = nullptr;

    const QFont appFont = QGuiApplication::font();
    QCOMPARE(appFont, *originalSystemFont);

    const QFont fontAfter = testFont();
    QCOMPARE(fontAfter, *originalSystemFont);
}

void tst_font::font_data()
{
    QTest::addColumn<QString>("testFile");
    QTest::addColumn<QFont>("expectedFont");

    QTest::newRow("Control") << "font-control-default.qml" << QFont();
    QTest::newRow("AppWindow") << "font-appwindow-default.qml" << QFont();
    QTest::newRow("Popup") << "font-popup-default.qml" << QFont();

    QFont customFont;
    customFont.setCapitalization(QFont::AllUppercase);
    customFont.setFamilies(QStringList{QLatin1String("Courier")});
    customFont.setItalic(true);
    customFont.setPixelSize(60);
    customFont.setStrikeOut(true);
    customFont.setUnderline(true);
    customFont.setWeight(QFont::DemiBold);

    QTest::newRow("Control:custom") << "font-control-custom.qml" << customFont;
    QTest::newRow("AppWindow:custom") << "font-appwindow-custom.qml" << customFont;
    QTest::newRow("Popup:custom") << "font-popup-custom.qml" << customFont;
}

void tst_font::font()
{
    QFETCH(QString, testFile);
    QFETCH(QFont, expectedFont);

    if (QSysInfo::productType().compare(QLatin1String("osx"), Qt::CaseInsensitive) == 0
            && qgetenv("QTEST_ENVIRONMENT").split(' ').contains("CI")) {
        QSKIP("This test crashes on macOS: QTBUG-70063");
    }

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl(testFile));

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    QVariant var = object->property("font");
    QVERIFY(var.isValid());

    QFont actualFont = var.value<QFont>();
    QCOMPARE(actualFont, expectedFont);
}

void tst_font::inheritance_data()
{
    QTest::addColumn<QString>("testFile");

    QTest::newRow("Control") << "inheritance-control.qml";
    QTest::newRow("Child Control") << "inheritance-childcontrol.qml";
    QTest::newRow("Dynamic Control") << "inheritance-dynamiccontrol.qml";
    QTest::newRow("Dynamic Child Control") << "inheritance-dynamicchildcontrol.qml";

    QTest::newRow("Popup") << "inheritance-popup.qml";
    QTest::newRow("Child Popup") << "inheritance-childpopup.qml";
    QTest::newRow("Dynamic Popup") << "inheritance-dynamicpopup.qml";
    QTest::newRow("Dynamic Child Popup") << "inheritance-dynamicchildpopup.qml";
}

void tst_font::inheritance()
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

    QCOMPARE(window->font(), QFont());

    QCOMPARE(control->property("font").value<QFont>(), QFont());
    QCOMPARE(child->property("font").value<QFont>(), QFont());
    QCOMPARE(grandChild->property("font").value<QFont>(), QFont());

    QFont childFont;
    childFont.setFamilies(QStringList{QLatin1String("Arial")});
    childFont.setPixelSize(80);
    childFont.setItalic(true);
    child->setProperty("font", childFont);
    QCOMPARE(child->property("font").value<QFont>(), childFont);
    QCOMPARE(grandChild->property("font").value<QFont>(), childFont);

    QFont grandChildFont(childFont);
    grandChildFont.setFamilies(QStringList{QLatin1String("Times New Roman")});
    grandChildFont.setUnderline(true);
    grandChild->setProperty("font", grandChildFont);
    QCOMPARE(child->property("font").value<QFont>(), childFont);
    QCOMPARE(grandChild->property("font").value<QFont>(), grandChildFont);

    QFont windowFont;
    windowFont.setWeight(QFont::Thin);
    window->setFont(windowFont);
    QCOMPARE(window->font(), windowFont);
    QCOMPARE(control->property("font").value<QFont>(), windowFont);

    childFont.setWeight(QFont::Thin);
    QCOMPARE(child->property("font").value<QFont>(), childFont);

    grandChildFont.setWeight(QFont::Thin);
    QCOMPARE(grandChild->property("font").value<QFont>(), grandChildFont);

    child->setProperty("font", QVariant());
    QCOMPARE(child->property("font").value<QFont>(), windowFont);
    QCOMPARE(grandChild->property("font").value<QFont>(), grandChildFont);

    grandChild->setProperty("font", QVariant());
    QCOMPARE(grandChild->property("font").value<QFont>(), windowFont);
}

class TestFontTheme : public QQuickTheme
{
public:
    static const int NFonts = QQuickTheme::Tumbler + 1;

    TestFontTheme()
    {
        for (int i = 0; i < NFonts; ++i) {
            QFont font = QFont();
            font.setPixelSize(i + 10);
            setFont(static_cast<Scope>(i), font);
        }
    }
};

Q_DECLARE_METATYPE(QQuickTheme::Scope)

void tst_font::defaultFont_data()
{
    QTest::addColumn<QString>("control");
    QTest::addColumn<QQuickTheme::Scope>("scope");

    QTest::newRow("AbstractButton") << "AbstractButton" << QQuickTheme::System;
    QTest::newRow("ApplicationWindow") << "ApplicationWindow" << QQuickTheme::System;
    QTest::newRow("Button") << "Button" << QQuickTheme::Button;
    QTest::newRow("CheckBox") << "CheckBox" << QQuickTheme::CheckBox;
    QTest::newRow("CheckDelegate") << "CheckDelegate" << QQuickTheme::ListView;
    QTest::newRow("ComboBox") << "ComboBox" << QQuickTheme::ComboBox;
    QTest::newRow("Container") << "Container" << QQuickTheme::System;
    QTest::newRow("Control") << "Control" << QQuickTheme::System;
    QTest::newRow("Dial") << "Dial" << QQuickTheme::System;
    QTest::newRow("Dialog") << "Dialog" << QQuickTheme::System;
    QTest::newRow("DialogButtonBox") << "DialogButtonBox" << QQuickTheme::System;
    QTest::newRow("Drawer") << "Drawer" << QQuickTheme::System;
    QTest::newRow("Frame") << "Frame" << QQuickTheme::System;
    QTest::newRow("GroupBox") << "GroupBox" << QQuickTheme::GroupBox;
    QTest::newRow("ItemDelegate") << "ItemDelegate" << QQuickTheme::ItemView;
    QTest::newRow("Label") << "Label" << QQuickTheme::Label;
    QTest::newRow("Menu") << "Menu" << QQuickTheme::Menu;
    QTest::newRow("MenuItem") << "MenuItem" << QQuickTheme::Menu;
    QTest::newRow("MenuSeparator") << "MenuSeparator" << QQuickTheme::Menu;
    QTest::newRow("Page") << "Page" << QQuickTheme::System;
    QTest::newRow("Pane") << "Pane" << QQuickTheme::System;
    QTest::newRow("Popup") << "Popup" << QQuickTheme::System;
    QTest::newRow("ProgressBar") << "ProgressBar" << QQuickTheme::System;
    QTest::newRow("RadioButton") << "RadioButton" << QQuickTheme::RadioButton;
    QTest::newRow("RadioDelegate") << "RadioDelegate" << QQuickTheme::ListView;
    QTest::newRow("RangeSlider") << "RangeSlider" << QQuickTheme::System;
    QTest::newRow("RoundButton") << "RoundButton" << QQuickTheme::Button;
    QTest::newRow("ScrollBar") << "ScrollBar" << QQuickTheme::System;
    QTest::newRow("ScrollIndicator") << "ScrollIndicator" << QQuickTheme::System;
    QTest::newRow("Slider") << "Slider" << QQuickTheme::System;
    QTest::newRow("SpinBox") << "SpinBox" << QQuickTheme::SpinBox;
    QTest::newRow("SwipeDelegate") << "SwipeDelegate" << QQuickTheme::ListView;
    QTest::newRow("Switch") << "Switch" << QQuickTheme::Switch;
    QTest::newRow("SwitchDelegate") << "SwitchDelegate" << QQuickTheme::ListView;
    QTest::newRow("TabBar") << "TabBar" << QQuickTheme::TabBar;
    QTest::newRow("TabButton") << "TabButton" << QQuickTheme::TabBar;
    QTest::newRow("TextArea") << "TextArea" << QQuickTheme::TextArea;
    QTest::newRow("TextField") << "TextField" << QQuickTheme::TextField;
    QTest::newRow("ToolBar") << "ToolBar" << QQuickTheme::ToolBar;
    QTest::newRow("ToolButton") << "ToolButton" << QQuickTheme::ToolBar;
    QTest::newRow("ToolSeparator") << "ToolSeparator" << QQuickTheme::ToolBar;
    QTest::newRow("ToolTip") << "ToolTip" << QQuickTheme::ToolTip;
    QTest::newRow("Tumbler") << "Tumbler" << QQuickTheme::Tumbler;
}

void tst_font::defaultFont()
{
    QFETCH(QString, control);
    QFETCH(QQuickTheme::Scope, scope);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QString("import QtQuick.Controls; %1 { }").arg(control).toUtf8(), QUrl());

    // The call to setData() above causes QQuickBasicTheme to be set as the current theme,
    // so we must make sure we only set our theme afterwards.
    QQuickThemePrivate::instance.reset(new TestFontTheme);

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    QVariant var = object->property("font");
    QVERIFY(var.isValid());

    QFont expectedFont = QQuickTheme::font(scope);
    QFont actualFont = var.value<QFont>();
    QCOMPARE(actualFont, expectedFont);
}

void tst_font::listView_data()
{
    QTest::addColumn<QString>("objectName");

    QTest::newRow("Control") << "control";
    QTest::newRow("Label") << "label";
    QTest::newRow("TextArea") << "textarea";
    QTest::newRow("TextField") << "textfield";
}

void tst_font::listView()
{
    QFETCH(QString, objectName);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("listview.qml"));

    QScopedPointer<QQuickApplicationWindow> window(qobject_cast<QQuickApplicationWindow *>(component.create()));
    QVERIFY2(!window.isNull(), qPrintable(component.errorString()));

    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));

    QQuickItem *listView = window->property("listView").value<QQuickItem *>();
    QVERIFY(listView);

    QQuickItem *contentItem = listView->property("contentItem").value<QQuickItem *>();
    QVERIFY(contentItem);

    QVERIFY(QMetaObject::invokeMethod(listView, "forceLayout"));

    QQuickItem *column = contentItem->childItems().value(0);
    QVERIFY(column);

    QQuickItem *control = column->property(objectName.toUtf8()).value<QQuickItem *>();
    QVERIFY(control);

    QCOMPARE(control->property("font").value<QFont>().pixelSize(), 55);
}

void tst_font::resolve()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("resolve.qml"));

    QScopedPointer<QObject> window(component.create());
    QVERIFY2(!window.isNull(), qPrintable(component.errorString()));

    auto control1 = window->property("control1").value<QQuickControl*>();
    QVERIFY(control1);

    QCOMPARE(window->property("font").value<QFont>().pointSize(),
             control1->property("font").value<QFont>().pointSize());
    QCOMPARE(window->property("font").value<QFont>().weight(),
             control1->property("font").value<QFont>().weight());
    QMetaObject::invokeMethod(window.get(), "changeFont1", Q_ARG(QVariant, QFont("Helvetica", 12, 12)));
    QVERIFY(window->property("font").value<QFont>().pointSize()
             != control1->property("font").value<QFont>().pointSize());
    QCOMPARE(window->property("font").value<QFont>().weight(),
             control1->property("font").value<QFont>().weight());

    QMetaObject::invokeMethod(window.get(), "changeFont2", Q_ARG(QVariant, QFont("Helvetica", 20, 20)));
    auto control2 = window->property("control2").value<QQuickControl*>();
    auto control2ChildControl = window->property("control2ChildControl").value<QQuickControl*>();
    auto control4 = window->property("control4").value<QQuickControl*>();
    QVERIFY(control2);
    QVERIFY(control2ChildControl);
    QVERIFY(control4);
    auto control2Font = control2->property("font").value<QFont>();
    auto control2ChildControlFont = control2ChildControl->property("font").value<QFont>();
    auto control4Font = control4->property("font").value<QFont>();
    QCOMPARE((int)control4Font.resolveMask(), 0);
    QCOMPARE(control4Font.resolveMask(), control2ChildControlFont.resolveMask());
    QCOMPARE(control2ChildControlFont, control2Font);
    QVERIFY(control2ChildControlFont != control4Font);
}

QTEST_MAIN(tst_font)

#include "tst_font.moc"
