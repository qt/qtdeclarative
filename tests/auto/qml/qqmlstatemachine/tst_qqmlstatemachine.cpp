/****************************************************************************
**
** Copyright (C) 2015 Ford Motor Company
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
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
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTest>
#include "../../shared/util.h"

class tst_qqmlstatemachine : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlstatemachine();

private slots:
    void tst_cppObjectSignal();
};


class CppObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ObjectState objectState READ objectState WRITE setObjectState NOTIFY objectStateChanged)
    Q_ENUMS(ObjectState)
public:
    enum ObjectState {
        State0,
        State1,
        State2
    };

public:
    CppObject()
        : QObject()
        , m_objectState(State0)
    {}

    ObjectState objectState() const { return m_objectState; }
    void setObjectState(ObjectState objectState) { m_objectState = objectState; emit objectStateChanged();}

signals:
    void objectStateChanged();
    void mySignal(int signalState);

private:
    ObjectState m_objectState;
};

tst_qqmlstatemachine::tst_qqmlstatemachine()
{
    QVERIFY(-1 != qmlRegisterUncreatableType<CppObject>("CppObjectEnum", 1, 0, "CppObject", QString()));
}

void tst_qqmlstatemachine::tst_cppObjectSignal()
{
    CppObject cppObject;
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("cppsignal.qml"));
    QVERIFY(!component.isError());

    QQmlContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("_cppObject", &cppObject);
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject != 0);

    // wait for state machine to start
    QTRY_VERIFY(rootObject->property("running").toBool());

    // emit signal from cpp
    emit cppObject.mySignal(CppObject::State1);

    // check if the signal was propagated
    QTRY_COMPARE(cppObject.objectState(), CppObject::State1);

    // emit signal from cpp
    emit cppObject.mySignal(CppObject::State2);

    // check if the signal was propagated
    QTRY_COMPARE(cppObject.objectState(), CppObject::State2);

    // wait for state machine to finish
    QTRY_VERIFY(!rootObject->property("running").toBool());
}


QTEST_MAIN(tst_qqmlstatemachine)

#include "tst_qqmlstatemachine.moc"
