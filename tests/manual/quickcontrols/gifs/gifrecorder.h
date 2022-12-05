// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef GIFRECORDER_H
#define GIFRECORDER_H

#include <QObject>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QDir>
#include <QString>
#include <QTimer>

class GifRecorder : public QObject
{
    Q_OBJECT

public:
    GifRecorder();

    void setRecordingDuration(int duration);
    void setRecordCursor(bool recordCursor);
    void setDataDirPath(const QString &path);
    void setOutputDir(const QDir &dir);
    void setOutputFileBaseName(const QString &fileBaseName);
    void setQmlFileName(const QString &fileName);
    void setView(QQuickWindow *mWindow);
    void setHighQuality(bool highQuality);

    QQuickWindow *window() const;

    void start();
    bool hasStarted() const;
    void waitForFinish();

private slots:
    void onByzanzError();
    void onByzanzFinished();

private:
    QString mDataDirPath;
    QDir mOutputDir;
    QString mOutputFileBaseName;
    QString mByzanzOutputFileName;
    QString mGifFileName;
    QString mQmlInputFileName;
    QQmlApplicationEngine mEngine;
    QQuickWindow *mWindow;
    bool mHighQuality;
    int mRecordingDuration;
    bool mRecordCursor;

    QProcess mByzanzProcess;
    bool mByzanzProcessFinished;
    QTimer mEventTimer;
};

#endif // GIFRECORDER_H
