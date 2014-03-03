/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquickshadereffectsource_p.h"

#include "qquickitem_p.h"
#include "qquickwindow_p.h"
#include <private/qsgadaptationlayer_p.h>
#include <QtQuick/private/qsgrenderer_p.h>
#include <qsgsimplerectnode.h>

#include "qopenglframebufferobject.h"
#include "qmath.h"
#include <QtQuick/private/qsgtexture_p.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlFboOverlay, QML_FBO_OVERLAY)
DEFINE_BOOL_CONFIG_OPTION(qmlFboFlushBeforeDetach, QML_FBO_FLUSH_BEFORE_DETACH)

namespace
{
    class BindableFbo : public QSGBindable
    {
    public:
        BindableFbo(QOpenGLFramebufferObject *fbo, QSGDepthStencilBuffer *depthStencil);
        virtual ~BindableFbo();
        virtual void bind() const;
    private:
        QOpenGLFramebufferObject *m_fbo;
        QSGDepthStencilBuffer *m_depthStencil;
    };

    BindableFbo::BindableFbo(QOpenGLFramebufferObject *fbo, QSGDepthStencilBuffer *depthStencil)
        : m_fbo(fbo)
        , m_depthStencil(depthStencil)
    {
    }

    BindableFbo::~BindableFbo()
    {
        if (qmlFboFlushBeforeDetach())
            glFlush();
        if (m_depthStencil)
            m_depthStencil->detach();
    }

    void BindableFbo::bind() const
    {
        m_fbo->bind();
        if (m_depthStencil)
            m_depthStencil->attach();
    }
}

class QQuickShaderEffectSourceTextureProvider : public QSGTextureProvider
{
    Q_OBJECT
public:
    QQuickShaderEffectSourceTextureProvider()
        : sourceTexture(0)
        , mipmapFiltering(QSGTexture::None)
        , filtering(QSGTexture::Nearest)
        , horizontalWrap(QSGTexture::ClampToEdge)
        , verticalWrap(QSGTexture::ClampToEdge)
    {
    }

    QSGTexture *texture() const {
        sourceTexture->setMipmapFiltering(mipmapFiltering);
        sourceTexture->setFiltering(filtering);
        sourceTexture->setHorizontalWrapMode(horizontalWrap);
        sourceTexture->setVerticalWrapMode(verticalWrap);
        return sourceTexture;
    }

    QQuickShaderEffectTexture *sourceTexture;

    QSGTexture::Filtering mipmapFiltering;
    QSGTexture::Filtering filtering;
    QSGTexture::WrapMode horizontalWrap;
    QSGTexture::WrapMode verticalWrap;
};
#include "qquickshadereffectsource.moc"


QQuickShaderEffectSourceNode::QQuickShaderEffectSourceNode()
{
    setFlag(UsePreprocess, true);
}

void QQuickShaderEffectSourceNode::markDirtyTexture()
{
    markDirty(DirtyMaterial);
}


QQuickShaderEffectTexture::QQuickShaderEffectTexture(QQuickItem *shaderSource)
    : QSGDynamicTexture()
    , m_item(0)
    , m_device_pixel_ratio(1)
    , m_format(GL_RGBA)
    , m_renderer(0)
    , m_fbo(0)
    , m_secondaryFbo(0)
    , m_transparentTexture(0)
#ifdef QSG_DEBUG_FBO_OVERLAY
    , m_debugOverlay(0)
#endif
    , m_context(QQuickItemPrivate::get(shaderSource)->sceneGraphRenderContext())
    , m_mipmap(false)
    , m_live(true)
    , m_recursive(false)
    , m_dirtyTexture(true)
    , m_multisamplingChecked(false)
    , m_multisampling(false)
    , m_grab(false)
{
}

QQuickShaderEffectTexture::~QQuickShaderEffectTexture()
{
    invalidated();
}

void QQuickShaderEffectTexture::invalidated()
{
    delete m_renderer;
    m_renderer = 0;
    delete m_fbo;
    delete m_secondaryFbo;
    m_fbo = m_secondaryFbo = 0;
#ifdef QSG_DEBUG_FBO_OVERLAY
    delete m_debugOverlay;
    m_debugOverlay = 0;
#endif
    if (m_transparentTexture) {
        glDeleteTextures(1, &m_transparentTexture);
        m_transparentTexture = 0;
    }
}

