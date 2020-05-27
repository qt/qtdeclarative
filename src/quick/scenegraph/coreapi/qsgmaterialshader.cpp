/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgmaterial.h"
#include "qsgrenderer_p.h"
#include "qsgmaterialshader_p.h"
#include <QtCore/QFile>

QT_BEGIN_NAMESPACE

/*!
    \class QSGMaterialShader
    \brief The QSGMaterialShader class represents a graphics API independent shader program.
    \inmodule QtQuick
    \ingroup qtquick-scenegraph-materials
    \since 5.14

    // ### glpurge Rewrite to not talk about OpenGL and GLSL directly anymore

    QSGMaterialShader is a modern, cross-platform alternative to
    QSGMaterialShader. The latter is tied to OpenGL and GLSL by design, whereas
    QSGMaterialShader is based on QShader, a container for multiple
    versions of a graphics shader together with reflection information.

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.

    The QSGMaterial and QSGMaterialShader form a tight relationship. For one
    scene graph (including nested graphs), there is one unique QSGMaterialShader
    instance which encapsulates the QOpenGLShaderProgram the scene graph uses
    to render that material, such as a shader to flat coloring of geometry.
    Each QSGGeometryNode can have a unique QSGMaterial containing the
    how the shader should be configured when drawing that node, such as
    the actual color used to render the geometry.

    An instance of QSGMaterialShader is never created explicitly by the user,
    it will be created on demand by the scene graph through
    QSGMaterial::createShader(). The scene graph will make sure that there
    is only one instance of each shader implementation through a scene graph.

    The source code returned from vertexShader() is used to control what the
    material does with the vertiex data that comes in from the geometry.
    The source code returned from the fragmentShader() is used to control
    what how the material should fill each individual pixel in the geometry.
    The vertex and fragment source code is queried once during initialization,
    changing what is returned from these functions later will not have
    any effect.

    The activate() function is called by the scene graph when a shader is
    is starting to be used. The deactivate function is called by the scene
    graph when the shader is no longer going to be used. While active,
    the scene graph may make one or more calls to updateState() which
    will update the state of the shader for each individual geometry to
    render.

    The attributeNames() returns the name of the attributes used in the
    vertexShader(). These are used in the default implementation of
    activate() and deactivate() to decide whice vertex registers are enabled.

    The initialize() function is called during program creation to allow
    subclasses to prepare for use, such as resolve uniform names in the
    vertexShader() and fragmentShader().

    A minimal example:
    \code
        class Shader : public QSGMaterialShader
        {
        public:
            const char *vertexShader() const {
                return
                "attribute highp vec4 vertex;          \n"
                "uniform highp mat4 matrix;            \n"
                "void main() {                         \n"
                "    gl_Position = matrix * vertex;    \n"
                "}";
            }

            const char *fragmentShader() const {
                return
                "uniform lowp float opacity;                            \n"
                "void main() {                                          \n"
                        "    gl_FragColor = vec4(1, 0, 0, 1) * opacity; \n"
                "}";
            }

            char const *const *attributeNames() const
            {
                static char const *const names[] = { "vertex", 0 };
                return names;
            }

            void initialize()
            {
                QSGMaterialShader::initialize();
                m_id_matrix = program()->uniformLocation("matrix");
                m_id_opacity = program()->uniformLocation("opacity");
            }

            void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
            {
                Q_ASSERT(program()->isLinked());
                if (state.isMatrixDirty())
                    program()->setUniformValue(m_id_matrix, state.combinedMatrix());
                if (state.isOpacityDirty())
                    program()->setUniformValue(m_id_opacity, state.opacity());
            }

        private:
            int m_id_matrix;
            int m_id_opacity;
        };
    \endcode

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.
 */

/*!
    \enum QSGMaterialShader::Flag
    Flag values to indicate special material properties.

    \value UpdatesGraphicsPipelineState Setting this flag enables calling
    updateGraphicsPipelineState().
 */

QShader QSGMaterialShaderPrivate::loadShader(const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to find shader" << filename;
        return QShader();
    }
    return QShader::fromSerialized(f.readAll());
}

