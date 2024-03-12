// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <private/qqmlmetatype_p.h>
#include <private/qquickanimation_p_p.h>
#include <QQmlContext>

class tst_animation : public QObject
{
    Q_OBJECT
public:
    tst_animation();

private slots:
    void abstractAnimation();

#if defined(QT_BUILD_INTERNAL)
    void bulkValueAnimator();
    void propertyUpdater();
#endif

    void animationtree_qml();

    void animationelements_data();
    void animationelements();

    void numberAnimation();
    void numberAnimationStarted();
    void numberAnimationMultipleTargets();
    void numberAnimationEmpty();

private:
    QQmlEngine engine;
};

tst_animation::tst_animation()
{
}

inline QUrl TEST_FILE(const QString &filename)
{
    return QUrl::fromLocalFile(QLatin1String(SRCDIR) + QLatin1String("/data/") + filename);
}

void tst_animation::abstractAnimation()
{
    QBENCHMARK {
        QAbstractAnimationJob *animation = new QAbstractAnimationJob;
        delete animation;
    }
}

#if defined(QT_BUILD_INTERNAL)
void tst_animation::bulkValueAnimator()
{
    QBENCHMARK {
        QQuickBulkValueAnimator *animator = new QQuickBulkValueAnimator;
        delete animator;
    }
}

void tst_animation::propertyUpdater()
{
    QBENCHMARK {
        QQuickAnimationPropertyUpdater *updater = new QQuickAnimationPropertyUpdater;
        delete updater;
    }
}
#endif // QT_BUILD_INTERNAL

void tst_animation::animationtree_qml()
{
    QQmlComponent component(&engine, TEST_FILE("animation.qml"));
    QObject *obj = component.create();
    delete obj;

    QBENCHMARK {
        QObject *obj = component.create();
        delete obj;
    }
}

void tst_animation::animationelements_data()
{
    QTest::addColumn<QString>("type");

    const QSet<QString> types = QQmlMetaType::qmlTypeNames().toSet();
    for (const QString &type : types) {
        if (type.contains(QLatin1String("Animation")))
            QTest::newRow(type.toLatin1()) << type;
    }

    QTest::newRow("QtQuick/Behavior") << "QtQuick/Behavior";
    QTest::newRow("QtQuick/Transition") << "QtQuick/Transition";
}

void tst_animation::animationelements()
{
    QFETCH(QString, type);
    QQmlType t = QQmlMetaType::qmlType(type, 2, 0);
    if (!t.isValid() || !t.isCreatable())
        QSKIP("Non-creatable type");

    QBENCHMARK {
        QObject *obj = t.create();
        delete obj;
    }
}

void tst_animation::numberAnimation()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\nItem { Rectangle { id: rect; NumberAnimation { target: rect; property: \"x\"; to: 100; duration: 500; easing.type: Easing.InOutQuad } } }", QUrl());

    QObject *obj = component.create();
    delete obj;

    QBENCHMARK {
        QObject *obj = component.create();
        delete obj;
    }
}

void tst_animation::numberAnimationStarted()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\nItem { Rectangle { id: rect; NumberAnimation { target: rect; property: \"x\"; to: 100; duration: 500; easing.type: Easing.InOutQuad; running: true; paused: true } } }", QUrl());

    QObject *obj = component.create();
    delete obj;

    QBENCHMARK {
        QObject *obj = component.create();
        delete obj;
    }
}

void tst_animation::numberAnimationMultipleTargets()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\nItem { Rectangle { id: rect; NumberAnimation { target: rect; properties: \"x,y,z,width,height,implicitWidth,implicitHeight\"; to: 100; duration: 500; easing.type: Easing.InOutQuad; running: true; paused: true } } }", QUrl());

    QObject *obj = component.create();
    delete obj;

    QBENCHMARK {
        QObject *obj = component.create();
        delete obj;
    }
}

void tst_animation::numberAnimationEmpty()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\nNumberAnimation { }", QUrl());

    QObject *obj = component.create();
    delete obj;

    QBENCHMARK {
        QObject *obj = component.create();
        delete obj;
    }
}

QTEST_MAIN(tst_animation)

#include "tst_animation.moc"
