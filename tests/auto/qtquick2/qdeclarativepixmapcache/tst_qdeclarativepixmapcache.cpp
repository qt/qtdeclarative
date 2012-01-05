/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QtTest/QtTest>
#include <QtQuick/private/qdeclarativepixmapcache_p.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativeimageprovider.h>
#include <QNetworkReply>
#include "../../shared/util.h"
#include "testhttpserver.h"

#ifndef QT_NO_CONCURRENT
#include <qtconcurrentrun.h>
#include <qfuture.h>
#endif

#define PIXMAP_DATA_LEAK_TEST 0

class tst_qdeclarativepixmapcache : public QDeclarativeDataTest
{
    Q_OBJECT
public:
    tst_qdeclarativepixmapcache() : server(14452) {}

private slots:
    void initTestCase();
    void single();
    void single_data();
    void parallel();
    void parallel_data();
    void massive();
    void cancelcrash();
    void shrinkcache();
#ifndef QT_NO_CONCURRENT
    void networkCrash();
#endif
    void lockingCrash();
#if PIXMAP_DATA_LEAK_TEST
    void dataLeak();
#endif
private:
    QDeclarativeEngine engine;
    TestHTTPServer server;
};

static int slotters=0;

class Slotter : public QObject
{
    Q_OBJECT
public:
    Slotter()
    {
        gotslot = false;
        slotters++;
    }
    bool gotslot;

public slots:
    void got()
    {
        gotslot = true;
        --slotters;
        if (slotters==0)
            QTestEventLoop::instance().exitLoop();
    }
};

#ifndef QT_NO_LOCALFILE_OPTIMIZED_QML
static const bool localfile_optimized = true;
#else
static const bool localfile_optimized = false;
#endif


void tst_qdeclarativepixmapcache::initTestCase()
{
    QDeclarativeDataTest::initTestCase();
    server.serveDirectory(testFile("http"));
}

void tst_qdeclarativepixmapcache::single_data()
{
    // Note, since QDeclarativePixmapCache is shared, tests affect each other!
    // so use different files fore all test functions.

    QTest::addColumn<QUrl>("target");
    QTest::addColumn<bool>("incache");
    QTest::addColumn<bool>("exists");
    QTest::addColumn<bool>("neterror");

    // File URLs are optimized
    QTest::newRow("local") << testFileUrl("exists.png") << localfile_optimized << true << false;
    QTest::newRow("local") << testFileUrl("notexists.png") << localfile_optimized << false << false;
    QTest::newRow("remote") << QUrl("http://127.0.0.1:14452/exists.png") << false << true << false;
    QTest::newRow("remote") << QUrl("http://127.0.0.1:14452/notexists.png") << false << false << true;
}

void tst_qdeclarativepixmapcache::single()
{
    QFETCH(QUrl, target);
    QFETCH(bool, incache);
    QFETCH(bool, exists);
    QFETCH(bool, neterror);

    QString expectedError;
    if (neterror) {
        expectedError = "Error downloading " + target.toString() + " - server replied: Not found";
    } else if (!exists) {
        expectedError = "Cannot open: " + target.toString();
    }

    QDeclarativePixmap pixmap;
    QVERIFY(pixmap.width() <= 0); // Check Qt assumption

    pixmap.load(&engine, target);

    if (incache) {
        QCOMPARE(pixmap.error(), expectedError);
        if (exists) {
            QVERIFY(pixmap.status() == QDeclarativePixmap::Ready);
            QVERIFY(pixmap.width() > 0);
        } else {
            QVERIFY(pixmap.status() == QDeclarativePixmap::Error);
            QVERIFY(pixmap.width() <= 0);
        }
    } else {
        QVERIFY(pixmap.width() <= 0);

        Slotter getter;
        pixmap.connectFinished(&getter, SLOT(got()));
        QTestEventLoop::instance().enterLoop(10);
        QVERIFY(!QTestEventLoop::instance().timeout());
        QVERIFY(getter.gotslot);
        if (exists) {
            QVERIFY(pixmap.status() == QDeclarativePixmap::Ready);
            QVERIFY(pixmap.width() > 0);
        } else {
            QVERIFY(pixmap.status() == QDeclarativePixmap::Error);
            QVERIFY(pixmap.width() <= 0);
        }
        QCOMPARE(pixmap.error(), expectedError);
    }
}