int QQuickShaderEffectTexture::textureId() const
{
    return m_fbo ? m_fbo->texture() : 0;
}

bool QQuickShaderEffectTexture::hasAlphaChannel() const
{
    return m_format != GL_RGB;
}

bool QQuickShaderEffectTexture::hasMipmaps() const
{
    return m_mipmap;
}


void QQuickShaderEffectTexture::bind()
{
#ifndef QT_NO_DEBUG
    if (!m_recursive && m_fbo && ((m_multisampling && m_secondaryFbo->isBound()) || m_fbo->isBound()))
        qWarning("ShaderEffectSource: \'recursive\' must be set to true when rendering recursively.");
#endif

    if (!m_fbo && m_format == GL_RGBA) {
        if (m_transparentTexture == 0) {
            glGenTextures(1, &m_transparentTexture);
            glBindTexture(GL_TEXTURE_2D, m_transparentTexture);
            const uint zero = 0;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &zero);
        } else {
            glBindTexture(GL_TEXTURE_2D, m_transparentTexture);
        }
    } else {
        glBindTexture(GL_TEXTURE_2D, m_fbo ? m_fbo->texture() : 0);
        updateBindOptions();
    }
}

bool QQuickShaderEffectTexture::updateTexture()
{
    bool doGrab = (m_live || m_grab) && m_dirtyTexture;
    if (doGrab)
        grab();
    if (m_grab)
        emit scheduledUpdateCompleted();
    m_grab = false;
    return doGrab;
}

void QQuickShaderEffectTexture::setHasMipmaps(bool mipmap)
{
    if (mipmap == m_mipmap)
        return;
    m_mipmap = mipmap;
    if (m_mipmap && m_fbo && !m_fbo->format().mipmap())
        markDirtyTexture();
}


void QQuickShaderEffectTexture::setItem(QSGNode *item)
{
    if (item == m_item)
        return;
    m_item = item;

    if (m_live && !m_item) {
        delete m_fbo;
        delete m_secondaryFbo;
        m_fbo = m_secondaryFbo = 0;
        m_depthStencilBuffer.clear();
    }

    markDirtyTexture();
}

void QQuickShaderEffectTexture::setRect(const QRectF &rect)
{
    if (rect == m_rect)
        return;
    m_rect = rect;
    markDirtyTexture();
}

void QQuickShaderEffectTexture::setSize(const QSize &size)
{
    if (size == m_size)
        return;
    m_size = size;

    if (m_live && m_size.isNull()) {
        delete m_fbo;
        delete m_secondaryFbo;
        m_fbo = m_secondaryFbo = 0;
        m_depthStencilBuffer.clear();
    }

    markDirtyTexture();
}

void QQuickShaderEffectTexture::setFormat(GLenum format)
{
    if (format == m_format)
        return;
    m_format = format;
    markDirtyTexture();
}

void QQuickShaderEffectTexture::setLive(bool live)
{
    if (live == m_live)
        return;
    m_live = live;

    if (m_live && (!m_item || m_size.isNull())) {
        delete m_fbo;
        delete m_secondaryFbo;
        m_fbo = m_secondaryFbo = 0;
        m_depthStencilBuffer.clear();
    }

    markDirtyTexture();
}

void QQuickShaderEffectTexture::scheduleUpdate()
{
    if (m_grab)
        return;
    m_grab = true;
    if (m_dirtyTexture)
        emit updateRequested();
}

void QQuickShaderEffectTexture::setRecursive(bool recursive)
{
    m_recursive = recursive;
}

void QQuickShaderEffectTexture::markDirtyTexture()
{
    m_dirtyTexture = true;
    if (m_live || m_grab)
        emit updateRequested();
}

