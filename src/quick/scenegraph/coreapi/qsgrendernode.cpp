// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgrendernode.h"
#include "qsgrendernode_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSGRenderNode
    \brief The QSGRenderNode class represents a set of custom rendering commands
    targeting the graphics API that is in use by the scenegraph.
    \inmodule QtQuick
    \since 5.8

    QSGRenderNode allows creating scene graph nodes that perform their own
    custom rendering via QRhi (the common approach from Qt 6.6 on), directly
    via a 3D graphics API such as OpenGL, Vulkan, or Metal, or, when the \c
    software backend is in use, via QPainter.

    QSGRenderNode is the enabler for one of the three ways to integrate custom
    2D/3D rendering into a Qt Quick scene. The other two options are to perform
    the rendering \c before or \c after the Qt Quick scene's own rendering,
    or to generate a whole separate render pass targeting a dedicated render
    target (a texture) and then have an item in the scene display the texture.
    The QSGRenderNode-based approach is similar to the former, in the sense
    that no additional render passes or render targets are involved, and allows
    injecting custom rendering commands "inline" with the Qt Quick scene's
    own rendering.

    \sa {Scene Graph - Custom QSGRenderNode}
 */

QSGRenderNode::QSGRenderNode()
    : QSGNode(RenderNodeType),
      d(new QSGRenderNodePrivate)
{
}

/*!
    Destructs the render node. Derived classes are expected to perform cleanup
    similar to releaseResources() in here.

    When a low-level graphics API is in use, the scenegraph will make sure
    there is a CPU-side wait for the GPU to complete all work submitted to the
    scenegraph's graphics command queue before the scenegraph's nodes are
    deleted. Therefore there is no need to issue additional waits here, unless
    the render() implementation is using additional command queues.

    With QRhi and resources such as QRhiBuffer, QRhiTexture,
    QRhiGraphicsPipeline, etc., it is often good practice to use smart
    pointers, such as std::unique_ptr, which can often avoid the need to
    implement a destructor, and lead to more compact source code. Keep in mind
    however that implementing releaseResources(), most likely issuing a number
    of reset() calls on the unique_ptrs, is still important.

    \sa releaseResources()
 */
QSGRenderNode::~QSGRenderNode()
{
    delete d;
}

QSGRenderNodePrivate::QSGRenderNodePrivate()
    : m_matrix(nullptr)
    , m_clip_list(nullptr)
    , m_opacity(1)
{
    m_projectionMatrix.resize(1);
}

/*!
    When the underlying rendering API is OpenGL, this function should return a
    mask where each bit represents graphics states changed by the \l render()
    function:

    \value DepthState   depth write mask, depth test enabled, depth comparison function
    \value StencilState stencil write masks, stencil test enabled, stencil operations,
                        stencil comparison functions
    \value ScissorState scissor enabled, scissor test enabled
    \value ColorState   clear color, color write mask
    \value BlendState   blend enabled, blend function
    \value CullState    front face, cull face enabled
    \value ViewportState viewport
    \value RenderTargetState render target

    With APIs other than OpenGL, the only relevant values are the ones that
    correspond to dynamic state changes recorded on the command list/buffer.
    For example, RSSetViewports, RSSetScissorRects, OMSetBlendState,
    OMSetDepthStencilState in case of D3D11, or vkCmdSetViewport, vkCmdSetScissor,
    vkCmdSetBlendConstants, vkCmdSetStencilRef in case of Vulkan, and only when
    such commands were added to the scenegraph's command list queried via the
    QSGRendererInterface::CommandList resource enum. States set in pipeline
    state objects do not need to be reported here. Similarly, draw call related
    settings (pipeline states, descriptor sets, vertex or index buffer
    bindings, root signature, descriptor heaps, etc.) are always set again by
    the scenegraph so render() can freely change them.

    RenderTargetState is no longer supported with APIs like Vulkan. This
    is by nature. render() is invoked while the Qt Quick scenegraph's main
    command buffer is recording a renderpass, so there is no possibility of
    changing the target and starting another renderpass (on that command buffer
    at least). Therefore returning a value with RenderTargetState set is not
    sensible.

    \note The \c software backend exposes its QPainter and saves and restores
    before and after invoking render(). Therefore reporting any changed states
    from here is not necessary.

    The function is called by the renderer so it can reset the states after
    rendering this node. This makes the implementation of render() simpler
    since it does not have to query and restore these states.

    The default implementation returns 0, meaning no relevant state was changed
    in render().

    \note This function may be called before render().

    \note With Qt 6 and QRhi-based rendering the only relevant values are
    ViewportState and ScissorState. Other values can be returned but are
    ignored in practice.
  */