void QSGMaterialShaderPrivate::clearCachedRendererData()
{
    for (int i = 0; i < MAX_SHADER_RESOURCE_BINDINGS; ++i)
        textureBindingTable[i] = nullptr;
    for (int i = 0; i < MAX_SHADER_RESOURCE_BINDINGS; ++i)
        samplerBindingTable[i] = nullptr;
}

static inline QRhiShaderResourceBinding::StageFlags toSrbStage(QShader::Stage stage)
{
    switch (stage) {
    case QShader::VertexStage:
        return QRhiShaderResourceBinding::VertexStage;
    case QShader::FragmentStage:
        return QRhiShaderResourceBinding::FragmentStage;
    default:
        Q_UNREACHABLE();
        break;
    }
    return { };
}

void QSGMaterialShaderPrivate::prepare(QShader::Variant vertexShaderVariant)
{
    ubufBinding = -1;
    ubufSize = 0;
    ubufStages = { };
    memset(combinedImageSamplerBindings, 0, sizeof(combinedImageSamplerBindings));
    vertexShader = fragmentShader = nullptr;
    masterUniformData.clear();

    clearCachedRendererData();

    for (QShader::Stage stage : { QShader::VertexStage, QShader::FragmentStage }) {
        auto it = shaderFileNames.find(stage);
        if (it != shaderFileNames.end()) {
            QString fn = *it;
            const QShader s = loadShader(*it);
            if (!s.isValid())
                continue;
            shaders[stage] = ShaderStageData(s);
            // load only once, subsequent prepare() calls will have it all in shaders already
            shaderFileNames.erase(it);
        }
    }

    auto vsIt = shaders.find(QShader::VertexStage);
    if (vsIt != shaders.end()) {
        vsIt->shaderVariant = vertexShaderVariant;
        vsIt->vertexInputLocations.clear();
        vsIt->qt_order_attrib_location = -1;

        const QShaderDescription desc = vsIt->shader.description();
        const QVector<QShaderDescription::InOutVariable> vertexInputs = desc.inputVariables();
        for (const QShaderDescription::InOutVariable &v : vertexInputs) {
            if (vertexShaderVariant == QShader::BatchableVertexShader
                && v.name == QByteArrayLiteral("_qt_order")) {
                vsIt->qt_order_attrib_location = v.location;
            } else {
                vsIt->vertexInputLocations.append(v.location);
            }
        }

        if (vsIt->vertexInputLocations.contains(vsIt->qt_order_attrib_location)) {
            qWarning("Vertex input clash in rewritten (batchable) vertex shader at input location %d. "
                     "Vertex shaders must avoid using this location.", vsIt->qt_order_attrib_location);
        }
    }

    for (auto it = shaders.begin(); it != shaders.end(); ++it) {
        const QShaderDescription desc = it->shader.description();

        const QVector<QShaderDescription::UniformBlock> ubufs = desc.uniformBlocks();
        const int ubufCount = ubufs.count();
        if (ubufCount > 1) {
            qWarning("Multiple uniform blocks found in shader. "
                     "This should be avoided as Qt Quick supports only one.");
        }
        for (int i = 0; i < ubufCount; ++i) {
            const QShaderDescription::UniformBlock &ubuf(ubufs[i]);
            if (ubufBinding == -1 && ubuf.binding >= 0) {
                ubufBinding = ubuf.binding;
                ubufSize = ubuf.size;
                ubufStages |= toSrbStage(it->shader.stage());
                masterUniformData.fill('\0', ubufSize);
            } else if (ubufBinding == ubuf.binding && ubuf.binding >= 0) {
                if (ubuf.size > ubufSize) {
                    ubufSize = ubuf.size;
                    masterUniformData.fill('\0', ubufSize);
                }
                ubufStages |= toSrbStage(it->shader.stage());
            } else {
                qWarning("Uniform block %s (binding %d) ignored", ubuf.blockName.constData(),
                         ubuf.binding);
            }
        }

        const QVector<QShaderDescription::InOutVariable> imageSamplers = desc.combinedImageSamplers();
        const int imageSamplersCount = imageSamplers.count();
        for (int i = 0; i < imageSamplersCount; ++i) {
            const QShaderDescription::InOutVariable &var(imageSamplers[i]);
            if (var.binding >= 0 && var.binding < MAX_SHADER_RESOURCE_BINDINGS)
                combinedImageSamplerBindings[var.binding] |= toSrbStage(it->shader.stage());
            else
                qWarning("Encountered invalid combined image sampler (%s) binding %d",
                         var.name.constData(), var.binding);
        }

        if (it.key() == QShader::VertexStage)
            vertexShader = &it.value();
        else if (it.key() == QShader::FragmentStage)
            fragmentShader = &it.value();
    }

    if (vertexShader && vertexShaderVariant == QShader::BatchableVertexShader && vertexShader->qt_order_attrib_location == -1)
        qWarning("No rewriter-inserted attribute found, this should not happen.");
}

