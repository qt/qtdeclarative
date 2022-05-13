// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause


#include <qqmlextensionplugin.h>

#include <qqmlengine.h>
#include <qquickimageprovider.h>
#include <QImage>
#include <QPainter>

class ColorImageProvider : public QQuickImageProvider
{
public:
    ColorImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Pixmap)
    {
    }

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        int width = 100;
        int height = 50;

        if (size)
            *size = QSize(width, height);
        QPixmap pixmap(requestedSize.width() > 0 ? requestedSize.width() : width,
                       requestedSize.height() > 0 ? requestedSize.height() : height);
        pixmap.fill(QColor(id).rgba());

        // write the color name
        QPainter painter(&pixmap);
        QFont f = painter.font();
        f.setPixelSize(20);
        painter.setFont(f);
        painter.setPen(Qt::black);
        if (requestedSize.isValid())
            painter.scale(requestedSize.width() / width, requestedSize.height() / height);
        painter.drawText(QRectF(0, 0, width, height), Qt::AlignCenter, id);

        return pixmap;
    }
};


class ImageProviderExtensionPlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)
public:
    void initializeEngine(QQmlEngine *engine, const char *uri) override
    {
        Q_UNUSED(uri);
        engine->addImageProvider("colors", new ColorImageProvider);
    }
};


#include "imageprovider.moc"
