/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QDebug>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QtGui/QScreen>
#include "../../shared/util.h"

class tst_qquickscreen : public QDeclarativeDataTest
{
    Q_OBJECT
private slots:
    void basicProperties();
};

void tst_qquickscreen::basicProperties()
{
    QQuickView view;
    view.setSource(testFileUrl("screen.qml"));
    view.show();
    QTest::qWaitForWindowShown(&view);

    QQuickItem* root = view.rootObject();
    QVERIFY(root);

    QScreen* screen = view.screen();
    QVERIFY(screen);

    QCOMPARE(screen->size().width(), root->property("w").toInt());
    QCOMPARE(screen->size().height(), root->property("h").toInt());
    QCOMPARE(int(screen->currentOrientation()), root->property("curOrientation").toInt());
    QCOMPARE(int(screen->primaryOrientation()), root->property("priOrientation").toInt());
}

QTEST_MAIN(tst_qquickscreen)

#include "tst_qquickscreen.moc"
