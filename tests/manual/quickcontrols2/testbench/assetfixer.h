// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ASSETFIXER_H
#define ASSETFIXER_H

#include <QObject>
#include <QDateTime>
#include <QFileSystemWatcher>
#include <QQmlParserStatus>
#include <QUrl>

class AssetFixer : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(bool shouldWatch READ shouldWatch WRITE setShouldWatch NOTIFY shouldWatchChanged FINAL)
    Q_PROPERTY(bool shouldFix READ shouldFix WRITE setShouldFix NOTIFY shouldFixChanged FINAL)
    Q_PROPERTY(QString assetDirectory READ assetDirectory WRITE setAssetDirectory NOTIFY assetDirectoryChanged FINAL)
    Q_PROPERTY(QUrl assetDirectoryUrl READ assetDirectoryUrl NOTIFY assetDirectoryChanged FINAL)
    Q_PROPERTY(QDateTime assetDirectoryLastModified READ assetDirectoryLastModified WRITE setAssetDirectoryLastModified
        NOTIFY assetDirectoryLastModifiedChanged FINAL)
    Q_INTERFACES(QQmlParserStatus)

public:
    explicit AssetFixer(QObject *parent = nullptr);

    bool shouldWatch() const;
    void setShouldWatch(bool shouldWatch);

    bool shouldFix() const;
    void setShouldFix(bool shouldFix);

    QString assetDirectory() const;
    void setAssetDirectory(const QString &assetDirectory);

    QUrl assetDirectoryUrl() const;

    QDateTime assetDirectoryLastModified() const;
    void setAssetDirectoryLastModified(const QDateTime &assetDirectoryLastModified);

signals:
    void shouldWatchChanged();
    void shouldFixChanged();
    void assetDirectoryChanged();
    void assetDirectoryLastModifiedChanged();

    void fixSuggested();
    void delayedFixSuggested();
    void reloadSuggested();

    void error(const QString &errorMessage);

public slots:
    void clearImageCache();
    void fixAssets();

protected:
    void componentComplete() override;
    void classBegin() override;

private slots:
    void onAssetsChanged();

private:
    void stopWatching();
    void startWatching();

    bool isAssetDirectoryValid(const QString &assetDirectory);

    bool mComponentComplete;
    bool mFirstWatch;
    bool mShouldWatch;
    bool mShouldFix;
    QString mAssetDirectory;
    QFileSystemWatcher mFileSystemWatcher;
    QDateTime mLastModified;
};

#endif // ASSETFIXER_H
