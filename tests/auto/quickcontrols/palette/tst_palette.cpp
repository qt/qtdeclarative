// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtGui/qpalette.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickcombobox_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickTemplates2/private/qquicktooltip_p.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QSignalSpy>

using namespace QQuickVisualTestUtils;
using namespace QQuickControlsTestUtils;

class tst_palette : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_palette();

private slots:
    void initTestCase() override;

    void palette_data();
    void palette();

    void inheritance_data();
    void inheritance();
    void childPopupInheritance();

    void defaultPalette_data();
    void defaultPalette();

    void listView_data();
    void listView();

    void setDynamicallyCreatedPalette();
    void createBindings();
    void updateBindings();

    void resolve();

    void resetColor();
    void updateBindingPalette();

    void comboBoxPopup_data();
    void comboBoxPopup();
    void comboBoxPopupWithThemeDefault_data();
    void comboBoxPopupWithThemeDefault();

    void toolTipPaletteUpdate();
};

tst_palette::tst_palette()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_palette::initTestCase()
{
    QQuickStyle::setStyle("Basic");

    QQmlDataTest::initTestCase();

    // Import QtQuick.Controls to initialize styles and themes so that
    // QQuickControlPrivate::themePalette() returns a palette from the
    // style's theme instead of the platform's theme.
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick.Controls; Control { }", QUrl());
    delete component.create();
}

void tst_palette::palette_data()
{
    QTest::addColumn<QString>("testFile");
    QTest::addColumn<QPalette>("expectedPalette");

    const QPalette defaultPalette = QQuickTheme::palette(QQuickTheme::System);

    QTest::newRow("Control") << "palette-control-default.qml" << defaultPalette;
    QTest::newRow("AppWindow") << "palette-appwindow-default.qml" << defaultPalette;
    QTest::newRow("Popup") << "palette-popup-default.qml" << defaultPalette;

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
    customPalette.setColor(QPalette::PlaceholderText, QColor("magenta"));
    customPalette.setColor(QPalette::Accent, QColor("darkkhaki"));

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

    QCOMPARE(var.value<QQuickPalette*>()->toQPalette(), expectedPalette);
}

void tst_palette::inheritance_data()
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

    const QPalette defaultPalette = QQuickTheme::palette(QQuickTheme::System);

    auto windowPalette = QQuickWindowPrivate::get(window.get())->palette();

    QCOMPARE(windowPalette->toQPalette(), defaultPalette);

    auto controlPalette = control->property("palette").value<QQuickPalette*>();
    auto childPalette = child->property("palette").value<QQuickPalette*>();
    auto grandChildPalette = grandChild->property("palette").value<QQuickPalette*>();
    QVERIFY(controlPalette && childPalette && grandChildPalette);

    QCOMPARE(controlPalette->toQPalette(), defaultPalette);
    QCOMPARE(childPalette->toQPalette(), defaultPalette);
    QCOMPARE(grandChildPalette->toQPalette(), defaultPalette);

    childPalette->setBase(Qt::red);
    childPalette->setText(Qt::green);
    childPalette->setButton(Qt::blue);

    QCOMPARE(childPalette->base(), grandChildPalette->base());
    QCOMPARE(childPalette->text(), grandChildPalette->text());
    QCOMPARE(childPalette->button(), grandChildPalette->button());

    windowPalette->setWindow(Qt::gray);
    QCOMPARE(controlPalette->window(), windowPalette->window());

    childPalette->setWindow(Qt::red);
    QCOMPARE(childPalette->window(), Qt::red);

    grandChildPalette->setWindow(Qt::blue);
    QCOMPARE(grandChildPalette->window(), Qt::blue);

    auto childMo = child->metaObject();
    childMo->property(childMo->indexOfProperty("palette")).reset(child);
    QCOMPARE(childPalette->window(), windowPalette->window());
    QCOMPARE(grandChildPalette->window(), Qt::blue);

    auto grandChildMo = grandChild->metaObject();
    grandChildMo->property(grandChildMo->indexOfProperty("palette")).reset(grandChild);
    QCOMPARE(grandChildPalette->window(), windowPalette->window());
}

