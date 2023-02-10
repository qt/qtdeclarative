// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
class CachingNetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
public:

    inline QNetworkAccessManager *create(QObject *parent) override
    {
        QNetworkAccessManager *networkAccessManager = new QNetworkAccessManager(parent);
        QNetworkDiskCache *diskCache = new QNetworkDiskCache(parent);
        diskCache->setCacheDirectory("requestCache");
        networkAccessManager->setCache(diskCache);

        return networkAccessManager;
    }
};
//! [0]

//! [1]
CachingNetworkAccessManagerFactory networkManagerFactory;
engine->setNetworkAccessManagerFactory(&networkManagerFactory);
//! [1]
