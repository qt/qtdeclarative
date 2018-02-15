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

#include <QtGui/qfont.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

using namespace QQuickVisualTestUtil;

class tst_font : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void font_data();
    void font();

    void inheritance_data();
    void inheritance();

    void defaultFont_data();
    void defaultFont();

    void listView_data();
    void listView();
};

void tst_font::font_data()
{
    QTest::addColumn<QString>("testFile");
    QTest::addColumn<QFont>("expectedFont");

    QTest::newRow("Control") << "font-control-default.qml" << QFont();
    QTest::newRow("AppWindow") << "font-appwindow-default.qml" << QFont();
    QTest::newRow("Popup") << "font-popup-default.qml" << QFont();

    QFont customFont;
    customFont.setCapitalization(QFont::AllUppercase);
    customFont.setFamily("Courier");
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
    childFont.setFamily("Arial");
    childFont.setPixelSize(80);
    childFont.setItalic(true);
    child->setProperty("font", childFont);
    QCOMPARE(child->property("font").value<QFont>(), childFont);
    QCOMPARE(grandChild->property("font").value<QFont>(), childFont);

    QFont grandChildFont(childFont);
    grandChildFont.setFamily("Times New Roman");
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

class TestFontTheme : public QQuickProxyTheme
{
public:
    TestFontTheme(QPlatformTheme *theme) : QQuickProxyTheme(theme)
    {
        std::fill(fonts, fonts + QQuickTheme::NFonts, static_cast<QFont *>(0));

        for (int i = QQuickTheme::SystemFont; i < QQuickTheme::NFonts; ++i) {
            QFont font = QFont();
            font.setPixelSize(i + 10);
            fonts[i] = new QFont(font);
        }

        QGuiApplicationPrivate::platform_theme = this;
    }

    const QFont *font(Font type = SystemFont) const override
    {
        return fonts[type];
    }

private:
    QFont *fonts[QQuickTheme::NFonts];
};

Q_DECLARE_METATYPE(QQuickTheme::Font)

void tst_font::defaultFont_data()
{
    QTest::addColumn<QString>("control");
    QTest::addColumn<QQuickTheme::Font>("fontType");

    QTest::newRow("AbstractButton") << "AbstractButton" << QQuickTheme::SystemFont;
    QTest::newRow("ApplicationWindow") << "ApplicationWindow" << QQuickTheme::SystemFont;
    QTest::newRow("Button") << "Button" << QQuickTheme::PushButtonFont;
    QTest::newRow("CheckBox") << "CheckBox" << QQuickTheme::CheckBoxFont;
    QTest::newRow("CheckDelegate") << "CheckDelegate" << QQuickTheme::ListViewFont;
    QTest::newRow("ComboBox") << "ComboBox" << QQuickTheme::ComboMenuItemFont;
    QTest::newRow("Container") << "Container" << QQuickTheme::SystemFont;
    QTest::newRow("Control") << "Control" << QQuickTheme::SystemFont;
    QTest::newRow("Dial") << "Dial" << QQuickTheme::SystemFont;
    QTest::newRow("Dialog") << "Dialog" << QQuickTheme::SystemFont;
    QTest::newRow("DialogButtonBox") << "DialogButtonBox" << QQuickTheme::SystemFont;
    QTest::newRow("Drawer") << "Drawer" << QQuickTheme::SystemFont;
    QTest::newRow("Frame") << "Frame" << QQuickTheme::SystemFont;
    QTest::newRow("GroupBox") << "GroupBox" << QQuickTheme::GroupBoxTitleFont;
    QTest::newRow("ItemDelegate") << "ItemDelegate" << QQuickTheme::ItemViewFont;
    QTest::newRow("Label") << "Label" << QQuickTheme::LabelFont;
    QTest::newRow("Menu") << "Menu" << QQuickTheme::MenuFont;
    QTest::newRow("MenuItem") << "MenuItem" << QQuickTheme::MenuItemFont;
    QTest::newRow("MenuSeparator") << "MenuSeparator" << QQuickTheme::SystemFont;
    QTest::newRow("Page") << "Page" << QQuickTheme::SystemFont;
    QTest::newRow("Pane") << "Pane" << QQuickTheme::SystemFont;
    QTest::newRow("Popup") << "Popup" << QQuickTheme::SystemFont;
    QTest::newRow("ProgressBar") << "ProgressBar" << QQuickTheme::SystemFont;
    QTest::newRow("RadioButton") << "RadioButton" << QQuickTheme::RadioButtonFont;
    QTest::newRow("RadioDelegate") << "RadioDelegate" << QQuickTheme::ListViewFont;
    QTest::newRow("RangeSlider") << "RangeSlider" << QQuickTheme::SystemFont;
    QTest::newRow("RoundButton") << "RoundButton" << QQuickTheme::PushButtonFont;
    QTest::newRow("ScrollBar") << "ScrollBar" << QQuickTheme::SystemFont;
    QTest::newRow("ScrollIndicator") << "ScrollIndicator" << QQuickTheme::SystemFont;
    QTest::newRow("Slider") << "Slider" << QQuickTheme::SystemFont;
    QTest::newRow("SpinBox") << "SpinBox" << QQuickTheme::EditorFont;
    QTest::newRow("SwipeDelegate") << "SwipeDelegate" << QQuickTheme::ListViewFont;
    QTest::newRow("Switch") << "Switch" << QQuickTheme::SystemFont; // ### TODO: add QQuickTheme::SwitchFont
    QTest::newRow("SwitchDelegate") << "SwitchDelegate" << QQuickTheme::ListViewFont;
    QTest::newRow("TabBar") << "TabBar" << QQuickTheme::SystemFont;
    QTest::newRow("TabButton") << "TabButton" << QQuickTheme::TabButtonFont;
    QTest::newRow("TextArea") << "TextArea" << QQuickTheme::EditorFont;
    QTest::newRow("TextField") << "TextField" << QQuickTheme::EditorFont;
    QTest::newRow("ToolBar") << "ToolBar" << QQuickTheme::SystemFont;
    QTest::newRow("ToolButton") << "ToolButton" << QQuickTheme::ToolButtonFont;
    QTest::newRow("ToolSeparator") << "ToolSeparator" << QQuickTheme::SystemFont;
    QTest::newRow("ToolTip") << "ToolTip" << QQuickTheme::TipLabelFont;
    QTest::newRow("Tumbler") << "Tumbler" << QQuickTheme::ItemViewFont;
}

void tst_font::defaultFont()
{
    QFETCH(QString, control);
    QFETCH(QQuickTheme::Font, fontType);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QString("import QtQuick.Controls 2.2; %1 { }").arg(control).toUtf8(), QUrl());

    // The call to setData() above causes QQuickDefaultTheme to be set as the platform theme,
    // so we must make sure we only set our theme afterwards.
    TestFontTheme theme(QGuiApplicationPrivate::platform_theme);

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    QVariant var = object->property("font");
    QVERIFY(var.isValid());

    const QFont *expectedFont = theme.font(fontType);
    QVERIFY(expectedFont);

    QFont actualFont = var.value<QFont>();

    if (actualFont != *expectedFont) {
        qDebug() << QTest::currentDataTag() << actualFont << *expectedFont;
    }

    QCOMPARE(actualFont, *expectedFont);
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

QTEST_MAIN(tst_font)

#include "tst_font.moc"