// The child popups in inheritance() don't test actual nested child popups,
// only popups that are children of items and the items within those popups.
// We need to specifically test this to prevent QTBUG-115707 from happening again.
void tst_palette::childPopupInheritance()
{
    QQuickControlsApplicationHelper helper(this, QLatin1String("childPopupInheritance.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());

    const auto *windowPrivate = QQuickWindowPrivate::get(helper.window);
    const auto windowsWindowTextColor = windowPrivate->palette()->toQPalette().color(QPalette::WindowText);

    // parentPopup sets windowText explicitly, so its label should use that color.
    auto *parentPopup = helper.appWindow->findChild<QQuickPopup *>("parentPopup");
    QVERIFY(parentPopup);
    parentPopup->open();
    QTRY_VERIFY(parentPopup->isOpened());
    auto *parentPopupLabel = helper.appWindow->findChild<QObject *>("parentPopupLabel");
    QVERIFY(parentPopupLabel);
    QCOMPARE(parentPopupLabel->property("color").value<QColor>(), "#ffdead");

    // All other child popups don't set anything explicitly, and should inherit from their window.
    auto *childPopup = helper.appWindow->findChild<QQuickPopup *>("childPopup");
    QVERIFY(childPopup);
    auto *childPopupLabel = helper.appWindow->findChild<QObject *>("childPopupLabel");
    QVERIFY(childPopupLabel);
    QCOMPARE(childPopupLabel->property("color").value<QColor>(), windowsWindowTextColor);

    auto *grandchildPopup = helper.appWindow->findChild<QQuickPopup *>("grandchildPopup");
    QVERIFY(grandchildPopup);
    auto *grandchildPopupLabel = helper.appWindow->findChild<QObject *>("grandchildPopupLabel");
    QVERIFY(grandchildPopupLabel);
    QCOMPARE(grandchildPopupLabel->property("color").value<QColor>(), windowsWindowTextColor);
}

class TestTheme : public QQuickTheme
{
public:
    static const uint NPalettes = QQuickTheme::Tumbler + 1;

    TestTheme()
    {
        for (uint i = 0; i < NPalettes; ++i)
            setPalette(static_cast<Scope>(i), QPalette(QColor::fromRgb(i)));
    }
};

Q_DECLARE_METATYPE(QQuickTheme::Scope)

void tst_palette::defaultPalette_data()
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

void tst_palette::defaultPalette()
{
    QFETCH(QString, control);
    QFETCH(QQuickTheme::Scope, scope);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QString("import QtQuick.Controls; %1 { }").arg(control).toUtf8(), QUrl());

    // The call to setData() above causes QQuickBasicTheme to be set as the current theme,
    // so we must make sure we only set our theme afterwards.
    std::unique_ptr<QQuickTheme> oldTheme(QQuickThemePrivate::instance.release());
    QQuickThemePrivate::instance.reset(new TestTheme);

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    QVariant var = object->property("palette");
    QVERIFY(var.isValid());

    QPalette expectedPalette = QQuickTheme::palette(scope);
    auto actualPalette = var.value<QQuickPalette*>();
    QVERIFY(actualPalette);
    QCOMPARE(actualPalette->toQPalette(), expectedPalette);

    QQuickThemePrivate::instance.reset(oldTheme.release());
}

void tst_palette::listView_data()
{
    QTest::addColumn<QString>("objectName");

    QTest::newRow("Control") << "control";
    QTest::newRow("Label") << "label";
    QTest::newRow("TextArea") << "textarea";
    QTest::newRow("TextField") << "textfield";
}

void tst_palette::listView()
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

    QCOMPARE(QQuickItemPrivate::get(control)->palette()->highlight(), Qt::red);
}

