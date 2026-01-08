// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtTest/QTest>
#include <QtTest/qtestaccessible.h>

#include <QtGui/qaccessible.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qaccessiblecache_p.h>

#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformaccessibility.h>

#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>
#include <QtQuick/private/qquickaccessibleattached_p.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquicktextinput_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

#include <QQmlComponent>
#include <QQmlApplicationEngine>

using namespace Qt::StringLiterals;

#define EXPECT(cond) \
    do { \
        if (!errorAt && !(cond)) { \
            errorAt = __LINE__; \
            qWarning("level: %d, middle: %d, role: %d (%s)", treelevel, middle, iface->role(), #cond); \
        } \
    } while (0)


//TESTED_FILES=

class tst_QQuickAccessible : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickAccessible();
    virtual ~tst_QQuickAccessible();

public slots:
    void initTestCase() override;
    void cleanupTestCase();
    void init() override;
    void cleanup();

private slots:
    void commonTests_data();
    void commonTests();

    void quickAttachedProperties();

    void quickAttachedProperties_description_data();
    void quickAttachedProperties_description();
    void attachedWins();
    void basicPropertiesTest();
    void hitTest();
    void checkableTest();
    void ignoredTest();
    void passwordTest();
    void announceTest();
    void eventTest();
    void relations_data();
    void relations();
    void controlsThatShouldSendObjectShow_data();
    void controlsThatShouldSendObjectShow();
};

tst_QQuickAccessible::tst_QQuickAccessible()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{

}

tst_QQuickAccessible::~tst_QQuickAccessible()
{

}

void tst_QQuickAccessible::initTestCase()
{
    QQmlDataTest::initTestCase();
    QTestAccessibility::initialize();
    QPlatformIntegration *pfIntegration = QGuiApplicationPrivate::platformIntegration();
    if (!pfIntegration->accessibility())
        QSKIP("This platform does not support accessibility");
    pfIntegration->accessibility()->setActive(true);
}

void tst_QQuickAccessible::cleanupTestCase()
{
    QTestAccessibility::cleanup();
}

void tst_QQuickAccessible::init()
{
    QQmlDataTest::init();
    QTestAccessibility::clearEvents();
}

void tst_QQuickAccessible::cleanup()
{
    const auto list = QTestAccessibility::events();
    if (!list.isEmpty()) {
        qWarning().noquote() << list.size()
                             << "accessibility event(s) were not handled in testfunction '"
                             << QTest::currentTestFunction() << "':";
        for (int i = 0; i < list.size(); ++i) {
            auto object = list.at(i)->object();
            QString objectInfo = object ? QDebug::toString(object)
                                        : u"[deleted object]"_s;
            qWarning().noquote() << " " << (i + 1) << objectInfo
                       << "Event: '" << qAccessibleEventString(list.at(i)->type())
                       << "' Child: " << list.at(i)->child();
        }
    }
    QTestAccessibility::clearEvents();
}

void tst_QQuickAccessible::commonTests_data()
{
    QTest::addColumn<QString>("accessibleRoleFileName");

    QTest::newRow("Text") << "text.qml";
    QTest::newRow("PushButton") << "pushbutton.qml";
}

void tst_QQuickAccessible::commonTests()
{
    QFETCH(QString, accessibleRoleFileName);

    qDebug() << "testing" << accessibleRoleFileName;

    auto view = std::make_unique<QQuickView>();
//    view->setFixedSize(240,320);
    view->setSource(testFileUrl(accessibleRoleFileName));
    view->show();
//    view->setFocus();
    QVERIFY(view->rootObject() != nullptr);

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(view.get());
    QVERIFY(iface);

    view.reset();
    QTestAccessibility::clearEvents();
}

