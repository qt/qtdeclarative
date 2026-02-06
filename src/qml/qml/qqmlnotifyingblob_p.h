// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLNOTIFYINGBLOB_P_H
#define QQMLNOTIFYINGBLOB_P_H

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

#include <private/qqmltypeloader_p.h>

QT_BEGIN_NAMESPACE

class Q_QML_EXPORT QQmlNotifyingBlob : public QQmlTypeLoader::Blob
{
public:
    QQmlNotifyingBlob(const QUrl &url, QQmlDataBlob::Type type, QQmlTypeLoader *loader)
        : Blob(url, type, loader)
    {}

    struct Q_QML_EXPORT Callback
    {
        virtual ~Callback();
        virtual void ready(QQmlNotifyingBlob *);
        virtual void progress(QQmlNotifyingBlob *, qreal);
    };

    void registerCallback(Callback *callback);
    void unregisterCallback(Callback *callback);

protected:
    void completed() override;
    void downloadProgressChanged(qreal) override;

private:
    QList<Callback *> m_callbacks;
};

QT_END_NAMESPACE

#endif // QQMLNOTIFYINGBLOB_P_H