void QQuickShaderEffectTexture::grab()
{
    if (!m_item || m_size.isNull()) {
        delete m_fbo;
        delete m_secondaryFbo;
        m_fbo = m_secondaryFbo = 0;
        m_depthStencilBuffer.clear();
        m_dirtyTexture = false;
        return;
    }
    QSGNode *root = m_item;
    while (root->firstChild() && root->type() != QSGNode::RootNodeType)
        root = root->firstChild();
    if (root->type() != QSGNode::RootNodeType)
        return;

    if (!m_renderer) {
        m_renderer = m_context->createRenderer();
        connect(m_renderer, SIGNAL(sceneGraphChanged()), this, SLOT(markDirtyTexture()));
    }
    m_renderer->setDevicePixelRatio(m_device_pixel_ratio);
    m_renderer->setRootNode(static_cast<QSGRootNode *>(root));

    bool deleteFboLater = false;
    if (!m_fbo || m_fbo->size() != m_size || m_fbo->format().internalTextureFormat() != m_format
        || (!m_fbo->format().mipmap() && m_mipmap))
    {
        if (!m_multisamplingChecked) {
            if (m_context->openglContext()->format().samples() <= 1) {
                m_multisampling = false;
            } else {
                const QSet<QByteArray> extensions = m_context->openglContext()->extensions();
                m_multisampling = extensions.contains(QByteArrayLiteral("GL_EXT_framebuffer_multisample"))
                    && extensions.contains(QByteArrayLiteral("GL_EXT_framebuffer_blit"));
            }
            m_multisamplingChecked = true;
        }
        if (m_multisampling) {
            // Don't delete the FBO right away in case it is used recursively.
            deleteFboLater = true;
            delete m_secondaryFbo;
            QOpenGLFramebufferObjectFormat format;

            format.setInternalTextureFormat(m_format);
            format.setSamples(m_context->openglContext()->format().samples());
            m_secondaryFbo = new QOpenGLFramebufferObject(m_size, format);
            m_depthStencilBuffer = m_context->depthStencilBufferForFbo(m_secondaryFbo);
        } else {
            QOpenGLFramebufferObjectFormat format;
            format.setInternalTextureFormat(m_format);
            format.setMipmap(m_mipmap);
            if (m_recursive) {
                deleteFboLater = true;
                delete m_secondaryFbo;
                m_secondaryFbo = new QOpenGLFramebufferObject(m_size, format);
                glBindTexture(GL_TEXTURE_2D, m_secondaryFbo->texture());
                updateBindOptions(true);
                m_depthStencilBuffer = m_context->depthStencilBufferForFbo(m_secondaryFbo);
            } else {
                delete m_fbo;
                delete m_secondaryFbo;
                m_fbo = new QOpenGLFramebufferObject(m_size, format);
                m_secondaryFbo = 0;
                glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
                updateBindOptions(true);
                m_depthStencilBuffer = m_context->depthStencilBufferForFbo(m_fbo);
            }
        }
    }

    if (m_recursive && !m_secondaryFbo) {
        // m_fbo already created, m_recursive was just set.
        Q_ASSERT(m_fbo);
        Q_ASSERT(!m_multisampling);

        m_secondaryFbo = new QOpenGLFramebufferObject(m_size, m_fbo->format());
        glBindTexture(GL_TEXTURE_2D, m_secondaryFbo->texture());
        updateBindOptions(true);
    }

    // Render texture.
    root->markDirty(QSGNode::DirtyForceUpdate); // Force matrix, clip and opacity update.
    m_renderer->nodeChanged(root, QSGNode::DirtyForceUpdate); // Force render list update.

#ifdef QSG_DEBUG_FBO_OVERLAY
    if (qmlFboOverlay()) {
        if (!m_debugOverlay)
            m_debugOverlay = new QSGSimpleRectNode();
        m_debugOverlay->setRect(QRectF(0, 0, m_size.width(), m_size.height()));
        m_debugOverlay->setColor(QColor(0xff, 0x00, 0x80, 0x40));
        root->appendChildNode(m_debugOverlay);
    }
#endif

    m_dirtyTexture = false;

    QOpenGLContext *ctx = m_context->openglContext();
    m_renderer->setDeviceRect(m_size);
    m_renderer->setViewportRect(m_size);
    QRectF mirrored(m_rect.left(), m_rect.bottom(), m_rect.width(), -m_rect.height());
    m_renderer->setProjectionMatrixToRect(mirrored);
    m_renderer->setClearColor(Qt::transparent);

    if (m_multisampling) {
        m_renderer->renderScene(BindableFbo(m_secondaryFbo, m_depthStencilBuffer.data()));

        if (deleteFboLater) {
            delete m_fbo;
            QOpenGLFramebufferObjectFormat format;
            format.setInternalTextureFormat(m_format);
            format.setAttachment(QOpenGLFramebufferObject::NoAttachment);
            format.setMipmap(m_mipmap);
            format.setSamples(0);
            m_fbo = new QOpenGLFramebufferObject(m_size, format);
            glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
            updateBindOptions(true);
        }

        QRect r(QPoint(), m_size);
        QOpenGLFramebufferObject::blitFramebuffer(m_fbo, r, m_secondaryFbo, r);
    } else {
        if (m_recursive) {
            m_renderer->renderScene(BindableFbo(m_secondaryFbo, m_depthStencilBuffer.data()));

            if (deleteFboLater) {
                delete m_fbo;
                QOpenGLFramebufferObjectFormat format;
                format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
                format.setInternalTextureFormat(m_format);
                format.setMipmap(m_mipmap);
                m_fbo = new QOpenGLFramebufferObject(m_size, format);
                glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
                updateBindOptions(true);
            }
            qSwap(m_fbo, m_secondaryFbo);
        } else {
            m_renderer->renderScene(BindableFbo(m_fbo, m_depthStencilBuffer.data()));
        }
    }

    if (m_mipmap) {
        glBindTexture(GL_TEXTURE_2D, textureId());
        ctx->functions()->glGenerateMipmap(GL_TEXTURE_2D);
    }

    root->markDirty(QSGNode::DirtyForceUpdate); // Force matrix, clip, opacity and render list update.

#ifdef QSG_DEBUG_FBO_OVERLAY
    if (qmlFboOverlay())
        root->removeChildNode(m_debugOverlay);
#endif
    if (m_recursive)
        markDirtyTexture(); // Continuously update if 'live' and 'recursive'.
}

