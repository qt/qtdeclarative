// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <qqmlextensionplugin.h>

#include <qqmlengine.h>
#include <qquickimageprovider.h>
#include <QImage>
#include <QPainter>
#include <QDebug>

class ColorImageProvider : public QQuickImageProvider
{
public:
    ColorImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Pixmap)
    {
    }

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
    {
        int width = 50;
        int height = 50;

        QPixmap pixmap(requestedSize.width() > 0 ? requestedSize.width() : width,
                       requestedSize.height() > 0 ? requestedSize.height() : height);
        if (size)
            *size = QSize(pixmap.width(), pixmap.height());
        pixmap.fill(QColor(id).rgba());

        // draw lines on even y offsets
        QPainter p(&pixmap);
        for (int y = 0; y < pixmap.height(); y+=2) {
            p.drawLine(0, y, pixmap.width(), y);
        }
        return pixmap;
    }
};


class ImageProviderExtensionPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    void registerTypes(const char *uri)
    {
        Q_UNUSED(uri);
    }

    void initializeEngine(QQmlEngine *engine, const char *uri)
    {
        Q_UNUSED(uri);
        engine->addImageProvider("colors", new ColorImageProvider);
    }

};


#include "imageprovider.moc"
