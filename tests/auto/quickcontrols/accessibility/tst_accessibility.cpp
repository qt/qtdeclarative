// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qloggingcategory.h>
#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>

#if QT_CONFIG(accessibility)
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/qpa/qplatformaccessibility.h>
#include <QtQuick/private/qquickaccessibleattached_p.h>
#endif

Q_LOGGING_CATEGORY(lcNoTransparentText, "qt.quick.controls.tests.accessibility.notransparenttext")

using namespace QQuickControlsTestUtils;

class tst_accessibility : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_accessibility();

private slots:
    void initTestCase() override;

    void a11y_data();
    void a11y();

    void override_data();
    void override();

    void ordering();

    void actionAccessibility();
    void actionAccessibilityImplicitName();

    void sliderTest();

    void accessibleName();
    void locale();

    void defaultButton();
    void noTransparentText();

private:
    /*
        This is a pointer because:

        #1: Using a new engine for each test row is very slow.
        #2: qmlClearTypeRegistrations cannot be called while an engine exists.

        Until QTBUG-134198 is implemented, we need to keep the engine around as long as possible
        for #1, and destroy (and recreate) it when necessary for #2.
    */
    std::unique_ptr<QQmlEngine> engine;
};

#if QT_CONFIG(accessibility)
static QPlatformAccessibility *platformAccessibility()
{
    QPlatformIntegration *pfIntegration = QGuiApplicationPrivate::platformIntegration();
    return pfIntegration ? pfIntegration->accessibility() : nullptr;
}
#endif

QString adjustFileBaseName(const QString &fileBaseName)
{
#if !QT_CONFIG(accessibility)
    if (fileBaseName == QLatin1Literal("dayofweekrow")
            || fileBaseName == QLatin1Literal("monthgrid")
            || fileBaseName == QLatin1Literal("weeknumbercolumn"))
        return fileBaseName += QLatin1Literal("-2");
#else
    return fileBaseName;
#endif
}

QQuickItem *findItem(QObject *object)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item) {
        QQuickPopup *popup = qobject_cast<QQuickPopup *>(object);
        if (popup)
            item = popup->popupItem();
    }
    return item;
}

tst_accessibility::tst_accessibility()
    : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::FailOnWarnings)
{
}

void tst_accessibility::initTestCase()
{
    QQmlDataTest::initTestCase();
#if !defined(Q_OS_ANDROID)
    // StyleInfo is not supported when cross-compiling: QTBUG-100191.
    StyleInfo::instance()->initialize(QQC2_IMPORT_PATH);
#endif
    engine.reset(new QQmlEngine);
}


