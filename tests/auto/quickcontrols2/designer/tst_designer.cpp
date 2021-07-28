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
