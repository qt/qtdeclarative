// Copyright (C) 2015 Canonical Limited and/or its subsidiary(-ies)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause


#include <qqmlextensionplugin.h>

#include <qqmlengine.h>
#include <qquickimageprovider.h>
#include <QDebug>
#include <QImage>
#include <QThreadPool>

class AsyncImageResponseRunnable : public QObject, public QRunnable
{
    Q_OBJECT

signals:
    void done(QImage image);

public:
    AsyncImageResponseRunnable(const QString &id, const QSize &requestedSize)
        : m_id(id), m_requestedSize(requestedSize) {}

    void run() override
    {
        auto image = QImage(50, 50, QImage::Format_RGB32);
        if (m_id == QLatin1String("slow")) {
            qDebug() << "Slow, red, sleeping for 5 seconds";
            QThread::sleep(5);
            image.fill(Qt::red);
        } else {
            qDebug() << "Fast, blue, sleeping for 1 second";
            QThread::sleep(1);
            image.fill(Qt::blue);
        }
        if (m_requestedSize.isValid())
            image = image.scaled(m_requestedSize);

        emit done(image);
    }

private:
    QString m_id;
    QSize m_requestedSize;
};

class AsyncImageResponse : public QQuickImageResponse
{
    public:
        AsyncImageResponse(const QString &id, const QSize &requestedSize, QThreadPool *pool)
        {
            auto runnable = new AsyncImageResponseRunnable(id, requestedSize);
            connect(runnable, &AsyncImageResponseRunnable::done, this, &AsyncImageResponse::handleDone);
            pool->start(runnable);
        }

        void handleDone(QImage image) {
            m_image = image;
            emit finished();
        }

        QQuickTextureFactory *textureFactory() const override
        {
            return QQuickTextureFactory::textureFactoryForImage(m_image);
        }

        QImage m_image;
};

class AsyncImageProvider : public QQuickAsyncImageProvider
{
public:
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override
    {
        AsyncImageResponse *response = new AsyncImageResponse(id, requestedSize, &pool);
        return response;
    }

private:
    QThreadPool pool;
};


class ImageProviderExtensionPlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)
public:
    void initializeEngine(QQmlEngine *engine, const char *uri) override
    {
        Q_UNUSED(uri);
        engine->addImageProvider("async", new AsyncImageProvider);
    }

};

#include "imageresponseprovider.moc"
