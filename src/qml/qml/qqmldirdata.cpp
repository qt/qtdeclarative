// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    m_imports.insert(blob, { std::move(import), priority });
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
