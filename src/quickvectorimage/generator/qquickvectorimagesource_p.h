// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKVECTORIMAGESOURCE_P_H
#define QQUICKVECTORIMAGESOURCE_P_H

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

#include <QtQuickVectorImageGenerator/qtquickvectorimagegeneratorexports.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlfile.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QDebug;
class QIODevice;

class Q_QUICKVECTORIMAGEGENERATOR_EXPORT QQuickVectorImageSource
{
public:
    QQuickVectorImageSource() = default;
    explicit QQuickVectorImageSource(const QString &fileName,
                                     const QByteArray &data = QByteArray{});
    QQuickVectorImageSource(const QQuickVectorImageSource &) = default;
    ~QQuickVectorImageSource();
    QQuickVectorImageSource &operator=(const QQuickVectorImageSource &) = default;

    QString fileName() const
    {
        return m_fileName;
    }

    void resolveLocalFileName(QQmlContext *ctx)
    {
        if (!m_source.isEmpty()) {
            QUrl resolvedUrl = ctx->resolvedUrl(m_source);
            m_fileName = QQmlFile::urlToLocalFileOrQrc(resolvedUrl);
        } else {
            m_fileName.clear();
        }
    }

    QUrl source() const
    {
        return m_source;
    }

    void setSource(const QUrl &source)
    {
        m_source = source;
    }

    QByteArray data() const
    {
        return m_data;
    }

    void setData(const QByteArray &data)
    {
        m_data = data;
    }

    bool isEmpty() const
    {
        return m_fileName.isEmpty() && m_data.isEmpty();
    }

    bool isFile() const
    {
        return m_data.isEmpty();
    }

private:
    QUrl m_source;
    QString m_fileName;
    QByteArray m_data;
};

#ifndef QT_NO_DEBUG_STREAM
Q_QUICKVECTORIMAGEGENERATOR_EXPORT QDebug operator<<(QDebug debug,
                                                     const QQuickVectorImageSource &source);
#endif

QT_END_NAMESPACE

#endif // QQUICKVECTORIMAGESOURCE_P_H