QSGRenderNode::StateFlags QSGRenderNode::changedStates() const
{
    return {};
}

/*!
    Called from the frame preparation phase. There is a call to this function
    before each invocation of render().

    Unlike render(), this function is called before the scenegraph starts
    recording the render pass for the current frame on the underlying command
    buffer. This is useful when doing rendering with graphics APIs, such as
    Vulkan, where copy type of operations will need to be recorded before the
    render pass.

    The default implementation is empty.

    When implementing a QSGRenderNode that uses QRhi to render, query the QRhi
    object from the QQuickWindow via \l{QQuickWindow::rhi()}. To get a
    QRhiCommandBuffer for submitting work to, call commandBuffer(). To query
    information about the active render target, call renderTarget(). See the
    \l{{Scene Graph - Custom QSGRenderNode}} example for details.

    \since 6.0
 */
void QSGRenderNode::prepare()
{
}

/*!
    \fn void QSGRenderNode::render(const RenderState *state)

    This function is called by the renderer and should paint this node with
    directly invoking commands in the graphics API (OpenGL, Direct3D, etc.)
    currently in use.

    The effective opacity can be retrieved with \l inheritedOpacity().

    The projection matrix is available through \a state, while the model-view
    matrix can be fetched with \l matrix(). The combined matrix is then the
    projection matrix times the model-view matrix. The correct stacking of the
    items in the scene is ensured by the projection matrix.

    When using the provided matrices, the coordinate system for vertex data
    follows the usual QQuickItem conventions: top-left is (0, 0), bottom-right
    is the corresponding QQuickItem's width() and height() minus one. For
    example, assuming a two float (x-y) per vertex coordinate layout, a
    triangle covering half of the item can be specified as (width - 1, height - 1),
    (0, 0), (0, height - 1) using counter-clockwise direction.

    \note QSGRenderNode is provided as a means to implement custom 2D or 2.5D
    Qt Quick items. It is not intended for integrating true 3D content into the
    Qt Quick scene. That use case is better supported by
    QQuickFramebufferObject, QQuickWindow::beforeRendering(), or the
    equivalents of those for APIs other than OpenGL.

    \note QSGRenderNode can perform significantly better than texture-based
    approaches (such as, QQuickFramebufferObject), especially on systems where
    the fragment processing power is limited. This is because it avoids
    rendering to a texture and then drawing a textured quad. Rather,
    QSGRenderNode allows recording draw calls in line with the scenegraph's
    other commands, avoiding an additional render target and the potentially
    expensive texturing and blending.

    Clip information is calculated before the function is called.
    Implementations wishing to take clipping into account can set up scissoring
    or stencil based on the information in \a state. The stencil buffer is
    filled with the necessary clip shapes, but it is up to the implementation
    to enable stencil testing.

    Some scenegraph backends, software in particular, use no scissor or
    stencil. There the clip region is provided as an ordinary QRegion.

    When implementing a QSGRenderNode that uses QRhi to render, query the QRhi
    object from the QQuickWindow via \l{QQuickWindow::rhi()}. To get a
    QRhiCommandBuffer for submitting work to, call commandBuffer(). To query
    information about the active render target, call renderTarget(). See the
    \l{{Scene Graph - Custom QSGRenderNode}} example for details.

    With Qt 6 and its QRhi-based scene graph renderer, no assumptions should be
    made about the active (OpenGL) state when this function is called, even
    when OpenGL is in use. Assume nothing about the pipelines and dynamic
    states bound on the command list/buffer when this function is called.

    \note Depth writes are expected to be disabled. Enabling depth writes can
    lead to unexpected results, depending on the scenegraph backend in use and
    the content in the scene, so exercise caution with this.

    \note In Qt 6, \l changedStates() has limited use. See the documentation
    for changedStates() for more information.

    With some graphics APIs, including when using QRhi directly, it can be
    necessary to reimplement prepare() in addition, or alternatively connect to
    the QQuickWindow::beforeRendering() signal. These are called/emitted before
    recording the beginning of a renderpass on the command buffer
    (vkCmdBeginRenderPass with Vulkan, or starting to encode via
    MTLRenderCommandEncoder in case of Metal. Recording copy operations cannot
    be done inside render() with such APIs. Rather, do such operations either
    in prepare() or the slot connected to beforeRendering (with
    DirectConnection).

    \sa QSGRendererInterface, QQuickWindow::rendererInterface()
  */

