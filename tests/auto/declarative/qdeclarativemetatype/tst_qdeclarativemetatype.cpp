/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <qdeclarative.h>

#include <private/qdeclarativemetatype_p.h>
#include <private/qdeclarativepropertyvalueinterceptor_p.h>

class tst_qdeclarativemetatype : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativemetatype() {}

private slots:
    void initTestCase();

    void qmlParserStatusCast();
    void qmlPropertyValueSourceCast();
    void qmlPropertyValueInterceptorCast();

    void isList();

    void defaultObject();
};

class TestType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo)

    Q_CLASSINFO("DefaultProperty", "foo")
public:
    int foo() { return 0; }
};
QML_DECLARE_TYPE(TestType);

class ParserStatusTestType : public QObject, public QDeclarativeParserStatus
{
    Q_OBJECT
    void classBegin(){}
    void componentComplete(){}
    Q_CLASSINFO("DefaultProperty", "foo") // Missing default property
    Q_INTERFACES(QDeclarativeParserStatus)
};
QML_DECLARE_TYPE(ParserStatusTestType);

class ValueSourceTestType : public QObject, public QDeclarativePropertyValueSource
{
    Q_OBJECT
    Q_INTERFACES(QDeclarativePropertyValueSource)
public:
    virtual void setTarget(const QDeclarativeProperty &) {}
};
QML_DECLARE_TYPE(ValueSourceTestType);

class ValueInterceptorTestType : public QObject, public QDeclarativePropertyValueInterceptor
{
    Q_OBJECT
    Q_INTERFACES(QDeclarativePropertyValueInterceptor)
public:
    virtual void setTarget(const QDeclarativeProperty &) {}
    virtual void write(const QVariant &) {}
};
QML_DECLARE_TYPE(ValueInterceptorTestType);

void tst_qdeclarativemetatype::initTestCase()
{
    qmlRegisterType<TestType>("Test", 1, 0, "TestType");
    qmlRegisterType<ParserStatusTestType>("Test", 1, 0, "ParserStatusTestType");
    qmlRegisterType<ValueSourceTestType>("Test", 1, 0, "ValueSourceTestType");
    qmlRegisterType<ValueInterceptorTestType>("Test", 1, 0, "ValueInterceptorTestType");
}

void tst_qdeclarativemetatype::qmlParserStatusCast()
{
    QVERIFY(QDeclarativeMetaType::qmlType(QVariant::Int) == 0);
    QVERIFY(QDeclarativeMetaType::qmlType(qMetaTypeId<TestType *>()) != 0);
    QCOMPARE(QDeclarativeMetaType::qmlType(qMetaTypeId<TestType *>())->parserStatusCast(), -1);
    QVERIFY(QDeclarativeMetaType::qmlType(qMetaTypeId<ValueSourceTestType *>()) != 0);
    QCOMPARE(QDeclarativeMetaType::qmlType(qMetaTypeId<ValueSourceTestType *>())->parserStatusCast(), -1);
            
    QVERIFY(QDeclarativeMetaType::qmlType(qMetaTypeId<ParserStatusTestType *>()) != 0);
    int cast = QDeclarativeMetaType::qmlType(qMetaTypeId<ParserStatusTestType *>())->parserStatusCast();
    QVERIFY(cast != -1);
    QVERIFY(cast != 0);

    ParserStatusTestType t;
    QVERIFY(reinterpret_cast<char *>((QObject *)&t) != reinterpret_cast<char *>((QDeclarativeParserStatus *)&t));

    QDeclarativeParserStatus *status = reinterpret_cast<QDeclarativeParserStatus *>(reinterpret_cast<char *>((QObject *)&t) + cast);
    QCOMPARE(status, (QDeclarativeParserStatus*)&t);
}