void tst_QQuickAccessible::quickAttachedProperties()
{
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nItem {\n"
                                "}", QUrl());
        auto object = std::unique_ptr<QObject>(component.create());
        QVERIFY(object != nullptr);

        QObject *attachedObject = QQuickAccessibleAttached::attachedProperties(object.get());
        QCOMPARE(attachedObject, static_cast<QObject*>(nullptr));
    }

    // Attaching to non-item
    {
        QObject parent;
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML QtObject: Accessible attached property must be attached to an object deriving from Item or Action");
        QQuickAccessibleAttached *attachedObj = new QQuickAccessibleAttached(&parent);

        QCOMPARE(attachedObj->ignored(), false);
        attachedObj->setIgnored(true);
        QCOMPARE(attachedObj->ignored(), false);
        attachedObj->setIgnored(false);
        QCOMPARE(attachedObj->ignored(), false);
    }

    // Attached property
    {
        QQuickItem parent;
        QQuickAccessibleAttached *attachedObj = new QQuickAccessibleAttached(&parent);

        attachedObj->name();

        QVariant pp = attachedObj->property("name");
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nItem {\n"
                                "Accessible.role: Accessible.Button\n"
                                "}", QUrl());
        auto object = std::unique_ptr<QObject>(component.create());
        QVERIFY(object != nullptr);

        const auto attachedObject = qobject_cast<QQuickAccessibleAttached*>(
            QQuickAccessibleAttached::attachedProperties(object.get()));
        QVERIFY(attachedObject);
        if (attachedObject) {
            QVariant p = attachedObject->property("role");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toInt(), int(QAccessible::PushButton));
            p = attachedObject->property("name");
            QCOMPARE(p.typeId(), QMetaType::QString);
            QVERIFY2(p.value<QString>().isEmpty(), QTest::toString(p));
            p = attachedObject->property("description");
            QCOMPARE(p.typeId(), QMetaType::QString);
            QVERIFY2(p.value<QString>().isEmpty(), QTest::toString(p));
            QCOMPARE(attachedObject->wasNameExplicitlySet(), false);
        }
    }

    // Attached property: QTBUG-133564
    {
        QQmlEngine engine;

        QQmlComponent component(&engine);
        component.setData("import QtQuick\n"
                          "Item {\n"
                          "    Accessible.role: Accessible.Button\n"
                          "    property int value: 0\n"
                          "    property int cursorPosition: 0\n"
                          "}",
                          QUrl());

        auto object = std::unique_ptr<QObject>(component.create());
        QVERIFY(object != nullptr);

        const auto attachedObject = qobject_cast<QQuickAccessibleAttached *>(
                QQuickAccessibleAttached::attachedProperties(object.get()));
        QVERIFY(attachedObject);

        const auto events = QTestAccessibility::events();

        // If the value interface is not implemented then
        // the valueChanged() signal should not be connected.
        object->setProperty("value", 1);
        QCOMPARE(QTestAccessibility::events(), events);

        // If the text interface is not implemented then
        // the cursorPositionChanged() signal should not be connected.
        object->setProperty("cursorPosition", 1);
        QCOMPARE(QTestAccessibility::events(), events);
    }

    // Attached property
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nItem {\n"
                                "Accessible.role: Accessible.Button\n"
                                "Accessible.name: \"Donald\"\n"
                                "Accessible.description: \"Duck\"\n"
                                "}", QUrl());
        auto object = std::unique_ptr<QObject>(component.create());
        QVERIFY(object != nullptr);

        const auto attachedObject = qobject_cast<QQuickAccessibleAttached*>(
            QQuickAccessibleAttached::attachedProperties(object.get()));
        QVERIFY(attachedObject);
        if (attachedObject) {
            QVariant p = attachedObject->property("role");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toInt(), int(QAccessible::PushButton));
            p = attachedObject->property("name");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toString(), QLatin1String("Donald"));
            p = attachedObject->property("description");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toString(), QLatin1String("Duck"));
            QCOMPARE(attachedObject->wasNameExplicitlySet(), true);
        }
    }

    // Check overriding of attached role for Text
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nText {\n"
                          "Accessible.role: Accessible.Button\n"
                          "Accessible.name: \"TextButton\"\n"
                          "Accessible.description: \"Text Button\"\n"
                          "}", QUrl());
        auto object = std::unique_ptr<QObject>(component.create());
        QVERIFY(object != nullptr);

        QObject *attachedObject = QQuickAccessibleAttached::attachedProperties(object.get());
        QVERIFY(attachedObject);
        if (attachedObject) {
            QVariant p = attachedObject->property("role");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toInt(), int(QAccessible::PushButton));
            p = attachedObject->property("name");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toString(), QLatin1String("TextButton"));
            p = attachedObject->property("description");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toString(), QLatin1String("Text Button"));
        }
    }
    // Check overriding of attached role for Text
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nListView {\n"
                          "id: list\n"
                          "model: 5\n"
                          "delegate: Text {\n"
                          "objectName: \"acc_text\"\n"
                          "Accessible.role: Accessible.Button\n"
                          "Accessible.name: \"TextButton\"\n"
                          "Accessible.description: \"Text Button\"\n"
                          "}\n"
                          "}", QUrl());
        auto object = std::unique_ptr<QObject>(component.create());
        QVERIFY(object != nullptr);

        QQuickListView *listview = qobject_cast<QQuickListView *>(object.get());
        QVERIFY(listview != nullptr);
        QQuickItem *contentItem = listview->contentItem();
        QQuickText *childItem = QQuickVisualTestUtils::findItem<QQuickText>(contentItem, "acc_text");
        QVERIFY(childItem != nullptr);

        QObject *attachedObject = QQuickAccessibleAttached::attachedProperties(childItem);
        QVERIFY(attachedObject);
        if (attachedObject) {
            QVariant p = attachedObject->property("role");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toInt(), int(QAccessible::PushButton));
            p = attachedObject->property("name");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toString(), QLatin1String("TextButton"));
            p = attachedObject->property("description");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toString(), QLatin1String("Text Button"));
        }
    }
    // Check that a name can be implicitly set.
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick 2.0
            Text {
                Accessible.role: Accessible.Button
                Accessible.description: "Text Button"
            })", QUrl());
        auto object = std::unique_ptr<QObject>(component.create());
        QVERIFY(object);

        const auto attachedObject = qobject_cast<QQuickAccessibleAttached*>(
            QQuickAccessibleAttached::attachedProperties(object.get()));
        QVERIFY(attachedObject);
        QVERIFY(!attachedObject->wasNameExplicitlySet());

        attachedObject->setNameImplicitly(QLatin1String("Implicit"));
        QCOMPARE(attachedObject->name(), QLatin1String("Implicit"));
        QVERIFY(!attachedObject->wasNameExplicitlySet());

        attachedObject->setName(QLatin1String("Explicit"));
        QCOMPARE(attachedObject->name(), QLatin1String("Explicit"));
        QVERIFY(attachedObject->wasNameExplicitlySet());
    }
    QTestAccessibility::clearEvents();
}