void tst_palette::setDynamicallyCreatedPalette()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("set-palette.qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    QVariant var = object->property("palette");
    QVERIFY(var.isValid());

    auto palette = var.value<QQuickPalette*>();
    QVERIFY(palette);

    QCOMPARE(palette->buttonText(), QColor("azure"));
    QCOMPARE(palette->button(), QColor("khaki"));

    QCOMPARE(palette->disabled()->buttonText(), QColor("lavender"));
    QCOMPARE(palette->disabled()->button(), QColor("coral"));
}

void tst_palette::createBindings()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("bindings.qml"));

    QScopedPointer<QObject> window(component.create());
    QVERIFY2(!window.isNull(), qPrintable(component.errorString()));

    auto disabledButton = window->property("disabledButton").value<QQuickButton*>();
    QVERIFY(disabledButton);

    auto enabledButton = window->property("enabledButton").value<QQuickButton*>();
    QVERIFY(enabledButton);

    QCOMPARE(QQuickItemPrivate::get(disabledButton)->palette()->button(), QColor("aqua"));
    QCOMPARE(QQuickItemPrivate::get(disabledButton)->palette()->buttonText(), QColor("azure"));

    QCOMPARE(QQuickItemPrivate::get(enabledButton)->palette()->button(), QColor("khaki"));
    QCOMPARE(QQuickItemPrivate::get(enabledButton)->palette()->buttonText(), QColor("bisque"));

    QCOMPARE(QQuickItemPrivate::get(enabledButton)->palette()->disabled()->button(), QColor("aqua"));
    QCOMPARE(QQuickItemPrivate::get(enabledButton)->palette()->disabled()->buttonText(), QColor("azure"));
}

void tst_palette::updateBindings()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("bindings.qml"));

    QScopedPointer<QObject> window(component.create());
    QVERIFY2(!window.isNull(), qPrintable(component.errorString()));

    auto disabledButton = window->property("disabledButton").value<QQuickButton*>();
    QVERIFY(disabledButton);

    auto enabledButton = window->property("enabledButton").value<QQuickButton*>();
    QVERIFY(enabledButton);

    QQuickItemPrivate::get(disabledButton)->palette()->disabled()->setButton(QColor("navy"));
    enabledButton->setEnabled(false);

    QCOMPARE(QQuickItemPrivate::get(enabledButton)->palette()->button(), QColor("navy"));
}

void tst_palette::resolve()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("resolve.qml"));

    QScopedPointer<QObject> window(component.create());
    QVERIFY2(!window.isNull(), qPrintable(component.errorString()));

    auto control = window->property("control").value<QQuickControl*>();
    QVERIFY(control);

    QCOMPARE(window->property("palette").value<QQuickPalette*>()->window(),
             control->property("palette").value<QQuickPalette*>()->window());
    QCOMPARE(window->property("palette").value<QQuickPalette*>()->windowText(),
             control->property("palette").value<QQuickPalette*>()->windowText());
    QMetaObject::invokeMethod(window.get(), "changeColors", Q_ARG(QVariant, QColor(Qt::red)));
    QVERIFY(window->property("palette").value<QQuickPalette*>()->window()
            != control->property("palette").value<QQuickPalette*>()->window());
    QCOMPARE(window->property("palette").value<QQuickPalette*>()->windowText(),
             control->property("palette").value<QQuickPalette*>()->windowText());
}

