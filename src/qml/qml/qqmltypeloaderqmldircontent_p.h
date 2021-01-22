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

#ifndef QQMLTYPELOADERQMLDIRCONTENT_P_H
#define QQMLTYPELOADERQMLDIRCONTENT_P_H

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

#include <private/qqmldirparser_p.h>

QT_BEGIN_NAMESPACE

class QQmlError;
class QQmlTypeLoaderQmldirContent
{
private:
    friend class QQmlTypeLoader;

    void setContent(const QString &location, const QString &content);
    void setError(const QQmlError &);

public:
    QQmlTypeLoaderQmldirContent();
    QQmlTypeLoaderQmldirContent(const QQmlTypeLoaderQmldirContent &) = default;
    QQmlTypeLoaderQmldirContent &operator=(const QQmlTypeLoaderQmldirContent &) = default;

    bool hasContent() const { return m_hasContent; }
    bool hasError() const;
    QList<QQmlError> errors(const QString &uri) const;

    QString typeNamespace() const;

    QQmlDirComponents components() const;
    QQmlDirScripts scripts() const;
    QQmlDirPlugins plugins() const;
    QStringList imports() const;

    QString pluginLocation() const;

    bool designerSupported() const;

private:
    QQmlDirParser m_parser;
    QString m_location;
    bool m_hasContent = false;
};

QT_END_NAMESPACE

#endif // QQMLTYPELOADERQMLDIRCONTENT_P_H
