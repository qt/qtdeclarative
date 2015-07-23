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

#include "gifrecorder.h"

#include <QtTest>

/*!
    QProcess wrapper around byzanz-record (sudo apt-get install byzanz).

    It is recommended to set the \c Qt::FramelessWindowHint flag on the view
    (this code has not been tested under other usage):

    view.setFlags(view.flags() | Qt::FramelessWindowHint);
*/

GifRecorder::GifRecorder() :
    QObject(Q_NULLPTR),
    mProcessName(QStringLiteral("byzanz-record")),
    mRecordingDuration(0),
    mRecordCursor(false),
    mView(Q_NULLPTR),
    mStarted(false)
{
    // Ensures output from the process goes directly into the console.
    mProcess.setProcessChannelMode(QProcess::ForwardedChannels);

    connect(&mProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onError()));
    connect(&mProcess, SIGNAL(finished(int)), this, SLOT(onFinished()));
}

void GifRecorder::setRecordingDuration(int duration)
{
    QVERIFY2(duration >= 1, qPrintable(QString::fromLatin1("Recording duration %1 must be larger than 1 second").arg(duration)));
    QVERIFY2(duration < 10, qPrintable(QString::fromLatin1("Recording duration %1 must be less than 10 seconds").arg(duration)));

    mRecordingDuration = duration;
}

void GifRecorder::setRecordCursor(bool recordCursor)
{
    mRecordCursor = recordCursor;
}

void GifRecorder::setDataDirPath(const QString &path)
{
    QVERIFY2(!path.isEmpty(), "Data directory path cannot be empty");
    mDataDirPath = path;
}

void GifRecorder::setOutputDir(const QDir &dir)
{
    QVERIFY2(dir.exists(), "Output directory must exist");
    mOutputDir = dir;
}

void GifRecorder::setOutputFileName(const QString &fileName)
{
    QVERIFY2(!fileName.isEmpty(), "Output file name cannot be empty");
    mOutputFileName = fileName;
}

void GifRecorder::setQmlFileName(const QString &fileName)
{
    QVERIFY2(!fileName.isEmpty(), "QML file name cannot be empty");
    mQmlInputFileName = fileName;
}

void GifRecorder::setView(QQuickView *view)
{
    this->mView = view;
}

void GifRecorder::start()
{
    QVERIFY2(mView, "Must have a view to record");

    QDir gifQmlDir(mDataDirPath);
    QVERIFY(gifQmlDir.entryList().contains(mQmlInputFileName));

    const QString qmlPath = gifQmlDir.absoluteFilePath(mQmlInputFileName);
    mView->setSource(QUrl::fromLocalFile(qmlPath));
    QVERIFY(mView->rootObject());

    mView->show();
    mView->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(mView, 500));

    mOutputFileName = mOutputDir.absoluteFilePath(mQmlInputFileName);
    mOutputFileName.replace(".qml", ".gif");

    QStringList args;
    args << "-d" << QString::number(mRecordingDuration) << "-v";
    if (mRecordCursor)
        args << "-c";
    args << "-x" << QString::number(mView->x()) << "-y" << QString::number(mView->y());
    args << "-w" << QString::number(mView->width()) << "-h" << QString::number(mView->height());
    args << mOutputFileName;
    qInfo() << "Starting" << mProcessName << "with the following arguments:" << args;
    mProcess.start(mProcessName, args);

    if (!mProcess.waitForStarted(1000)) {
        QString message = QString::fromLatin1("Could not launch %1 with the following arguments: %2\nError:\n%3");
        message = message.arg(mProcessName).arg(args.join(QLatin1Char(' '))).arg(mProcess.errorString());
        QFAIL(qPrintable(message));
    }

    mStarted = true;
}

bool GifRecorder::hasStarted() const
{
    return mStarted;
}

void GifRecorder::waitForFinish()
{
    // Give it an extra couple of seconds on top of its recording duration.
    QTRY_VERIFY_WITH_TIMEOUT(mFinished, (mRecordingDuration + 2) * 1000);

    if (!QFileInfo::exists(mOutputFileName)) {
        const QString message = QString::fromLatin1(
            "The process said it finished successfully, but no GIF was generated.");
        QFAIL(qPrintable(message));
    }
}

void GifRecorder::onError()
{
    const QString message = QString::fromLatin1("%1 failed to finish: %2");
    QFAIL(qPrintable(message.arg(mProcessName).arg(mProcess.errorString())));
}

void GifRecorder::onFinished()
{
    mFinished = true;
}
