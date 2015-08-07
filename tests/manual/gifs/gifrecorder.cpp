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

#include <QLoggingCategory>
#include <QtTest>

/*!
    QProcess wrapper around byzanz-record (sudo apt-get install byzanz).

    \note The following programs must be installed if \c setHighQuality(true)
    is called:

    \li \e avconv (sudo apt-get install libav-tools)
    \li \e convert (sudo apt-get install imagemagick)
    \li \e gifsicle (sudo apt-get install gifsicle)

    It is recommended to set the \c Qt::FramelessWindowHint flag on the view
    (this code has not been tested under other usage):

    view.setFlags(view.flags() | Qt::FramelessWindowHint);
*/

Q_LOGGING_CATEGORY(lcGifRecorder, "qt.gifrecorder")

GifRecorder::GifRecorder() :
    QObject(Q_NULLPTR),
    mView(Q_NULLPTR),
    mHighQuality(false),
    mRecordingDuration(0),
    mRecordCursor(false),
    mByzanzProcessName(QStringLiteral("byzanz-record")),
    mByzanzProcessFinished(false),
    mAvconvProcessName(QStringLiteral("avconv")),
    mAvconvProcessFinished(false),
    mConvertProcessName(QStringLiteral("convert")),
    mConvertProcessFinished(false)
{
    if (lcGifRecorder().isDebugEnabled()) {
        // Ensures output from the process goes directly into the console.
        mByzanzProcess.setProcessChannelMode(QProcess::ForwardedChannels);
    }

    connect(&mByzanzProcess, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(onByzanzError()));
    connect(&mByzanzProcess, SIGNAL(finished(int)), this, SLOT(onByzanzFinished()));
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

void GifRecorder::setOutputFileBaseName(const QString &fileBaseName)
{
    mOutputFileBaseName = fileBaseName;
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

/*!
    If \a highQuality is \c true, records as .flv (lossless) and then converts
    to .gif in order to retain more color information, at the expense of a
    larger file size. Otherwise, records directly to .gif using a limited
    amount of colors, resulting in a smaller file size.

    Set this to \c true if any of the items have transparency, for example.

    The default value is \c false.
*/
void GifRecorder::setHighQuality(bool highQuality)
{
    mHighQuality = highQuality;
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

    if (mOutputFileBaseName.isEmpty()) {
        mOutputFileBaseName = mOutputDir.absoluteFilePath(mQmlInputFileName);
        mOutputFileBaseName.replace(".qml", "");
    }

    mByzanzOutputFileName = mOutputDir.absoluteFilePath(mOutputFileBaseName);
    if (mHighQuality) {
        mByzanzOutputFileName.append(QLatin1String(".flv"));
        mGifFileName = mByzanzOutputFileName;
        mGifFileName.replace(QLatin1String(".flv"), QLatin1String(".gif"));
    } else {
        mByzanzOutputFileName.append(QLatin1String(".gif"));
    }

    QStringList args;
    args << "-d" << QString::number(mRecordingDuration) << "-v";
    if (mRecordCursor)
        args << "-c";
    args << "-x" << QString::number(mView->x()) << "-y" << QString::number(mView->y());
    args << "-w" << QString::number(mView->width()) << "-h" << QString::number(mView->height());
    args << mByzanzOutputFileName;
    qCDebug(lcGifRecorder) << "Starting" << mByzanzProcessName << "with the following arguments:" << args;
    mByzanzProcess.start(mByzanzProcessName, args);

    if (!mByzanzProcess.waitForStarted(1000)) {
        QString message = QString::fromLatin1("Could not launch %1 with the following arguments: %2\nError:\n%3");
        message = message.arg(mByzanzProcessName).arg(args.join(QLatin1Char(' '))).arg(mByzanzProcess.errorString());
        QFAIL(qPrintable(message));
    }
}

void GifRecorder::waitForFinish()
{
    // Give it an extra couple of seconds on top of its recording duration.
    const qreal waitDuration = (mRecordingDuration + 2) * 1000;
    QTRY_VERIFY_WITH_TIMEOUT(mByzanzProcessFinished, waitDuration);

    if (!QFileInfo::exists(mByzanzOutputFileName)) {
        const QString message = QString::fromLatin1(
            "The process said it finished successfully, but %1 was not generated.").arg(mByzanzOutputFileName);
        QFAIL(qPrintable(message));
    }

    if (mHighQuality) {
        mAvconvProcess.setStandardOutputProcess(&mConvertProcess);

        mAvconvArgs = QString::fromLatin1("%1 -i %2 -r 20 -f image2pipe -vcodec ppm -").arg(mAvconvProcessName).arg(mByzanzOutputFileName);
        qCDebug(lcGifRecorder) << "Starting" << mAvconvProcessName << "with the following arguments:" << mAvconvArgs;
        mAvconvProcess.start(mAvconvArgs);
        if (!mAvconvProcess.waitForStarted(1000)) {
            QString message = QString::fromLatin1("Could not launch %1 with the following arguments: %2\nError:\n%3");
            message = message.arg(mAvconvProcessName).arg(mAvconvArgs).arg(mAvconvProcess.errorString());
            QFAIL(qPrintable(message));
        } else {
            qCDebug(lcGifRecorder) << "Successfully started" << mAvconvProcessName;
        }

        mConvertArgs = QString::fromLatin1("%1 -delay 5 -loop 0 - %2").arg(mConvertProcessName).arg(mGifFileName);
        qCDebug(lcGifRecorder) << "Starting" << mConvertProcessName << "with the following arguments:" << mConvertArgs;
        mConvertProcess.start(mConvertArgs);
        if (!mConvertProcess.waitForStarted(1000)) {
            QString message = QString::fromLatin1("Could not launch %1 with the following arguments: %2\nError:\n%3");
            message = message.arg(mConvertProcessName).arg(mConvertArgs).arg(mConvertProcess.errorString());
            QFAIL(qPrintable(message));
        } else {
            qCDebug(lcGifRecorder) << "Successfully started" << mConvertProcessName;
        }

        if (!mAvconvProcess.waitForFinished(waitDuration)) {
            const QString message = QString::fromLatin1("%1 failed to finish: %2");
            QFAIL(qPrintable(message.arg(mAvconvProcessName).arg(mAvconvProcess.errorString())));
        } else {
            qCDebug(lcGifRecorder) << mAvconvProcessName << "finished";
        }

        if (!mConvertProcess.waitForFinished(waitDuration)) {
            const QString message = QString::fromLatin1("%1 failed to finish: %2");
            QFAIL(qPrintable(message.arg(mConvertProcessName).arg(mConvertProcess.errorString())));
        } else {
            qCDebug(lcGifRecorder) << mConvertProcessName << "finished";
        }

        if (QFile::exists(mByzanzOutputFileName)) {
            QVERIFY(QFile::remove(mByzanzOutputFileName));
        }
    }
}

void GifRecorder::onByzanzError()
{
    const QString message = QString::fromLatin1("%1 failed to finish: %2");
    QFAIL(qPrintable(message.arg(mByzanzProcessName).arg(mByzanzProcess.errorString())));
}

void GifRecorder::onByzanzFinished()
{
    qCDebug(lcGifRecorder) << mByzanzProcessName << "finished";
    mByzanzProcessFinished = true;
}