void tst_QQuickAccessible::quickAttachedProperties_description_data()
{
    QTest::addColumn<QString>("qmlCode");
    QTest::addColumn<QString>("initialExplicitDescription");
    QTest::addColumn<QString>("implicitDescription");
    QTest::addColumn<QString>("implicitDescription2");
    QTest::addColumn<QString>("explicitDescription2");
    QTest::addColumn<QString>("implicitDescription3");

    QTest::addColumn<QAccessible::Role>("expectedRole");
    QTest::addColumn<QString>("expectedDescription");

    QTest::newRow("TextArea with initial explicit description") << R"(
            import QtQuick
            import QtQuick.Controls
            TextArea {
                Accessible.role: Accessible.EditableText
                %1
            })"
            << "Explicit description"
            << "Initial implicit"           // implicitDescription
            << "Updated implicit"           // implicitDescription2
            << "Explicit"                   // explicitDescription2
            << "Updated implicit again"     // implicitDescription3

            // EXPECTED
            << QAccessible::EditableText
            << "Explicit description";


    QTest::newRow("TextArea, no initial explicit description") << R"(
            import QtQuick
            import QtQuick.Controls
            TextArea {
                Accessible.role: Accessible.EditableText
                %1
            })"
            << QString()
            << "Initial implicit"           // implicitDescription
            << "Updated implicit"           // implicitDescription2
            << "Explicit"                   // explicitDescription2
            << "Updated implicit again"     // implicitDescription3

            // EXPECTED
            << QAccessible::EditableText
            << "Explicit description";
}

void tst_QQuickAccessible::quickAttachedProperties_description()
{
    QFETCH(QString, qmlCode);
    QFETCH(QString, initialExplicitDescription);
    QFETCH(QString, implicitDescription);
    QFETCH(QString, implicitDescription2);
    QFETCH(QString, explicitDescription2);
    QFETCH(QString, implicitDescription3);
    QFETCH(QAccessible::Role, expectedRole);
    QFETCH(QString, expectedDescription);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    QString extraCode;
    if (!initialExplicitDescription.isNull())
        extraCode.append(QString::fromLatin1(R"(Accessible.description: "%1")").arg(initialExplicitDescription));
    extraCode.append(QString::fromLatin1("\nplaceholderText: \"%2\"").arg(implicitDescription));

    component.setData(qmlCode.arg(extraCode).toUtf8(), QUrl());
    auto object = std::unique_ptr<QObject>(component.create());
    QVERIFY(object);

    const auto attachedObject = qobject_cast<QQuickAccessibleAttached*>(
            QQuickAccessibleAttached::attachedProperties(object.get()));
    QVERIFY(attachedObject);
    QCOMPARE(attachedObject->role(), expectedRole);

    const QString &explicitDesc = initialExplicitDescription;
    QCOMPARE(attachedObject->description(), explicitDesc.isNull() ? implicitDescription : explicitDesc);

    object->setProperty("placeholderText", implicitDescription2);
    QCOMPARE(attachedObject->description(), explicitDesc.isNull() ? implicitDescription2 : explicitDesc);

    attachedObject->setDescription(explicitDescription2);
    QCOMPARE(attachedObject->description(), explicitDescription2);

    object->setProperty("placeholderText", implicitDescription2);
    QCOMPARE(attachedObject->description(), explicitDescription2);
    QTestAccessibility::clearEvents();
}