QImage QQuickShaderEffectTexture::toImage() const
{
    if (m_fbo)
        return m_fbo->toImage();

    return QImage();
}

/*!
    \qmltype ShaderEffectSource
    \instantiates QQuickShaderEffectSource
    \inqmlmodule QtQuick
    \since 5.0
    \inherits Item
    \ingroup qtquick-effects
    \brief Renders a \l {Qt Quick} item into a texture and displays it

    The ShaderEffectSource type renders \l sourceItem into a texture and
    displays it in the scene. \l sourceItem is drawn into the texture as though
    it was a fully opaque root item. Thus \l sourceItem itself can be
    invisible, but still appear in the texture.

    ShaderEffectSource can be used as:
    \list
    \li a texture source in a \l ShaderEffect.
       This allows you to apply custom shader effects to any \l {Qt Quick} item.
    \li a cache for a complex item.
       The complex item can be rendered once into the texture, which can
       then be animated freely without the need to render the complex item
       again every frame.
    \li an opacity layer.
       ShaderEffectSource allows you to apply an opacity to items as a group
       rather than each item individually.
    \endlist

    \table
    \row
    \li \image declarative-shadereffectsource.png
    \li \qml
        import QtQuick 2.0

        Rectangle {
            width: 200
            height: 100
            gradient: Gradient {
                GradientStop { position: 0; color: "white" }
                GradientStop { position: 1; color: "black" }
            }
            Row {
                opacity: 0.5
                Item {
                    id: foo
                    width: 100; height: 100
                    Rectangle { x: 5; y: 5; width: 60; height: 60; color: "red" }
                    Rectangle { x: 20; y: 20; width: 60; height: 60; color: "orange" }
                    Rectangle { x: 35; y: 35; width: 60; height: 60; color: "yellow" }
                }
                ShaderEffectSource {
                    width: 100; height: 100
                    sourceItem: foo
                }
            }
        }
        \endqml
    \endtable

    The ShaderEffectSource type does not redirect any mouse or keyboard
    input to \l sourceItem. If you hide the \l sourceItem by setting
    \l{Item::visible}{visible} to false or \l{Item::opacity}{opacity} to zero,
    it will no longer react to input. In cases where the ShaderEffectSource is
    meant to replace the \l sourceItem, you typically want to hide the
    \l sourceItem while still handling input. For this, you can use
    the \l hideSource property.

    \note If \l sourceItem is a \l Rectangle with border, by default half the
    border width falls outside the texture. To get the whole border, you can
    extend the \l sourceRect.

    \note The ShaderEffectSource relies on FBO multisampling support
    to antialias edges. If the underlying hardware does not support this,
    which is the case for most embedded graphics chips, edges rendered
    inside a ShaderEffectSource will not be antialiased. One way to remedy
    this is to double the size of the effect source and render it with
    \c {smooth: true} (this is the default value of smooth).
    This will be equivalent to 4x multisampling, at the cost of lower performance
    and higher memory use.

    \warning In most cases, using a ShaderEffectSource will decrease
    performance, and in all cases, it will increase video memory usage.
    Rendering through a ShaderEffectSource might also lead to lower quality
    since some OpenGL implementations support multisampled backbuffer,
    but not multisampled framebuffer objects.
*/

