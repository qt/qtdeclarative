// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QNetworkAccessManager>
#include <QNetworkProxy>

#include <QQmlEngine>
#include <QQmlNetworkAccessManagerFactory>
#include <QtQuick/QQuickView>


/*
   This example illustrates using a QQmlNetworkAccessManagerFactory to
   create a QNetworkAccessManager with a proxy.

   Usage:
     networkaccessmanagerfactory [-host <proxy> -port <port>] [file]
*/

#if QT_CONFIG(networkproxy)
static QString proxyHost;
static int proxyPort = 0;
#endif // networkproxy

class MyNetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent) override;
};

QNetworkAccessManager *MyNetworkAccessManagerFactory::create(QObject *parent)
{
    QNetworkAccessManager *nam = new QNetworkAccessManager(parent);
#if QT_CONFIG(networkproxy)
    if (!proxyHost.isEmpty()) {
        qDebug() << "Created QNetworkAccessManager using proxy" << (proxyHost + ":" + QString::number(proxyPort));
        QNetworkProxy proxy(QNetworkProxy::HttpCachingProxy, proxyHost, proxyPort);
        nam->setProxy(proxy);
    }
#endif // networkproxy

    return nam;
}

int main(int argc, char ** argv)
{
    QUrl source("qrc:view.qml");

    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
#if QT_CONFIG(networkproxy)
    QCommandLineOption proxyHostOption("host", "The proxy host to use.", "host");
    parser.addOption(proxyHostOption);
    QCommandLineOption proxyPortOption("port", "The proxy port to use.", "port", "0");
    parser.addOption(proxyPortOption);
#endif // networkproxy
    parser.addPositionalArgument("file", "The file to use.");
    QCommandLineOption helpOption = parser.addHelpOption();
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    QStringList arguments = QCoreApplication::arguments();
    if (!parser.parse(arguments)) {
        qWarning() << parser.helpText() << '\n' << parser.errorText();
        exit(1);
    }
    if (parser.isSet(helpOption)) {
        qWarning() << parser.helpText();
        exit(0);
    }
#if QT_CONFIG(networkproxy)
    if (parser.isSet(proxyHostOption))
        proxyHost = parser.value(proxyHostOption);
    if (parser.isSet(proxyPortOption)) {
        bool ok = true;
        proxyPort = parser.value(proxyPortOption).toInt(&ok);
        if (!ok || proxyPort < 1 || proxyPort > 65535) {
            qWarning() << parser.helpText() << "\nNo valid port given. It should\
                          be a number between 1 and 65535";
            exit(1);
        }
    }
#endif // networkproxy
    if (parser.positionalArguments().count() == 1)
        source = QUrl::fromLocalFile(parser.positionalArguments().first());

    QQuickView view;
    MyNetworkAccessManagerFactory networkManagerFactory;
    view.engine()->setNetworkAccessManagerFactory(&networkManagerFactory);

    view.setSource(source);
    view.show();

    return app.exec();
}

