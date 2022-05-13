// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui/QGuiApplication>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickImageProvider>
#include <QtQuick/QQuickView>
#include <QtCore/QUrl>
#include <QtCore/QSize>
#include <QtGui/QPixmap>
#include <QtGui/QColor>

//![0]
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
       return pixmap;
    }
};
//![0]
//![1]
int main(int argc, char *argv[])
{
//![1]
    QGuiApplication app(argc, argv);
//![2]
    QQuickView view;
    QQmlEngine *engine = view.engine();
    engine->addImageProvider(QLatin1String("colors"), new ColorImageProvider);
    view.setSource(QUrl::fromLocalFile(QStringLiteral("imageprovider-example.qml")));
    view.show();
    return app.exec();
}
//![2]
