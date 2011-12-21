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
#include <QtQuick/private/qquickrectangle_p.h>
#include <private/qquickimage_p.h>
#include <private/qquickanimatedimage_p.h>
#include <QSignalSpy>
#include <QtDeclarative/qdeclarativecontext.h>

#include "../../shared/testhttpserver.h"
#include "../../shared/util.h"

Q_DECLARE_METATYPE(QQuickImageBase::Status)

class tst_qquickanimatedimage : public QDeclarativeDataTest
{
    Q_OBJECT
public:
    tst_qquickanimatedimage() {}

private slots:
    void play();
    void pause();
    void stopped();
    void setFrame();
    void frameCount();
    void mirror_running();
    void mirror_notRunning();
    void mirror_notRunning_data();
    void remote();
    void remote_data();
    void sourceSize();
    void sourceSizeReadOnly();
    void invalidSource();
    void qtbug_16520();
    void progressAndStatusChanges();

};

void tst_qquickanimatedimage::play()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, testFileUrl("stickman.qml"));
    QQuickAnimatedImage *anim = qobject_cast<QQuickAnimatedImage *>(component.create());
    QVERIFY(anim);
    QVERIFY(anim->isPlaying());

    delete anim;
}

void tst_qquickanimatedimage::pause()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, testFileUrl("stickmanpause.qml"));
    QQuickAnimatedImage *anim = qobject_cast<QQuickAnimatedImage *>(component.create());
    QVERIFY(anim);
    QVERIFY(anim->isPlaying());
    QVERIFY(anim->isPaused());

    delete anim;
}

void tst_qquickanimatedimage::stopped()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, testFileUrl("stickmanstopped.qml"));
    QQuickAnimatedImage *anim = qobject_cast<QQuickAnimatedImage *>(component.create());
    QVERIFY(anim);
    QVERIFY(!anim->isPlaying());
    QCOMPARE(anim->currentFrame(), 0);

    delete anim;
}

void tst_qquickanimatedimage::setFrame()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, testFileUrl("stickmanpause.qml"));
    QQuickAnimatedImage *anim = qobject_cast<QQuickAnimatedImage *>(component.create());
    QVERIFY(anim);
    QVERIFY(anim->isPlaying());
    QCOMPARE(anim->currentFrame(), 2);

    delete anim;
}

void tst_qquickanimatedimage::frameCount()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, testFileUrl("colors.qml"));
    QQuickAnimatedImage *anim = qobject_cast<QQuickAnimatedImage *>(component.create());
    QVERIFY(anim);
    QVERIFY(anim->isPlaying());
    QCOMPARE(anim->frameCount(), 3);

    delete anim;
}

void tst_qquickanimatedimage::mirror_running()
{
    // test where mirror is set to true after animation has started

    QQuickView *canvas = new QQuickView;
    canvas->show();

    canvas->setSource(testFileUrl("hearts.qml"));
    QQuickAnimatedImage *anim = qobject_cast<QQuickAnimatedImage *>(canvas->rootObject());
    QVERIFY(anim);

    int width = anim->property("width").toInt();

    QCOMPARE(anim->currentFrame(), 0);
    QPixmap frame0 = QPixmap::fromImage(canvas->grabFrameBuffer());

    anim->setCurrentFrame(1);
    QPixmap frame1 = QPixmap::fromImage(canvas->grabFrameBuffer());

    anim->setCurrentFrame(0);

    QSignalSpy spy(anim, SIGNAL(frameChanged()));
    anim->setPlaying(true);

    QTRY_VERIFY(spy.count() == 1); spy.clear();
    anim->setProperty("mirror", true);

    QCOMPARE(anim->currentFrame(), 1);
    QPixmap frame1_flipped = QPixmap::fromImage(canvas->grabFrameBuffer());

    QTRY_VERIFY(spy.count() == 1); spy.clear();
    QCOMPARE(anim->currentFrame(), 0);  // animation only has 2 frames, should cycle back to first
    QPixmap frame0_flipped = QPixmap::fromImage(canvas->grabFrameBuffer());

    QSKIP("Skip while QTBUG-19351 and QTBUG-19252 are not resolved");

    QTransform transform;
    transform.translate(width, 0).scale(-1, 1.0);
    QPixmap frame0_expected = frame0.transformed(transform);
    QPixmap frame1_expected = frame1.transformed(transform);

    QCOMPARE(frame0_flipped, frame0_expected);
    QCOMPARE(frame1_flipped, frame1_expected);

    delete canvas;
}

