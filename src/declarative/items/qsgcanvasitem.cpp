/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/qpainter.h>

#include "private/qsgadaptationlayer_p.h"
#include "qsgcanvasitem_p.h"
#include "qsgpainteditem_p.h"
#include "qsgcontext2d_p.h"
#include "private/qsgpainternode_p.h"
#include <qdeclarativeinfo.h>
#include "qdeclarativeengine_p.h"
#include <QtCore/QBuffer>

QT_BEGIN_NAMESPACE

class QSGCanvasItemPrivate : public QSGPaintedItemPrivate
{
public:
    QSGCanvasItemPrivate();
    QSGContext2D* context;
};


/*!
    \internal
*/
QSGCanvasItemPrivate::QSGCanvasItemPrivate()
    : QSGPaintedItemPrivate()
    , context(0)
{
}

/*!
    Constructs a QSGCanvasItem with the given \a parent item.
 */
QSGCanvasItem::QSGCanvasItem(QSGItem *parent)
    : QSGPaintedItem(*(new QSGCanvasItemPrivate), parent)
{
}

/*!
    Destroys the QSGCanvasItem.
*/
QSGCanvasItem::~QSGCanvasItem()
{
}

void QSGCanvasItem::paint(QPainter *painter)
{
    Q_D(QSGCanvasItem);

    if (d->context) {
        d->context->paint(painter);
        emit canvasUpdated();
    }
}


QSGContext2D* QSGCanvasItem::getContext(const QString &contextId)
{
    Q_D(QSGCanvasItem);
    if (contextId == QLatin1String("2d")) {
        if (!d->context) {
            d->context = new QSGContext2D(this);
            connect(d->context, SIGNAL(changed()), this, SLOT(requestPaint()));
        }
        return d->context;
    }
    qDebug("Canvas:requesting unsupported context");
    return 0;
}

void QSGCanvasItem::requestPaint()
{
    Q_D(QSGCanvasItem);
    //TODO:update(d->context->dirtyRect());
    update();
}

bool QSGCanvasItem::save(const QString &filename) const
{
    Q_D(const QSGCanvasItem);
    QSGPainterNode* node = static_cast<QSGPainterNode*>(d->paintNode);
    if (node) {
        QImage image = node->toImage();
        image.save(filename);
    }
    return false;
}

QString QSGCanvasItem::toDataURL(const QString& mimeType) const
{
    Q_D(const QSGCanvasItem);

    QSGPainterNode* node = static_cast<QSGPainterNode*>(d->paintNode);
    if (node) {
        QImage image = node->toImage();
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        QString mime = mimeType;
        QString type;
        if (mimeType == QLatin1String("image/bmp"))
            type = "BMP";
        else if (mimeType == QLatin1String("image/jpeg"))
            type = "JPEG";
        else if (mimeType == QLatin1String("image/x-portable-pixmap"))
            type = "PPM";
        else if (mimeType == QLatin1String("image/tiff"))
            type = "TIFF";
        else if (mimeType == QLatin1String("image/xbm"))
            type = "XBM";
        else if (mimeType == QLatin1String("image/xpm"))
            type = "XPM";
        else {
            type = "PNG";
            mime = QLatin1String("image/png");
        }
        image.save(&buffer, type.toAscii());
        buffer.close();
        QString dataUrl = QLatin1String("data:%1;base64,%2");
        return dataUrl.arg(mime).arg(ba.toBase64().constData());
    }
    return QLatin1String("data:,");
}
//CanvasItemTextureProvider::CanvasItemTextureProvider(QObject *parent)
//    : QSGTextureProvider(parent)
//    , m_ctx2d(0)
//    , m_fbo(0)
//    , m_multisampledFbo(0)
//    , m_dirtyTexture(true)
//    , m_multisamplingSupportChecked(false)
//    , m_multisampling(false)
//{
//}

//CanvasItemTextureProvider::~CanvasItemTextureProvider()
//{
//    delete m_fbo;
//    delete m_multisampledFbo;
//}

//void CanvasItemTextureProvider::updateTexture()
//{
//    if (m_dirtyTexture) {
//        if (!m_ctx2d->isDirty())
//            return;
//        if (m_size.isEmpty()) {
//            m_texture = QSGTextureRef();
//            delete m_fbo;
//            delete m_multisampledFbo;
//            m_multisampledFbo = m_fbo = 0;
//            return;
//        }