// Verify that a role can be explicitly set, and that the values from the
// attached object are used even if the item has a default role - QTBUG-110114
void tst_QQuickAccessible::attachedWins()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQuick
        import QtQuick.Controls
        Button {
            text: "Button"
            objectName: "button"
            Accessible.role: Accessible.RadioButton
            Accessible.description: "Radio Button"
        })", QUrl());
    auto button = std::unique_ptr<QObject>(component.create());
    QVERIFY(button);

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(button.get());
    QVERIFY(iface);

    QCOMPARE(iface->role(), QAccessible::RadioButton);
    QTestAccessibility::clearEvents();
}


void tst_QQuickAccessible::basicPropertiesTest()
{
    QAccessibleInterface *app = QAccessible::queryAccessibleInterface(qApp);
    QCOMPARE(app->childCount(), 0);

    auto window = std::make_unique<QQuickView>();
    window->setSource(testFileUrl("text.qml"));
    window->show();
    QCOMPARE(app->childCount(), 1);

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(window.get());
    QVERIFY(iface);
    QCOMPARE(iface->childCount(), 1);

    QAccessibleInterface *item = iface->child(0);
    QVERIFY(item);
    QCOMPARE(item->childCount(), 7);
    QCOMPARE(item->rect().size(), QSize(400, 400));
    QCOMPARE(item->role(), QAccessible::Client);
    QCOMPARE(iface->indexOfChild(item), 0);

    QAccessibleInterface *text = item->child(0);
    QVERIFY(text);
    QCOMPARE(text->childCount(), 0);

    QCOMPARE(text->text(QAccessible::Name), QLatin1String("Hello Accessibility"));
    QCOMPARE(text->rect().size(), QSize(200, 50));
    QCOMPARE(text->rect().x(), item->rect().x() + 100);
    QCOMPARE(text->rect().y(), item->rect().y() + 20);
    QCOMPARE(text->role(), QAccessible::StaticText);
    QCOMPARE(item->indexOfChild(text), 0);

    QAccessibleInterface *text2 = item->child(1);
    QVERIFY(text2);
    QCOMPARE(text2->childCount(), 0);

    QCOMPARE(text2->text(QAccessible::Name), QLatin1String("The Hello 2 accessible text"));
    QCOMPARE(text2->rect().size(), QSize(100, 40));
    QCOMPARE(text2->rect().x(), item->rect().x() + 100);
    QCOMPARE(text2->rect().y(), item->rect().y() + 40);
    QCOMPARE(text2->role(), QAccessible::StaticText);
    QCOMPARE(item->indexOfChild(text2), 1);
    QVERIFY(!text2->state().editable);
    QVERIFY(text2->state().readOnly);
    QVERIFY(text2->state().focusable);

    QCOMPARE(iface->indexOfChild(text2), -1);
    QCOMPARE(text2->indexOfChild(item), -1);

    // TextInput
    QAccessibleInterface *textInput = item->child(2);
    QVERIFY(textInput);
    QCOMPARE(textInput->childCount(), 0);
    QCOMPARE(textInput->role(), QAccessible::EditableText);
    QVERIFY(textInput->state().editable);
    QVERIFY(!textInput->state().readOnly);
    QVERIFY(!textInput->state().multiLine);
    QVERIFY(textInput->state().focusable);
    QCOMPARE(textInput->text(QAccessible::Value), "A text input");
    auto textInterface = textInput->textInterface();
    QVERIFY(textInterface);
    auto editableTextInterface = textInput->editableTextInterface();
    QEXPECT_FAIL("", "EditableTextInterface is not implemented", Continue);
    QVERIFY(editableTextInterface);
    auto newText = QString("a new text");
    textInput->setText(QAccessible::Value, newText);
    QCOMPARE(textInput->text(QAccessible::Value), newText);
    QCOMPARE(textInterface->selectionCount(), 0);
    textInterface->setSelection(0, 1, 4);
    QCOMPARE(textInterface->selectionCount(), 1);
    int selectionStartOffset = 0, selectionEndOffset = 0;
    textInterface->selection(0, &selectionStartOffset, &selectionEndOffset);
    QCOMPARE(selectionStartOffset, 1);
    QCOMPARE(selectionEndOffset, 4);
    textInterface->removeSelection(0);
    QCOMPARE(textInterface->selectionCount(), 0);

    // TextEdit
    QAccessibleInterface *textEdit = item->child(3);
    QVERIFY(textEdit);
    QCOMPARE(textEdit->childCount(), 0);
    QCOMPARE(textEdit->role(), QAccessible::EditableText);
    QVERIFY(textEdit->state().editable);
    QVERIFY(!textEdit->state().readOnly);
    QVERIFY(textEdit->state().focusable);
    QCOMPARE(textEdit->text(QAccessible::Value), "A multi-line text edit\nTesting Accessibility.");

    auto textEditTextInterface = textEdit->textInterface();
    QVERIFY(textEditTextInterface);
    QCOMPARE(textEditTextInterface->selectionCount(), 0);
    textEditTextInterface->setSelection(0, 1, 4);
    QCOMPARE(textEditTextInterface->selectionCount(), 1);
    int selectionStart = 0, selectionEnd = 0;
    textEditTextInterface->selection(0, &selectionStart, &selectionEnd);
    QCOMPARE(selectionStart, 1);
    QCOMPARE(selectionEnd, 4);
    textEditTextInterface->removeSelection(0);
    QCOMPARE(textEditTextInterface->selectionCount(), 0);

    auto textEditEditableTextInterface = textEdit->editableTextInterface();
    QEXPECT_FAIL("", "EditableTextInterface is not implemented", Continue);
    QVERIFY(textEditEditableTextInterface);
    textEdit->setText(QAccessible::Value, newText);
    QCOMPARE(textEdit->text(QAccessible::Value), newText);
    QEXPECT_FAIL("", "multi line is not implemented", Continue);
    QVERIFY(textInput->state().multiLine);

    // Text "Hello 3"
    QAccessibleInterface *text3 = item->child(4);
    QVERIFY(text3);
    QCOMPARE(text3->childCount(), 0);
    QCOMPARE(text3->text(QAccessible::Name), QLatin1String("Hello 3"));
    QCOMPARE(text3->role(), QAccessible::StaticText);
    QCOMPARE(item->indexOfChild(text3), 4);
    QVERIFY(!text3->state().editable);
    QVERIFY(!text3->state().readOnly);
    // test implicit state values due to role change
    QQuickAccessibleAttached *attached = QQuickAccessibleAttached::attachedProperties(text3->object());
    attached->setRole(QAccessible::StaticText);
    QCOMPARE(text3->role(), QAccessible::StaticText);
    QVERIFY(text3->state().readOnly);

    // Text "Rich text"
    QAccessibleInterface *richText = item->child(5);
    QVERIFY(richText);
    QCOMPARE(richText->childCount(), 2);
    QCOMPARE(richText->text(QAccessible::Name), QLatin1String("Rich text with links:\nWebsite or blog"));
    QCOMPARE(richText->role(), QAccessible::StaticText);
    QCOMPARE(item->indexOfChild(richText), 5);
    QVERIFY(!richText->state().editable);
    QVERIFY(!richText->state().readOnly);

    // Check for hyperlink child nodes
    for (int i = 0; i < richText->childCount(); ++i) {
        static const char *linkUrls[2][2] = {
            {"Website", "https://qt.io"},
            {"blog", "https://qt.io/blog"}
        };
        QAccessibleInterface *link1 = richText->child(i);
        QVERIFY(link1);
        QCOMPARE(link1->role(), QAccessible::Link);
        QAccessibleHyperlinkInterface *link = link1->hyperlinkInterface();
        QVERIFY(link);
        QCOMPARE(link->anchor(), QLatin1String(linkUrls[i][0]));
        QCOMPARE(link->anchorTarget(), QLatin1String(linkUrls[i][1]));
    }

    // TextField
    QAccessibleInterface *textField = item->child(6);
    QVERIFY(textField);
    QCOMPARE(textField->childCount(), 0);
    QCOMPARE(textField->text(QAccessible::Value), QLatin1String("text is TextField"));
    QCOMPARE(textField->text(QAccessible::Description), QLatin1String("This is the TextField description"));
    QCOMPARE(textField->role(), QAccessible::EditableText);
    QCOMPARE(item->indexOfChild(textField), 6);
    QVERIFY(textField->state().editable);
    QVERIFY(!textField->state().readOnly);

    // see if implicit changes back
    attached->setRole(QAccessible::EditableText);
    QEXPECT_FAIL("", "EditableText does not implicitly set readOnly to false", Continue);
    QVERIFY(!text3->state().readOnly);
    // explicitly set state
    attached->set_readOnly(false);
    attached->setRole(QAccessible::StaticText);
    QVERIFY(!text3->state().readOnly);

    window.reset();
    QTestAccessibility::clearEvents();
}