QQuickShaderEffectSource::QQuickShaderEffectSource(QQuickItem *parent)
    : QQuickItem(parent)
    , m_provider(0)
    , m_texture(0)
    , m_wrapMode(ClampToEdge)
    , m_sourceItem(0)
    , m_textureSize(0, 0)
    , m_format(RGBA)
    , m_live(true)
    , m_hideSource(false)
    , m_mipmap(false)
    , m_recursive(false)
    , m_grab(true)
{
    setFlag(ItemHasContents);
}

QQuickShaderEffectSource::~QQuickShaderEffectSource()
{
    if (m_texture)
        m_texture->deleteLater();

    if (m_provider)
        m_provider->deleteLater();

    if (m_sourceItem) {
        QQuickItemPrivate *sd = QQuickItemPrivate::get(m_sourceItem);
        sd->removeItemChangeListener(this, QQuickItemPrivate::Geometry);
        sd->derefFromEffectItem(m_hideSource);
        if (window())
            sd->derefWindow();
    }
}

void QQuickShaderEffectSource::ensureTexture()
{
    if (m_texture)
        return;

    Q_ASSERT_X(QQuickItemPrivate::get(this)->window
               && QQuickItemPrivate::get(this)->sceneGraphRenderContext()
               && QThread::currentThread() == QQuickItemPrivate::get(this)->sceneGraphRenderContext()->thread(),
               "QQuickShaderEffectSource::ensureTexture",
               "Cannot be used outside the rendering thread");

    m_texture = new QQuickShaderEffectTexture(this);
    connect(QQuickItemPrivate::get(this)->window, SIGNAL(sceneGraphInvalidated()), m_texture, SLOT(invalidated()), Qt::DirectConnection);
    connect(m_texture, SIGNAL(updateRequested()), this, SLOT(update()));
    connect(m_texture, SIGNAL(scheduledUpdateCompleted()), this, SIGNAL(scheduledUpdateCompleted()));
}

static void get_wrap_mode(QQuickShaderEffectSource::WrapMode mode, QSGTexture::WrapMode *hWrap, QSGTexture::WrapMode *vWrap);

QSGTextureProvider *QQuickShaderEffectSource::textureProvider() const
{
    const QQuickItemPrivate *d = QQuickItemPrivate::get(this);
    if (!d->window || !d->sceneGraphRenderContext() || QThread::currentThread() != d->sceneGraphRenderContext()->thread()) {
        qWarning("QQuickShaderEffectSource::textureProvider: can only be queried on the rendering thread of an exposed window");
        return 0;
    }

    if (!m_provider) {
        const_cast<QQuickShaderEffectSource *>(this)->m_provider = new QQuickShaderEffectSourceTextureProvider();
        const_cast<QQuickShaderEffectSource *>(this)->ensureTexture();
        connect(m_texture, SIGNAL(updateRequested()), m_provider, SIGNAL(textureChanged()));

        get_wrap_mode(m_wrapMode, &m_provider->horizontalWrap, &m_provider->verticalWrap);
        m_provider->mipmapFiltering = mipmap() ? QSGTexture::Linear : QSGTexture::None;
        m_provider->filtering = smooth() ? QSGTexture::Linear : QSGTexture::Nearest;
        m_provider->sourceTexture = m_texture;
    }
    return m_provider;
}

/*!
    \qmlproperty enumeration QtQuick::ShaderEffectSource::wrapMode

    This property defines the OpenGL wrap modes associated with the texture.
    Modifying this property makes most sense when the item is used as a
    source texture of a \l ShaderEffect.

    \list
    \li ShaderEffectSource.ClampToEdge - GL_CLAMP_TO_EDGE both horizontally and vertically
    \li ShaderEffectSource.RepeatHorizontally - GL_REPEAT horizontally, GL_CLAMP_TO_EDGE vertically
    \li ShaderEffectSource.RepeatVertically - GL_CLAMP_TO_EDGE horizontally, GL_REPEAT vertically
    \li ShaderEffectSource.Repeat - GL_REPEAT both horizontally and vertically
    \endlist

    \note Some OpenGL ES 2 implementations do not support the GL_REPEAT
    wrap mode with non-power-of-two textures.
*/