//#ifndef QSGCANVASITEM_PAINTING_ON_IMAGE
//        //create texture
//        if (!m_fbo || m_fbo->size() != m_size )
//        {
//            const QGLContext *ctx = QSGContext::current->glContext();
//            if (!m_multisamplingSupportChecked) {
//                QList<QByteArray> extensions = QByteArray((const char *)glGetString(GL_EXTENSIONS)).split(' ');
//                m_multisampling = extensions.contains("GL_EXT_framebuffer_multisample")
//                                && extensions.contains("GL_EXT_framebuffer_blit");
//                m_multisamplingSupportChecked = true;
//            }

//            if (ctx->format().sampleBuffers() && m_multisampling) {
//                delete m_fbo;
//                delete m_multisampledFbo;
//                QGLFramebufferObjectFormat format;

//                format.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
//                format.setSamples(ctx->format().samples());
//                m_multisampledFbo = new QGLFramebufferObject(m_size, format);
//                {
//                    QGLFramebufferObjectFormat format;
//                    format.setAttachment(QGLFramebufferObject::NoAttachment);
//                    m_fbo = new QGLFramebufferObject(m_size, format);
//                }

//                QSGPlainTexture *tex = new QSGPlainTexture;
//                tex->setTextureId(m_fbo->texture());
//                tex->setOwnsTexture(false);
//                tex->setHasAlphaChannel(true);
//                setOpaque(!tex->hasAlphaChannel());
//                m_texture = QSGTextureRef(tex);
//            } else {
//                delete m_fbo;
//                QGLFramebufferObjectFormat format;
//                format.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
//                m_fbo = new QGLFramebufferObject(m_size, format);
//                QSGPlainTexture *tex = new QSGPlainTexture;
//                tex->setTextureId(m_fbo->texture());
//                tex->setOwnsTexture(false);
//                tex->setHasAlphaChannel(true);
//                setOpaque(!tex->hasAlphaChannel());
//                m_texture = QSGTextureRef(tex);
//            }
//        }
//#endif

//#ifdef QSGCANVASITEM_DEBUG
//        qDebug() << "painting interval:" << m_elapsedTimer.nsecsElapsed();
//        m_elapsedTimer.restart();
//#endif
//        //paint 2d
//        if (m_ctx2d) {
//            QPainter p;
//#ifndef QSGCANVASITEM_PAINTING_ON_IMAGE
//            if (m_multisampledFbo)
//                p.begin(m_multisampledFbo);
//            else if (m_fbo)
//                p.begin(m_fbo);
//            else
//                return;
//            // move the origin of coordinates to the down left corner and
//            // scale coordinates and turn y-axis up
//            QSize size = m_ctx2d->size();
//            p.translate( 0, size.height());
//            p.scale(1, -1);

//            m_ctx2d->paint(&p);

//            p.end();

//            if (m_multisampledFbo) {
//                QRect r(0, 0, m_fbo->width(), m_fbo->height());
//                QGLFramebufferObject::blitFramebuffer(m_fbo, r, m_multisampledFbo, r);
//            }

//            if (m_ctx2d->requireCachedImage())
//                m_ctx2d->setCachedImage(m_fbo->toImage());

//#else
//            m_painter.begin(m_ctx2d->paintDevice());
//            m_ctx2d->paint(&m_painter);
//            m_painter.end();

//            if (m_texture.isNull()) {
//                m_texture = QSGContext::current->createTexture(m_ctx2d->toImage());
//            } else {
//                QSGPlainTexture* t =static_cast<QSGPlainTexture*>(m_texture.texture());
//                t->setImage(m_ctx2d->toImage());
//            }
//            m_ctx2d->setCachedImage(m_ctx2d->toImage());

//#endif

//#ifdef QSGCANVASITEM_DEBUG
//            qDebug() << "painting time:" << m_elapsedTimer.nsecsElapsed();
//            m_elapsedTimer.restart();
//#endif
//            emit painted();
//        }
//    }
//}

//QSGTextureRef CanvasItemTextureProvider::texture()
//{
//    return m_texture;
//}
//void CanvasItemTextureProvider::setContext2D(QSGContext2D *ctx2d)
//{
//    if (ctx2d && m_ctx2d != ctx2d) {
//        m_ctx2d = ctx2d;
//        connect(this, SIGNAL(painted()), m_ctx2d, SIGNAL(painted()));
//    }
//}
//void CanvasItemTextureProvider::setRect(const QRectF &rect)
//{
//    if (rect == m_rect)
//        return;
//    m_rect = rect;
//    markDirtyTexture();
//}

