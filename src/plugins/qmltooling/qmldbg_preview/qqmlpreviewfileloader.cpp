// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpreviewfileloader.h"
#include "qqmlpreviewservice.h"

#include <QtQml/qqmlfile.h>

#include <QtCore/qlibraryinfo.h>
#include <QtCore/qstandardpaths.h>

QT_BEGIN_NAMESPACE

QQmlPreviewFileLoader::QQmlPreviewFileLoader(QQmlPreviewServiceImpl *service) : m_service(service)
{
    // Exclude some resource paths used by Qt itself. There is no point in loading those from the
    // client as the client will not have the files (or even worse, it may have different ones).
    m_blacklist.blacklist(":/qt-project.org");
    m_blacklist.blacklist(":/QtQuick/Controls/Styles");
    m_blacklist.blacklist(":/ExtrasImports/QtQuick/Controls/Styles");

    // Target specific configuration should not replaced with files from the host.
    m_blacklist.blacklist("/etc");

    for (int loc = QLibraryInfo::PrefixPath; loc < QLibraryInfo::TestsPath; ++loc) {
        m_blacklist.blacklist(QLibraryInfo::path(
                                  static_cast<QLibraryInfo::LibraryPath>(loc)));
    }
    m_blacklist.blacklist(QLibraryInfo::path(QLibraryInfo::SettingsPath));

    static const QStandardPaths::StandardLocation blackListLocations[] = {
        QStandardPaths::CacheLocation,
        QStandardPaths::GenericDataLocation,
        QStandardPaths::ConfigLocation,
        QStandardPaths::GenericCacheLocation,
        QStandardPaths::GenericConfigLocation,
        QStandardPaths::AppDataLocation,
        QStandardPaths::AppConfigLocation
    };

    for (auto locationType : blackListLocations) {
        const QStringList locations = QStandardPaths::standardLocations(locationType);
        for (const QString &location : locations)
            m_blacklist.blacklist(location);
    }

    m_blacklist.whitelist(QLibraryInfo::path(QLibraryInfo::TestsPath));

    connect(this, &QQmlPreviewFileLoader::request, service, &QQmlPreviewServiceImpl::forwardRequest,
            Qt::DirectConnection);
    connect(service, &QQmlPreviewServiceImpl::directory, this, &QQmlPreviewFileLoader::directory);
    connect(service, &QQmlPreviewServiceImpl::file, this, &QQmlPreviewFileLoader::file);
    connect(service, &QQmlPreviewServiceImpl::error, this, &QQmlPreviewFileLoader::error);
    connect(service, &QQmlPreviewServiceImpl::clearCache, this, &QQmlPreviewFileLoader::clearCache);
    moveToThread(&m_thread);
    m_thread.start();
}

QQmlPreviewFileLoader::~QQmlPreviewFileLoader() {
    m_thread.quit();
    m_thread.wait();
}

QQmlPreviewFileLoader::Result QQmlPreviewFileLoader::load(const QString &path)
{
    QMutexLocker locker(&m_contentMutex);
    m_path = path;

    auto fileIterator = m_fileCache.constFind(path);
    if (fileIterator != m_fileCache.constEnd()) {
        m_result = File;
        m_contents = *fileIterator;
        m_entries.clear();
        return m_result;
    }

    auto dirIterator = m_directoryCache.constFind(path);
    if (dirIterator != m_directoryCache.constEnd()) {
        m_result = Directory;
        m_contents.clear();
        m_entries = *dirIterator;
        return m_result;
    }

    m_result = Unknown;
    m_entries.clear();
    m_contents.clear();
    emit request(path);
    m_waitCondition.wait(&m_contentMutex);
    return m_result;
}

QByteArray QQmlPreviewFileLoader::contents()
{
    QMutexLocker locker(&m_contentMutex);
    return m_contents;
}

QStringList QQmlPreviewFileLoader::entries()
{
    QMutexLocker locker(&m_contentMutex);
    return m_entries;
}

void QQmlPreviewFileLoader::whitelist(const QUrl &url)
{
    const QString path = QQmlFile::urlToLocalFileOrQrc(url);
    if (!path.isEmpty()) {
        QMutexLocker locker(&m_contentMutex);
        m_blacklist.whitelist(path);
    }
}

bool QQmlPreviewFileLoader::isBlacklisted(const QString &path)
{
    QMutexLocker locker(&m_contentMutex);
    return m_blacklist.isBlacklisted(path);
}

void QQmlPreviewFileLoader::file(const QString &path, const QByteArray &contents)
{
    QMutexLocker locker(&m_contentMutex);
    m_blacklist.whitelist(path);
    m_fileCache[path] = contents;
    if (path == m_path) {
        m_contents = contents;
        m_result = File;
        m_waitCondition.wakeOne();
    }
}

void QQmlPreviewFileLoader::directory(const QString &path, const QStringList &entries)
{
    QMutexLocker locker(&m_contentMutex);
    m_blacklist.whitelist(path);
    m_directoryCache[path] = entries;
    if (path == m_path) {
        m_entries = entries;
        m_result = Directory;
        m_waitCondition.wakeOne();
    }
}

void QQmlPreviewFileLoader::error(const QString &path)
{
    QMutexLocker locker(&m_contentMutex);
    m_blacklist.blacklist(path);
    if (path == m_path) {
        m_result = Fallback;
        m_waitCondition.wakeOne();
    }
}

void QQmlPreviewFileLoader::clearCache()
{
    QMutexLocker locker(&m_contentMutex);
    m_fileCache.clear();
    m_directoryCache.clear();
}

QT_END_NAMESPACE

#include "moc_qqmlpreviewfileloader.cpp"