/*!
    Constructs a new QSGMaterialShader.
 */
QSGMaterialShader::QSGMaterialShader()
    : d_ptr(new QSGMaterialShaderPrivate(this))
{
}

/*!
    \internal
 */
QSGMaterialShader::QSGMaterialShader(QSGMaterialShaderPrivate &dd)
    : d_ptr(&dd)
{
}

/*!
    \internal
 */
QSGMaterialShader::~QSGMaterialShader()
{
}

// We have our own enum as QShader is not initially public. Internally
// everything works with QShader::Stage however. So convert.
static inline QShader::Stage toShaderStage(QSGMaterialShader::Stage stage)
{
    switch (stage) {
    case QSGMaterialShader::VertexStage:
        return QShader::VertexStage;
    case QSGMaterialShader::FragmentStage:
        return QShader::FragmentStage;
    default:
        Q_UNREACHABLE();
        return QShader::VertexStage;
    }
}

/*!
    Sets the \a shader for the specified \a stage.
 */
void QSGMaterialShader::setShader(Stage stage, const QShader &shader)
{
    Q_D(QSGMaterialShader);
    d->shaders[toShaderStage(stage)] = QSGMaterialShaderPrivate::ShaderStageData(shader);
}

/*!
    Sets the \a filename for the shader for the specified \a stage.

    The file is expected to contain a serialized QShader.
 */
void QSGMaterialShader::setShaderFileName(Stage stage, const QString &filename)
{
    Q_D(QSGMaterialShader);
    d->shaderFileNames[toShaderStage(stage)] = filename;
}

/*!
    \return the currently set flags for this material shader.
 */
QSGMaterialShader::Flags QSGMaterialShader::flags() const
{
    Q_D(const QSGMaterialShader);
    return d->flags;
}

/*!
    Sets the \a flags on this material shader if \a on is true;
    otherwise clears the specified flags.
*/
void QSGMaterialShader::setFlag(Flags flags, bool on)
{
    Q_D(QSGMaterialShader);
    if (on)
        d->flags |= flags;
    else
        d->flags &= ~flags;
}

/*!
    This function is called by the scene graph to get the contents of the
    shader program's uniform buffer updated. The implementation is not expected
    to perform any real graphics operations, it is merely responsible for
    copying data to the QByteArray returned from RenderState::uniformData().
    The scene graph takes care of making that buffer visible in the shaders.

    The current rendering \a state is passed from the scene graph. If the state
    indicates that any relevant state is dirty, the implementation must update
    the appropriate region in the buffer data that is accessible via
    RenderState::uniformData(). When a state, such as, matrix or opacity, is
    not dirty, there is no need to touch the corresponding region since the
    data is persistent.

    The return value must be \c true whenever any change was made to the uniform data.

    The subclass specific state, such as the color of a flat color material,
    should be extracted from \a newMaterial to update the relevant regions in
    the buffer accordingly.

    \a oldMaterial can be used to minimize buffer changes (which are typically
    memcpy calls) when updating material states. When \a oldMaterial is null,
    this shader was just activated.
 */
bool QSGMaterialShader::updateUniformData(RenderState &state,
                                          QSGMaterial *newMaterial,
                                          QSGMaterial *oldMaterial)
{
    Q_UNUSED(state);
    Q_UNUSED(newMaterial);
    Q_UNUSED(oldMaterial);
    return false;
}