QQuickShaderEffectSource::WrapMode QQuickShaderEffectSource::wrapMode() const
{
    return m_wrapMode;
}

void QQuickShaderEffectSource::setWrapMode(WrapMode mode)
{
    if (mode == m_wrapMode)
        return;
    m_wrapMode = mode;
    update();
    emit wrapModeChanged();
}

/*!
    \qmlproperty Item QtQuick::ShaderEffectSource::sourceItem

    This property holds the item to be rendered into the texture.
    Setting this to null while \l live is true, will release the texture
    resources.
*/

QQuickItem *QQuickShaderEffectSource::sourceItem() const
{
    return m_sourceItem;
}

void QQuickShaderEffectSource::itemGeometryChanged(QQuickItem *item, const QRectF &newRect, const QRectF &oldRect)
{
    Q_ASSERT(item == m_sourceItem);
    Q_UNUSED(item);
    if (newRect.size() != oldRect.size())
        update();
}

void QQuickShaderEffectSource::setSourceItem(QQuickItem *item)
{
    if (item == m_sourceItem)
        return;
    if (m_sourceItem) {
        QQuickItemPrivate *d = QQuickItemPrivate::get(m_sourceItem);
        d->derefFromEffectItem(m_hideSource);
        d->removeItemChangeListener(this, QQuickItemPrivate::Geometry);
        disconnect(m_sourceItem, SIGNAL(destroyed(QObject*)), this, SLOT(sourceItemDestroyed(QObject*)));
        if (window())
            d->derefWindow();
    }
    m_sourceItem = item;

    if (item) {
        QQuickItemPrivate *d = QQuickItemPrivate::get(item);
        // 'item' needs a window to get a scene graph node. It usually gets one through its
        // parent, but if the source item is "inline" rather than a reference -- i.e.
        // "sourceItem: Item { }" instead of "sourceItem: foo" -- it will not get a parent.
        // In those cases, 'item' should get the window from 'this'.
        if (window())
            d->refWindow(window());
        d->refFromEffectItem(m_hideSource);
        d->addItemChangeListener(this, QQuickItemPrivate::Geometry);
        connect(m_sourceItem, SIGNAL(destroyed(QObject*)), this, SLOT(sourceItemDestroyed(QObject*)));
    }
    update();
    emit sourceItemChanged();
}

void QQuickShaderEffectSource::sourceItemDestroyed(QObject *item)
{
    Q_ASSERT(item == m_sourceItem);
    Q_UNUSED(item);
    m_sourceItem = 0;
    update();
    emit sourceItemChanged();
}


/*!
    \qmlproperty rect QtQuick::ShaderEffectSource::sourceRect

    This property defines which rectangular area of the \l sourceItem to
    render into the texture. The source rectangle can be larger than
    \l sourceItem itself. If the rectangle is null, which is the default,
    the whole \l sourceItem is rendered to texture.
*/

QRectF QQuickShaderEffectSource::sourceRect() const
{
    return m_sourceRect;
}

void QQuickShaderEffectSource::setSourceRect(const QRectF &rect)
{
    if (rect == m_sourceRect)
        return;
    m_sourceRect = rect;
    update();
    emit sourceRectChanged();
}

/*!
    \qmlproperty size QtQuick::ShaderEffectSource::textureSize

    This property holds the requested size of the texture. If it is empty,
    which is the default, the size of the source rectangle is used.

    \note Some platforms have a limit on how small framebuffer objects can be,
    which means the actual texture size might be larger than the requested
    size.
*/

QSize QQuickShaderEffectSource::textureSize() const
{
    return m_textureSize;
}

void QQuickShaderEffectSource::setTextureSize(const QSize &size)
{
    if (size == m_textureSize)
        return;
    m_textureSize = size;
    update();
    emit textureSizeChanged();
}

/*!
    \qmlproperty enumeration QtQuick::ShaderEffectSource::format

    This property defines the internal OpenGL format of the texture.
    Modifying this property makes most sense when the item is used as a
    source texture of a \l ShaderEffect. Depending on the OpenGL
    implementation, this property might allow you to save some texture memory.

    \list
    \li ShaderEffectSource.Alpha - GL_ALPHA
    \li ShaderEffectSource.RGB - GL_RGB
    \li ShaderEffectSource.RGBA - GL_RGBA
    \endlist

    \note Some OpenGL implementations do not support the GL_ALPHA format.
*/