QAccessibleInterface *topLevelChildAt(QAccessibleInterface *iface, int x, int y)
{
    QAccessibleInterface *child = iface->childAt(x, y);
    if (!child)
        return nullptr;

    QAccessibleInterface *childOfChild;
    while ( ( childOfChild = child->childAt(x, y)) ) {
        child = childOfChild;
    }
    return child;
}

void tst_QQuickAccessible::hitTest()
{
    auto window = std::make_unique<QQuickView>();
    window->setSource(testFileUrl("hittest.qml"));
    window->show();

    QAccessibleInterface *windowIface = QAccessible::queryAccessibleInterface(window.get());
    QVERIFY(windowIface);
    QAccessibleInterface *rootItem = windowIface->child(0);
    // on Android the main window is always shown fullscreen
    QRect rootRect = QRect(window->x(), window->y(), window->width(), window->height());

    // check the root item from app
    QAccessibleInterface *appIface = QAccessible::queryAccessibleInterface(qApp);
    QVERIFY(appIface);
    QAccessibleInterface *itemHit = appIface->childAt(rootRect.x() + 200, rootRect.y() + 50);
    QVERIFY(itemHit);
    QCOMPARE(itemHit->rect(), rootRect);

    QAccessibleInterface *rootItemIface;
    for (int c = 0; c < rootItem->childCount(); ++c) {
        QAccessibleInterface *iface = rootItem->child(c);
        QString name = iface->text(QAccessible::Name);
        if (name == QLatin1String("rect1")) {
            // hit rect1
            QAccessibleInterface *rect1 = iface;
            QRect rect1Rect = rect1->rect();
            QAccessibleInterface *rootItemIface = rootItem->childAt(rect1Rect.x() + 10, rect1Rect.y() + 10);
            QVERIFY(rootItemIface);
            QCOMPARE(rect1Rect, rootItemIface->rect());
            QCOMPARE(rootItemIface->text(QAccessible::Name), QLatin1String("rect1"));

            // should also work from top level (app)
            QAccessibleInterface *app(QAccessible::queryAccessibleInterface(qApp));
            QAccessibleInterface *itemHit2(topLevelChildAt(app, rect1Rect.x() + 10, rect1Rect.y() + 10));
            QVERIFY(itemHit2);
            QCOMPARE(itemHit2->rect(), rect1Rect);
            QCOMPARE(itemHit2->text(QAccessible::Name), QLatin1String("rect1"));
        } else if (name == QLatin1String("rect2")) {
            QAccessibleInterface *rect2 = iface;
            // FIXME: This is seems broken on OS X
            // QCOMPARE(rect2->rect().translated(rootItem->rect().x(), rootItem->rect().y()), QRect(0, 50, 100, 100));
            QAccessibleInterface *rect20 = rect2->child(0);
            QVERIFY(rect20);
            QCOMPARE(rect20->text(QAccessible::Name), QLatin1String("rect20"));
            QPoint p = rect20->rect().bottomRight() + QPoint(20, 20);
            QAccessibleInterface *rect201 = rect20->childAt(p.x(), p.y());
            QVERIFY(rect201);
            QCOMPARE(rect201->text(QAccessible::Name), QLatin1String("rect201"));
            rootItemIface = topLevelChildAt(windowIface, p.x(), p.y());
            QVERIFY(rootItemIface);
            QCOMPARE(rootItemIface->text(QAccessible::Name), QLatin1String("rect201"));

        }
    }

    window.reset();
    QTestAccessibility::clearEvents();
}

