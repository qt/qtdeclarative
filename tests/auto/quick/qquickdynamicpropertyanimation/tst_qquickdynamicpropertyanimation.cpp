// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtTest/QtTest>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlProperty>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickItem>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtCore/QRect>
#include <QtGui/QColor>

#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickdynamicpropertyanimation : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickdynamicpropertyanimation() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private:
    template<class T>
    void dynamicPropertyAnimation(const QByteArray & propertyName, T toValue)
    {
        QQuickView view(testFileUrl("MyItem.qml"));
        QQuickItem * item = qobject_cast<QQuickItem *>(view.rootObject());
        QVERIFY(item);
        QQmlProperty testProp(item, propertyName);
        QPropertyAnimation animation(item, propertyName, this);
        animation.setEndValue(toValue);
        QCOMPARE(animation.targetObject(), item);
        QCOMPARE(animation.propertyName(), propertyName);
        QCOMPARE(animation.endValue().value<T>(), toValue);
        animation.start();
        QCOMPARE(animation.state(), QAbstractAnimation::Running);
        QTest::qWait(animation.duration());
        QTRY_COMPARE(testProp.read().value<T>(), toValue);
    }

private slots:
    void initTestCase() override
    {
        QQmlEngine engine;  // ensure types are registered
        QQmlDataTest::initTestCase();
    }

    void dynamicIntPropertyAnimation();
    void dynamicDoublePropertyAnimation();
    void dynamicRealPropertyAnimation();
    void dynamicPointPropertyAnimation();
    void dynamicSizePropertyAnimation();
    void dynamicRectPropertyAnimation();
    void dynamicColorPropertyAnimation();
    void dynamicVarPropertyAnimation();
};

void tst_qquickdynamicpropertyanimation::dynamicIntPropertyAnimation()
{
    dynamicPropertyAnimation("testInt", 1);
}

void tst_qquickdynamicpropertyanimation::dynamicDoublePropertyAnimation()
{
    dynamicPropertyAnimation("testDouble", 1.0);
}

void tst_qquickdynamicpropertyanimation::dynamicRealPropertyAnimation()
{
    dynamicPropertyAnimation("testReal", qreal(1.0));
}

void tst_qquickdynamicpropertyanimation::dynamicPointPropertyAnimation()
{
    dynamicPropertyAnimation("testPoint", QPoint(1, 1));
}

void tst_qquickdynamicpropertyanimation::dynamicSizePropertyAnimation()
{
    dynamicPropertyAnimation("testSize", QSize(1,1));
}

void tst_qquickdynamicpropertyanimation::dynamicRectPropertyAnimation()
{
    dynamicPropertyAnimation("testRect", QRect(1, 1, 1, 1));
}

void tst_qquickdynamicpropertyanimation::dynamicColorPropertyAnimation()
{
    dynamicPropertyAnimation("testColor", QColor::fromRgbF(1.0, 1.0, 1.0, 1.0));
}

void tst_qquickdynamicpropertyanimation::dynamicVarPropertyAnimation()
{
    dynamicPropertyAnimation("testVar", QVariant::fromValue(1));
}

QTEST_MAIN(tst_qquickdynamicpropertyanimation)

#include "tst_qquickdynamicpropertyanimation.moc"
