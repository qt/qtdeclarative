/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

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

signals:
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
