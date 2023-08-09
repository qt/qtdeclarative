// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qqmltypeloaderqmldircontent_p.h>
#include <private/qqmlsourcecoordinate_p.h>
#include <QtQml/qqmlerror.h>

QT_BEGIN_NAMESPACE

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

void QQmlTypeLoaderQmldirContent::setContent(const QString &location, const QString &content)
{
    Q_ASSERT(!m_hasContent);
    m_hasContent = true;
    m_location = location;
    m_parser.parse(content);
    m_parser.disambiguateFileSelectors();
}

void QQmlTypeLoaderQmldirContent::setError(const QQmlError &error)
{
    QQmlJS::DiagnosticMessage parseError;
    parseError.loc.startLine = error.line();
    parseError.loc.startColumn = error.column();
    parseError.message = error.description();
    m_parser.setError(parseError);
}

QT_END_NAMESPACE