void tst_QQuickAccessible::checkableTest()
{
    auto window = std::make_unique<QQuickView>();
    window->setSource(testFileUrl("checkbuttons.qml"));
    window->show();

    QQuickItem *contentItem = window->contentItem();
    QVERIFY(contentItem);
    QQuickItem *rootItem = contentItem->childItems().first();
    QVERIFY(rootItem);

    // the window becomes active
    QAccessible::State activatedChange;
    activatedChange.active = true;

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(window.get());
    QVERIFY(iface);
    QAccessibleInterface *root = iface->child(0);

    QAccessibleInterface *button1 = root->child(0);
    QCOMPARE(button1->role(), QAccessible::Button);
    QVERIFY(!(button1->state().checked));
    QVERIFY(!(button1->state().checkable));

    QVERIFY(button1->state().focusable);
    QVERIFY(!button1->state().focused);

    QTestAccessibility::clearEvents();

    // set properties
    QQuickItem *button1item = qobject_cast<QQuickItem*>(rootItem->childItems().at(0));
    QVERIFY(button1item);
    QCOMPARE(button1item->objectName(), QLatin1String("button1"));
    button1item->forceActiveFocus();
    QVERIFY(button1->state().focusable);
    QVERIFY(button1->state().focused);

    QAccessibleEvent focusEvent(button1item, QAccessible::Focus);
    QVERIFY_EVENT(&focusEvent);

    QAccessibleInterface *button2 = root->child(1);
    QVERIFY(!(button2->state().checked));
    QVERIFY(button2->state().checkable);
    QQuickItem *button2item = qobject_cast<QQuickItem*>(rootItem->childItems().at(1));
    QVERIFY(button2item);
    QCOMPARE(button2item->objectName(), QLatin1String("button2"));

    QAccessibleInterface *button3 = root->child(2);
    QVERIFY(button3->state().checked);
    QVERIFY(button3->state().checkable);

    QAccessibleInterface *checkBox1 = root->child(3);
    QCOMPARE(checkBox1->role(), QAccessible::CheckBox);
    QVERIFY(checkBox1->state().checked);
    QVERIFY(checkBox1->state().checkable);
    QQuickItem *checkbox1item = qobject_cast<QQuickItem*>(rootItem->childItems().at(3));
    QVERIFY(checkbox1item);
    QCOMPARE(checkbox1item->objectName(), QLatin1String("checkbox1"));

    checkbox1item->setProperty("checked", false);
    QVERIFY(!checkBox1->state().checked);
    QAccessible::State checkState;
    checkState.checked = true;
    QAccessibleStateChangeEvent checkChanged(checkbox1item, checkState);
    QVERIFY_EVENT(&checkChanged);

    checkbox1item->setProperty("checked", true);
    QVERIFY(checkBox1->state().checked);
    QVERIFY_EVENT(&checkChanged);

    QAccessibleInterface *checkBox2 = root->child(4);
    QVERIFY(!(checkBox2->state().checked));
    QVERIFY(checkBox2->state().checkable);

    QTestAccessibility::clearEvents();
}

