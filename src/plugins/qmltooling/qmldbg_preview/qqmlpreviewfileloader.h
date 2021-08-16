/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QML preview debug service.
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

#ifndef QQMLPREVIEWFILELOADER_H
#define QQMLPREVIEWFILELOADER_H

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

#include "qqmlpreviewblacklist.h"

#include <QtCore/qobject.h>
#include <QtCore/qthread.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qvector.h>
#include <QtCore/qurl.h>
#include <QtCore/qpointer.h>
#include <QtCore/qset.h>

QT_BEGIN_NAMESPACE

class QQmlPreviewServiceImpl;
class QQmlPreviewFileLoader : public QObject
{
    Q_OBJECT
public:
    enum Result {
        File,
        Directory,
        Fallback,
        Unknown
    };

    QQmlPreviewFileLoader(QQmlPreviewServiceImpl *service);
    ~QQmlPreviewFileLoader();

    QMutex *loadMutex() { return &m_loadMutex; }
    Result load(const QString &file);

    QByteArray contents();
    QStringList entries();

    void whitelist(const QUrl &url);
    bool isBlacklisted(const QString &file);

signals:
    void request(const QString &file);

private:
    QMutex m_loadMutex;
    QMutex m_contentMutex;
    QWaitCondition m_waitCondition;

    QThread m_thread;
    QPointer<QQmlPreviewServiceImpl> m_service;

    QString m_path;
    QByteArray m_contents;
    QStringList m_entries;
    Result m_result;

    QQmlPreviewBlacklist m_blacklist;
    QHash<QString, QByteArray> m_fileCache;
    QHash<QString, QStringList> m_directoryCache;

    void file(const QString &file, const QByteArray &contents);
    void directory(const QString &file, const QStringList &entries);
    void error(const QString &file);
    void clearCache();
};

QT_END_NAMESPACE

#endif // QQMLPREVIEWFILELOADER_H