void tst_accessibility::a11y_data()
{
    QTest::addColumn<QString>("fileBaseName");
    QTest::addColumn<QAccessible::Role>("role");
    QTest::addColumn<QString>("text");

    QTest::newRow("AbstractButton") << "abstractbutton" << QAccessible::Button << "AbstractButton";
    QTest::newRow("BusyIndicator") << "busyindicator" << QAccessible::Indicator << "";
    QTest::newRow("Button") << "button" << QAccessible::Button << "Button";
    QTest::newRow("CheckBox") << "checkbox" << QAccessible::CheckBox << "CheckBox";
    QTest::newRow("CheckDelegate") << "checkdelegate" << QAccessible::CheckBox << "CheckDelegate";
    QTest::newRow("ComboBox") << "combobox" << QAccessible::ComboBox << "ComboBox";
    QTest::newRow("Container") << "container" << QAccessible::NoRole << "";
    QTest::newRow("Control") << "control" << QAccessible::NoRole << "";
    QTest::newRow("Dial") << "dial" << QAccessible::Dial << "";
    QTest::newRow("Dialog") << "dialog" << QAccessible::Dialog << "Dialog";
    QTest::newRow("Drawer") << "drawer" << QAccessible::Dialog << "";
    QTest::newRow("Frame") << "frame" << QAccessible::Border << "";
    QTest::newRow("GroupBox") << "groupbox" << QAccessible::Grouping << "GroupBox";
    QTest::newRow("ItemDelegate") << "itemdelegate" << QAccessible::ListItem << "ItemDelegate";
    QTest::newRow("Label") << "label" << QAccessible::StaticText << "Label";
    QTest::newRow("Menu") << "menu" << QAccessible::PopupMenu << "";
    QTest::newRow("MenuItem") << "menuitem" << QAccessible::MenuItem << "MenuItem";
    QTest::newRow("Page") << "page" << QAccessible::Pane << "Page";
    QTest::newRow("PageIndicator") << "pageindicator" << QAccessible::Indicator << "";
    QTest::newRow("Pane") << "pane" << QAccessible::Pane << "";
    QTest::newRow("Popup") << "popup" << QAccessible::Dialog << "";
    QTest::newRow("ProgressBar") << "progressbar" << QAccessible::ProgressBar << "";
    QTest::newRow("RadioButton") << "radiobutton" << QAccessible::RadioButton << "RadioButton";
    QTest::newRow("RadioDelegate") << "radiodelegate" << QAccessible::RadioButton << "RadioDelegate";
    QTest::newRow("RangeSlider") << "rangeslider" << QAccessible::Slider << "";
    QTest::newRow("RoundButton") << "roundbutton" << QAccessible::Button << "RoundButton";
    QTest::newRow("ScrollBar") << "scrollbar" << QAccessible::ScrollBar << "";
    QTest::newRow("ScrollIndicator") << "scrollindicator" << QAccessible::Indicator << "";
    QTest::newRow("Slider") << "slider" << QAccessible::Slider << "";
    QTest::newRow("SpinBox") << "spinbox" << QAccessible::SpinBox << "";
    QTest::newRow("StackView") << "stackview" << QAccessible::LayeredPane << "";
    QTest::newRow("SwipeDelegate") << "swipedelegate" << QAccessible::ListItem << "SwipeDelegate";
    QTest::newRow("SwipeView") << "swipeview" << QAccessible::PageTabList << "";
    QTest::newRow("Switch") << "switch" << QAccessible::CheckBox << "Switch";
    QTest::newRow("SwitchDelegate") << "switchdelegate" << QAccessible::ListItem << "SwitchDelegate";
    QTest::newRow("TabBar") << "tabbar" << QAccessible::PageTabList << "";
    QTest::newRow("TabButton") << "tabbutton" << QAccessible::PageTab << "TabButton";
    QTest::newRow("TextArea") << "textarea" << QAccessible::EditableText << "";
    QTest::newRow("TextField") << "textfield" << QAccessible::EditableText << "";
    QTest::newRow("ToolBar") << "toolbar" << QAccessible::ToolBar << "";
    QTest::newRow("ToolButton") << "toolbutton" << QAccessible::Button << "ToolButton";
    QTest::newRow("ToolTip") << "tooltip" << QAccessible::ToolTip << "ToolTip";
    QTest::newRow("Tumbler") << "tumbler" << QAccessible::NoRole << ""; // TODO
}

