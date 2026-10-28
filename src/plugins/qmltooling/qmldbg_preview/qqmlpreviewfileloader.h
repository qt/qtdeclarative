// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

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

#include <QtCore/qfiledevice.h>
#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qset.h>
#include <QtCore/qthread.h>
#include <QtCore/qurl.h>
#include <QtCore/qvector.h>
#include <QtCore/qwaitcondition.h>

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

    QMutex *loadMutex() { return &m_loadMutex; }

    // If we don't know the permissions we assume read access for everyone.
    static constexpr QFileDevice::Permissions DefaultPermissions
            = QFileDevice::ReadOwner | QFileDevice::ReadUser
            | QFileDevice::ReadGroup | QFileDevice::ReadOther;

    class Data
    {
        Q_DISABLE_COPY(Data)
    public:
        Data() noexcept {}
        explicit Data(Result result, QFileDevice::Permissions permissions = DefaultPermissions)
            : m_result(result), m_permissions(permissions)
        {
            Q_ASSERT(result != File && result != Directory);
        }

        Data(const QByteArray &contents, QFileDevice::Permissions permissions)
            : m_result(File), m_permissions(permissions)
        {
            new (&m_contents) QByteArray(contents);
        }

        Data(const QStringList &entries, QFileDevice::Permissions permissions)
            : m_result(Directory), m_permissions(permissions)
        {
            new (&m_entries) QStringList(entries);
        }

        Data(Data &&other) noexcept : m_result(other.m_result), m_permissions(other.m_permissions)
        {
            switch (m_result) {
            case File:
                new (&m_contents) QByteArray(std::move(other.m_contents));
                break;
            case Directory:
                new (&m_entries) QStringList(std::move(other.m_entries));
                break;
            default:
                break;
            }
        }

        Data &operator=(Data &&other) noexcept
        {
            if (&other == this)
                return *this;

            destroy();
            m_result = std::exchange(other.m_result, Unknown);
            m_permissions = std::exchange(other.m_permissions, DefaultPermissions);
            switch (m_result) {
            case File:
                new (&m_contents) QByteArray(std::move(other.m_contents));
                other.m_contents.~QByteArray();
                break;
            case Directory:
                new (&m_entries) QStringList(std::move(other.m_entries));
                other.m_entries.~QStringList();
                break;
            default:
                break;
            }

            return *this;
        }

        ~Data() { destroy(); }

        Result result() const { return m_result; }
        QFileDevice::Permissions permissions() const { return m_permissions; }

        const QByteArray &contents() const
        {
            Q_ASSERT(m_result == File);
            return m_contents;
        }

        const QStringList &entries() const
        {
            Q_ASSERT(m_result == Directory);
            return m_entries;
        }

    private:
        void destroy() noexcept
        {
            switch (m_result) {
            case File:
                m_contents.~QByteArray();
                break;
            case Directory:
                m_entries.~QStringList();
                break;
            default:
                break;
            }
            m_result = Unknown;
        }

        Result m_result = Unknown;
        QFileDevice::Permissions m_permissions = DefaultPermissions;
        union {
            QByteArray m_contents;
            QStringList m_entries;
        };
    };

    Data load(const QString &file);

    void whitelist(const QUrl &url);
    bool isBlacklisted(const QString &file);

Q_SIGNALS:
    void request(const QString &file);

private:
    struct CachedFile {
        QByteArray contents;
        QFileDevice::Permissions permissions;
    };
    struct CachedDirectory {
        QStringList entries;
        QFileDevice::Permissions permissions;
    };

    QMutex m_loadMutex;
    QMutex m_contentMutex;
    QWaitCondition m_waitCondition;

    QString m_path;
    Data m_requestedData;

    QQmlPreviewBlacklist m_blacklist;

    QHash<QString, CachedFile> m_fileCache;
    QHash<QString, CachedDirectory> m_directoryCache;

    void file(const QString &file, const QByteArray &contents,
              QFileDevice::Permissions permissions);
    void directory(const QString &file, const QStringList &entries,
                   QFileDevice::Permissions permissions);
    void error(const QString &file);
    void clearCache();
};

QT_END_NAMESPACE

#endif // QQMLPREVIEWFILELOADER_H