void tst_palette::resetColor()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("reset-color.qml"));

    QScopedPointer<QQuickApplicationWindow> window(qobject_cast<QQuickApplicationWindow*>(component.create()));
    QVERIFY2(!window.isNull(), qPrintable(component.errorString()));
    auto windowPalette = window->property("palette").value<QQuickPalette*>();
    QVERIFY(windowPalette);

    auto control = window->property("control").value<QQuickControl*>();
    QVERIFY(control);
    auto controlPalette = control->property("palette").value<QQuickPalette*>();
    QVERIFY(controlPalette);
    auto item1Palette = window->property("item1Palette").value<QQuickPalette*>();
    QVERIFY(item1Palette);
    auto item2Palette = window->property("item2Palette").value<QQuickPalette*>();
    QVERIFY(item2Palette);

    QCOMPARE(controlPalette->disabled()->window(), item2Palette->disabled()->window());
    QCOMPARE(controlPalette->disabled()->text(), item1Palette->disabled()->text());
    QCOMPARE(controlPalette->disabled()->windowText(), windowPalette->disabled()->windowText());

    {
        QSignalSpy spy(controlPalette, &QQuickPalette::changed);
        item1Palette->disabled()->setText(Qt::red);
        QVERIFY(spy.count() == 1 || spy.wait());
        QCOMPARE(controlPalette->disabled()->text(), QColor(Qt::red));
    }

    {
        QSignalSpy spy(controlPalette, &QQuickPalette::changed);
        item1Palette->disabled()->setWindowText(Qt::red);
        QVERIFY(spy.count() == 1 || spy.wait());
        QCOMPARE(controlPalette->disabled()->windowText(), QColor(Qt::red));
    }

    {
        QSignalSpy spy(controlPalette, &QQuickPalette::changed);
        item2Palette->disabled()->setWindowText(Qt::blue);
        QVERIFY(spy.count() == 1 || spy.wait());
        QCOMPARE(controlPalette->disabled()->windowText(), QColor(Qt::blue));
    }

    {
        QSignalSpy spy(controlPalette, &QQuickPalette::changed);
        QMetaObject::invokeMethod(window.get(), "resetColor", Qt::DirectConnection);
        QCOMPARE(controlPalette->window(), windowPalette->window());
        windowPalette->setWindow(Qt::green);
        QCOMPARE(controlPalette->window(), QColor(Qt::green));
        QVERIFY(spy.count() >= 2);
    }

    {
        QSignalSpy spy(controlPalette, &QQuickPalette::changed);
        QMetaObject::invokeMethod(window.get(), "resetGroup", Qt::DirectConnection);
        QCOMPARE(controlPalette->disabled()->windowText(), windowPalette->disabled()->windowText());
        windowPalette->disabled()->setWindow(Qt::blue);
        QCOMPARE(controlPalette->disabled()->window(), QColor(Qt::blue));
        item2Palette->disabled()->setWindow(Qt::red);
        QCOMPARE(controlPalette->disabled()->window(), QColor(Qt::blue));
        if (spy.count() == 0)
            spy.wait();
        QCOMPARE(spy.count(), 2);
    }
}

void tst_palette::updateBindingPalette()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("palette-appwindow-bindingpalette.qml"));

    QScopedPointer<QQuickApplicationWindow> window(qobject_cast<QQuickApplicationWindow*>(component.create()));
    QVERIFY2(!window.isNull(), qPrintable(component.errorString()));
    auto *windowPalette = window->property("palette").value<QQuickPalette *>();
    QVERIFY(windowPalette);
    auto *customPalette = window->property("cstmPalette").value<QQuickPalette *>();
    QVERIFY(customPalette);

    QCOMPARE(windowPalette->buttonText(), QColor("white"));

    QColor buttonTextColor("red");
    customPalette->setButtonText(buttonTextColor);
    QCOMPARE(customPalette->buttonText(), buttonTextColor);
    QCOMPARE(windowPalette->buttonText(), customPalette->buttonText());
}

void tst_palette::comboBoxPopup_data()
{
    QTest::addColumn<QString>("style");
    QTest::addColumn<QString>("qmlFilePath");

    QTest::newRow("Window, Basic") << "Basic" << "comboBoxPopupWithWindow.qml";
    QTest::newRow("ApplicationWindow, Basic") << "Basic" << "comboBoxPopupWithApplicationWindow.qml";
    QTest::newRow("Window, Fusion") << "Fusion" << "comboBoxPopupWithWindow.qml";
    QTest::newRow("ApplicationWindow, Fusion") << "Fusion" << "comboBoxPopupWithApplicationWindow.qml";
}

