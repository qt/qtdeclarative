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
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtQuick/qquickview.h>
#include <private/qquickflipable_p.h>
#include <private/qdeclarativevaluetype_p.h>
#include <QFontMetrics>
#include <QtQuick/private/qquickrectangle_p.h>
#include <math.h>
#include <QtOpenGL/QGLShaderProgram>
#include "../../shared/util.h"

class tst_qquickflipable : public QObject
{
    Q_OBJECT
public:
    tst_qquickflipable();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void create();
    void checkFrontAndBack();
    void setFrontAndBack();

    // below here task issues
    void QTBUG_9161_crash();
    void QTBUG_8474_qgv_abort();

private:
    QDeclarativeEngine engine;
};

tst_qquickflipable::tst_qquickflipable()
{
}
void tst_qquickflipable::initTestCase()
{
}

void tst_qquickflipable::cleanupTestCase()
{

}

void tst_qquickflipable::create()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(TESTDATA("test-flipable.qml")));
    QQuickFlipable *obj = qobject_cast<QQuickFlipable*>(c.create());

    QVERIFY(obj != 0);
    delete obj;
}

void tst_qquickflipable::checkFrontAndBack()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(TESTDATA("test-flipable.qml")));
    QQuickFlipable *obj = qobject_cast<QQuickFlipable*>(c.create());

    QVERIFY(obj != 0);
    QVERIFY(obj->front() != 0);
    QVERIFY(obj->back() != 0);
    delete obj;
}

void tst_qquickflipable::setFrontAndBack()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(TESTDATA("test-flipable.qml")));
    QQuickFlipable *obj = qobject_cast<QQuickFlipable*>(c.create());

    QVERIFY(obj != 0);
    QVERIFY(obj->front() != 0);
    QVERIFY(obj->back() != 0);

    QString message = c.url().toString() + ":3:1: QML Flipable: front is a write-once property";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(message));
    obj->setFront(new QQuickRectangle());

    message = c.url().toString() + ":3:1: QML Flipable: back is a write-once property";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(message));
    obj->setBack(new QQuickRectangle());
    delete obj;
}

void tst_qquickflipable::QTBUG_9161_crash()
{
    QQuickView *canvas = new QQuickView;
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("crash.qml")));
    QQuickItem *root = canvas->rootObject();
    QVERIFY(root != 0);
    canvas->show();
    delete canvas;
}

void tst_qquickflipable::QTBUG_8474_qgv_abort()
{
    QQuickView *canvas = new QQuickView;
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("flipable-abort.qml")));
    QQuickItem *root = canvas->rootObject();
    QVERIFY(root != 0);
    canvas->show();
    delete canvas;
}

QTEST_MAIN(tst_qquickflipable)

#include "tst_qquickflipable.moc"
