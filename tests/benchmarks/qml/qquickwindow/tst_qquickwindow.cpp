// Copyright (C) 2016 - 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtQuick/QQuickWindow>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/private/qquickwindow_p.h>

#include <qtest.h>
#include <QtTest/QtTest>

class tst_qquickwindow : public QObject
{
    Q_OBJECT
public:
    tst_qquickwindow();

private slots:
    void tst_updateCursor();
    void cleanupTestCase();
private:
    QQuickWindow* window;
};

tst_qquickwindow::tst_qquickwindow()
{
    window = new QQuickWindow;
    window->resize(250, 250);
    window->setPosition(100, 100);
    for ( int i=0; i<8000; i++ ) {
        QQuickRectangle *r =new QQuickRectangle(window->contentItem());
        for ( int j=0; j<10; ++j ) {
            new QQuickRectangle(r);
        }
    }
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
}

void tst_qquickwindow::cleanupTestCase()
{
    delete window;
}

void tst_qquickwindow::tst_updateCursor()
{
    QBENCHMARK {
        QQuickWindowPrivate::get(window)->updateCursor(QPoint(100,100));
    }
}

QTEST_MAIN(tst_qquickwindow);

#include "tst_qquickwindow.moc"
