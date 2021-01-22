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