/*!
    This function is called by the scene graph to prepare using a sampled image
    in the shader, typically in form of a combined image sampler.

    \a binding is the binding number of the sampler. The function is called for
    each variable in the material's shaders'
    \l{QShaderDescription::combinedImageSamplers()}.

    When *\a{texture} is null, it must be set to a QSGTexture pointer before
    returning. When non-null, it is up to the material to decide if a new
    \c{QSGTexture *} is stored to it, or if it updates some parameters on the
    already known QSGTexture. The ownership of the QSGTexture is not
    transferred.

    The current rendering \a state is passed from the scene graph. It is up to
    the material to enqueue the texture data uploads to the
    QRhiResourceUpdateBatch retriveable via RenderState::resourceUpdateBatch().

    The subclass specific state can be extracted from \a newMaterial.

    \a oldMaterial can be used to minimize changes. When \a oldMaterial is null,
    this shader was just activated.
 */
void QSGMaterialShader::updateSampledImage(RenderState &state,
                                           int binding,
                                           QSGTexture **texture,
                                           QSGMaterial *newMaterial,
                                           QSGMaterial *oldMaterial)
{
    Q_UNUSED(state);
    Q_UNUSED(binding);
    Q_UNUSED(texture);
    Q_UNUSED(newMaterial);
    Q_UNUSED(oldMaterial);
}

/*!
    This function is called by the scene graph to enable the material to
    provide a custom set of graphics state. The set of states that are
    customizable by material is limited to blending and related settings.

    \note This function is only called when the UpdatesGraphicsPipelineState
    flag was enabled via setFlags(). By default it is not set, and so this
    function is never called.

    The return value must be \c true whenever a change was made to any of the
    members in \a ps.

    \note The contents of \a ps is not persistent between invocations of this
    function.

    The current rendering \a state is passed from the scene graph.

    The subclass specific state can be extracted from \a newMaterial. When \a
    oldMaterial is null, this shader was just activated.
 */
bool QSGMaterialShader::updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps,
                                                    QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(state);
    Q_UNUSED(ps);
    Q_UNUSED(newMaterial);
    Q_UNUSED(oldMaterial);
    return false;
}

/*!
    \class QSGMaterialShader::RenderState

    \brief Encapsulates the current rendering state during a call to
    QSGMaterialShader::updateUniformData() and the other \c update type of
    functions.

    \inmodule QtQuick
    \since 5.14

    The render state contains a number of accessors that the shader needs to
    respect in order to conform to the current state of the scene graph.
 */

/*!
    \enum QSGMaterialShader::RenderState::DirtyState

    \value DirtyMatrix Used to indicate that the matrix has changed and must be
    updated.

    \value DirtyOpacity Used to indicate that the opacity has changed and must
    be updated.

    \value DirtyAll Used to indicate that everything needs to be updated.
 */

/*!
    \fn bool QSGMaterialShader::RenderState::isMatrixDirty() const

    Returns \c true if the dirtyStates() contain the dirty matrix state,
    otherwise returns \c false.
 */

/*!
    \fn bool QSGMaterialShader::RenderState::isOpacityDirty() const

    Returns \c true if the dirtyStates() contains the dirty opacity state,
    otherwise returns \c false.
 */

/*!
    \fn QSGMaterialShader::RenderState::DirtyStates QSGMaterialShader::RenderState::dirtyStates() const

    Returns which rendering states that have changed and needs to be updated
    for geometry rendered with this material to conform to the current
    rendering state.
 */

/*!
    \class QSGMaterialShader::GraphicsPipelineState

    \brief Describes state changes that the material wants to apply to the
    currently active graphics pipeline state.

    \inmodule QtQuick
    \since 5.14

    Unlike QSGMaterialShader, directly issuing state change commands with the
    underlying graphics API is not possible with QSGMaterialShader. This is
    mainly because the concept of individually changeable states is considered
    deprecated and not supported with modern graphics APIs.

    Therefore, it is up to QSGMaterialShader to expose a data structure with
    the set of supported states, which the material can change in its
    updatePipelineState() implementation, if there is one. The scenegraph will
    then internally apply these changes to the active graphics pipeline state,
    then rolling them back as appropriate.
 */

/*!
    \enum QSGMaterialShader::GraphicsPipelineState::BlendFactor
    \since 5.14

    \value Zero
    \value One
    \value SrcColor
    \value OneMinusSrcColor
    \value DstColor
    \value OneMinusDstColor
    \value SrcAlpha
    \value OneMinusSrcAlpha
    \value DstAlpha
    \value OneMinusDstAlpha
    \value ConstantColor
    \value OneMinusConstantColor
    \value ConstantAlpha
    \value OneMinusConstantAlpha
    \value SrcAlphaSaturate
    \value Src1Color
    \value OneMinusSrc1Color
    \value Src1Alpha
    \value OneMinusSrc1Alpha
 */