/*!
    This function is called when all custom graphics resources allocated by
    this node have to be freed immediately. In case the node does not directly
    allocate graphics resources (buffers, textures, render targets, fences,
    etc.) through the graphics API that is in use, there is nothing to do here.

    Failing to release all custom resources can lead to incorrect behavior in
    graphics device loss scenarios on some systems since subsequent
    reinitialization of the graphics system may fail.

    \note Some scenegraph backends may choose not to call this function.
    Therefore it is expected that QSGRenderNode implementations perform cleanup
    both in their destructor and in releaseResources().

    Unlike with the destructor, it is expected that render() can reinitialize
    all resources it needs when called after a call to releaseResources().

    With OpenGL, the scenegraph's OpenGL context will be current both when
    calling the destructor and this function.
 */
void QSGRenderNode::releaseResources()
{
}

/*!
  \enum QSGRenderNode::StateFlag

  This enum is a bit mask identifying several states.

  \value DepthState         Depth
  \value StencilState       Stencil
  \value ScissorState       Scissor
  \value ColorState         Color
  \value BlendState         Blend
  \value CullState          Cull
  \value ViewportState      View poirt
  \value RenderTargetState  Render target

 */

/*!
    \enum QSGRenderNode::RenderingFlag

    Possible values for the bitmask returned from flags().

    \value BoundedRectRendering Indicates that the implementation of render()
    does not render outside the area reported from rect() in item
    coordinates. Such node implementations can lead to more efficient rendering,
    depending on the scenegraph backend. For example, the \c software backend can
    continue to use the more optimal partial update path when all render nodes
    in the scene have this flag set.

    \value DepthAwareRendering Indicates that the implementations of render()
    conforms to scenegraph expectations by only generating a Z value of 0 in
    scene coordinates which is then transformed by the matrices retrieved from
    RenderState::projectionMatrix() and matrix(), as described in the notes for
    render(). Such node implementations can lead to more efficient rendering,
    depending on the scenegraph backend. For example, the batching OpenGL
    renderer can continue to use a more optimal path when all render nodes in
    the scene have this flag set.

    \value OpaqueRendering Indicates that the implementation of render() writes
    out opaque pixels for the entire area reported from rect(). By default the
    renderers must assume that render() can also output semi or fully
    transparent pixels. Setting this flag can improve performance in some
    cases.

    \value NoExternalRendering Indicates that the implementation of prepare()
    and render() use the QRhi family of APIs, instead of directly calling a 3D
    API such as OpenGL, Vulkan, or Metal.

    \sa render(), prepare(), rect(), QRhi
 */

/*!
    \return flags describing the behavior of this render node.

    The default implementation returns 0.

    \sa RenderingFlag, rect()
 */
QSGRenderNode::RenderingFlags QSGRenderNode::flags() const
{
    return {};
}

/*!
    \return the bounding rectangle in item coordinates for the area render()
    touches. The value is only in use when flags() includes
    BoundedRectRendering, ignored otherwise.

    Reporting the rectangle in combination with BoundedRectRendering is
    particularly important with the \c software backend because otherwise
    having a rendernode in the scene would trigger fullscreen updates, skipping
    all partial update optimizations.

    For rendernodes covering the entire area of a corresponding QQuickItem the
    return value will be (0, 0, item->width(), item->height()).

    \note Nodes are also free to render outside the boundaries specified by the
    item's width and height, since the scenegraph nodes are not bounded by the
    QQuickItem geometry, as long as this is reported correctly from this function.

    \sa flags()
*/
QRectF QSGRenderNode::rect() const
{
    return QRectF();
}

/*!
    \return pointer to the current projection matrix.

    In render() this is the same matrix that is returned from
    RenderState::projectionMatrix(). This getter exists so that prepare() also
    has a way to query the projection matrix.

    When working with a modern graphics API, or Qt's own graphics abstraction
    layer, it is more than likely that one will want to load
    \c{*projectionMatrix() * *matrix()} into a uniform buffer. That is however
    something that needs to be done in prepare(), so outside the recording of a
    render pass. That is why both matrices are queriable directly from the
    QSGRenderNode, both in prepare() and render().

    \since 6.5
 */
const QMatrix4x4 *QSGRenderNode::projectionMatrix() const
{
    return &d->m_projectionMatrix[0];
}

/*!
    \internal
 */
const QMatrix4x4 *QSGRenderNode::projectionMatrix(qsizetype index) const
{
    return &d->m_projectionMatrix[index];
}

/*!
    \return pointer to the current model-view matrix.
 */
const QMatrix4x4 *QSGRenderNode::matrix() const
{
    return d->m_matrix;
}

/*!
    \return the current clip list.
 */
