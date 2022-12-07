// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLPREVIEWFILESYSTEMWATCHER_H
#define QMLPREVIEWFILESYSTEMWATCHER_H

#include <QtCore/qfilesystemwatcher.h>
#include <QtCore/qobject.h>
#include <QtCore/qset.h>

class QmlPreviewFileSystemWatcher : public QObject
{
    Q_OBJECT

public:
    explicit QmlPreviewFileSystemWatcher(QObject *parent = nullptr);

    void addFile(const QString &file);
    void removeFile(const QString &file);
    bool watchesFile(const QString &file) const;

    void addDirectory(const QString &file);
    void removeDirectory(const QString &file);
    bool watchesDirectory(const QString &file) const;

Q_SIGNALS:
    void fileChanged(const QString &path);
    void directoryChanged(const QString &path);

private:
    using WatchEntrySet = QSet<QString>;
    using WatchEntrySetIterator = WatchEntrySet::iterator;

    void onDirectoryChanged(const QString &path);

    WatchEntrySet m_files;
    WatchEntrySet m_directories;

    // Directories watched either explicitly or implicitly through files contained in them.
    QHash<QString, int> m_directoryCount;

    QFileSystemWatcher *m_watcher = nullptr;
};

#endif // QMLPREVIEWFILESYSTEMWATCHER_H
