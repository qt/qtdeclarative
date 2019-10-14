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

#include <private/qqmldirdata_p.h>

QT_BEGIN_NAMESPACE

QQmlQmldirData::QQmlQmldirData(const QUrl &url, QQmlTypeLoader *loader)
    : QQmlTypeLoader::Blob(url, QmldirFile, loader)
{
}

const QString &QQmlQmldirData::content() const
{
    return m_content;
}

QQmlTypeLoader::Blob::PendingImportPtr QQmlQmldirData::import(QQmlTypeLoader::Blob *blob) const
{
    auto it = m_imports.find(blob);
    if (it == m_imports.end())
        return nullptr;
    return *it;
}

void QQmlQmldirData::setImport(QQmlTypeLoader::Blob *blob, QQmlTypeLoader::Blob::PendingImportPtr import)
{
    m_imports[blob] = std::move(import);
}

int QQmlQmldirData::priority(QQmlTypeLoader::Blob *blob) const
{
    QHash<QQmlTypeLoader::Blob *, int>::const_iterator it = m_priorities.find(blob);
    if (it == m_priorities.end())
        return 0;
    return *it;
}

void QQmlQmldirData::setPriority(QQmlTypeLoader::Blob *blob, int priority)
{
    m_priorities[blob] = priority;
}

void QQmlQmldirData::dataReceived(const SourceCodeData &data)
{
    QString error;
    m_content = data.readAll(&error);
    if (!error.isEmpty()) {
        setError(error);
        return;
    }
}

void QQmlQmldirData::initializeFromCachedUnit(const QV4::CompiledData::Unit *)
{
    Q_UNIMPLEMENTED();
}

QT_END_NAMESPACE
