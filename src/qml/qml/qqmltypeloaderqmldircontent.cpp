/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
