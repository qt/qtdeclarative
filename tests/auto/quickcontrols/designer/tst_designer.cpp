// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>
#include <QtQuick>

#include <QtQuickControls2>
#include <QQmlComponent>
#include <QDir>

#include <private/qquickdesignersupportitems_p.h>

class tst_Designer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void test_controls();
    void test_controls_data();
};


void tst_Designer::initTestCase()
{
    QQuickStyle::setStyle("Basic");
}

void doComponentCompleteRecursive(QObject *object)
{
    if (object) {
        QQuickItem *item = qobject_cast<QQuickItem*>(object);

        if (item && DesignerSupport::isComponentComplete(item))
            return;

        DesignerSupport::emitComponentCompleteSignalForAttachedProperty(qobject_cast<QQuickItem*>(object));

        QList<QObject*> childList = object->children();

        if (item) {
            for (QQuickItem *childItem : item->childItems()) {
                if (!childList.contains(childItem))
                    childList.append(childItem);
            }
        }

        for (QObject *child : childList)
                doComponentCompleteRecursive(child);

        if (item) {
            static_cast<QQmlParserStatus*>(item)->componentComplete();
        } else {
            QQmlParserStatus *qmlParserStatus = dynamic_cast< QQmlParserStatus*>(object);
            if (qmlParserStatus)
                qmlParserStatus->componentComplete();
        }
    }
}


void tst_Designer::test_controls()
{
    QFETCH(QString, type);

    const QByteArray before("import QtQuick\n"
                   "import QtQuick.Controls\n"
                   "Item {\n");

    QByteArray source = before;
    source.append(type.toUtf8());

    const QByteArray after(" {"
                        "}\n"
                        "}\n");

    source.append(after);

    QQmlEngine engine;
    QQmlComponent component(&engine);

    {
        ComponentCompleteDisabler disableComponentComplete;
        component.setData(source, QUrl::fromLocalFile(QDir::current().absolutePath()));
    }

    QObject *root = component.create();
    QVERIFY(root);
    doComponentCompleteRecursive(root);
}

void tst_Designer::test_controls_data()
{
    QTest::addColumn<QString>("type");

    QTest::newRow("Button") << "Button";
    QTest::newRow("CheckBox") << "CheckBox";
    QTest::newRow("ComboBox") << "ComboBox";
    QTest::newRow("DelayButton") << "DelayButton";
    QTest::newRow("Dial") << "Dial";
    QTest::newRow("Frame") << "Frame";
    QTest::newRow("GroupBox") << "GroupBox";
    QTest::newRow("Label") << "Label";
    QTest::newRow("Page") << "Page";
    QTest::newRow("Pane") << "Pane";
    QTest::newRow("ProgressBar") << "ProgressBar";
    QTest::newRow("RadioButton") << "RadioButton";
    QTest::newRow("RangeSlider") << "RangeSlider";
    QTest::newRow("RoundButton") << "RoundButton";
    QTest::newRow("ScrollView") << "ScrollView";
    QTest::newRow("Slider") << "Slider";
    QTest::newRow("SpinBox") << "SpinBox";
    QTest::newRow("StackView") << "StackView";
    QTest::newRow("SwipeView") << "SwipeView";
    QTest::newRow("Switch") << "Switch";
    QTest::newRow("Switch") << "Switch";
    QTest::newRow("TabBar") << "TabBar";
    QTest::newRow("TabButton") << "TabButton";
    QTest::newRow("TextArea") << "TextArea";
    QTest::newRow("TextField") << "TextField";
    QTest::newRow("ToolBar") << "ToolBar";
    QTest::newRow("ToolButton") << "ToolButton";
    QTest::newRow("Tumbler") << "Tumbler";
}

QTEST_MAIN(tst_Designer)

#include "tst_designer.moc"
