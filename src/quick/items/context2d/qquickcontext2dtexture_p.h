/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKCONTEXT2DTEXTURE_P_H
#define QQUICKCONTEXT2DTEXTURE_P_H

#include <QtQuick/qsgtexture.h>
#include "qquickcanvasitem_p.h"
#include "qquickcontext2d_p.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>

#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QThread>

QT_BEGIN_NAMESPACE

class QQuickContext2DTile;
class QQuickContext2DCommandBuffer;

class QQuickContext2DTexture : public QSGDynamicTexture
{
    Q_OBJECT
public:
    QQuickContext2DTexture();
    ~QQuickContext2DTexture();

    virtual bool hasAlphaChannel() const {return true;}
    virtual bool hasMipmaps() const {return false;}
    virtual QSize textureSize() const;
    virtual QQuickCanvasItem::RenderTarget renderTarget() const = 0;
    static QRect tiledRect(const QRectF& window, const QSize& tileSize);

    bool setCanvasSize(const QSize &size);
    bool setTileSize(const QSize &size);
    bool setCanvasWindow(const QRect& canvasWindow);
    void setSmooth(bool smooth);
    void setAntialiasing(bool antialiasing);
    bool setDirtyRect(const QRect &dirtyRect);
    bool canvasDestroyed();

Q_SIGNALS:
    void textureChanged();

public Q_SLOTS:
    void markDirtyTexture();
    void setItem(QQuickCanvasItem* item);
    void canvasChanged(const QSize& canvasSize, const QSize& tileSize, const QRect& canvasWindow, const QRect& dirtyRect, bool smooth, bool antialiasing);
    void paint(QQuickContext2DCommandBuffer *ccb);
    virtual void grabImage(const QRectF& region = QRectF()) = 0;

protected:
    void paintWithoutTiles(QQuickContext2DCommandBuffer *ccb);
    virtual QPaintDevice* beginPainting() {m_painting = true; return 0; }
    virtual void endPainting() {m_painting = false;}
    virtual QQuickContext2DTile* createTile() const = 0;
    virtual void compositeTile(QQuickContext2DTile* tile) = 0;

    void clearTiles();
    virtual QSize adjustedTileSize(const QSize &ts);
    QRect createTiles(const QRect& window);

    QList<QQuickContext2DTile*> m_tiles;
    QQuickContext2D* m_context;

    QQuickContext2D::State m_state;

    QQuickCanvasItem* m_item;
    QSize m_canvasSize;
    QSize m_tileSize;
    QRect m_canvasWindow;

    uint m_dirtyCanvas : 1;
    uint m_canvasWindowChanged : 1;
    uint m_dirtyTexture : 1;
    uint m_smooth : 1;
    uint m_antialiasing : 1;
    uint m_tiledCanvas : 1;
    uint m_painting : 1;
};

class QQuickContext2DFBOTexture : public QQuickContext2DTexture
{
    Q_OBJECT

public:
    QQuickContext2DFBOTexture();
    ~QQuickContext2DFBOTexture();
    virtual int textureId() const;
    virtual bool updateTexture();
    virtual QQuickContext2DTile* createTile() const;
    virtual QPaintDevice* beginPainting();
    virtual void endPainting();
    QRectF normalizedTextureSubRect() const;
    virtual QQuickCanvasItem::RenderTarget renderTarget() const;
    virtual void compositeTile(QQuickContext2DTile* tile);
    virtual void bind();
    QSize adjustedTileSize(const QSize &ts);

public Q_SLOTS:
    virtual void grabImage(const QRectF& region = QRectF());

private:
    bool doMultisampling() const;
    QOpenGLFramebufferObject *m_fbo;
    QOpenGLFramebufferObject *m_multisampledFbo;
    QMutex m_mutex;
    QWaitCondition m_condition;
    QSize m_fboSize;
    QPaintDevice *m_paint_device;
};

class QSGPlainTexture;
class QQuickContext2DImageTexture : public QQuickContext2DTexture
{
    Q_OBJECT

public:
    QQuickContext2DImageTexture();
    ~QQuickContext2DImageTexture();
    virtual int textureId() const;
    virtual void bind();

    virtual QQuickCanvasItem::RenderTarget renderTarget() const;

    virtual bool updateTexture();
    virtual QQuickContext2DTile* createTile() const;
    virtual QPaintDevice* beginPainting();
    virtual void compositeTile(QQuickContext2DTile* tile);

public Q_SLOTS:
    virtual void grabImage(const QRectF& region = QRectF());

private:
    QSGPlainTexture *imageTexture() const;
    QImage m_image;
    QPainter m_painter;
    QSGPlainTexture*  m_texture;
};

QT_END_NAMESPACE

#endif // QQUICKCONTEXT2DTEXTURE_P_H
