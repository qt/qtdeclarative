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

#include "qsgshadereffectsource_p.h"

#include "qsgitem_p.h"
#include "qsgcanvas_p.h"
#include <private/qsgadaptationlayer_p.h>
#include <private/qsgrenderer_p.h>

#include "qglframebufferobject.h"
#include "qmath.h"
#include <private/qsgtexture_p.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlFboOverlay, QML_FBO_OVERLAY)

QSGShaderEffectSourceNode::QSGShaderEffectSourceNode()
{
    setFlag(UsePreprocess, true);
}

void QSGShaderEffectSourceNode::markDirtyTexture()
{
    markDirty(DirtyMaterial);
}


QSGShaderEffectTexture::QSGShaderEffectTexture(QSGItem *shaderSource)
    : QSGDynamicTexture()
    , m_item(0)
    , m_format(GL_RGBA)
    , m_shaderSource(shaderSource)
    , m_renderer(0)
    , m_fbo(0)
    , m_secondaryFbo(0)
#ifdef QSG_DEBUG_FBO_OVERLAY
    , m_debugOverlay(0)
#endif
    , m_mipmap(false)
    , m_live(true)
    , m_recursive(false)
    , m_dirtyTexture(true)
    , m_multisamplingSupportChecked(false)
    , m_multisampling(false)
    , m_grab(false)
{
}

QSGShaderEffectTexture::~QSGShaderEffectTexture()
{
    delete m_renderer;
    delete m_fbo;
    delete m_secondaryFbo;
#ifdef QSG_DEBUG_FBO_OVERLAY
    delete m_debugOverlay;
#endif
}


int QSGShaderEffectTexture::textureId() const
{
    return m_fbo ? m_fbo->texture() : 0;
}

bool QSGShaderEffectTexture::hasAlphaChannel() const
{
    return m_format != GL_RGB;
}

bool QSGShaderEffectTexture::hasMipmaps() const
{
    return m_mipmap;
}


void QSGShaderEffectTexture::bind()
{
#ifndef QT_NO_DEBUG
    if (!m_recursive && m_fbo && ((m_multisampling && m_secondaryFbo->isBound()) || m_fbo->isBound()))
        qWarning("ShaderEffectSource: \'recursive\' must be set to true when rendering recursively.");
#endif
    glBindTexture(GL_TEXTURE_2D, m_fbo ? m_fbo->texture() : 0);
    updateBindOptions();
}

bool QSGShaderEffectTexture::updateTexture()
{
    if ((m_live || m_grab) && m_dirtyTexture) {
        grab();
        m_grab = false;
        return true;
    }
    return false;
}

void QSGShaderEffectTexture::setHasMipmaps(bool mipmap)
{
    if (mipmap == m_mipmap)
        return;
    m_mipmap = mipmap;
    if (m_mipmap && m_fbo && !m_fbo->format().mipmap())
        markDirtyTexture();
}


void QSGShaderEffectTexture::setItem(QSGNode *item)
{
    if (item == m_item)
        return;
    m_item = item;
    markDirtyTexture();
}

void QSGShaderEffectTexture::setRect(const QRectF &rect)
{
    if (rect == m_rect)
        return;
    m_rect = rect;
    markDirtyTexture();
}

void QSGShaderEffectTexture::setSize(const QSize &size)
{
    if (size == m_size)
        return;
    m_size = size;
    markDirtyTexture();
}

void QSGShaderEffectTexture::setFormat(GLenum format)
{
    if (format == m_format)
        return;
    m_format = format;
    markDirtyTexture();
}

void QSGShaderEffectTexture::setLive(bool live)
{
    if (live == m_live)
        return;
    m_live = live;
    markDirtyTexture();
}

void QSGShaderEffectTexture::scheduleUpdate()
{
    if (m_grab)
        return;
    m_grab = true;
    if (m_dirtyTexture)
        emit textureChanged();
}

void QSGShaderEffectTexture::setRecursive(bool recursive)
{
    m_recursive = recursive;
}

void QSGShaderEffectTexture::markDirtyTexture()
{
    m_dirtyTexture = true;
    if (m_live || m_grab)
        emit textureChanged();
}

