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
#include <QDeclarativeEngine>
#include <QDeclarativeComponent>
#include <private/qdeclarativemetatype_p.h>
#include <QDeclarativeContext>

class tst_animation : public QObject
{
    Q_OBJECT
public:
    tst_animation();

private slots:
    void animationtree_qml();

    void animationelements_data();
    void animationelements();

    void numberAnimation();
    void numberAnimationStarted();
    void numberAnimationMultipleTargets();

private:
    QDeclarativeEngine engine;
};

tst_animation::tst_animation()
{
}

inline QUrl TEST_FILE(const QString &filename)
{
    return QUrl::fromLocalFile(QLatin1String(SRCDIR) + QLatin1String("/data/") + filename);
}

void tst_animation::animationtree_qml()
{
    QDeclarativeComponent component(&engine, TEST_FILE("animation.qml"));
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

    QSet<QString> types = QDeclarativeMetaType::qmlTypeNames().toSet();
    foreach (const QString &type, types) {
        if (type.contains(QLatin1String("Animation")))
            QTest::newRow(type.toLatin1()) << type;
    }

    QTest::newRow("QtQuick/Behavior") << "QtQuick/Behavior";
    QTest::newRow("QtQuick/Transition") << "QtQuick/Transition";
}

void tst_animation::animationelements()
{
    QFETCH(QString, type);
    QDeclarativeType *t = QDeclarativeMetaType::qmlType(type, 2, 0);
    if (!t || !t->isCreatable())
        QSKIP("Non-creatable type", SkipSingle);

    QBENCHMARK {
        QObject *obj = t->create();
        delete obj;
    }
}

void tst_animation::numberAnimation()
{
    QDeclarativeComponent component(&engine);
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
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 2.0\nItem { Rectangle { id: rect; NumberAnimation { target: rect; property: \"x\"; to: 100; duration: 500; easing.type: Easing.InOutQuad; running: true; Component.onCompleted: pause() } } }", QUrl());

    QObject *obj = component.create();
    delete obj;

    QBENCHMARK {
        QObject *obj = component.create();
        delete obj;
    }
}

void tst_animation::numberAnimationMultipleTargets()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 2.0\nItem { Rectangle { id: rect; NumberAnimation { target: rect; properties: \"x,y,z,width,height,implicitWidth,implicitHeight\"; to: 100; duration: 500; easing.type: Easing.InOutQuad; running: true; Component.onCompleted: pause() } } }", QUrl());

    QObject *obj = component.create();
    delete obj;

    QBENCHMARK {
        QObject *obj = component.create();
        delete obj;
    }
}

QTEST_MAIN(tst_animation)

#include "tst_animation.moc"