//void CanvasItemTextureProvider::setSize(const QSize &size)
//{
//    if (size == m_size)
//        return;
//    m_size = size;
//    markDirtyTexture();
//}

//void CanvasItemTextureProvider::markDirtyTexture()
//{
//    m_dirtyTexture = true;
//    emit textureChanged();
//}
//QSGCanvasItem::QSGCanvasItem(QSGItem *parent)
//    : TextureItem(parent)
//    , m_textureProvider(0)
//    , m_context2dChanged(false)
//    , m_context2d( new QSGContext2D(this))
//    , m_fillMode(QSGCanvasItem::Stretch)
//    , m_color(Qt::white)
//{
//    m_textureProvider = new CanvasItemTextureProvider(this);
//    m_textureProvider->setContext2D(m_context2d);
//    setTextureProvider(m_textureProvider, true);
//    setFlag(QSGItem::ItemHasContents, true);
//}

//QSGCanvasItem::~QSGCanvasItem()
//{
//}

//void QSGCanvasItem::componentComplete()
//{
//    m_context2d->setSize(width(), height());
//    qDebug() << "m_context2d.size:" << m_context2d->size();
//    connect(m_context2d, SIGNAL(changed()), this, SLOT(requestPaint()));
//    QScriptEngine* scriptEngine = QDeclarativeEnginePrivate::getScriptEngine(qmlEngine(this));
//    if (scriptEngine != m_context2d->scriptEngine())
//        m_context2d->setScriptEngine(scriptEngine);
//    QSGItem::componentComplete();
//}


//void QSGCanvasItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
//{
//    if (width() == 0 && height()
//        && newGeometry.width() > 0 && newGeometry.height() > 0) {
//        m_context2d->setSize(width(), height());
//    }
//    TextureItem::geometryChanged(newGeometry, oldGeometry);
//}

//void QSGCanvasItem::setFillMode(FillMode mode)
//{
//    if (m_fillMode == mode)
//        return;

//    m_fillMode = mode;
//    update();
//    emit fillModeChanged();
//}

//QColor QSGCanvasItem::color()
//{
//    return m_color;
//}

//void QSGCanvasItem::setColor(const QColor &color)
//{
//    if (m_color !=color) {
//        m_color = color;
//        colorChanged();
//    }
//}

//QSGCanvasItem::FillMode QSGCanvasItem::fillMode() const
//{
//    return m_fillMode;
//}



//Node *QSGCanvasItem::updatePaintNode(Node *oldNode, UpdatePaintNodeData *data)
//{
//    if (width() <= 0 || height() <= 0) {
//        delete oldNode;
//        return 0;
//    }

//    TextureNodeInterface *node = static_cast<TextureNodeInterface *>(oldNode);

//    if (node && m_context2d->isDirty()) {
//        QRectF bounds = boundingRect();

//        if (m_textureProvider) {
//            m_textureProvider->setRect(QRectF(bounds.x(), bounds.y(), width(), height()));

//            m_textureProvider->setSize(QSize(width(), height()));
//            //m_textureProvider->setOpaque(true);
//            m_textureProvider->setHorizontalWrapMode(QSGTextureProvider::ClampToEdge);
//            m_textureProvider->setVerticalWrapMode(QSGTextureProvider::ClampToEdge);
//            node->setTargetRect(bounds);
//            node->setSourceRect(QRectF(0, 0, 1, 1));
//            //            node->setTargetRect(image.rect());
////            node->setSourceRect(QRectF(0, 0, 1, 1));
////            d->textureProvider->setHorizontalWrapMode(QSGTextureProvider::ClampToEdge);
////            d->textureProvider->setVerticalWrapMode(QSGTextureProvider::ClampToEdge);
////            d->textureProvider->setFiltering(d->smooth ? QSGTextureProvider::Linear : QSGTextureProvider::Nearest);
//        }

//        if (m_context2dChanged) {
//            //force textnode update the content
//            node->setTexture(0);
//            node->setTexture(m_textureProvider);
//            m_context2dChanged = false;
//        }
//    } else {
//        if (m_context2d->requireCachedImage())
//            m_context2d->setCachedImage(QImage(width(), height(), QImage::Format_ARGB32_Premultiplied));
//    }

//    return TextureItem::updatePaintNode(oldNode, data);
//}

QT_END_NAMESPACE