void tst_qdeclarativemetatype::qmlPropertyValueSourceCast()
{
    QVERIFY(QDeclarativeMetaType::qmlType(QVariant::Int) == 0);
    QVERIFY(QDeclarativeMetaType::qmlType(qMetaTypeId<TestType *>()) != 0);
    QCOMPARE(QDeclarativeMetaType::qmlType(qMetaTypeId<TestType *>())->propertyValueSourceCast(), -1);
    QVERIFY(QDeclarativeMetaType::qmlType(qMetaTypeId<ParserStatusTestType *>()) != 0);
    QCOMPARE(QDeclarativeMetaType::qmlType(qMetaTypeId<ParserStatusTestType *>())->propertyValueSourceCast(), -1);
            
    QVERIFY(QDeclarativeMetaType::qmlType(qMetaTypeId<ValueSourceTestType *>()) != 0);
    int cast = QDeclarativeMetaType::qmlType(qMetaTypeId<ValueSourceTestType *>())->propertyValueSourceCast();
    QVERIFY(cast != -1);
    QVERIFY(cast != 0);

    ValueSourceTestType t;
    QVERIFY(reinterpret_cast<char *>((QObject *)&t) != reinterpret_cast<char *>((QDeclarativePropertyValueSource *)&t));

    QDeclarativePropertyValueSource *source = reinterpret_cast<QDeclarativePropertyValueSource *>(reinterpret_cast<char *>((QObject *)&t) + cast);
    QCOMPARE(source, (QDeclarativePropertyValueSource*)&t);
}

void tst_qdeclarativemetatype::qmlPropertyValueInterceptorCast()
{
    QVERIFY(QDeclarativeMetaType::qmlType(QVariant::Int) == 0);
    QVERIFY(QDeclarativeMetaType::qmlType(qMetaTypeId<TestType *>()) != 0);
    QCOMPARE(QDeclarativeMetaType::qmlType(qMetaTypeId<TestType *>())->propertyValueInterceptorCast(), -1);
    QVERIFY(QDeclarativeMetaType::qmlType(qMetaTypeId<ParserStatusTestType *>()) != 0);
    QCOMPARE(QDeclarativeMetaType::qmlType(qMetaTypeId<ParserStatusTestType *>())->propertyValueInterceptorCast(), -1);
            
    QVERIFY(QDeclarativeMetaType::qmlType(qMetaTypeId<ValueInterceptorTestType *>()) != 0);
    int cast = QDeclarativeMetaType::qmlType(qMetaTypeId<ValueInterceptorTestType *>())->propertyValueInterceptorCast();
    QVERIFY(cast != -1);
    QVERIFY(cast != 0);

    ValueInterceptorTestType t;
    QVERIFY(reinterpret_cast<char *>((QObject *)&t) != reinterpret_cast<char *>((QDeclarativePropertyValueInterceptor *)&t));

    QDeclarativePropertyValueInterceptor *interceptor = reinterpret_cast<QDeclarativePropertyValueInterceptor *>(reinterpret_cast<char *>((QObject *)&t) + cast);
    QCOMPARE(interceptor, (QDeclarativePropertyValueInterceptor*)&t);
}

void tst_qdeclarativemetatype::isList()
{
    QCOMPARE(QDeclarativeMetaType::isList(QVariant::Invalid), false);
    QCOMPARE(QDeclarativeMetaType::isList(QVariant::Int), false);

    QDeclarativeListProperty<TestType> list;

    QCOMPARE(QDeclarativeMetaType::isList(qMetaTypeId<QDeclarativeListProperty<TestType> >()), true);
}

void tst_qdeclarativemetatype::defaultObject()
{
    QVERIFY(QDeclarativeMetaType::defaultProperty(&QObject::staticMetaObject).name() == 0);
    QVERIFY(QDeclarativeMetaType::defaultProperty(&ParserStatusTestType::staticMetaObject).name() == 0);
    QCOMPARE(QString(QDeclarativeMetaType::defaultProperty(&TestType::staticMetaObject).name()), QString("foo"));

    QObject o;
    TestType t;
    ParserStatusTestType p;

    QVERIFY(QDeclarativeMetaType::defaultProperty((QObject *)0).name() == 0);
    QVERIFY(QDeclarativeMetaType::defaultProperty(&o).name() == 0);
    QVERIFY(QDeclarativeMetaType::defaultProperty(&p).name() == 0);
    QCOMPARE(QString(QDeclarativeMetaType::defaultProperty(&t).name()), QString("foo"));
}

QTEST_MAIN(tst_qdeclarativemetatype)

#include "tst_qdeclarativemetatype.moc"
