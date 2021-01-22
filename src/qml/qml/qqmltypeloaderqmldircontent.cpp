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

#include <private/qqmltypeloaderqmldircontent_p.h>
#include <private/qqmlsourcecoordinate_p.h>
#include <QtQml/qqmlerror.h>

QT_BEGIN_NAMESPACE

QQmlTypeLoaderQmldirContent::QQmlTypeLoaderQmldirContent()
{
}

bool QQmlTypeLoaderQmldirContent::hasError() const
{
    return m_parser.hasError();
}

QList<QQmlError> QQmlTypeLoaderQmldirContent::errors(const QString &uri) const
{
    QList<QQmlError> errors;
    const QUrl url(uri);
    const auto parseErrors = m_parser.errors(uri);
    for (const auto &parseError : parseErrors) {
        QQmlError error;
        error.setUrl(url);
        error.setLine(qmlConvertSourceCoordinate<quint32, int>(parseError.loc.startLine));
        error.setColumn(qmlConvertSourceCoordinate<quint32, int>(parseError.loc.startColumn));
        error.setDescription(parseError.message);
        errors.append(error);
    }
    return errors;
}

QString QQmlTypeLoaderQmldirContent::typeNamespace() const
{
    return m_parser.typeNamespace();
}

void QQmlTypeLoaderQmldirContent::setContent(const QString &location, const QString &content)
{
    Q_ASSERT(!m_hasContent);
    m_hasContent = true;
    m_location = location;
    m_parser.parse(content);
}

void QQmlTypeLoaderQmldirContent::setError(const QQmlError &error)
{
    QQmlJS::DiagnosticMessage parseError;
    parseError.loc.startLine = error.line();
    parseError.loc.startColumn = error.column();
    parseError.message = error.description();
    m_parser.setError(parseError);
}

QQmlDirComponents QQmlTypeLoaderQmldirContent::components() const
{
    return m_parser.components();
}

QQmlDirScripts QQmlTypeLoaderQmldirContent::scripts() const
{
    return m_parser.scripts();
}

QQmlDirPlugins QQmlTypeLoaderQmldirContent::plugins() const
{
    return m_parser.plugins();
}

QStringList QQmlTypeLoaderQmldirContent::imports() const
{
    return m_parser.imports();
}

QString QQmlTypeLoaderQmldirContent::pluginLocation() const
{
    return m_location;
}

bool QQmlTypeLoaderQmldirContent::designerSupported() const
{
    return m_parser.designerSupported();
}

QT_END_NAMESPACE
