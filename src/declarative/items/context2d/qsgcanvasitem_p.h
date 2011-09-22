/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGCANVASITEM_P_H
#define QSGCANVASITEM_P_H

#include "qsgitem.h"
#include <private/qv8engine_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)
class QSGContext2D;
class QSGCanvasItemPrivate;
class Q_DECLARATIVE_EXPORT QSGCanvasItem : public QSGItem
{
    Q_OBJECT
    Q_ENUMS(RenderTarget)
    Q_ENUMS(ImageFilterMode)

    Q_PROPERTY(QSizeF canvasSize READ canvasSize WRITE setCanvasSize NOTIFY canvasSizeChanged)
    Q_PROPERTY(QSize tileSize READ tileSize WRITE setTileSize NOTIFY tileSizeChanged)
    Q_PROPERTY(QRectF canvasWindow READ canvasWindow WRITE setCanvasWindow NOTIFY canvasWindowChanged)
    Q_PROPERTY(bool renderInThread READ renderInThread WRITE setRenderInThread NOTIFY renderInThreadChanged)
    Q_PROPERTY(RenderTarget renderTarget READ renderTarget WRITE setRenderTarget NOTIFY renderTargetChanged)
public:
    enum RenderTarget {
        Image,
        FramebufferObject
    };

    enum ImageFilterMode {
        Threshold,
        Mono,
        GrayScale,
        Brightness,
        Invert,
        Blur,
        Opaque,
        Convolute
    };

    QSGCanvasItem(QSGItem *parent = 0);
    ~QSGCanvasItem();

    QSizeF canvasSize() const;
    void setCanvasSize(const QSizeF &);

    QSize tileSize() const;
    void setTileSize(const QSize &);

    QRectF canvasWindow() const;
    void setCanvasWindow(const QRectF& rect);

    bool renderInThread() const;
    void setRenderInThread(bool renderInThread);

    RenderTarget renderTarget() const;
    void setRenderTarget(RenderTarget target);

    QSGContext2D* context() const;
    QImage toImage(const QRectF& region = QRectF()) const;

    QImage loadedImage(const QUrl& url);
Q_SIGNALS:
    void paint(QDeclarativeV8Handle context, const QRect &region);
    void painted();
    void canvasSizeChanged();
    void tileSizeChanged();
    void renderInThreadChanged();
    void textureChanged();
    void canvasWindowChanged();
    void renderTargetChanged();
    void imageLoaded();
public Q_SLOTS:
    QString toDataURL(const QString& type = QLatin1String("image/png")) const;
    QDeclarativeV8Handle getContext(const QString & = QLatin1String("2d"));
    void markDirty(const QRectF& region);
    void requestPaint() {markDirty(canvasWindow());}
    // Save current canvas to disk
    bool save(const QString& filename) const;
    void loadImage(const QUrl& url);
    void unloadImage(const QUrl& url);
    bool isImageLoaded(const QUrl& url) const;
    bool isImageLoading(const QUrl& url) const;
    bool isImageError(const QUrl& url) const;
private Q_SLOTS:
    void _doPainting(const QRectF& region);
protected:
    virtual void componentComplete();
    virtual QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);
    virtual void geometryChanged(const QRectF &newGeometry,
                                 const QRectF &oldGeometry);
    virtual void updatePolish();
private:
    void createContext();
    void createTexture();
    Q_DECLARE_PRIVATE(QSGCanvasItem)
    friend class QSGContext2D;
    friend class QSGContext2DTexture;
};
QT_END_NAMESPACE

QML_DECLARE_TYPE(QSGCanvasItem)

QT_END_HEADER

#endif //QSGCANVASITEM_P_H
