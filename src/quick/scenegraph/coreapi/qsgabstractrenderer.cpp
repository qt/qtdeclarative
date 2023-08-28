// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgabstractrenderer_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSGAbstractRenderer
    \brief QSGAbstractRenderer gives access to the scene graph nodes and rendering.
    \inmodule QtQuick
    \since 5.4
    \internal
 */

/*!
    \enum QSGAbstractRenderer::ClearModeBit

    Used with setClearMode() to indicate which buffer should
    be cleared before the scene render.

    \value ClearColorBuffer Clear the color buffer using clearColor().
    \value ClearDepthBuffer Clear the depth buffer.
    \value ClearStencilBuffer Clear the stencil buffer.

    \sa setClearMode(), setClearColor()
 */

/*!
    \enum QSGAbstractRenderer::MatrixTransformFlag

    Used with setProjectionMatrixToRect() to indicate the expectations towards
    the generated projection matrix.

    \value MatrixTransformFlipY The traditional assumption in Qt Quick is that
    Y points up in the normalized device coordinate system. There is at least
    one modern graphics API where this is not the case (Vulkan). This flag can
    then be used to get a projection that is appropriate for such an API.

    \sa setProjectionMatrixToRect()

    \since 5.14
 */

/*!
    \fn void QSGAbstractRenderer::renderScene()

    Renders the scene.
 */

/*!
    \fn void QSGAbstractRenderer::sceneGraphChanged()

    This signal is emitted on the first modification of a node in
    the tree after the last scene render.
 */

/*!
    \internal
 */
QSGAbstractRendererPrivate::QSGAbstractRendererPrivate()
    : m_root_node(nullptr)
    , m_clear_color(Qt::transparent)
    , m_clear_mode(QSGAbstractRenderer::ClearColorBuffer | QSGAbstractRenderer::ClearDepthBuffer)
{
}

/*!
    \internal
 */
QSGAbstractRenderer::QSGAbstractRenderer(QObject *parent)
    : QObject(*new QSGAbstractRendererPrivate, parent)
{
}

/*!
    \internal
 */
QSGAbstractRenderer::~QSGAbstractRenderer()
{
}

/*!
    Sets the \a node as the root of the QSGNode scene
    that you want to render. You need to provide a \a node
    before trying to render the scene.

    \note This doesn't take ownership of \a node.

    \sa rootNode()
*/
void QSGAbstractRenderer::setRootNode(QSGRootNode *node)
{
    Q_D(QSGAbstractRenderer);
    if (d->m_root_node == node)
        return;
    if (d->m_root_node) {
        d->m_root_node->m_renderers.removeOne(this);
        nodeChanged(d->m_root_node, QSGNode::DirtyNodeRemoved);
    }
    d->m_root_node = node;
    if (d->m_root_node) {
        Q_ASSERT(!d->m_root_node->m_renderers.contains(this));
        d->m_root_node->m_renderers << this;
        nodeChanged(d->m_root_node, QSGNode::DirtyNodeAdded);
    }
}

/*!
    Returns the root of the QSGNode scene.

    \sa setRootNode()
*/
QSGRootNode *QSGAbstractRenderer::rootNode() const
{
    Q_D(const QSGAbstractRenderer);
    return d->m_root_node;
}


/*!
    \fn void QSGAbstractRenderer::setDeviceRect(const QSize &size)
    \overload

    Sets the \a size of the surface being rendered to.

    \sa deviceRect()
 */

/*!
    Sets \a rect as the geometry of the surface being rendered to.

    \sa deviceRect()
 */
void QSGAbstractRenderer::setDeviceRect(const QRect &rect)
{
    Q_D(QSGAbstractRenderer);
    d->m_device_rect = rect;
}

/*!
    Returns the device rect of the surface being rendered to.

    \sa setDeviceRect()
 */
QRect QSGAbstractRenderer::deviceRect() const
{
    Q_D(const QSGAbstractRenderer);
    return d->m_device_rect;
}

/*!
    \fn void QSGAbstractRenderer::setViewportRect(const QSize &size)
    \overload

    Sets the \a size of the viewport to render
    on the surface.

    \sa viewportRect()
 */

/*!
    Sets \a rect as the geometry of the viewport to render
    on the surface.

    \sa viewportRect()
 */
void QSGAbstractRenderer::setViewportRect(const QRect &rect)
{
    Q_D(QSGAbstractRenderer);
    d->m_viewport_rect = rect;
}

/*!
    Returns the rect of the viewport to render.

    \sa setViewportRect()
 */
QRect QSGAbstractRenderer::viewportRect() const
{
    Q_D(const QSGAbstractRenderer);
    return d->m_viewport_rect;
}

/*!
    Convenience method that calls setProjectionMatrix() with an
    orthographic matrix generated from \a rect.

    \note This function assumes that the graphics API uses Y up in its
    normalized device coordinate system.

    \sa setProjectionMatrix(), projectionMatrix()
 */