QQuickShaderEffectSource::Format QQuickShaderEffectSource::format() const
{
    return m_format;
}

void QQuickShaderEffectSource::setFormat(QQuickShaderEffectSource::Format format)
{
    if (format == m_format)
        return;
    m_format = format;
    update();
    emit formatChanged();
}

/*!
    \qmlproperty bool QtQuick::ShaderEffectSource::live

    If this property is true, the texture is updated whenever the
    \l sourceItem updates. Otherwise, it will be a frozen image, even if
    \l sourceItem is assigned a new item. The property is true by default.
*/

bool QQuickShaderEffectSource::live() const
{
    return m_live;
}

void QQuickShaderEffectSource::setLive(bool live)
{
    if (live == m_live)
        return;
    m_live = live;
    update();
    emit liveChanged();
}

/*!
    \qmlproperty bool QtQuick::ShaderEffectSource::hideSource

    If this property is true, the \l sourceItem is hidden, though it will still
    be rendered into the texture. As opposed to hiding the \l sourceItem by
    setting \l{Item::visible}{visible} to false, setting this property to true
    will not prevent mouse or keyboard input from reaching \l sourceItem.
    The property is useful when the ShaderEffectSource is anchored on top of,
    and meant to replace the \l sourceItem.
*/

bool QQuickShaderEffectSource::hideSource() const
{
    return m_hideSource;
}

void QQuickShaderEffectSource::setHideSource(bool hide)
{
    if (hide == m_hideSource)
        return;
    if (m_sourceItem) {
        QQuickItemPrivate::get(m_sourceItem)->refFromEffectItem(hide);
        QQuickItemPrivate::get(m_sourceItem)->derefFromEffectItem(m_hideSource);
    }
    m_hideSource = hide;
    update();
    emit hideSourceChanged();
}

/*!
    \qmlproperty bool QtQuick::ShaderEffectSource::mipmap

    If this property is true, mipmaps are generated for the texture.

    \note Some OpenGL ES 2 implementations do not support mipmapping of
    non-power-of-two textures.
*/

bool QQuickShaderEffectSource::mipmap() const
{
    return m_mipmap;
}

void QQuickShaderEffectSource::setMipmap(bool enabled)
{
    if (enabled == m_mipmap)
        return;
    m_mipmap = enabled;
    update();
    emit mipmapChanged();
}

/*!
    \qmlproperty bool QtQuick::ShaderEffectSource::recursive

    Set this property to true if the ShaderEffectSource has a dependency on
    itself. ShaderEffectSources form a dependency chain, where one
    ShaderEffectSource can be part of the \l sourceItem of another.
    If there is a loop in this chain, a ShaderEffectSource could end up trying
    to render into the same texture it is using as source, which is not allowed
    by OpenGL. When this property is set to true, an extra texture is allocated
    so that ShaderEffectSource can keep a copy of the texture from the previous
    frame. It can then render into one texture and use the texture from the
    previous frame as source.

    Setting both this property and \l live to true will cause the scene graph
    to render continuously. Since the ShaderEffectSource depends on itself,
    updating it means that it immediately becomes dirty again.
*/

bool QQuickShaderEffectSource::recursive() const
{
    return m_recursive;
}

void QQuickShaderEffectSource::setRecursive(bool enabled)
{
    if (enabled == m_recursive)
        return;
    m_recursive = enabled;
    emit recursiveChanged();
}

/*!
    \qmlmethod QtQuick::ShaderEffectSource::scheduleUpdate()

    Schedules a re-rendering of the texture for the next frame.
    Use this to update the texture when \l live is false.
*/

void QQuickShaderEffectSource::scheduleUpdate()
{
    if (m_grab)
        return;
    m_grab = true;
    update();
}

static void get_wrap_mode(QQuickShaderEffectSource::WrapMode mode, QSGTexture::WrapMode *hWrap, QSGTexture::WrapMode *vWrap)
{
    switch (mode) {
    case QQuickShaderEffectSource::RepeatHorizontally:
        *hWrap = QSGTexture::Repeat;
        *vWrap = QSGTexture::ClampToEdge;
        break;
    case QQuickShaderEffectSource::RepeatVertically:
        *vWrap = QSGTexture::Repeat;
        *hWrap = QSGTexture::ClampToEdge;
        break;
    case QQuickShaderEffectSource::Repeat:
        *hWrap = *vWrap = QSGTexture::Repeat;
        break;
    default:
        // QQuickShaderEffectSource::ClampToEdge
        *hWrap = *vWrap = QSGTexture::ClampToEdge;
        break;
    }
}


