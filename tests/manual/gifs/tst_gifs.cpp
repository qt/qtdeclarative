/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>
#include <QtQuick>

#include "gifrecorder.h"
#include "eventcapturer.h"

//#define GENERATE_EVENT_CODE

class tst_Gifs : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void tumblerWrap();

private:
    QQuickView view;
    QString dataDirPath;
    QDir outputDir;
};

void tst_Gifs::initTestCase()
{
    dataDirPath = QFINDTESTDATA("data");
    QVERIFY(!dataDirPath.isEmpty());
    qInfo() << "data directory:" << dataDirPath;

    outputDir = QDir(QDir::current().filePath("gifs"));
    QVERIFY(outputDir.exists() || QDir::current().mkpath("gifs"));
    qInfo() << "output directory:" << outputDir.absolutePath();

    view.setFlags(view.flags() | Qt::FramelessWindowHint);
}

void tst_Gifs::tumblerWrap()
{
    GifRecorder gifRecorder;
    gifRecorder.setDataDirPath(dataDirPath);
    gifRecorder.setOutputDir(outputDir);
    gifRecorder.setRecordingDuration(4);
    gifRecorder.setQmlFileName("qtquickextras2-tumbler-wrap.qml");
    gifRecorder.setOutputFileName("wrap.gif");
    gifRecorder.setView(&view);

    view.show();

    gifRecorder.start();

    // Left as an example. Usually EventCapturer code would be removed after
    // the GIF has been generated.
    EventCapturer eventCapturer;
#ifdef GENERATE_EVENT_CODE
    eventCapturer.setMoveEventTrimFlags(EventCapturer::TrimAll);
    eventCapturer.startCapturing(&view, 4000);
#else
    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, QPoint(89, 75), 326);
    QTest::mouseMove(&view, QPoint(89, 76), 31);
    QTest::mouseMove(&view, QPoint(89, 80), 10);
    QTest::mouseMove(&view, QPoint(93, 93), 10);
    QTest::mouseMove(&view, QPoint(95, 101), 10);
    QTest::mouseMove(&view, QPoint(97, 109), 11);
    QTest::mouseMove(&view, QPoint(101, 125), 10);
    QTest::mouseMove(&view, QPoint(103, 133), 11);
    QTest::mouseMove(&view, QPoint(103, 141), 11);
    QTest::mouseMove(&view, QPoint(105, 158), 10);
    QTest::mouseMove(&view, QPoint(105, 162), 13);
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, QPoint(105, 162), 0);
    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, QPoint(154, 130), 1098);
    QTest::mouseMove(&view, QPoint(154, 129), 50);
    QTest::mouseMove(&view, QPoint(153, 128), 0);
    QTest::mouseMove(&view, QPoint(153, 125), 16);
    QTest::mouseMove(&view, QPoint(152, 121), 0);
    QTest::mouseMove(&view, QPoint(152, 117), 17);
    QTest::mouseMove(&view, QPoint(151, 113), 0);
    QTest::mouseMove(&view, QPoint(151, 106), 16);
    QTest::mouseMove(&view, QPoint(150, 99), 1);
    QTest::mouseMove(&view, QPoint(148, 93), 16);
    QTest::mouseMove(&view, QPoint(148, 88), 0);
    QTest::mouseMove(&view, QPoint(148, 84), 17);
    QTest::mouseMove(&view, QPoint(147, 81), 0);
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, QPoint(147, 81), 0);
    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, QPoint(147, 74), 550);
    QTest::mouseMove(&view, QPoint(147, 75), 17);
    QTest::mouseMove(&view, QPoint(147, 76), 17);
    QTest::mouseMove(&view, QPoint(147, 80), 0);
    QTest::mouseMove(&view, QPoint(148, 85), 16);
    QTest::mouseMove(&view, QPoint(148, 92), 0);
    QTest::mouseMove(&view, QPoint(148, 103), 17);
    QTest::mouseMove(&view, QPoint(150, 119), 17);
    QTest::mouseMove(&view, QPoint(151, 138), 16);
    QTest::mouseMove(&view, QPoint(151, 145), 1);
    QTest::mouseMove(&view, QPoint(153, 151), 16);
    QTest::mouseMove(&view, QPoint(153, 157), 0);
    QTest::mouseMove(&view, QPoint(153, 163), 17);
    QTest::mouseMove(&view, QPoint(153, 167), 0);
    QTest::mouseMove(&view, QPoint(155, 171), 17);
    QTest::mouseMove(&view, QPoint(155, 175), 0);
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, QPoint(155, 175), 0);
#endif

    gifRecorder.waitForFinish();

    foreach (CapturedEvent event, eventCapturer.capturedEvents())
        qDebug().noquote() << event.cppCommand();

}

QTEST_MAIN(tst_Gifs)

#include "tst_gifs.moc"
