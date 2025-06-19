// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QSsl>
#include <private/qqmlsslconfiguration_p.h>

class tst_QQmlSslConfiguration : public QObject
{
    Q_OBJECT

    enum class ConfType : quint8
    {
        DefaultSsl,
        DefaultDtls,
    };
private Q_SLOTS:
    void sslOptionsInSync_data();
    void sslOptionsInSync();
};

static QList<QSsl::SslOption> getSslOptionsFromConfig(const QSslConfiguration &c)
{
    // Keep in sync with QSsl::SslOptions!
    static constexpr
    std::array<QSsl::SslOption, 8> allOptions { QSsl::SslOptionDisableEmptyFragments,
                                                QSsl::SslOptionDisableSessionTickets,
                                                QSsl::SslOptionDisableCompression,
                                                QSsl::SslOptionDisableServerNameIndication,
                                                QSsl::SslOptionDisableLegacyRenegotiation,
                                                QSsl::SslOptionDisableSessionSharing,
                                                QSsl::SslOptionDisableSessionPersistence,
                                                QSsl::SslOptionDisableServerCipherPreference };

    QList<QSsl::SslOption> result;
    for (auto opt : allOptions) {
        if (c.testSslOption(opt))
            result.append(opt);
    }

    // return a sorted list
    std::sort(result.begin(), result.end());
    return result;
}

void tst_QQmlSslConfiguration::sslOptionsInSync_data()
{
    QTest::addColumn<ConfType>("type");

    QTest::newRow("DefaultSslConfiguration") << ConfType::DefaultSsl;
    QTest::newRow("DefaultDtlsConfiguration") << ConfType::DefaultDtls;
}

void tst_QQmlSslConfiguration::sslOptionsInSync()
{
    QFETCH(ConfType, type);
    std::unique_ptr<QQmlSslConfiguration> conf;

    if (type == ConfType::DefaultSsl)
        conf.reset(new QQmlSslDefaultConfiguration);
    else
        conf.reset(new QQmlSslDefaultDtlsConfiguration);

    // Default config has some options, and originally they're in sync
    auto opts = conf->sslOptions();
    std::sort(opts.begin(), opts.end());
    QCOMPARE_EQ(opts, getSslOptionsFromConfig(conf->configuration()));

    // set new options
    QList<QSsl::SslOption> newOptions { QSsl::SslOptionDisableCompression,
                                        QSsl::SslOptionDisableServerCipherPreference };
    conf->setSslOptions(newOptions);

    std::sort(newOptions.begin(), newOptions.end());

    opts = conf->sslOptions();
    std::sort(opts.begin(), opts.end());
    // reading back the correct value
    QCOMPARE_EQ(opts, newOptions);

    // reading the values from the underlying configuration
    QEXPECT_FAIL("", "QTBUG-137900", Continue);
    QCOMPARE_EQ(getSslOptionsFromConfig(conf->configuration()), newOptions);
}

QTEST_MAIN(tst_QQmlSslConfiguration)

#include "tst_qqmlsslconfiguration_cpp.moc"
