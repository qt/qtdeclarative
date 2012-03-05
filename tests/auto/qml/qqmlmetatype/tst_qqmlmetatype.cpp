/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <qqml.h>

#include <private/qqmlmetatype_p.h>
#include <private/qqmlpropertyvalueinterceptor_p.h>

class tst_qqmlmetatype : public QObject
{
    Q_OBJECT
public:
    tst_qqmlmetatype() {}

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

class ParserStatusTestType : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    void classBegin(){}
    void componentComplete(){}
    Q_CLASSINFO("DefaultProperty", "foo") // Missing default property
    Q_INTERFACES(QQmlParserStatus)
};
QML_DECLARE_TYPE(ParserStatusTestType);

class ValueSourceTestType : public QObject, public QQmlPropertyValueSource
{
    Q_OBJECT
    Q_INTERFACES(QQmlPropertyValueSource)
public:
    virtual void setTarget(const QQmlProperty &) {}
};
QML_DECLARE_TYPE(ValueSourceTestType);

class ValueInterceptorTestType : public QObject, public QQmlPropertyValueInterceptor
{
    Q_OBJECT
    Q_INTERFACES(QQmlPropertyValueInterceptor)
public:
    virtual void setTarget(const QQmlProperty &) {}
    virtual void write(const QVariant &) {}
};
QML_DECLARE_TYPE(ValueInterceptorTestType);

void tst_qqmlmetatype::initTestCase()
{
    qmlRegisterType<TestType>("Test", 1, 0, "TestType");
    qmlRegisterType<ParserStatusTestType>("Test", 1, 0, "ParserStatusTestType");
    qmlRegisterType<ValueSourceTestType>("Test", 1, 0, "ValueSourceTestType");
    qmlRegisterType<ValueInterceptorTestType>("Test", 1, 0, "ValueInterceptorTestType");
}

void tst_qqmlmetatype::qmlParserStatusCast()
{
    QVERIFY(QQmlMetaType::qmlType(QVariant::Int) == 0);
    QVERIFY(QQmlMetaType::qmlType(qMetaTypeId<TestType *>()) != 0);
    QCOMPARE(QQmlMetaType::qmlType(qMetaTypeId<TestType *>())->parserStatusCast(), -1);
    QVERIFY(QQmlMetaType::qmlType(qMetaTypeId<ValueSourceTestType *>()) != 0);
    QCOMPARE(QQmlMetaType::qmlType(qMetaTypeId<ValueSourceTestType *>())->parserStatusCast(), -1);
            
    QVERIFY(QQmlMetaType::qmlType(qMetaTypeId<ParserStatusTestType *>()) != 0);
    int cast = QQmlMetaType::qmlType(qMetaTypeId<ParserStatusTestType *>())->parserStatusCast();
    QVERIFY(cast != -1);
    QVERIFY(cast != 0);

    ParserStatusTestType t;
    QVERIFY(reinterpret_cast<char *>((QObject *)&t) != reinterpret_cast<char *>((QQmlParserStatus *)&t));

    QQmlParserStatus *status = reinterpret_cast<QQmlParserStatus *>(reinterpret_cast<char *>((QObject *)&t) + cast);
    QCOMPARE(status, (QQmlParserStatus*)&t);
}

void tst_qqmlmetatype::qmlPropertyValueSourceCast()
{
    QVERIFY(QQmlMetaType::qmlType(QVariant::Int) == 0);
    QVERIFY(QQmlMetaType::qmlType(qMetaTypeId<TestType *>()) != 0);
    QCOMPARE(QQmlMetaType::qmlType(qMetaTypeId<TestType *>())->propertyValueSourceCast(), -1);
    QVERIFY(QQmlMetaType::qmlType(qMetaTypeId<ParserStatusTestType *>()) != 0);
    QCOMPARE(QQmlMetaType::qmlType(qMetaTypeId<ParserStatusTestType *>())->propertyValueSourceCast(), -1);
            
    QVERIFY(QQmlMetaType::qmlType(qMetaTypeId<ValueSourceTestType *>()) != 0);
    int cast = QQmlMetaType::qmlType(qMetaTypeId<ValueSourceTestType *>())->propertyValueSourceCast();
    QVERIFY(cast != -1);
    QVERIFY(cast != 0);

    ValueSourceTestType t;
    QVERIFY(reinterpret_cast<char *>((QObject *)&t) != reinterpret_cast<char *>((QQmlPropertyValueSource *)&t));

    QQmlPropertyValueSource *source = reinterpret_cast<QQmlPropertyValueSource *>(reinterpret_cast<char *>((QObject *)&t) + cast);
    QCOMPARE(source, (QQmlPropertyValueSource*)&t);
}

void tst_qqmlmetatype::qmlPropertyValueInterceptorCast()
{
    QVERIFY(QQmlMetaType::qmlType(QVariant::Int) == 0);
    QVERIFY(QQmlMetaType::qmlType(qMetaTypeId<TestType *>()) != 0);
    QCOMPARE(QQmlMetaType::qmlType(qMetaTypeId<TestType *>())->propertyValueInterceptorCast(), -1);
    QVERIFY(QQmlMetaType::qmlType(qMetaTypeId<ParserStatusTestType *>()) != 0);
    QCOMPARE(QQmlMetaType::qmlType(qMetaTypeId<ParserStatusTestType *>())->propertyValueInterceptorCast(), -1);
            
    QVERIFY(QQmlMetaType::qmlType(qMetaTypeId<ValueInterceptorTestType *>()) != 0);
    int cast = QQmlMetaType::qmlType(qMetaTypeId<ValueInterceptorTestType *>())->propertyValueInterceptorCast();
    QVERIFY(cast != -1);
    QVERIFY(cast != 0);

    ValueInterceptorTestType t;
    QVERIFY(reinterpret_cast<char *>((QObject *)&t) != reinterpret_cast<char *>((QQmlPropertyValueInterceptor *)&t));

    QQmlPropertyValueInterceptor *interceptor = reinterpret_cast<QQmlPropertyValueInterceptor *>(reinterpret_cast<char *>((QObject *)&t) + cast);
    QCOMPARE(interceptor, (QQmlPropertyValueInterceptor*)&t);
}

void tst_qqmlmetatype::isList()
{
    QCOMPARE(QQmlMetaType::isList(QVariant::Invalid), false);
    QCOMPARE(QQmlMetaType::isList(QVariant::Int), false);

    QQmlListProperty<TestType> list;

    QCOMPARE(QQmlMetaType::isList(qMetaTypeId<QQmlListProperty<TestType> >()), true);
}

void tst_qqmlmetatype::defaultObject()
{
    QVERIFY(QQmlMetaType::defaultProperty(&QObject::staticMetaObject).name() == 0);
    QVERIFY(QQmlMetaType::defaultProperty(&ParserStatusTestType::staticMetaObject).name() == 0);
    QCOMPARE(QString(QQmlMetaType::defaultProperty(&TestType::staticMetaObject).name()), QString("foo"));

    QObject o;
    TestType t;
    ParserStatusTestType p;

    QVERIFY(QQmlMetaType::defaultProperty((QObject *)0).name() == 0);
    QVERIFY(QQmlMetaType::defaultProperty(&o).name() == 0);
    QVERIFY(QQmlMetaType::defaultProperty(&p).name() == 0);
    QCOMPARE(QString(QQmlMetaType::defaultProperty(&t).name()), QString("foo"));
}

QTEST_MAIN(tst_qqmlmetatype)

#include "tst_qqmlmetatype.moc"
