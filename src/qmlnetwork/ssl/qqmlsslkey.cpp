// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtCore/qfile.h>
#include "qqmlsslkey_p.h"

QT_BEGIN_NAMESPACE

QSslKey QQmlSslKey::getSslKey() const
{
    if (m_keyFile.isEmpty()) {
        qWarning() << "SslConfiguration::getSslKey: No key paths set";
        return QSslKey();
    }

    QFile file(m_keyFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "SslConfiguration::getSslKey: Couldn't open file:" << m_keyFile;
        return QSslKey();
    }

    return QSslKey(file.readAll(),
                   m_keyAlgorithm,
                   m_keyFormat,
                   m_keyType,
                   m_keyPassPhrase);
}

void QQmlSslKey::setKeyFile(const QString &key)
{
    if (m_keyFile == key)
        return;

    m_keyFile = key;
}

void QQmlSslKey::setKeyAlgorithm(QSsl::KeyAlgorithm value)
{
    if (m_keyAlgorithm == value)
        return;

    m_keyAlgorithm = value;
}

void QQmlSslKey::setKeyFormat(QSsl::EncodingFormat value)
{
    if (m_keyFormat == value)
        return;

    m_keyFormat = value;
}

void QQmlSslKey::setKeyPassPhrase(const QByteArray &value)
{
    if (m_keyPassPhrase == value)
        return;

    m_keyPassPhrase = value;
}

void QQmlSslKey::setKeyType(QSsl::KeyType type)
{
    if (m_keyType == type)
        return;
    m_keyType = type;
}

QT_END_NAMESPACE