void tst_QQuickAccessible::ignoredTest()
{
    auto window = std::make_unique<QQuickView>();
    window->setSource(testFileUrl("ignored.qml"));
    window->show();

    QQuickItem *contentItem = window->contentItem();
    QVERIFY(contentItem);
    QQuickItem *rootItem = contentItem->childItems().first();
    QVERIFY(rootItem);

    // the window becomes active
    QAccessible::State activatedChange;
    activatedChange.active = true;

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(window.get());
    QVERIFY(iface);
    QAccessibleInterface *rectangleA = iface->child(0);

    QCOMPARE(rectangleA->role(), QAccessible::StaticText);
    QCOMPARE(rectangleA->text(QAccessible::Name), QLatin1String("A"));
    static const char *expected = "BEFIHD";
    // check if node "C" and "G" is skipped and that the order is as expected.
    for (int i = 0; i < rectangleA->childCount(); ++i) {
        QAccessibleInterface *child = rectangleA->child(i);
        QCOMPARE(child->text(QAccessible::Name), QString(QLatin1Char(expected[i])));
    }
    QTestAccessibility::clearEvents();
}

void tst_QQuickAccessible::passwordTest()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nTextInput {\n"
                      "Accessible.role: Accessible.EditableText\n"
                      "Accessible.name: \"Password\"\n"
                      "Accessible.passwordEdit: true\n"
                      "echoMode: TextInput.Password\n"
                      "text: \"Green\"\n"
                      "}", QUrl());
    auto object = std::unique_ptr<QObject>(component.create());
    QVERIFY(object != nullptr);

    QQuickTextInput *textInput = qobject_cast<QQuickTextInput *>(object.get());
    QVERIFY(textInput != nullptr);

    const auto passwordCharacter = textInput->passwordCharacter();
    const auto passwordLength = textInput->text().length();
    const auto password = passwordCharacter.repeated(passwordLength);

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(object.get());
    QVERIFY(iface);
    QCOMPARE(iface->text(QAccessible::Value), password);

    QTestAccessibility::clearEvents();
}

void tst_QQuickAccessible::announceTest()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nItem {\n"
                      "Component.onCompleted: Accessible.announce('I am complete!')"
                      "}",
                      QUrl());
    auto object = std::unique_ptr<QObject>(component.create());
    QVERIFY(object != nullptr);

    QAccessibleEvent createdEvent(object.get(), QAccessible::ObjectCreated);
    QVERIFY_EVENT(&createdEvent);
    QAccessibleAnnouncementEvent event(object.get(), QStringLiteral("I am complete!"));
    QVERIFY_EVENT(&event);

    QTestAccessibility::clearEvents();
}