void tst_qdeclarativepixmapcache::parallel_data()
{
    // Note, since QDeclarativePixmapCache is shared, tests affect each other!
    // so use different files fore all test functions.

    QTest::addColumn<QUrl>("target1");
    QTest::addColumn<QUrl>("target2");
    QTest::addColumn<int>("incache");
    QTest::addColumn<int>("cancel"); // which one to cancel

    QTest::newRow("local")
            << testFileUrl("exists1.png")
            << testFileUrl("exists2.png")
            << (localfile_optimized ? 2 : 0)
            << -1;

    QTest::newRow("remote")
            << QUrl("http://127.0.0.1:14452/exists2.png")
            << QUrl("http://127.0.0.1:14452/exists3.png")
            << 0
            << -1;

    QTest::newRow("remoteagain")
            << QUrl("http://127.0.0.1:14452/exists2.png")
            << QUrl("http://127.0.0.1:14452/exists3.png")
            << 2
            << -1;

    QTest::newRow("remotecopy")
            << QUrl("http://127.0.0.1:14452/exists4.png")
            << QUrl("http://127.0.0.1:14452/exists4.png")
            << 0
            << -1;

    QTest::newRow("remotecopycancel")
            << QUrl("http://127.0.0.1:14452/exists5.png")
            << QUrl("http://127.0.0.1:14452/exists5.png")
            << 0
            << 0;
}

void tst_qdeclarativepixmapcache::parallel()
{
    QFETCH(QUrl, target1);
    QFETCH(QUrl, target2);
    QFETCH(int, incache);
    QFETCH(int, cancel);

    QList<QUrl> targets;
    targets << target1 << target2;

    QList<QDeclarativePixmap *> pixmaps;
    QList<bool> pending;
    QList<Slotter*> getters;

    for (int i=0; i<targets.count(); ++i) {
        QUrl target = targets.at(i);
        QDeclarativePixmap *pixmap = new QDeclarativePixmap;

        pixmap->load(&engine, target);

        QVERIFY(pixmap->status() != QDeclarativePixmap::Error);
        pixmaps.append(pixmap);
        if (pixmap->isReady()) {
            QVERIFY(pixmap->width() >  0);
            getters.append(0);
            pending.append(false);
        } else {
            QVERIFY(pixmap->width() <= 0);
            getters.append(new Slotter);
            pixmap->connectFinished(getters[i], SLOT(got()));
            pending.append(true);
        }
    }

    QCOMPARE(incache+slotters, targets.count());

    if (cancel >= 0) {
        pixmaps.at(cancel)->clear(getters[cancel]);
        slotters--;
    }

    if (slotters) {
        QTestEventLoop::instance().enterLoop(10);
        QVERIFY(!QTestEventLoop::instance().timeout());
    }

    for (int i=0; i<targets.count(); ++i) {
        QDeclarativePixmap *pixmap = pixmaps[i];

        if (i == cancel) {
            QVERIFY(!getters[i]->gotslot);
        } else {
            if (pending[i]) 
                QVERIFY(getters[i]->gotslot);

            QVERIFY(pixmap->isReady());
            QVERIFY(pixmap->width() > 0);
            delete getters[i];
        }
    }

    qDeleteAll(pixmaps);
}

void tst_qdeclarativepixmapcache::massive()
{
    QDeclarativeEngine engine;
    QUrl url = testFileUrl("massive.png");

    // Confirm that massive images remain in the cache while they are
    // in use by the application.
    {
    qint64 cachekey = 0;
    QDeclarativePixmap p(&engine, url);
    QVERIFY(p.isReady());
    QVERIFY(p.image().size() == QSize(10000, 1000));
    cachekey = p.image().cacheKey();

    QDeclarativePixmap p2(&engine, url);
    QVERIFY(p2.isReady());
    QVERIFY(p2.image().size() == QSize(10000, 1000));

    QVERIFY(p2.image().cacheKey() == cachekey);
    }

    // Confirm that massive images are removed from the cache when
    // they become unused
    {
    qint64 cachekey = 0;
    {
        QDeclarativePixmap p(&engine, url);
        QVERIFY(p.isReady());
        QVERIFY(p.image().size() == QSize(10000, 1000));
        cachekey = p.image().cacheKey();
    }

    QDeclarativePixmap p2(&engine, url);
    QVERIFY(p2.isReady());
    QVERIFY(p2.image().size() == QSize(10000, 1000));

    QVERIFY(p2.image().cacheKey() != cachekey);
    }
}

