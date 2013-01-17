/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef FILEINFOTHREAD_P_H
#define FILEINFOTHREAD_P_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QFileSystemWatcher>
#include <QFileInfo>
#include <QDir>

#include "fileproperty_p.h"

class FileInfoThread : public QThread
{
    Q_OBJECT

Q_SIGNALS:
    void directoryChanged(const QString &directory, const QList<FileProperty> &list) const;
    void directoryUpdated(const QString &directory, const QList<FileProperty> &list, int fromIndex, int toIndex) const;
    void sortFinished(const QList<FileProperty> &list) const;

public:
    FileInfoThread(QObject *parent = 0);
    ~FileInfoThread();

    void clear();
    void removePath(const QString &path);
    void setPath(const QString &path);
    void setRootPath(const QString &path);
    void setSortFlags(QDir::SortFlags flags);
    void setNameFilters(const QStringList & nameFilters);
    void setShowDirs(bool showFolders);
    void setShowDirsFirst(bool show);
    void setShowDotDot(bool on);
    void setShowOnlyReadable(bool on);

public Q_SLOTS:
#ifndef QT_NO_FILESYSTEMWATCHER
    void dirChanged(const QString &directoryPath);
    void updateFile(const QString &path);
#endif

protected:
    void run();
    void getFileInfos(const QString &path);
    void findChangeRange(const QList<FileProperty> &list, int &fromIndex, int &toIndex);

private:
    QMutex mutex;
    QWaitCondition condition;
    volatile bool abort;

#ifndef QT_NO_FILESYSTEMWATCHER
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
    bool showDirs;
    bool showDirsFirst;
    bool showDotDot;
    bool showOnlyReadable;
};

#endif // FILEINFOTHREAD_P_H
