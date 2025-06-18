// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtCore/qfile.h>
#include <QtNetwork/qsslcipher.h>
#include "qqmlsslconfiguration_p.h"
#include <array>

QT_BEGIN_NAMESPACE
static constexpr std::array<QSsl::SslOption, 8> SslOptions = {
    QSsl::SslOptionDisableEmptyFragments,
    QSsl::SslOptionDisableSessionTickets,
    QSsl::SslOptionDisableCompression,
    QSsl::SslOptionDisableServerNameIndication,
    QSsl::SslOptionDisableLegacyRenegotiation,
    QSsl::SslOptionDisableSessionSharing,
    QSsl::SslOptionDisableSessionPersistence,
    QSsl::SslOptionDisableServerCipherPreference
};

QString QQmlSslConfiguration::ciphers() const
{
    return m_ciphers;
}

QList<QSsl::SslOption> QQmlSslConfiguration::sslOptions() const
{
    return m_sslOptions;
}

QSsl::SslProtocol QQmlSslConfiguration::protocol() const
{
    return m_configuration.protocol();
}

QSslSocket::PeerVerifyMode QQmlSslConfiguration::peerVerifyMode() const
{
    return m_configuration.peerVerifyMode();
}

int QQmlSslConfiguration::peerVerifyDepth() const
{
    return m_configuration.peerVerifyDepth();
}

QByteArray QQmlSslConfiguration::sessionTicket() const
{
    return m_configuration.sessionTicket();
}

QSslConfiguration const QQmlSslConfiguration::configuration()
{
    return m_configuration;
}

void QQmlSslConfiguration::setCertificateFiles(const QStringList &certificateFiles)
{
    if (m_certificateFiles == certificateFiles)
        return;

    m_certificateFiles = certificateFiles;
    QList<QSslCertificate> certificates;
    for (const QString &fileName: m_certificateFiles) {
        QFile certificateFile(fileName);
        if (certificateFile.open(QIODevice::ReadOnly)) {
            QByteArray cert = certificateFile.readAll();
            certificates.append(QSslCertificate(cert));
        } else {
            qWarning() << "File: " << fileName << "is not found. It will be skipped.";
        }
    }

    if (!certificates.isEmpty())
        m_configuration.setCaCertificates(certificates);
    else
        qWarning() << "No certificates loaded.";
}

void QQmlSslConfiguration::setProtocol(QSsl::SslProtocol protocol)
{
    if (m_configuration.protocol() == protocol)
        return;

    m_configuration.setProtocol(protocol);
}

void QQmlSslConfiguration::setPeerVerifyMode(QSslSocket::PeerVerifyMode mode)
{
    if (m_configuration.peerVerifyMode() == mode)
        return;

    m_configuration.setPeerVerifyMode(mode);
}

void QQmlSslConfiguration::setPeerVerifyDepth(int depth)
{
    if (m_configuration.peerVerifyDepth() == depth)
        return;

    m_configuration.setPeerVerifyDepth(depth);
}

void QQmlSslConfiguration::setCiphers(const QString &ciphers)
{
    if (ciphers == m_ciphers)
        return;

    m_ciphers = ciphers;
    m_configuration.setCiphers(ciphers); // split(":") is used inside
}

void QQmlSslConfiguration::setSslOptions(const QList<QSsl::SslOption> &options)
{
    if (m_sslOptions == options)
        return;

    m_sslOptions = options;
    for (QSsl::SslOption option: m_sslOptions)
        m_configuration.setSslOption(option, true);
}

void QQmlSslConfiguration::setSessionTicket(const QByteArray &sessionTicket)
{
    if (m_configuration.sessionTicket() == sessionTicket)
        return;

    m_configuration.setSessionTicket(sessionTicket);
}

void QQmlSslConfiguration::setPrivateKey(const QQmlSslKey &privateKey)
{
    m_configuration.setPrivateKey(privateKey.getSslKey());
}

void QQmlSslConfiguration::setSslOptionsList(const QSslConfiguration &configuration)
{
    for (QSsl::SslOption option: SslOptions) {
        if (configuration.testSslOption(option))
            m_sslOptions.append(option);
    }
}

void QQmlSslConfiguration::setCiphersList(const QSslConfiguration &configuration)
{
    QList<QSslCipher> ciphers = configuration.ciphers();
    for (int i = 0; i < ciphers.size(); ++i) {
        if (i != 0) {
            m_ciphers += QString::fromUtf8(":");
        }
        m_ciphers += ciphers[i].name();
    }
}

QQmlSslDefaultConfiguration::QQmlSslDefaultConfiguration()
    : QQmlSslConfiguration()
{
    m_configuration = QSslConfiguration::defaultConfiguration();
    setSslOptionsList(m_configuration);
    setCiphersList(m_configuration);
}

QQmlSslDefaultDtlsConfiguration::QQmlSslDefaultDtlsConfiguration()
    : QQmlSslConfiguration()
{
#if QT_CONFIG(dtls)
    m_configuration = QSslConfiguration::defaultDtlsConfiguration();
#else
    qWarning() << "No dtls support enabled";
    m_configuration = QSslConfiguration::defaultConfiguration();
#endif // QT_CONFIG(dtls)
    setSslOptionsList(m_configuration);
    setCiphersList(m_configuration);
}

QT_END_NAMESPACE