const QSGClipNode *QSGRenderNode::clipList() const
{
    return d->m_clip_list;
}

/*!
    \return the current effective opacity.
 */
qreal QSGRenderNode::inheritedOpacity() const
{
    return d->m_opacity;
}

/*!
    \return the current render target.

    This is provided mainly to enable prepare() and render() implementations
    that use QRhi accessing the QRhiRenderTarget's
    \l{QRhiRenderPassDescriptor}{renderPassDescriptor} or
    \l{QRhiRenderTarget::pixelSize()}{pixel size}.

    To build a QRhiGraphicsPipeline, which implies having to provide a
    QRhiRenderPassDescriptor, query the renderPassDescriptor from the render
    target. Be aware however that the render target may change over the
    lifetime of the custom QQuickItem and the QSGRenderNode. For example,
    consider what happens when dynamically setting \c{layer.enabled: true} on
    the item or an ancestor of it: this triggers rendering into a texture, not
    directly to the window, which means the QSGRenderNode is going to work with
    a different render target from then on. The new render target may then have
    a different pixel format, which can make already built graphics pipelines
    incompatible. This can be handled with logic such as the following:

    \code
      if (m_pipeline && renderTarget()->renderPassDescriptor()->serializedFormat() != m_renderPassFormat) {
          delete m_pipeline;
          m_pipeline = nullptr;
      }
      if (!m_pipeline) {
          // Build a new QRhiGraphicsPipeline.
          // ...
          // Store the serialized format for fast and simple comparisons later on.
          m_renderPassFormat = renderTarget()->renderPassDescriptor()->serializedFormat();
      }
    \endcode

    \since 6.6

    \sa commandBuffer()
 */
QRhiRenderTarget *QSGRenderNode::renderTarget() const
{
    return d->m_rt.rt;
}

/*!
    \return the current command buffer.

    \since 6.6

    \sa renderTarget()
 */
QRhiCommandBuffer *QSGRenderNode::commandBuffer() const
{
    return d->m_rt.cb;
}

QSGRenderNode::RenderState::~RenderState()
{
}

/*!
    \class QSGRenderNode::RenderState
    \brief Provides information about the projection matrix and clipping.
    \inmodule QtQuick

    The render state contains information for the renderer when invoking
    commands to the scenegraph backends.

    \sa QSGRenderNode::render()
 */

/*!
    \fn const QMatrix4x4 *QSGRenderNode::RenderState::projectionMatrix() const

    \return pointer to the current projection matrix.

    The model-view matrix can be retrieved with QSGRenderNode::matrix().
    Typically \c{projection * modelview} is the matrix that is then used in the
    vertex shader to transform the vertices.
 */

/*!
    \fn const QMatrix4x4 *QSGRenderNode::RenderState::scissorRect() const

    \return the current scissor rectangle when clipping is active. x and y are
    the bottom left coordinates.
 */

/*!
    \fn const QMatrix4x4 *QSGRenderNode::RenderState::scissorEnabled() const

    \return the current state of scissoring.

    \note Only relevant for graphics APIs that have a dedicated on/off state of
    scissoring.
 */

/*!
    \fn const QMatrix4x4 *QSGRenderNode::RenderState::stencilValue() const

    \return the current stencil reference value when clipping is active.
 */

/*!
    \fn const QMatrix4x4 *QSGRenderNode::RenderState::stencilEnabled() const

    \return the current state of stencil testing.

    \note With graphics APIs where stencil testing is enabled in pipeline state
    objects, instead of individual state-setting commands, it is up to the
    implementation of render() to enable stencil testing with operations
    \c KEEP, comparison function \c EQUAL, and a read and write mask of \c 0xFF.
 */

/*!
    \fn const QRegion *QSGRenderNode::RenderState::clipRegion() const

    \return the current clip region or null for backends where clipping is
    implemented via stencil or scissoring.

    The \c software backend uses no projection, scissor or stencil, meaning most
    of the render state is not in use. However, the clip region that can be set
    on the QPainter still has to be communicated since reconstructing this
    manually in render() is not reasonable. It can therefore be queried via
    this function. The region is in world coordinates and can be passed
    to QPainter::setClipRegion() with Qt::ReplaceClip. This must be done before
    calling QPainter::setTransform() since the clip region is already mapped to
    the transform provided in QSGRenderNode::matrix().
 */

/*!
    \return pointer to a \a state value.

    Reserved for future use.
 */
void *QSGRenderNode::RenderState::get(const char *state) const
{
    Q_UNUSED(state);
    return nullptr;
}

QT_END_NAMESPACE
