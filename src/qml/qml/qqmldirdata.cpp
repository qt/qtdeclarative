/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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

QV4::CompiledData::Location QQmlQmldirData::importLocation(QQmlTypeLoader::Blob *blob) const
{
    auto it = m_imports.constFind(blob);
    if (it == m_imports.constEnd())
        return QV4::CompiledData::Location();
    return it->import->location;
}

void QQmlQmldirData::setPriority(QQmlTypeLoader::Blob *blob,
                                 QQmlTypeLoader::Blob::PendingImportPtr import, int priority)
{
    m_imports.insert(blob, { import, priority });
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

void QQmlQmldirData::initializeFromCachedUnit(const QQmlPrivate::CachedQmlUnit *)
{
    Q_UNIMPLEMENTED();
}

QT_END_NAMESPACE