void tst_QQuickAccessible::eventTest()
{
    auto clearEvents = qScopeGuard([]{ QTestAccessibility::clearEvents(); });
    std::unique_ptr<QQuickView> window(new QQuickView());

    window->setSource(testFileUrl("eventTest.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.get()));

    QQuickItem *contentItem = window->contentItem();
    QVERIFY(contentItem);
    QQuickItem *rootItem = contentItem->childItems().first();
    QVERIFY(rootItem);

    // move an item that is not accessible
    QQuickItem *textItem = rootItem->findChild<QQuickItem*>(QLatin1String("text"));
    QTestAccessibility::clearEvents();
    textItem->setX(textItem->x() + 2);
    QCOMPARE(QTestAccessibility::events().size(), 0);

    // move an item that is accessible
    QQuickItem *buttonItem = rootItem->findChild<QQuickItem*>(QLatin1String("button"));
    auto buttonIface = QAccessible::queryAccessibleInterface(buttonItem);
    QVERIFY(buttonIface);
    QTestAccessibility::clearEvents();
    buttonItem->setX(buttonItem->x() + 2);
    QCOMPARE(QTestAccessibility::events().size(), 1);
    QAccessibleEvent ev(buttonItem, QAccessible::LocationChanged);
    QTestAccessibility::verifyEvent(&ev);

    auto windowIface = QAccessible::queryAccessibleInterface(window.get());
    QAccessibleObjectDestroyedEvent ev2(buttonIface);
    QAccessibleObjectDestroyedEvent ev3(windowIface);
    window.reset();
    QTestAccessibility::verifyEvent(&ev2);
    QTestAccessibility::verifyEvent(&ev3);
}

void tst_QQuickAccessible::relations_data()
{
    QTest::addColumn<QString>("item");
    QTest::addColumn<QString>("other");
    QTest::addColumn<QAccessible::Relation>("relation");

    QTest::addRow("label labels textInput")
        << "label" << "textInput"
        << QAccessible::Relation(QAccessible::Labelled);
    QTest::addRow("textInput labelled by label")
        << "textInput" << "label"
        << QAccessible::Relation(QAccessible::Label);
}

void tst_QQuickAccessible::relations()
{
    auto clearEvents = qScopeGuard([]{ QTestAccessibility::clearEvents(); });

    QFETCH(const QString, item);
    QFETCH(const QString, other);
    QFETCH(const QAccessible::Relation, relation);

    auto window = std::make_unique<QQuickView>();
    window->setSource(testFileUrl("relations.qml"));
    window->show();

    QObject *itemObject = window->findChild<QQuickItem *>(item);
    QVERIFY(itemObject);
    QAccessibleInterface *itemIface = QAccessible::queryAccessibleInterface(itemObject);
    QVERIFY(itemIface);
    QObject *otherObject = window->findChild<QQuickItem *>(other);
    QVERIFY(otherObject);
    QAccessibleInterface *otherIface = QAccessible::queryAccessibleInterface(otherObject);
    QVERIFY(otherIface);

    const QList<std::pair<QAccessibleInterface *, QAccessible::Relation>> expected{
        {otherIface, relation}
    };

    const auto itemRelations = itemIface->relations();
    QCOMPARE(itemRelations, expected);

    const auto otherRelations = otherIface->relations();
    QVERIFY(!otherRelations.isEmpty());
}

void tst_QQuickAccessible::controlsThatShouldSendObjectShow_data()
{
    QTest::addColumn<QByteArray>("qmlSnippet");

    QTest::newRow("BusyIndicator") << QByteArray("BusyIndicator { running: true }");
    QTest::newRow("Button") << QByteArray("Button { text: 'Button' }");
    QTest::newRow("ComboBox") << QByteArray("ComboBox { model: 3 }");
    QTest::newRow("Dial") << QByteArray("Dial { value: 0.5 }");
    // Label without a text is a bit unusual,
    // but the background can be an image with meaningful info...
    QTest::newRow("Label") << QByteArray("Label {\nbackground: Rectangle {\ncolor: 'red'\n}\nwidth: 50\nheight: 20}");
    QTest::newRow("ProgressBar") << QByteArray("ProgressBar { value: 0.5 }");
    QTest::newRow("RangeSlider") << QByteArray("RangeSlider { from: 1; to: 100; second.value: 50 }");
    QTest::newRow("RoundButton") << QByteArray("RoundButton { text: 'Yes, please' }");
    QTest::newRow("Slider") << QByteArray("Slider { value: 0.5 }");
    QTest::newRow("Switch") << QByteArray("Switch { text: 'Switch me' }");
    QTest::newRow("TextArea") << QByteArray("TextArea { width: 50}");
    QTest::newRow("TextField") << QByteArray("TextField { width: 50}");

    QTest::newRow("CheckBox") << QByteArray("CheckBox { text: 'checkBox' }");
    QTest::newRow("DelayButton") << QByteArray("DelayButton { text: 'Are you really sure?' }");
    QTest::newRow("RadioButton") << QByteArray("RadioButton { text: 'RadioButton' }");
    QTest::newRow("TabButton") << QByteArray("TabButton { text: 'Home' }");
}

void tst_QQuickAccessible::controlsThatShouldSendObjectShow()
{
    QFETCH(QByteArray, qmlSnippet);

    auto clearEvents = qScopeGuard([]{ QTestAccessibility::clearEvents(); });
    QQmlApplicationEngine engine;
    engine.loadData(QByteArray(R"(import QtQuick
import QtQuick.Controls
Window { visible: true
)") + qmlSnippet + "}",
                    QUrl());

    QVERIFY(engine.rootObjects().count() > 0);
    QQuickWindow *window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    QQuickItem *contentItem = window->contentItem();
    QVERIFY(contentItem);
    QQuickItem *rootItem = contentItem->childItems().first();
    QVERIFY(rootItem);

    QAccessibleEvent ev(rootItem, QAccessible::ObjectShow);
    // Don't use QVERIFY_EVENT, because it gets very noisy when additional events are found
    QVERIFY(QTestAccessibility::containsEvent(&ev));
}

QTEST_MAIN(tst_QQuickAccessible)

#include "tst_qquickaccessible.moc"