// Unlike regular popups, which should inherit their palette from the window and not the parent popup,
// combo box popups should inherit their palette from the combo box itself.
void tst_palette::comboBoxPopup()
{
    QFETCH(QString, style);
    QFETCH(QString, qmlFilePath);

    qmlClearTypeRegistrations();
    QQuickStyle::setStyle(style);

    QQuickApplicationHelper helper(this, qmlFilePath);
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    const auto *windowPalette = window->property("palette").value<QQuickPalette *>();
    QVERIFY(windowPalette);

    const auto *popup = window->property("popup").value<QQuickPopup *>();
    QVERIFY(popup);
    const auto *popupBackground = popup->background();
    QCOMPARE(popupBackground->property("color"), QColorConstants::Red);
    QCOMPARE(popupBackground->property("palette").value<QQuickPalette*>()->toQPalette().window().color(),
        QColorConstants::Red);

    // This has the default palette.
    const auto *topLevelComboBox = window->property("topLevelComboBox").value<QQuickComboBox *>();
    QVERIFY(topLevelComboBox);
    const auto *topLevelComboBoxBackground = topLevelComboBox->popup()->background();
    QCOMPARE_NE(topLevelComboBoxBackground->property("color"), QColorConstants::Red);
    QCOMPARE_NE(topLevelComboBoxBackground->property("palette").value<QQuickPalette*>()->toQPalette().window().color(),
        QColorConstants::Red);

    // The popup that this combo box is in has its window role set to red,
    // so the combo box's popup background should be red too.
    const auto *comboBoxInPopup = window->property("comboBoxInPopup").value<QQuickComboBox *>();
    QVERIFY(comboBoxInPopup);
    const auto *comboBoxInPopupBackground = comboBoxInPopup->popup()->background();
    QCOMPARE(comboBoxInPopupBackground->property("color"), QColorConstants::Red);
    QCOMPARE(comboBoxInPopupBackground->property("palette").value<QQuickPalette*>()->toQPalette().window().color(),
        QColorConstants::Red);
}

void tst_palette::comboBoxPopupWithThemeDefault_data()
{
    QTest::addColumn<QString>("style");
    QTest::addColumn<QColor>("expectedComboBoxPopupBackgroundColor");

    QTest::newRow("Basic") << "Basic" << QColor::fromRgb(0xFFFFFF);

    // We can't test Fusion because it uses the default application palette,
    // which is the default-constructed QPalette, so the test would always pass.
}

void tst_palette::comboBoxPopupWithThemeDefault()
{
    QFETCH(QString, style);
    QFETCH(QColor, expectedComboBoxPopupBackgroundColor);

    qmlClearTypeRegistrations();
    QQuickStyle::setStyle(style);

    QQuickApplicationHelper helper(this, "comboBoxPopupWithThemeDefault.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    const auto *comboBox = window->property("comboBox").value<QQuickComboBox *>();
    QVERIFY(comboBox);
    const auto *comboBoxBackground = comboBox->popup()->background();
    QCOMPARE(comboBoxBackground->property("color"), expectedComboBoxPopupBackgroundColor);
}

void tst_palette::toolTipPaletteUpdate()
{
    QQuickApplicationHelper helper(this, "toolTipPaletteUpdate.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *button = window->findChild<QQuickButton *>("button");
    QVERIFY(button);
    auto *attachedToolTip = button->findChild<QQuickToolTipAttached *>();
    QVERIFY(attachedToolTip);
    auto *toolTip = attachedToolTip->toolTip();
    QVERIFY(toolTip);

    auto windowPalette = QQuickWindowPrivate::get(window)->palette();
    auto toolTipPalette = QQuickPopupPrivate::get(toolTip)->palette();

    QCOMPARE(toolTipPalette->toolTipBase(), windowPalette->toolTipBase());
    QCOMPARE(toolTipPalette->toolTipText(), windowPalette->toolTipText());

    windowPalette->setToolTipBase(Qt::blue);
    windowPalette->setToolTipText(Qt::red);

    QCOMPARE(toolTipPalette->toolTipBase(), windowPalette->toolTipBase());
    QCOMPARE(toolTipPalette->toolTipText(), windowPalette->toolTipText());
}

QTEST_MAIN(tst_palette)

#include "tst_palette.moc"