void QQuickShaderEffectSource::releaseResources()
{
    if (m_texture) {
        m_texture->deleteLater();
        m_texture = 0;
    }
    if (m_provider) {
        m_provider->deleteLater();
        m_provider = 0;
    }
}

QSGNode *QQuickShaderEffectSource::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (!m_sourceItem || m_sourceItem->width() <= 0 || m_sourceItem->height() <= 0) {
        if (m_texture)
            m_texture->setItem(0);
        delete oldNode;
        return 0;
    }

    ensureTexture();

    m_texture->setLive(m_live);
    m_texture->setItem(QQuickItemPrivate::get(m_sourceItem)->itemNode());
    QRectF sourceRect = m_sourceRect.width() == 0 || m_sourceRect.height() == 0
                      ? QRectF(0, 0, m_sourceItem->width(), m_sourceItem->height())
                      : m_sourceRect;
    m_texture->setRect(sourceRect);
    QSize textureSize = m_textureSize.isEmpty()
                      ? QSize(qCeil(qAbs(sourceRect.width())), qCeil(qAbs(sourceRect.height())))
                      : m_textureSize;
    Q_ASSERT(!textureSize.isEmpty());

    QQuickItemPrivate *d = static_cast<QQuickItemPrivate *>(QObjectPrivate::get(this));

    // Crate large textures on high-dpi displays.
    if (sourceItem())
        textureSize *= d->window->devicePixelRatio();

    const QSize minTextureSize = d->sceneGraphContext()->minimumFBOSize();
    // Keep power-of-two by doubling the size.
    while (textureSize.width() < minTextureSize.width())
        textureSize.rwidth() *= 2;
    while (textureSize.height() < minTextureSize.height())
        textureSize.rheight() *= 2;

    m_texture->setDevicePixelRatio(d->window->devicePixelRatio());
    m_texture->setSize(textureSize);
    m_texture->setRecursive(m_recursive);
    m_texture->setFormat(GLenum(m_format));
    m_texture->setHasMipmaps(m_mipmap);

    if (m_grab)
        m_texture->scheduleUpdate();
    m_grab = false;

    QSGTexture::Filtering filtering = QQuickItemPrivate::get(this)->smooth
                                            ? QSGTexture::Linear
                                            : QSGTexture::Nearest;
    QSGTexture::Filtering mmFiltering = m_mipmap ? filtering : QSGTexture::None;
    QSGTexture::WrapMode hWrap, vWrap;
    get_wrap_mode(m_wrapMode, &hWrap, &vWrap);

    if (m_provider) {
        m_provider->mipmapFiltering = mmFiltering;
        m_provider->filtering = filtering;
        m_provider->horizontalWrap = hWrap;
        m_provider->verticalWrap = vWrap;
    }

    // Don't create the paint node if we're not spanning any area
    if (width() == 0 || height() == 0) {
        delete oldNode;
        return 0;
    }

    QQuickShaderEffectSourceNode *node = static_cast<QQuickShaderEffectSourceNode *>(oldNode);
    if (!node) {
        node = new QQuickShaderEffectSourceNode;
        node->setTexture(m_texture);
        connect(m_texture, SIGNAL(updateRequested()), node, SLOT(markDirtyTexture()));
    }

    // If live and recursive, update continuously.
    if (m_live && m_recursive)
        node->markDirty(QSGNode::DirtyMaterial);

    node->setMipmapFiltering(mmFiltering);
    node->setFiltering(filtering);
    node->setHorizontalWrapMode(hWrap);
    node->setVerticalWrapMode(vWrap);
    node->setTargetRect(QRectF(0, 0, width(), height()));
    node->setInnerTargetRect(QRectF(0, 0, width(), height()));
    node->update();

    return node;
}

void QQuickShaderEffectSource::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == QQuickItem::ItemSceneChange && m_sourceItem) {
        // See comment in QQuickShaderEffectSource::setSourceItem().
        if (value.window)
            QQuickItemPrivate::get(m_sourceItem)->refWindow(value.window);
        else
            QQuickItemPrivate::get(m_sourceItem)->derefWindow();
    }
    QQuickItem::itemChange(change, value);
}

QT_END_NAMESPACE