void QSGShaderEffectTexture::grab()
{
    if (!m_item || m_size.isNull()) {
        delete m_fbo;
        delete m_secondaryFbo;
        m_fbo = m_secondaryFbo = 0;
        m_dirtyTexture = false;
        return;
    }
    QSGNode *root = m_item;
    while (root->childCount() && root->type() != QSGNode::RootNodeType)
        root = root->childAtIndex(0);
    if (root->type() != QSGNode::RootNodeType)
        return;

    if (m_size.isEmpty()) {
        delete m_fbo;
        delete m_secondaryFbo;
        m_secondaryFbo = m_fbo = 0;
        return;
    }

    QSGContext *context = QSGItemPrivate::get(m_shaderSource)->sceneGraphContext();

    if (!m_renderer) {
        m_renderer = context->createRenderer();
        connect(m_renderer, SIGNAL(sceneGraphChanged()), this, SLOT(markDirtyTexture()), Qt::DirectConnection);
    }
    m_renderer->setRootNode(static_cast<QSGRootNode *>(root));

    bool deleteFboLater = false;
    if (!m_fbo || m_fbo->size() != m_size || m_fbo->format().internalTextureFormat() != m_format
        || (!m_fbo->format().mipmap() && m_mipmap))
    {
        if (!m_multisamplingSupportChecked) {
            QList<QByteArray> extensions = QByteArray((const char *)glGetString(GL_EXTENSIONS)).split(' ');
            m_multisampling = extensions.contains("GL_EXT_framebuffer_multisample")
                            && extensions.contains("GL_EXT_framebuffer_blit");
            m_multisamplingSupportChecked = true;
        }
        if (m_multisampling) {
            // Don't delete the FBO right away in case it is used recursively.
            deleteFboLater = true;
            delete m_secondaryFbo;
            QGLFramebufferObjectFormat format;

            format.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
            format.setInternalTextureFormat(m_format);
            format.setSamples(8);
            m_secondaryFbo = new QGLFramebufferObject(m_size, format);
        } else {
            QGLFramebufferObjectFormat format;
            format.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
            format.setInternalTextureFormat(m_format);
            format.setMipmap(m_mipmap);
            if (m_recursive) {
                deleteFboLater = true;
                delete m_secondaryFbo;
                m_secondaryFbo = new QGLFramebufferObject(m_size, format);
                glBindTexture(GL_TEXTURE_2D, m_secondaryFbo->texture());
                updateBindOptions(true);
            } else {
                delete m_fbo;
                delete m_secondaryFbo;
                m_fbo = new QGLFramebufferObject(m_size, format);
                m_secondaryFbo = 0;
                glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
                updateBindOptions(true);
            }
        }
    }

    if (m_recursive && !m_secondaryFbo) {
        // m_fbo already created, m_recursive was just set.
        Q_ASSERT(m_fbo);
        Q_ASSERT(!m_multisampling);

        m_secondaryFbo = new QGLFramebufferObject(m_size, m_fbo->format());
        glBindTexture(GL_TEXTURE_2D, m_secondaryFbo->texture());
        updateBindOptions(true);
    }

    // Render texture.
    root->markDirty(QSGNode::DirtyForceUpdate); // Force matrix, clip and opacity update.
    m_renderer->nodeChanged(root, QSGNode::DirtyForceUpdate); // Force render list update.

#ifdef QSG_DEBUG_FBO_OVERLAY
    if (qmlFboOverlay()) {
        if (!m_debugOverlay)
            m_debugOverlay = context->createRectangleNode();
        m_debugOverlay->setRect(QRectF(0, 0, m_size.width(), m_size.height()));
        m_debugOverlay->setColor(QColor(0xff, 0x00, 0x80, 0x40));
        m_debugOverlay->setPenColor(QColor());
        m_debugOverlay->setPenWidth(0);
        m_debugOverlay->setRadius(0);
        m_debugOverlay->update();
        root->appendChildNode(m_debugOverlay);
    }
#endif

    m_dirtyTexture = false;

    const QGLContext *ctx = QGLContext::currentContext();
    m_renderer->setDeviceRect(m_size);
    m_renderer->setViewportRect(m_size);
    QRectF mirrored(m_rect.left(), m_rect.bottom(), m_rect.width(), -m_rect.height());
    m_renderer->setProjectMatrixToRect(mirrored);
    m_renderer->setClearColor(Qt::transparent);

    if (m_multisampling) {
        m_renderer->renderScene(BindableFbo(m_secondaryFbo));

        if (deleteFboLater) {
            delete m_fbo;
            QGLFramebufferObjectFormat format;
            format.setInternalTextureFormat(m_format);
            format.setAttachment(QGLFramebufferObject::NoAttachment);
            format.setMipmap(m_mipmap);
            format.setSamples(0);
            m_fbo = new QGLFramebufferObject(m_size, format);
            glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
            updateBindOptions(true);
        }

        QRect r(QPoint(), m_size);
        QGLFramebufferObject::blitFramebuffer(m_fbo, r, m_secondaryFbo, r);
    } else {
        if (m_recursive) {
            m_renderer->renderScene(BindableFbo(m_secondaryFbo));

            if (deleteFboLater) {
                delete m_fbo;
                QGLFramebufferObjectFormat format;
                format.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
                format.setInternalTextureFormat(m_format);
                format.setMipmap(m_mipmap);
                m_fbo = new QGLFramebufferObject(m_size, format);
                glBindTexture(GL_TEXTURE_2D, m_fbo->texture());
                updateBindOptions(true);
            }
            qSwap(m_fbo, m_secondaryFbo);
        } else {
            m_renderer->renderScene(BindableFbo(m_fbo));
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

/*!
    \qmlclass ShaderEffectSource QSGShaderEffectSource
    \since 5.0
    \ingroup qml-basic-visual-elements
    \brief The ShaderEffectSource element renders a QML element into a texture
    and displays it.
    \inherits Item

    The ShaderEffectSource element renders \l sourceItem into a texture and
    displays it in the scene. \l sourceItem is drawn into the texture as though
    it was a fully opaque root element. Thus \l sourceItem itself can be
    invisible, but still appear in the texture.

    ShaderEffectSource can be used as:
    \list
    \o a texture source in a \l ShaderEffectItem.
       This allows you to apply custom shader effects to any QML element.
    \o a cache for a complex element.
       The complex element can be rendered once into the texture, which can
       then be animated freely without the need to render the complex element
       again every frame.
    \o an opacity layer. 
       ShaderEffectSource allows you to apply an opacity to elements as a group
       rather than each element individually.
    \endlist

    \row
    \o \image declarative-shadereffectsource.png
    \o \qml
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
    \endrow

    The ShaderEffectSource element does not redirect any mouse or keyboard
    input to \l sourceItem. If you hide the \l sourceItem by setting
    \l{Item::visible}{visible} to false or \l{Item::opacity}{opacity} to zero,
    it will no longer react to input. In cases where the ShaderEffectSource is
    meant to replace the \l sourceItem, you typically want to hide the
    \l sourceItem while still handling input. For this, you can use
    the \l hideSource property.

    \note If \l sourceItem is a \l Rectangle with border, by default half the
    border width falls outside the texture. To get the whole border, you can
    extend the \l sourceRect.

    \warning In most cases, using a ShaderEffectSource will decrease
    performance, and in all cases, it will increase video memory usage.
    Rendering through a ShaderEffectSource might also lead to lower quality
    since some OpenGL implementations support multisampled backbuffer,
    but not multisampled framebuffer objects.
*/

QSGShaderEffectSource::QSGShaderEffectSource(QSGItem *parent)
    : QSGItem(parent)
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
    m_texture = new QSGShaderEffectTexture(this);
    connect(m_texture, SIGNAL(textureChanged()), this, SIGNAL(textureChanged()), Qt::DirectConnection);
    connect(m_texture, SIGNAL(textureChanged()), this, SLOT(update()));
}

QSGShaderEffectSource::~QSGShaderEffectSource()
{
    delete m_texture;
    if (m_sourceItem)
        QSGItemPrivate::get(m_sourceItem)->derefFromEffectItem(m_hideSource);
}

/*!
    \qmlproperty enumeration ShaderEffectSource::wrapMode

    This property defines the OpenGL wrap modes associated with the texture.
    Modifying this property makes most sense when the element is used as a
    source texture of a \l ShaderEffectItem.

    \list
    \o ShaderEffectSource.ClampToEdge - GL_CLAMP_TO_EDGE both horizontally and vertically
    \o ShaderEffectSource.RepeatHorizontally - GL_REPEAT horizontally, GL_CLAMP_TO_EDGE vertically
    \o ShaderEffectSource.RepeatVertically - GL_CLAMP_TO_EDGE horizontally, GL_REPEAT vertically
    \o ShaderEffectSource.Repeat - GL_REPEAT both horizontally and vertically
    \endlist

    \note Some OpenGL ES 2 implementations do not support the GL_REPEAT
    wrap mode with non-power-of-two textures.
*/

QSGShaderEffectSource::WrapMode QSGShaderEffectSource::wrapMode() const
{
    return m_wrapMode;
}

void QSGShaderEffectSource::setWrapMode(WrapMode mode)
{
    if (mode == m_wrapMode)
        return;
    m_wrapMode = mode;
    update();
    emit wrapModeChanged();
}

/*!
    \qmlproperty Item ShaderEffectSource::sourceItem

    This property holds the element to be rendered into the texture.
*/

QSGItem *QSGShaderEffectSource::sourceItem() const
{
    return m_sourceItem;
}

void QSGShaderEffectSource::setSourceItem(QSGItem *item)
{
    if (item == m_sourceItem)
        return;
    if (m_sourceItem)
        QSGItemPrivate::get(m_sourceItem)->derefFromEffectItem(m_hideSource);
    m_sourceItem = item;
    if (m_sourceItem) {
        // TODO: Find better solution.
        // 'm_sourceItem' needs a canvas to get a scenegraph node.
        // The easiest way to make sure it gets a canvas is to
        // make it a part of the same item tree as 'this'.
        if (m_sourceItem->parentItem() == 0) {
            m_sourceItem->setParentItem(this);
            m_sourceItem->setVisible(false);
        }
        QSGItemPrivate::get(m_sourceItem)->refFromEffectItem(m_hideSource);
    }
    update();
    emit sourceItemChanged();
}

/*!
    \qmlproperty rect ShaderEffectSource::sourceRect

    This property defines which rectangular area of the \l sourceItem to
    render into the texture. The source rectangle can be larger than
    \l sourceItem itself. If the rectangle is null, which is the default,
    the whole \l sourceItem is rendered to texture.
*/

QRectF QSGShaderEffectSource::sourceRect() const
{
    return m_sourceRect;
}

void QSGShaderEffectSource::setSourceRect(const QRectF &rect)
{
    if (rect == m_sourceRect)
        return;
    m_sourceRect = rect;
    update();
    emit sourceRectChanged();
}

/*!
    \qmlproperty size ShaderEffectSource::textureSize

    This property holds the size of the texture. If it is empty, which is the
    default, the size of the source rectangle is used.
*/

QSize QSGShaderEffectSource::textureSize() const
{
    return m_textureSize;
}

void QSGShaderEffectSource::setTextureSize(const QSize &size)
{
    if (size == m_textureSize)
        return;
    m_textureSize = size;
    update();
    emit textureSizeChanged();
}

/*!
    \qmlproperty enumeration ShaderEffectSource::format

    This property defines the internal OpenGL format of the texture.
    Modifying this property makes most sense when the element is used as a
    source texture of a \l ShaderEffectItem. Depending on the OpenGL
    implementation, this property might allow you to save some texture memory.

    \list
    \o ShaderEffectSource.Alpha - GL_ALPHA
    \o ShaderEffectSource.RGB - GL_RGB
    \o ShaderEffectSource.RGBA - GL_RGBA
    \endlist

    \note Some OpenGL implementations do not support the GL_ALPHA format.
*/

QSGShaderEffectSource::Format QSGShaderEffectSource::format() const
{
    return m_format;
}

void QSGShaderEffectSource::setFormat(QSGShaderEffectSource::Format format)
{
    if (format == m_format)
        return;
    m_format = format;
    update();
    emit formatChanged();
}

/*!
    \qmlproperty bool ShaderEffectSource::live

    If this property is true, the texture is updated whenever the
    \l sourceItem changes. Otherwise, it will be a frozen image of the
    \l sourceItem. The property is true by default.
*/

bool QSGShaderEffectSource::live() const
{
    return m_live;
}

void QSGShaderEffectSource::setLive(bool live)
{
    if (live == m_live)
        return;
    m_live = live;
    update();
    emit liveChanged();
}

/*!
    \qmlproperty bool ShaderEffectSource::hideSource

    If this property is true, the \l sourceItem is hidden, though it will still
    be rendered into the texture. As opposed to hiding the \l sourceItem by
    setting \l{Item::visible}{visible} to false, setting this property to true
    will not prevent mouse or keyboard input from reaching \l sourceItem.
    The property is useful when the ShaderEffectSource is anchored on top of,
    and meant to replace the \l sourceItem.
*/

bool QSGShaderEffectSource::hideSource() const
{
    return m_hideSource;
}

void QSGShaderEffectSource::setHideSource(bool hide)
{
    if (hide == m_hideSource)
        return;
    if (m_sourceItem) {
        QSGItemPrivate::get(m_sourceItem)->refFromEffectItem(hide);
        QSGItemPrivate::get(m_sourceItem)->derefFromEffectItem(m_hideSource);
    }
    m_hideSource = hide;
    update();
    emit hideSourceChanged();
}

/*!
    \qmlproperty bool ShaderEffectSource::mipmap

    If this property is true, mipmaps are generated for the texture.

    \note Some OpenGL ES 2 implementations do not support mipmapping of
    non-power-of-two textures.
*/

bool QSGShaderEffectSource::mipmap() const
{
    return m_mipmap;
}

void QSGShaderEffectSource::setMipmap(bool enabled)
{
    if (enabled == m_mipmap)
        return;
    m_mipmap = enabled;
    update();
    emit mipmapChanged();
}

/*!
    \qmlproperty bool ShaderEffectSource::recursive

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

bool QSGShaderEffectSource::recursive() const
{
    return m_recursive;
}

void QSGShaderEffectSource::setRecursive(bool enabled)
{
    if (enabled == m_recursive)
        return;
    m_recursive = enabled;
    emit recursiveChanged();
}

/*!
    \qmlmethod ShaderEffectSource::scheduleUpdate()

    Schedules a re-rendering of the texture for the next frame.
    Use this to update the texture when \l live is false.
*/

void QSGShaderEffectSource::scheduleUpdate()
{
    if (m_grab)
        return;
    m_grab = true;
    update();
}

static void get_wrap_mode(QSGShaderEffectSource::WrapMode mode, QSGTexture::WrapMode *hWrap, QSGTexture::WrapMode *vWrap)
{
    switch (mode) {
    case QSGShaderEffectSource::RepeatHorizontally:
        *hWrap = QSGTexture::Repeat;
        *vWrap = QSGTexture::ClampToEdge;
        break;
    case QSGShaderEffectSource::RepeatVertically:
        *vWrap = QSGTexture::Repeat;
        *hWrap = QSGTexture::ClampToEdge;
        break;
    case QSGShaderEffectSource::Repeat:
        *hWrap = *vWrap = QSGTexture::Repeat;
        break;
    default:
        // QSGShaderEffectSource::ClampToEdge
        *hWrap = *vWrap = QSGTexture::ClampToEdge;
        break;
    }
}


QSGTexture *QSGShaderEffectSource::texture() const
{
    m_texture->setMipmapFiltering(m_mipmap ? QSGTexture::Linear : QSGTexture::None);
    m_texture->setFiltering(QSGItemPrivate::get(this)->smooth ? QSGTexture::Linear : QSGTexture::Nearest);
    QSGTexture::WrapMode h, v;
    get_wrap_mode(m_wrapMode, &h, &v);
    m_texture->setHorizontalWrapMode(h);
    m_texture->setVerticalWrapMode(v);
    return m_texture;
}

QSGNode *QSGShaderEffectSource::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (!m_sourceItem) {
        delete oldNode;
        return 0;
    }

    QSGShaderEffectSourceNode *node = static_cast<QSGShaderEffectSourceNode *>(oldNode);
    if (!node) {
        node = new QSGShaderEffectSourceNode;
        node->setTexture(m_texture);
        connect(m_texture, SIGNAL(textureChanged()), node, SLOT(markDirtyTexture()), Qt::DirectConnection);
    }

    // If live and recursive, update continuously.
    if (m_live && m_recursive)
        node->markDirty(QSGNode::DirtyMaterial);

    QSGShaderEffectTexture *tex = qobject_cast<QSGShaderEffectTexture *>(m_texture);

    tex->setLive(m_live);
    tex->setItem(QSGItemPrivate::get(m_sourceItem)->itemNode());
    QRectF sourceRect = m_sourceRect.isEmpty()
                      ? QRectF(0, 0, m_sourceItem->width(), m_sourceItem->height())
                      : m_sourceRect;
    tex->setRect(sourceRect);
    QSize textureSize = m_textureSize.isEmpty()
                      ? QSize(qCeil(sourceRect.width()), qCeil(sourceRect.height()))
                      : m_textureSize;
    tex->setSize(textureSize);
    tex->setRecursive(m_recursive);
    tex->setFormat(GLenum(m_format));
    tex->setHasMipmaps(m_mipmap);

    if (m_grab)
        tex->scheduleUpdate();
    m_grab = false;

    QSGTexture::Filtering filtering = QSGItemPrivate::get(this)->smooth
                                            ? QSGTexture::Linear
                                            : QSGTexture::Nearest;
    QSGTexture::Filtering mmFiltering = m_mipmap ? filtering : QSGTexture::None;
    node->setMipmapFiltering(mmFiltering);
    node->setFiltering(filtering);

    QSGTexture::WrapMode hWrap, vWrap;
    get_wrap_mode(m_wrapMode, &hWrap, &vWrap);

    node->setHorizontalWrapMode(hWrap);
    node->setVerticalWrapMode(vWrap);
    node->setTargetRect(QRectF(0, 0, width(), height()));
    node->setSourceRect(QRectF(0, 0, 1, 1));
    node->update();

    return node;
}

QT_END_NAMESPACE
