/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef FILEINFOTHREAD_P_H
#define FILEINFOTHREAD_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#if QT_CONFIG(filesystemwatcher)
#include <QFileSystemWatcher>
#endif
#include <QFileInfo>
#include <QDir>

#include "fileproperty_p.h"
#include "qquickfolderlistmodel.h"

class FileInfoThread : public QThread
{
    Q_OBJECT

Q_SIGNALS:
    void directoryChanged(const QString &directory, const QList<FileProperty> &list) const;
    void directoryUpdated(const QString &directory, const QList<FileProperty> &list, int fromIndex, int toIndex) const;
    void sortFinished(const QList<FileProperty> &list) const;
    void statusChanged(QQuickFolderListModel::Status status) const;

public:
    FileInfoThread(QObject *parent = 0);
    ~FileInfoThread();

    void clear();
    void removePath(const QString &path);
    void setPath(const QString &path);
    void setRootPath(const QString &path);
    void setSortFlags(QDir::SortFlags flags);
    void setNameFilters(const QStringList & nameFilters);
    void setShowFiles(bool show);
    void setShowDirs(bool showFolders);
    void setShowDirsFirst(bool show);
    void setShowDotAndDotDot(bool on);
    void setShowHidden(bool on);
    void setShowOnlyReadable(bool on);
    void setCaseSensitive(bool on);

public Q_SLOTS:
#if QT_CONFIG(filesystemwatcher)
    void dirChanged(const QString &directoryPath);
    void updateFile(const QString &path);
#endif

protected:
    void run() override;
    void runOnce();
    void initiateScan();
    void getFileInfos(const QString &path);
    void findChangeRange(const QList<FileProperty> &list, int &fromIndex, int &toIndex);

private:
    QMutex mutex;
    QWaitCondition condition;
    volatile bool abort;
    bool scanPending;

#if QT_CONFIG(filesystemwatcher)
    QFileSystemWatcher *watcher;
#endif
    QList<FileProperty> currentFileList;
    QDir::SortFlags sortFlags;
    QString currentPath;
    QString rootPath;
    QStringList nameFilters;
    bool needUpdate;
    bool folderUpdate;
    bool sortUpdate;
    bool showFiles;
    bool showDirs;
    bool showDirsFirst;
    bool showDotAndDotDot;
    bool showHidden;
    bool showOnlyReadable;
    bool caseSensitive;
};

#endif // FILEINFOTHREAD_P_H