void tst_qquickanimatedimage::mirror_notRunning()
{
    QFETCH(QUrl, fileUrl);

    QQuickView *canvas = new QQuickView;
    canvas->show();

    canvas->setSource(fileUrl);
    QQuickAnimatedImage *anim = qobject_cast<QQuickAnimatedImage *>(canvas->rootObject());
    QVERIFY(anim);

    int width = anim->property("width").toInt();
    QPixmap screenshot = QPixmap::fromImage(canvas->grabFrameBuffer());

    QTransform transform;
    transform.translate(width, 0).scale(-1, 1.0);
    QPixmap expected = screenshot.transformed(transform);

    int frame = anim->currentFrame();
    bool playing = anim->isPlaying();
    bool paused = anim->isPlaying();

    anim->setProperty("mirror", true);
    screenshot = QPixmap::fromImage(canvas->grabFrameBuffer());

    QSKIP("Skip while QTBUG-19351 and QTBUG-19252 are not resolved");
    QCOMPARE(screenshot, expected);

    // mirroring should not change the current frame or playing status
    QCOMPARE(anim->currentFrame(), frame);
    QCOMPARE(anim->isPlaying(), playing);
    QCOMPARE(anim->isPaused(), paused);

    delete canvas;
}

void tst_qquickanimatedimage::mirror_notRunning_data()
{
    QTest::addColumn<QUrl>("fileUrl");

    QTest::newRow("paused") << testFileUrl("stickmanpause.qml");
    QTest::newRow("stopped") << testFileUrl("stickmanstopped.qml");
}

void tst_qquickanimatedimage::remote()
{
    QFETCH(QString, fileName);
    QFETCH(bool, paused);

    TestHTTPServer server(14449);
    QVERIFY(server.isValid());
    server.serveDirectory(dataDirectory());

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, QUrl("http://127.0.0.1:14449/" + fileName));
    QTRY_VERIFY(component.isReady());

    QQuickAnimatedImage *anim = qobject_cast<QQuickAnimatedImage *>(component.create());
    QVERIFY(anim);

    QTRY_VERIFY(anim->isPlaying());
    if (paused) {
        QTRY_VERIFY(anim->isPaused());
        QCOMPARE(anim->currentFrame(), 2);
    }
    QVERIFY(anim->status() != QQuickAnimatedImage::Error);

    delete anim;
}

void tst_qquickanimatedimage::sourceSize()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, testFileUrl("stickmanscaled.qml"));
    QQuickAnimatedImage *anim = qobject_cast<QQuickAnimatedImage *>(component.create());
    QVERIFY(anim);
    QCOMPARE(anim->width(),240.0);
    QCOMPARE(anim->height(),180.0);
    QCOMPARE(anim->sourceSize(),QSize(160,120));

    delete anim;
}

void tst_qquickanimatedimage::sourceSizeReadOnly()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, testFileUrl("stickmanerror1.qml"));
    QVERIFY(component.isError());
    QCOMPARE(component.errors().at(0).description(), QString("Invalid property assignment: \"sourceSize\" is a read-only property"));
}

void tst_qquickanimatedimage::remote_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<bool>("paused");

    QTest::newRow("playing") << "stickman.qml" << false;
    QTest::newRow("paused") << "stickmanpause.qml" << true;
}