void tst_accessibility::a11y()
{
    QFETCH(QString, fileBaseName);
    QFETCH(QAccessible::Role, role);
    QFETCH(QString, text);

    QQmlComponent component(engine.get());
    component.loadUrl(testFileUrl("defaults/" + adjustFileBaseName(fileBaseName) + ".qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    QQuickItem *item = findItem(object.data());
    QVERIFY(item);

#if QT_CONFIG(accessibility)
    QQuickAccessibleAttached *attached = QQuickAccessibleAttached::attachedProperties(item);
    if (QAccessible::isActive()) {
        QVERIFY(attached);
    } else {
        QVERIFY(!attached);
        QPlatformAccessibility *accessibility = platformAccessibility();
        if (!accessibility)
            QSKIP("No QPlatformAccessibility available.");
        accessibility->setActive(true);
        attached = QQuickAccessibleAttached::attachedProperties(item);
    }
    QVERIFY(attached);
    QCOMPARE(attached->role(), role);
    QCOMPARE(attached->name(), text);
#else
    Q_UNUSED(role);
    Q_UNUSED(text);
#endif
}

void tst_accessibility::override_data()
{
    QTest::addColumn<QAccessible::Role>("role");

    QTest::newRow("AbstractButton") << QAccessible::Button;
    QTest::newRow("BusyIndicator") << QAccessible::Indicator;
    QTest::newRow("Button") << QAccessible::Button;
    QTest::newRow("CheckBox") << QAccessible::CheckBox;
    QTest::newRow("CheckDelegate") << QAccessible::CheckBox;
    QTest::newRow("ComboBox") << QAccessible::ComboBox;
    QTest::newRow("Container") << QAccessible::NoRole;
    QTest::newRow("Control") << QAccessible::NoRole;
    QTest::newRow("Dial") << QAccessible::Dial;
    QTest::newRow("Dialog") << QAccessible::Dialog;
    QTest::newRow("Drawer") << QAccessible::Dialog;
    QTest::newRow("Frame") << QAccessible::Border;
    QTest::newRow("GroupBox") << QAccessible::Grouping;
    QTest::newRow("ItemDelegate") << QAccessible::ListItem;
    QTest::newRow("Label") << QAccessible::StaticText;
    QTest::newRow("Menu") << QAccessible::PopupMenu;
    QTest::newRow("MenuItem") << QAccessible::MenuItem;
    QTest::newRow("Page") << QAccessible::Pane;
    QTest::newRow("PageIndicator") << QAccessible::Indicator;
    QTest::newRow("Pane") << QAccessible::Pane;
    QTest::newRow("Popup") << QAccessible::Dialog;
    QTest::newRow("ProgressBar") << QAccessible::ProgressBar;
    QTest::newRow("RadioButton") << QAccessible::RadioButton;
    QTest::newRow("RadioDelegate") << QAccessible::RadioButton;
    QTest::newRow("RangeSlider") << QAccessible::Slider;
    QTest::newRow("RoundButton") << QAccessible::Button;
    QTest::newRow("ScrollBar") << QAccessible::ScrollBar;
    QTest::newRow("ScrollIndicator") << QAccessible::Indicator;
    QTest::newRow("Slider") << QAccessible::Slider;
    QTest::newRow("SpinBox") << QAccessible::SpinBox;
    QTest::newRow("StackView") << QAccessible::LayeredPane;
    QTest::newRow("SwipeDelegate") << QAccessible::ListItem;
    QTest::newRow("SwipeView") << QAccessible::PageTabList;
    QTest::newRow("Switch") << QAccessible::CheckBox;
    QTest::newRow("SwitchDelegate") << QAccessible::ListItem;
    QTest::newRow("TabBar") << QAccessible::PageTabList;
    QTest::newRow("TabButton") << QAccessible::PageTab;
    QTest::newRow("TextArea") << QAccessible::EditableText;
    QTest::newRow("TextField") << QAccessible::EditableText;
    QTest::newRow("ToolBar") << QAccessible::ToolBar;
    QTest::newRow("ToolButton") << QAccessible::Button;
    QTest::newRow("ToolTip") << QAccessible::ToolTip;
    QTest::newRow("Tumbler") << QAccessible::NoRole;
}

void tst_accessibility::override()
{
    QFETCH(QAccessible::Role, role);

    static const QStringList ignoredWarningTypes = { "Dialog", "Drawer", "Menu", "Popup", "ToolTip" };
    if (ignoredWarningTypes.contains(QTest::currentDataTag())) {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
            ".*Accessible attached property must be attached to an object deriving from Item or Action"));
    }

    const QString name = QTest::currentDataTag();
    const QString fileBaseName = name.toLower();

    QQmlComponent component(engine.get());
    component.loadUrl(testFileUrl("override/" + adjustFileBaseName(fileBaseName) + ".qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    QQuickItem *item = findItem(object.data());
    QVERIFY(item);

#if QT_CONFIG(accessibility)
    QQuickAccessibleAttached *attached = QQuickAccessibleAttached::attachedProperties(item);
    if (QAccessible::isActive()) {
        QVERIFY(attached);
    } else {
        QPlatformAccessibility *accessibility = platformAccessibility();
        if (!accessibility)
            QSKIP("No QPlatformAccessibility available.");
        accessibility->setActive(true);
        if (!attached)
            attached = QQuickAccessibleAttached::attachedProperties(item);
    }

    QVERIFY(attached);
    QCOMPARE(attached->role(), role);
    QCOMPARE(attached->name(), name + "Override");
#else
    Q_UNUSED(role);
    Q_UNUSED(text);
#endif
}
template <typename Predicate>
void a11yDescendants(QAccessibleInterface *iface, Predicate pred)
{
    for (int i = 0; i < iface->childCount(); ++i) {
        if (QAccessibleInterface *child = iface->child(i)) {
            pred(child);
            a11yDescendants(child, pred);
        }
    }
}

void tst_accessibility::ordering()
{
    QQmlComponent component(engine.get());
    component.loadUrl(testFileUrl("ordering/page.qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

#if QT_CONFIG(accessibility)
    QQuickItem *item = findItem(object.data());
    QVERIFY(item);
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(item);
    QVERIFY(iface);
    QStringList strings;
    a11yDescendants(iface, [&](QAccessibleInterface *iface) {strings << iface->text(QAccessible::Name);});
    QCOMPARE(strings.join(QLatin1String(", ")), "Header, Content item 1, Content item 2, Footer");
#endif
}

void tst_accessibility::actionAccessibility()
{
#if QT_CONFIG(accessibility)
    if (!QAccessible::isActive()) {
        QPlatformAccessibility *accessibility = platformAccessibility();
        if (!accessibility)
            QSKIP("No QPlatformAccessibility available.");
        accessibility->setActive(true);
    }

    QQmlComponent component(engine.get());
    component.loadUrl(testFileUrl("actionAccessibility/button.qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    const QString description = "Show peaches some love";
    QCOMPARE(item->property("text"), description);
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(item);
    QVERIFY(iface);
    QCOMPARE(iface->text(QAccessible::Name), "Peach");
    QCOMPARE(iface->text(QAccessible::Description), description);

    QVERIFY(iface->actionInterface()->actionNames().contains(QAccessibleActionInterface::pressAction()));
    iface->actionInterface()->doAction(QAccessibleActionInterface::pressAction());
    QCOMPARE(item->property("pressCount").toInt(), 1);
#endif
}

void tst_accessibility::actionAccessibilityImplicitName()
{
#if QT_CONFIG(accessibility)
    if (!QAccessible::isActive()) {
        QPlatformAccessibility *accessibility = platformAccessibility();
        if (!accessibility)
            QSKIP("No QPlatformAccessibility available.");
        accessibility->setActive(true);
    }

    QQmlComponent component(engine.get());
    component.loadUrl(testFileUrl("actionAccessibility/button2.qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    const QString description = "Show pears some love";
    QCOMPARE(item->property("text"), "Pears");
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(item);
    QVERIFY(iface);
    QCOMPARE(iface->text(QAccessible::Name), "Pears"); // We get the action.text implicitly
    QCOMPARE(iface->text(QAccessible::Description), description);
#endif
}

void tst_accessibility::sliderTest()
{
#if QT_CONFIG(accessibility)
    if (!QAccessible::isActive()) {
        QPlatformAccessibility *accessibility = platformAccessibility();
        if (!accessibility)
            QSKIP("No QPlatformAccessibility available.");
        accessibility->setActive(true);
    }

    QQmlComponent component(engine.get());
    component.loadUrl(testFileUrl("item.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    auto root = QAccessible::queryAccessibleInterface(object.get());
    QVERIFY(root);
    QCOMPARE(root->childCount(), 4);

    for (int childIndex = 0; childIndex < 4; ++childIndex)
    {
        auto item = root->child(childIndex);
        auto actionIface = item->actionInterface();
        QVERIFY(actionIface);
        auto valueIface = item->valueInterface();
        QVERIFY(valueIface);

        QVERIFY(actionIface->actionNames().contains(QAccessibleActionInterface::increaseAction()));
        QVERIFY(actionIface->actionNames().contains(QAccessibleActionInterface::decreaseAction()));
        QCOMPARE(valueIface->currentValue(), 25);
        QCOMPARE(valueIface->minimumValue(), 0);
        QCOMPARE(valueIface->maximumValue(), 100);

        valueIface->setCurrentValue(30);
        QCOMPARE(valueIface->currentValue(), 30);

        const auto stepSize = valueIface->minimumStepSize();

        actionIface->doAction(QAccessibleActionInterface::increaseAction());
        QCOMPARE(valueIface->currentValue(), 30 + stepSize.toDouble());

        actionIface->doAction(QAccessibleActionInterface::decreaseAction());
        QCOMPARE(valueIface->currentValue(), 30);
        QCOMPARE(valueIface->minimumValue(), 0);
        QCOMPARE(valueIface->maximumValue(), 100);
    }
#endif
}

void tst_accessibility::accessibleName()
{
#if QT_CONFIG(accessibility)
    if (!QAccessible::isActive()) {
        QPlatformAccessibility *accessibility = platformAccessibility();
        if (!accessibility)
            QSKIP("No QPlatformAccessibility available.");
        accessibility->setActive(true);
    }

    QQmlComponent component(engine.get());

    // verify that accessible name matches the button text if none was set explicitly
    component.loadUrl(testFileUrl("accessibleName/button.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));
    QAccessibleInterface *buttonAcc = QAccessible::queryAccessibleInterface(object.get());
    QVERIFY(buttonAcc);
    QCOMPARE(buttonAcc->text(QAccessible::Name), "Hello world");

    // verify that ampersand ('&') for mnemonic and to escape literal ampersand in button
    // text are not contained in accessible name
    component.loadUrl(testFileUrl("accessibleName/button2.qml"));
    QScopedPointer<QObject> object2(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));
    QAccessibleInterface *button2Acc = QAccessible::queryAccessibleInterface(object2.get());
    QVERIFY(button2Acc);
    QCOMPARE(button2Acc->text(QAccessible::Name), "This & that");

    // verify that explicitly set accesible name is used
    component.loadUrl(testFileUrl("accessibleName/button3.qml"));
    QScopedPointer<QObject> object3(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));
    QAccessibleInterface *button3Acc = QAccessible::queryAccessibleInterface(object3.get());
    QVERIFY(button3Acc);
    QCOMPARE(button3Acc->text(QAccessible::Name), "Explicitly set accessible name");
#endif
}

void tst_accessibility::locale()
{
#if QT_CONFIG(accessibility)
    if (!QAccessible::isActive()) {
        QPlatformAccessibility *accessibility = platformAccessibility();
        if (!accessibility)
            QSKIP("No QPlatformAccessibility available.");
        accessibility->setActive(true);
    }

    QQmlComponent component(engine.get());

    // verify that locale is the default locale if none was set explicitly
    component.loadUrl(testFileUrl("locale/button.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));
    QAccessibleInterface *buttonAcc = QAccessible::queryAccessibleInterface(object.get());
    QVERIFY(buttonAcc);
    QVERIFY(buttonAcc->attributesInterface());
    QVERIFY(buttonAcc->attributesInterface()->attributeKeys().contains(
            QAccessible::Attribute::Locale));
    const QVariant localeVariant =
            buttonAcc->attributesInterface()->attributeValue(QAccessible::Attribute::Locale);
    QVERIFY(localeVariant.isValid() && localeVariant.canConvert<QLocale>());
    QCOMPARE(localeVariant.toLocale(), QLocale());

    // verify that locale is the one explicitly set for the button
    component.loadUrl(testFileUrl("locale/button2.qml"));
    QScopedPointer<QObject> object2(component.create());
    QVERIFY2(!object2.isNull(), qPrintable(component.errorString()));
    QAccessibleInterface *chineseButtonAcc = QAccessible::queryAccessibleInterface(object2.get());
    QVERIFY(chineseButtonAcc);
    QVERIFY(chineseButtonAcc->attributesInterface());
    QVERIFY(chineseButtonAcc->attributesInterface()->attributeKeys().contains(
            QAccessible::Attribute::Locale));
    const QVariant chineseLocaleVariant =
            chineseButtonAcc->attributesInterface()->attributeValue(QAccessible::Attribute::Locale);
    QVERIFY(chineseLocaleVariant.isValid() && localeVariant.canConvert<QLocale>());
    QCOMPARE(chineseLocaleVariant.toLocale(), QLocale("zh_CN"));
#endif
}

void tst_accessibility::defaultButton()
{
#if QT_CONFIG(accessibility)
    if (!QAccessible::isActive()) {
        QPlatformAccessibility *accessibility = platformAccessibility();
        if (!accessibility)
            QSKIP("No QPlatformAccessibility available.");
        accessibility->setActive(true);
    }
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick.Controls\nDialogButtonBox {\n"
                      "standardButtons: DialogButtonBox.Ok | DialogButtonBox.No\n"
                      "defaultStandardButton: DialogButtonBox.Ok\n"
                      "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    QQuickDialogButtonBox *buttonBox = qobject_cast<QQuickDialogButtonBox *>(object.get());
    QVERIFY(buttonBox);
    QVERIFY(QQuickTest::qWaitForPolish(buttonBox));
    QCOMPARE(buttonBox->count(), 2);

    for (int i = 0; i < buttonBox->count(); ++i) {
        QQuickButton *button = qobject_cast<QQuickButton *>(buttonBox->itemAt(i));
        QVERIFY(button);
        QAccessibleInterface *buttonAccessible = QAccessible::queryAccessibleInterface(button);
        QVERIFY(buttonAccessible);
        QCOMPARE(buttonAccessible->state().defaultButton, button->isHighlighted());
    }
#endif
}

// Not using data rows for this since they are generated at runtime rather than being hard-coded,
// which could mess with test metrics.
void tst_accessibility::noTransparentText()
{
#if defined(Q_OS_ANDROID)
    QSKIP("StyleInfo is not supported when cross-compiling: QTBUG-100191");
#endif

    // We need to exclude types that QQuickView can't load, or controls that
    // we know don't have text. By manually excluding rather than manually including,
    // we ensure that new types are automatically tested. If they shouldn't be tested,
    // or need to be accounted for in the test, they will cause a failure indicating that.
    const QStringList exclusions = {
        // Non-visual.
        "AbstractButton",
        "Action",
        "ActionGroup",
        "ButtonGroup",
        "Calendar",
        "CalendarModel",

        // Required properties.
        "HorizontalHeaderViewDelegate",
        "TableViewDelegate",
        "TreeViewDelegate",
        "VerticalHeaderViewDelegate",

        // No text property.
        "ApplicationWindow",
        "BusyIndicator",
        "Container",
        "Control",
        "Dial",
        "DialogButtonBox",
        "Drawer",
        "Frame",
        "HorizontalHeaderView",
        "VerticalHeaderView",
        "Menu",
        "MenuBar",
        "MenuSeparator",
        "Page",
        "PageIndicator",
        "Pane",
        "Popup",
        "ProgressBar",
        "RangeSlider",
        "ScrollBar",
        "ScrollIndicator",
        "ScrollView",
        "SelectionRectangle",
        "Slider",
        "SplitView",
        "StackView",
        "SwipeView",
        "TabBar",
        "ToolBar",
        "ToolSeparator",
        "Tumbler",

        // Style-specific non-Controls types.
        // FluentWinUI3
        "Config",
        "StyleImage"
    };

    struct TextPropertyAccessInfo {
        // Default: "text"
        QString setterName;
        // Default: "contentItem"
        QString itemExpression;
        // Allows overriding the above.
        QHash<QString, QString> styleSpecificItemExpressions;
    };

    const QHash<QString, TextPropertyAccessInfo> textPropertyAccess = {
        { "ComboBox", { "displayText", {}, {} } },
        { "DayOfWeekRow", { "doNotSet", "contentItem.children[0]", {} } },
        { "DelayButton", {
            {},
            "contentItem.children[0]",
            {
                { "Imagine", "contentItem" },
                { "Material", "contentItem" },
                { "Universal", "contentItem" }
            }
        }},
        { "Dialog", { "title", "header", {} } },
        { "DoubleSpinBox", { "doNotSet", {}, {} } },
        { "Label", { {}, "this", {} } },
        { "GroupBox", { "title", "label", {} } },
        { "MonthGrid", { "doNotSet", "contentItem.children[0]", {} } },
        { "SpinBox", { "doNotSet", {}, {} } },
        { "TextArea", { {}, "this", {} } },
        { "TextField", { {}, "this", {} } },
        { "WeekNumberColumn", { "doNotSet", "contentItem.children[0]", {} } }
    };

    QList<StyleInfo::QmlFileData> installedQmlFiles = StyleInfo::instance()->installedQmlFiles();
    const auto exclusionRemover = [&exclusions](const StyleInfo::QmlFileData &qmlFileData) {
        for (const QString &exclusion : exclusions) {
            if (qmlFileData.relativePath.endsWith(exclusion + QLatin1String(".qml")))
                return true;
        }
        return false;
    };
    installedQmlFiles.erase(
        std::remove_if(installedQmlFiles.begin(), installedQmlFiles.end(), exclusionRemover), installedQmlFiles.end());

    qCDebug(lcNoTransparentText) << "Installed QML files after removing exclusions:";
    for (auto it = installedQmlFiles.begin(); it != installedQmlFiles.end(); ++it) {
        qCDebug(lcNoTransparentText) << "-" << it->styleName << it->typeName
            << it->relativePath << it->absolutePath;
    }

    for (auto it = installedQmlFiles.constBegin(); it != installedQmlFiles.constEnd(); ++it) {
        qCDebug(lcNoTransparentText) << "Testing" << it->styleName << it->relativePath
            << it->absolutePath;

        if (QQuickStyle::name() != it->styleName) {
            // Can't have an engine alive when qmlClearTypeRegistrations is called.
            auto cleanup = qScopeGuard([this](){ engine.reset(new QQmlEngine); });
            engine.reset();
            qmlClearTypeRegistrations();
            QQuickStyle::setStyle(it->styleName);
        }

        // QTBUG-129447
        if (it->styleName == "Imagine" && it->typeName == "Dialog") {
            for (int i = 0; i < 3; ++i)
                QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*QML Dialog: Binding loop detected for property.*"));
        }

        // Load the QML for this type.
        QQmlComponent component(engine.get());
        const TextPropertyAccessInfo alternativePropertyAcess = textPropertyAccess.value(it->typeName);
        const QString setterName = alternativePropertyAcess.setterName.isEmpty()
            ? QLatin1String("text") : alternativePropertyAcess.setterName;
        // "doNotSet" means we don't need to set anything.
        const QString setterBinding = setterName != "doNotSet" ? setterName + ": 'Some text'" : "";
        const QString qml = QString("import QtQuick.Controls.%1; %2 { %3 }")
            .arg(it->styleName, it->typeName, setterBinding);
        component.setData(qml.toUtf8(), QUrl());
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(component.isReady(), qPrintable(QString::fromLatin1("Failed to load QML for %1: %2QML:\n%3")
            .arg(it->typeName, component.errorString(), qml)));

        // Get the item that we should query for the text and color properties.
        QString styleSpecificItemExpression
            = alternativePropertyAcess.styleSpecificItemExpressions.value(it->styleName);
        if (styleSpecificItemExpression.isEmpty()) {
            // No style-specific expression was given; use the generic one.
            styleSpecificItemExpression = alternativePropertyAcess.itemExpression;
        }
        const QString textItemExpression = styleSpecificItemExpression.isEmpty()
            ? "contentItem" : styleSpecificItemExpression;
        QQuickItem *textItem = nullptr;
        // "this" means use the root object.
        if (textItemExpression == QLatin1String("this")) {
            textItem = qobject_cast<QQuickItem *>(root.get());
        } else {
            QQmlExpression itemQmlExpression(qmlContext(root.get()), root.get(), textItemExpression);
            const QVariant evaluationResult = itemQmlExpression.evaluate();
            if (!evaluationResult.isValid()) {
                QString failureMessage = QString::fromLatin1("itemExpression \"%1\" for %2 %3 is invalid")
                    .arg(textItemExpression, it->styleName, it->typeName);
                if (itemQmlExpression.hasError())
                    failureMessage += QLatin1String(": ") + itemQmlExpression.error().toString();
                QFAIL(qPrintable(failureMessage));
            }
            textItem = evaluationResult.value<QQuickItem *>();
        }
        QVERIFY2(textItem, qPrintable(QString::fromLatin1(
            "The item (%1) to use for the text and color properties of %2 %3 is null")
            .arg(textItemExpression, it->styleName, it->typeName)));

        // If it has an icon property, set icon.color to "transparent", which should only affect the
        // icon color (make it use the original color), not the text color.
        if (root->property("icon").isValid()) {
            auto *asAbstractButton = qobject_cast<QQuickAbstractButton *>(root.get());
            QVERIFY(asAbstractButton);
            auto icon = asAbstractButton->icon();
            icon.setColor(Qt::transparent);
            asAbstractButton->setIcon(icon);
            QCOMPARE(asAbstractButton->icon().color(), Qt::transparent);
        }

        // Confirm that the text is not empty and the color is not transparent.
        QVERIFY(!textItem->property("text").toString().isEmpty());
        const QVariant colorProperty = textItem->property("color");
        QVERIFY2(colorProperty.isValid(), qPrintable(QString::fromLatin1(
            "The item (%1) to use for %2 %3 has no color property")
            .arg(textItemExpression, it->styleName, it->typeName)));
        const auto color = colorProperty.value<QColor>();
        QVERIFY2(color.isValid(), qPrintable(QString::fromLatin1(
            "color property of the item (%1) to use for %2 %3 is invalid")
            .arg(textItemExpression, it->styleName, it->typeName)));
        QVERIFY2(color.alpha() != 0, qPrintable(QString::fromLatin1(
            "color property of the item (%1) to use for %2 %3 is transparent")
            .arg(textItemExpression, it->styleName, it->typeName)));
    }
}

QTEST_MAIN(tst_accessibility)

#include "tst_accessibility.moc"
