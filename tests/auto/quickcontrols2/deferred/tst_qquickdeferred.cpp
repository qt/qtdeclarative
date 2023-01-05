/****************************************************************************
**
** Copyright (C) 2023 The Qt Company Ltd.
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

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtTest/qtest.h>
#include <QQmlEngine>
#include <QtQuick/qquickitem.h>
#include <QtQuickTemplates2/private/qquickdeferredexecute_p_p.h>

class DeferredPropertyTester : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *objectProperty READ objectProperty WRITE setObjectProperty NOTIFY objectChanged)
    Q_CLASSINFO("DeferredPropertyNames", "objectProperty")

public:
    DeferredPropertyTester() {}


    QQuickItem *objectProperty() {

        if (!m_object.wasExecuted()) {
            quickBeginDeferred(this, "objectProperty", m_object);
            quickCompleteDeferred(this, "objectProperty", m_object);
        }

        return m_object;
    }
    void setObjectProperty(QQuickItem *obj) {
        if (m_object == obj)
            return;
        m_object = obj;
        if (!m_object.isExecuting()) // first read
            emit objectChanged();
    }

signals:
    void objectChanged();

private:
    QQuickDeferredPointer<QQuickItem> m_object = nullptr;
};

class tst_qquickdeferred : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickdeferred() : QQmlDataTest(QT_QMLTEST_DATADIR) {}
private slots:
    void noSpuriousBinding();
};



void tst_qquickdeferred::noSpuriousBinding() {
    qmlRegisterType<DeferredPropertyTester>("test", 1, 0, "DeferredPropertyTester");

    QQmlEngine engine;
    QQmlComponent comp(&engine, testFileUrl("noSpuriousBinding.qml"));
    std::unique_ptr<QObject> root(comp.create());
    QVERIFY2(root, qPrintable(comp.errorString()));
    root->setProperty("toggle", false);
}

QTEST_MAIN(tst_qquickdeferred)

#include "tst_qquickdeferred.moc"
