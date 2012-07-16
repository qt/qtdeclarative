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
#include <QQmlEngine>
#include <QQmlComponent>
#include <QDebug>
#include <private/qquickvaluetypes_p.h>
#include "../../shared/util.h"
#include "testtypes.h"

QT_BEGIN_NAMESPACE
extern int qt_defaultDpi(void);
QT_END_NAMESPACE

// There is some overlap between the qqmllanguage and qqmlvaluetypes
// test here, but it needs to be separate to ensure that no QML plugins
// are loaded prior to these tests, which could contaminate the type
// system with more providers.

class tst_qqmlvaluetypeproviders : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlvaluetypeproviders() {}

private slots:
    void initTestCase();

    void qtqmlValueTypes();   // This test function _must_ be the first test function run.
    void qtquickValueTypes();
    void comparisonSemantics();
    void cppIntegration();
    void jsObjectConversion();
    void invokableFunctions();
};

void tst_qqmlvaluetypeproviders::initTestCase()
{
    QQmlDataTest::initTestCase();
    registerTypes();
}

void tst_qqmlvaluetypeproviders::qtqmlValueTypes()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("qtqmlValueTypes.qml"));
    QVERIFY(!component.isError());
    QVERIFY(component.errors().isEmpty());
    QObject *object = component.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("qtqmlTypeSuccess").toBool());
    QVERIFY(object->property("qtquickTypeSuccess").toBool());
    delete object;
}

void tst_qqmlvaluetypeproviders::qtquickValueTypes()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("qtquickValueTypes.qml"));
    QVERIFY(!component.isError());
    QVERIFY(component.errors().isEmpty());
    QObject *object = component.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("qtqmlTypeSuccess").toBool());
    QVERIFY(object->property("qtquickTypeSuccess").toBool());
    delete object;
}

void tst_qqmlvaluetypeproviders::comparisonSemantics()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("comparisonSemantics.qml"));
    QVERIFY(!component.isError());
    QVERIFY(component.errors().isEmpty());
    QObject *object = component.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("comparisonSuccess").toBool());
    delete object;
}

void tst_qqmlvaluetypeproviders::cppIntegration()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("cppIntegration.qml"));
    QVERIFY(!component.isError());
    QVERIFY(component.errors().isEmpty());
    QObject *object = component.create();
    QVERIFY(object != 0);

    // ensure accessing / comparing / assigning cpp-defined props
    // and qml-defined props works in QML.
    QVERIFY(object->property("success").toBool());

    // ensure types match
    QCOMPARE(object->property("g").userType(), object->property("rectf").userType());
    QCOMPARE(object->property("p").userType(), object->property("pointf").userType());
    QCOMPARE(object->property("z").userType(), object->property("sizef").userType());
    QCOMPARE(object->property("v2").userType(), object->property("vector2").userType());
    QCOMPARE(object->property("v3").userType(), object->property("vector").userType());
    QCOMPARE(object->property("v4").userType(), object->property("vector4").userType());
    QCOMPARE(object->property("q").userType(), object->property("quaternion").userType());
    QCOMPARE(object->property("m").userType(), object->property("matrix").userType());
    QCOMPARE(object->property("c").userType(), object->property("color").userType());
    QCOMPARE(object->property("f").userType(), object->property("font").userType());

    // ensure values match
    QCOMPARE(object->property("g").value<QRectF>(), object->property("rectf").value<QRectF>());
    QCOMPARE(object->property("p").value<QPointF>(), object->property("pointf").value<QPointF>());
    QCOMPARE(object->property("z").value<QSizeF>(), object->property("sizef").value<QSizeF>());
    QCOMPARE(object->property("v2").value<QVector2D>(), object->property("vector2").value<QVector2D>());
    QCOMPARE(object->property("v3").value<QVector3D>(), object->property("vector").value<QVector3D>());
    QCOMPARE(object->property("v4").value<QVector4D>(), object->property("vector4").value<QVector4D>());
    QCOMPARE(object->property("q").value<QQuaternion>(), object->property("quaternion").value<QQuaternion>());
    QCOMPARE(object->property("m").value<QMatrix4x4>(), object->property("matrix").value<QMatrix4x4>());
    QCOMPARE(object->property("c").value<QColor>(), object->property("color").value<QColor>());
    QCOMPARE(object->property("f").value<QFont>(), object->property("font").value<QFont>());

    delete object;
}

void tst_qqmlvaluetypeproviders::jsObjectConversion()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("jsObjectConversion.qml"));
    QVERIFY(!component.isError());
    QVERIFY(component.errors().isEmpty());
    QObject *object = component.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("qtquickTypeSuccess").toBool());
    delete object;
}

void tst_qqmlvaluetypeproviders::invokableFunctions()
{
    QQmlEngine e;
    QQmlComponent component(&e, testFileUrl("invokableFunctions.qml"));
    QVERIFY(!component.isError());
    QVERIFY(component.errors().isEmpty());
    QObject *object = component.create();
    QVERIFY(object != 0);
    QVERIFY(object->property("complete").toBool());
    QVERIFY(object->property("success").toBool());
    delete object;
}

QTEST_MAIN(tst_qqmlvaluetypeproviders)

#include "tst_qqmlvaluetypeproviders.moc"