/*!
    \enum QSGMaterialShader::GraphicsPipelineState::ColorMaskComponent
    \since 5.14

    \value R
    \value G
    \value B
    \value A
 */

/*!
    \enum QSGMaterialShader::GraphicsPipelineState::CullMode
    \since 5.14

    \value CullNone
    \value CullFront
    \value CullBack
 */

/*!
    Returns the accumulated opacity to be used for rendering.
 */
float QSGMaterialShader::RenderState::opacity() const
{
    Q_ASSERT(m_data);
    return float(static_cast<const QSGRenderer *>(m_data)->currentOpacity());
}

/*!
    Returns the modelview determinant to be used for rendering.
 */
float QSGMaterialShader::RenderState::determinant() const
{
    Q_ASSERT(m_data);
    return float(static_cast<const QSGRenderer *>(m_data)->determinant());
}

/*!
    Returns the matrix combined of modelview matrix and project matrix.
 */
QMatrix4x4 QSGMaterialShader::RenderState::combinedMatrix() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentCombinedMatrix();
}

/*!
   Returns the ratio between physical pixels and device-independent pixels
   to be used for rendering.
*/
float QSGMaterialShader::RenderState::devicePixelRatio() const
{
    Q_ASSERT(m_data);
    return float(static_cast<const QSGRenderer *>(m_data)->devicePixelRatio());
}

/*!
    Returns the model view matrix.

    If the material has the RequiresFullMatrix flag set, this is guaranteed to
    be the complete transform matrix calculated from the scenegraph.

    However, if this flag is not set, the renderer may choose to alter this
    matrix. For example, it may pre-transform vertices on the CPU and set this
    matrix to identity.

    In a situation such as the above, it is still possible to retrieve the
    actual matrix determinant by setting the RequiresDeterminant flag in the
    material and calling the determinant() accessor.
 */
QMatrix4x4 QSGMaterialShader::RenderState::modelViewMatrix() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentModelViewMatrix();
}

/*!
    Returns the projection matrix.
 */
QMatrix4x4 QSGMaterialShader::RenderState::projectionMatrix() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentProjectionMatrix();
}

/*!
    Returns the viewport rect of the surface being rendered to.
 */
QRect QSGMaterialShader::RenderState::viewportRect() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->viewportRect();
}

/*!
    Returns the device rect of the surface being rendered to
 */
QRect QSGMaterialShader::RenderState::deviceRect() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->deviceRect();
}

/*!
    Returns a pointer to the data for the uniform (constant) buffer in the
    shader. Uniform data must only be updated from
    QSGMaterialShader::updateUniformData(). The return value is null in the
    other reimplementable functions, such as,
    QSGMaterialShader::updateSampledImage().

    \note It is strongly recommended to declare the uniform block with \c
    std140 in the shader, and to carefully study the standard uniform block
    layout as described in section 7.6.2.2 of the OpenGL specification. It is
    up to the QSGMaterialShader implementation to ensure data gets placed
    at the right location in this QByteArray, taking alignment requirements
    into account. Shader code translated to other shading languages is expected
    to use the same offsets for block members, even when the target language
    uses different packing rules by default.

    \note Avoid copying from C++ POD types, such as, structs, in order to
    update multiple members at once, unless it has been verified that the
    layouts of the C++ struct and the GLSL uniform block match.
 */
QByteArray *QSGMaterialShader::RenderState::uniformData()
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentUniformData();
}

/*!
    Returns a resource update batch to which upload and copy operatoins can be
    queued. This is typically used by
    QSGMaterialShader::updateSampledImage() to enqueue texture image
    content updates.
 */
QRhiResourceUpdateBatch *QSGMaterialShader::RenderState::resourceUpdateBatch()
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentResourceUpdateBatch();
}

/*!
    Returns the current QRhi.
 */
QRhi *QSGMaterialShader::RenderState::rhi()
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentRhi();
}

QT_END_NAMESPACE