void tst_qquickanimatedimage::invalidSource()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 2.0\n AnimatedImage { source: \"no-such-file.gif\" }", QUrl::fromLocalFile(""));
    QVERIFY(component.isReady());

    QTest::ignoreMessage(QtWarningMsg, "file::2:2: QML AnimatedImage: Error Reading Animated Image File file:no-such-file.gif");

    QQuickAnimatedImage *anim = qobject_cast<QQuickAnimatedImage *>(component.create());
    QVERIFY(anim);

    QVERIFY(!anim->isPlaying());
    QVERIFY(!anim->isPaused());
    QCOMPARE(anim->currentFrame(), 0);
    QCOMPARE(anim->frameCount(), 0);
    QTRY_VERIFY(anim->status() == 3);
}

void tst_qquickanimatedimage::qtbug_16520()
{
    TestHTTPServer server(14449);
    QVERIFY(server.isValid());
    server.serveDirectory(dataDirectory());

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, testFileUrl("qtbug-16520.qml"));
    QTRY_VERIFY(component.isReady());

    QQuickRectangle *root = qobject_cast<QQuickRectangle *>(component.create());
    QVERIFY(root);
    QQuickAnimatedImage *anim = root->findChild<QQuickAnimatedImage*>("anim");

    anim->setProperty("source", "http://127.0.0.1:14449/stickman.gif");

    QTRY_VERIFY(anim->opacity() == 0);
    QTRY_VERIFY(anim->opacity() == 1);

    delete anim;
}

void tst_qquickanimatedimage::progressAndStatusChanges()
{
    TestHTTPServer server(14449);
    QVERIFY(server.isValid());
    server.serveDirectory(dataDirectory());

    QDeclarativeEngine engine;
    QString componentStr = "import QtQuick 2.0\nAnimatedImage { source: srcImage }";
    QDeclarativeContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("srcImage", testFileUrl("stickman.gif"));
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
    QVERIFY(obj != 0);
    QVERIFY(obj->status() == QQuickImage::Ready);
    QTRY_VERIFY(obj->progress() == 1.0);

    qRegisterMetaType<QQuickImageBase::Status>();
    QSignalSpy sourceSpy(obj, SIGNAL(sourceChanged(const QUrl &)));
    QSignalSpy progressSpy(obj, SIGNAL(progressChanged(qreal)));
    QSignalSpy statusSpy(obj, SIGNAL(statusChanged(QQuickImageBase::Status)));

    // Loading local file
    ctxt->setContextProperty("srcImage", testFileUrl("colors.gif"));
    QTRY_VERIFY(obj->status() == QQuickImage::Ready);
    QTRY_VERIFY(obj->progress() == 1.0);
    QTRY_COMPARE(sourceSpy.count(), 1);
    QTRY_COMPARE(progressSpy.count(), 0);
    QTRY_COMPARE(statusSpy.count(), 0);

    // Loading remote file
    ctxt->setContextProperty("srcImage", "http://127.0.0.1:14449/stickman.gif");
    QTRY_VERIFY(obj->status() == QQuickImage::Loading);
    QTRY_VERIFY(obj->progress() == 0.0);
    QTRY_VERIFY(obj->status() == QQuickImage::Ready);
    QTRY_VERIFY(obj->progress() == 1.0);
    QTRY_COMPARE(sourceSpy.count(), 2);
    QTRY_VERIFY(progressSpy.count() > 1);
    QTRY_COMPARE(statusSpy.count(), 2);

    ctxt->setContextProperty("srcImage", "");
    QTRY_VERIFY(obj->status() == QQuickImage::Null);
    QTRY_VERIFY(obj->progress() == 0.0);
    QTRY_COMPARE(sourceSpy.count(), 3);
    QTRY_VERIFY(progressSpy.count() > 2);
    QTRY_COMPARE(statusSpy.count(), 3);
}

QTEST_MAIN(tst_qquickanimatedimage)

#include "tst_qquickanimatedimage.moc"