void QSGAbstractRenderer::setProjectionMatrixToRect(const QRectF &rect)
{
    setProjectionMatrixToRect(rect, {}, false);
}

/*!
    Convenience method that calls setProjectionMatrix() with an
    orthographic matrix generated from \a rect.

    Set MatrixTransformFlipY in \a flags when the graphics API uses Y down in
    its normalized device coordinate system (for example, Vulkan).

    \sa setProjectionMatrix(), projectionMatrix()

    \since 5.14
 */
void QSGAbstractRenderer::setProjectionMatrixToRect(const QRectF &rect, MatrixTransformFlags flags)
{
    setProjectionMatrixToRect(rect, flags, flags.testFlag(MatrixTransformFlipY));
}

/*!
    Convenience method that calls setProjectionMatrix() with an
    orthographic matrix generated from \a rect.

    Set MatrixTransformFlipY in \a flags when the graphics API uses Y down in
    its normalized device coordinate system (for example, Vulkan).

    Convenience method that calls setProjectionMatrixWithNativeNDC() with an
    orthographic matrix generated from \a rect.

    Set true to \a nativeNDCFlipY to flip the Y axis relative to
    projection matrix in its normalized device coordinate system.

    \sa setProjectionMatrix(), projectionMatrix()
    \sa setProjectionMatrixWithNativeNDC(), projectionMatrixWithNativeNDC()

    \since 6.7
 */
void QSGAbstractRenderer::setProjectionMatrixToRect(const QRectF &rect, MatrixTransformFlags flags,
                                                    bool nativeNDCFlipY)
{
    const bool flipY = flags.testFlag(MatrixTransformFlipY);

    const float left = rect.x();
    const float right = rect.x() + rect.width();
    float bottom = rect.y() + rect.height();
    float top = rect.y();

    if (flipY)
        std::swap(top, bottom);

    QMatrix4x4 matrix;
    matrix.ortho(left, right, bottom, top, 1, -1);
    setProjectionMatrix(matrix);

    if (nativeNDCFlipY) {
        std::swap(top, bottom);

        matrix.setToIdentity();
        matrix.ortho(left, right, bottom, top, 1, -1);
    }
    setProjectionMatrixWithNativeNDC(matrix);
}

/*!
    Use \a matrix to project the QSGNode coordinates onto surface pixels.

    \sa projectionMatrix(), setProjectionMatrixToRect()
 */
void QSGAbstractRenderer::setProjectionMatrix(const QMatrix4x4 &matrix)
{
    Q_D(QSGAbstractRenderer);
    d->m_projection_matrix = matrix;
}

/*!
    \internal
 */
void QSGAbstractRenderer::setProjectionMatrixWithNativeNDC(const QMatrix4x4 &matrix)
{
    Q_D(QSGAbstractRenderer);
    d->m_projection_matrix_native_ndc = matrix;
}

/*!
    Returns the projection matrix

    \sa setProjectionMatrix(), setProjectionMatrixToRect()
 */
QMatrix4x4 QSGAbstractRenderer::projectionMatrix() const
{
    Q_D(const QSGAbstractRenderer);
    return d->m_projection_matrix;
}

/*!
    \internal
 */
QMatrix4x4 QSGAbstractRenderer::projectionMatrixWithNativeNDC() const
{
    Q_D(const QSGAbstractRenderer);
    return d->m_projection_matrix_native_ndc;
}

/*!
    Use \a color to clear the framebuffer when clearMode() is
    set to QSGAbstractRenderer::ClearColorBuffer.

    \sa clearColor(), setClearMode()
 */
void QSGAbstractRenderer::setClearColor(const QColor &color)
{
    Q_D(QSGAbstractRenderer);
    d->m_clear_color = color;
}

/*!
    Returns the color that clears the framebuffer at the beginning
    of the rendering.

    \sa setClearColor(), clearMode()
 */
QColor QSGAbstractRenderer::clearColor() const
{
    Q_D(const QSGAbstractRenderer);
    return d->m_clear_color;
}

/*!
    Defines which attachment of the framebuffer should be cleared
    before each scene render with the \a mode flag.

    \sa clearMode(), setClearColor()
 */
void QSGAbstractRenderer::setClearMode(ClearMode mode)
{
    Q_D(QSGAbstractRenderer);
    d->m_clear_mode = mode;
}

/*!
    Flags defining which attachment of the framebuffer will be cleared
    before each scene render.

    \sa setClearMode(), clearColor()
 */
QSGAbstractRenderer::ClearMode QSGAbstractRenderer::clearMode() const
{
    Q_D(const QSGAbstractRenderer);
    return d->m_clear_mode;
}

/*!
    \fn void QSGAbstractRenderer::nodeChanged(QSGNode *node, QSGNode::DirtyState state)
    \internal
 */

void QSGAbstractRenderer::prepareSceneInline()
{
}

void QSGAbstractRenderer::renderSceneInline()
{
}

QT_END_NAMESPACE

#include "moc_qsgabstractrenderer_p.cpp"
