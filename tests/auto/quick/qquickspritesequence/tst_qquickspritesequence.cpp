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
#include <QtTest/QtTest>
#include "../../shared/util.h"
#include <QtQuick/qquickview.h>
#include <private/qquickspritesequence_p.h>

class tst_qquickspritesequence : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickspritesequence(){}

private slots:
    void test_properties();
    void test_framerateAdvance();//Separate codepath for QQuickSpriteEngine
    void test_jumpToCrash();
};

void tst_qquickspritesequence::test_properties()
{
    QQuickView *window = new QQuickView(0);

    window->setSource(testFileUrl("basic.qml"));
    window->show();
    QTest::qWaitForWindowShown(window);

    QVERIFY(window->rootObject());
    QQuickSpriteSequence* sprite = window->rootObject()->findChild<QQuickSpriteSequence*>("sprite");
    QVERIFY(sprite);

    QVERIFY(sprite->running());
    QVERIFY(sprite->interpolate());

    sprite->setRunning(false);
    QVERIFY(!sprite->running());
    sprite->setInterpolate(false);
    QVERIFY(!sprite->interpolate());

    delete window;
}

void tst_qquickspritesequence::test_framerateAdvance()
{
    QQuickView *window = new QQuickView(0);

    window->setSource(testFileUrl("advance.qml"));
    window->show();
    QTest::qWaitForWindowShown(window);

    QVERIFY(window->rootObject());
    QQuickSpriteSequence* sprite = window->rootObject()->findChild<QQuickSpriteSequence*>("sprite");
    QVERIFY(sprite);

    QTRY_COMPARE(sprite->currentSprite(), QLatin1String("secondState"));
    delete window;
}

void tst_qquickspritesequence::test_jumpToCrash()
{
    QQuickView *window = new QQuickView(0);

    window->setSource(testFileUrl("crashonstart.qml"));
    window->show();
    QTest::qWaitForWindowShown(window);
    //verify: Don't crash

    delete window;
}

QTEST_MAIN(tst_qquickspritesequence)

#include "tst_qquickspritesequence.moc"