// QTBUG-12729
void tst_qdeclarativepixmapcache::cancelcrash()
{
    QUrl url("http://127.0.0.1:14452/cancelcrash_notexist.png");
    for (int ii = 0; ii < 1000; ++ii) {
        QDeclarativePixmap pix(&engine, url);
    }
}

class MyPixmapProvider : public QDeclarativeImageProvider
{
public:
    MyPixmapProvider()
    : QDeclarativeImageProvider(Pixmap) {}

    virtual QPixmap requestPixmap(const QString &d, QSize *, const QSize &) {
        Q_UNUSED(d)
        QPixmap pix(800, 600);
        pix.fill(Qt::red);
        return pix;
    }
};

// QTBUG-13345
void tst_qdeclarativepixmapcache::shrinkcache()
{
    QDeclarativeEngine engine;
    engine.addImageProvider(QLatin1String("mypixmaps"), new MyPixmapProvider);

    for (int ii = 0; ii < 4000; ++ii) {
        QUrl url("image://mypixmaps/" + QString::number(ii));
        QDeclarativePixmap p(&engine, url);
    }
}

#ifndef QT_NO_CONCURRENT

void createNetworkServer()
{
   QEventLoop eventLoop;
   TestHTTPServer server(14453);
   server.serveDirectory(QDeclarativeDataTest::instance()->testFile("http"));
   QTimer::singleShot(100, &eventLoop, SLOT(quit()));
   eventLoop.exec();
}

#ifndef QT_NO_CONCURRENT
// QT-3957
void tst_qdeclarativepixmapcache::networkCrash()
{
    QFuture<void> future = QtConcurrent::run(createNetworkServer);
    QDeclarativeEngine engine;
    for (int ii = 0; ii < 100 ; ++ii) {
        QDeclarativePixmap* pixmap = new QDeclarativePixmap;
        pixmap->load(&engine,  QUrl(QString("http://127.0.0.1:14453/exists.png")));
        QTest::qSleep(1);
        pixmap->clear();
        delete pixmap;
    }
    future.cancel();
}
#endif

#endif

// QTBUG-22125
void tst_qdeclarativepixmapcache::lockingCrash()
{
    TestHTTPServer server(14453);
    server.serveDirectory(testFile("http"), TestHTTPServer::Delay);

    {
        QDeclarativePixmap* p = new QDeclarativePixmap;
        {
            QDeclarativeEngine e;
            p->load(&e,  QUrl(QString("http://127.0.0.1:14453/exists6.png")));
        }
        p->clear();
        QVERIFY(p->isNull());
        delete p;
    }
}


#if PIXMAP_DATA_LEAK_TEST
// This test should not be enabled by default as it
// produces spurious output in the expected case.
#include <QtQuick/QQuickView>
class DataLeakView : public QQuickView
{
    Q_OBJECT

public:
    explicit DataLeakView() : QQuickView()
    {
        setSource(testFileUrl("dataLeak.qml"));
    }

    void showFor2Seconds()
    {
        showFullScreen();
        QTimer::singleShot(2000, this, SIGNAL(ready()));
    }

signals:
    void ready();
};

// QTBUG-22742
Q_GLOBAL_STATIC(QDeclarativePixmap, dataLeakPixmap)
void tst_qdeclarativepixmapcache::dataLeak()
{
    // Should not leak cached QDeclarativePixmapData.
    // Unfortunately, since the QDeclarativePixmapStore
    // is a global static, and it releases the cache
    // entries on dtor (application exit), we must use
    // valgrind to determine whether it leaks or not.
    QDeclarativePixmap *p1 = new QDeclarativePixmap;
    QDeclarativePixmap *p2 = new QDeclarativePixmap;
    {
        QScopedPointer<DataLeakView> test(new DataLeakView);
        test->showFor2Seconds();
        dataLeakPixmap()->load(test->engine(), testFileUrl("exists.png"));
        p1->load(test->engine(), testFileUrl("exists.png"));
        p2->load(test->engine(), testFileUrl("exists2.png"));
        QTest::qWait(2005); // 2 seconds + a few more millis.
    }

    // When the (global static) dataLeakPixmap is deleted, it
    // shouldn't attempt to dereference a QDeclarativePixmapData
    // which has been deleted by the QDeclarativePixmapStore
    // destructor.
}
#endif
#undef PIXMAP_DATA_LEAK_TEST

QTEST_MAIN(tst_qdeclarativepixmapcache)

#include "tst_qdeclarativepixmapcache.moc"
