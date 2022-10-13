// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

    QSGMaterialShader represents a combination of vertex and fragment shaders,
    data that define the graphics pipeline state changes, and logic that
    updates graphics resources, such as uniform buffers and textures.

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.

    The QSGMaterial and QSGMaterialShader form a tight relationship. For one
    scene graph (including nested graphs), there is one unique
    QSGMaterialShader instance that encapsulates the shaders and other data
    the scene graph uses to render an object with that material. Each
    QSGGeometryNode can have a unique QSGMaterial that defines how the graphics
    pipeline must be configured while drawing the node. An instance of
    QSGMaterialShader is never created explicitly by the user, it will be
    created on demand by the scene graph through QSGMaterial::createShader().
    The scene graph creates an instance of QSGMaterialShader by calling the
    QSGMaterial::createShader() method, ensuring that there is only one
    instance of each shader implementation.

    In Qt 5, QSGMaterialShader was tied to OpenGL. It was built directly on
    QOpenGLShaderProgram and had functions like \c updateState() that could
    issue arbitrary OpenGL commands. This is no longer the case in Qt 6.
    QSGMaterialShader is not strictly data-oriented, meaning it provides data
    (shaders and the desired pipeline state changes) together with logic that
    updates data in a uniform buffer. Graphics API access is not provided.  This
    means that a QSGMaterialShader cannot make OpenGL, Vulkan, Metal, or Direct
    3D calls on its own. Together with the unified shader management, this
    allows a QSGMaterialShader to be written once, and be functional with any of
    the supported graphics APIs at run time.

    The shaders set by calling the protected setShaderFileName() function
    control what material does with the vertex data from the geometry, and how
    the fragments are shaded. A QSGMaterialShader will typically set a vertex
    and a fragment shader during construction. Changing the shaders afterwards
    may not lead to the desired effect and must be avoided.

    In Qt 6, the default approach is to ship \c{.qsb} files with the application,
    typically embedded via the resource system, and referenced when calling
    setShaderFileName(). The \c{.qsb} files are generated offline, or at latest
    at application build time, from Vulkan-style GLSL source code using the \c
    qsb tool from the Qt Shader Tools module.

    There are three virtuals that can be overridden. These provide the data, or
    the logic to generate the data, for uniform buffers, textures, and pipeline
    state changes.

    updateUniformData() is the function that is most commonly reimplemented in
    subclasses. This function is expected to update the contents of a
    QByteArray that will then be exposed to the shaders as a uniform buffer.
    Any QSGMaterialShader that has a uniform block in its vertex or fragment
    shader must reimplement updateUniformData().

    updateSampledImage() is relevant when the shader code samples textures. The
    function will be invoked for each sampler (or combined image sampler, in
    APIs where relevant), giving it the option to specify which QSGTexture
    should be exposed to the shader.

    The shader pipeline state changes are less often used. One use case is
    materials that wish to use a specific blend mode. The relevant function is
    updateGraphicsPipelineState(). This function is not called unless the
    QSGMaterialShader has opted in by setting the flag
    UpdatesGraphicsPipelineState. The task of the function is to update the
    GraphicsPipelineState struct instance that is passed to it with the
    desired changes. Currently only blending and culling-related features are
    available, other states cannot be controlled by materials.

    A minimal example, that also includes texture support, could be the
    following. Here we assume that Material is the QSGMaterial that creates an
    instance of Shader in its \l{QSGMaterial::createShader()}{createShader()},
    and that it holds a QSGTexture we want to sample in the fragment shader. The
    vertex shader relies only on the modelview-projection matrix.

    \code
        class Shader : public QSGMaterialShader
        {
        public:
            Shader()
            {
                setShaderFileName(VertexStage, QLatin1String(":/materialshader.vert.qsb"));
                setShaderFileName(FragmentStage, QLatin1String(":/materialshader.frag.qsb"));
            }

            bool updateUniformData(RenderState &state, QSGMaterial *, QSGMaterial *)
            {
                bool changed = false;
                QByteArray *buf = state.uniformData();
                if (state.isMatrixDirty()) {
                    const QMatrix4x4 m = state.combinedMatrix();
                    memcpy(buf->data(), m.constData(), 64);
                    changed = true;
                }
                return changed;
            }

            void updateSampledImage(RenderState &, int binding, QSGTexture **texture, QSGMaterial *newMaterial, QSGMaterial *)
            {
                Material *mat = static_cast<Material *>(newMaterial);
                if (binding == 1)
                    *texture = mat->texture();
            }
        };
    \endcode

    The Vulkan-style GLSL source code for the shaders could look like the
    following. These are expected to be preprocessed offline using the \c qsb
    tool, which generates the \c{.qsb} files referenced in the Shader()
    constructor.

    \badcode
        #version 440
        layout(location = 0) in vec4 aVertex;
        layout(location = 1) in vec2 aTexCoord;
        layout(location = 0) out vec2 vTexCoord;
        layout(std140, binding = 0) uniform buf {
            mat4 qt_Matrix;
        } ubuf;
        out gl_PerVertex { vec4 gl_Position; };
        void main() {
            gl_Position = ubuf.qt_Matrix * aVertex;
            vTexCoord = aTexCoord;
        }
    \endcode

    \badcode
        #version 440
        layout(location = 0) in vec2 vTexCoord;
        layout(location = 0) out vec4 fragColor;
        layout(binding = 1) uniform sampler2D srcTex;
        void main() {
            vec4 c = texture(srcTex, vTexCoord);
            fragColor = vec4(c.rgb * 0.5, 1.0);
        }
    \endcode

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.

    \sa QSGMaterial, {Scene Graph - Custom Material}, {Scene Graph - Two Texture Providers}, {Scene Graph - Graph}
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
        textureBindingTable[i].clear();
    for (int i = 0; i < MAX_SHADER_RESOURCE_BINDINGS; ++i)
        samplerBindingTable[i].clear();
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
    memset(static_cast<void *>(combinedImageSamplerBindings), 0, sizeof(combinedImageSamplerBindings));
    memset(static_cast<void *>(combinedImageSamplerCount), 0, sizeof(combinedImageSamplerCount));
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
        const int ubufCount = ubufs.size();
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
        const int imageSamplersCount = imageSamplers.size();
        for (int i = 0; i < imageSamplersCount; ++i) {
            const QShaderDescription::InOutVariable &var(imageSamplers[i]);

            if (var.binding < 0)
                continue;

            if (var.binding < MAX_SHADER_RESOURCE_BINDINGS) {
                combinedImageSamplerBindings[var.binding] |= toSrbStage(it->shader.stage());

                int count = 1;
                for (int dim : var.arrayDims)
                    count *= dim;

                combinedImageSamplerCount[var.binding] = count;
            } else {
                qWarning("Encountered invalid combined image sampler (%s) binding %d",
                         var.name.constData(), var.binding);
            }
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
        Q_UNREACHABLE_RETURN(QShader::VertexStage);
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
    Sets the \a flags for this material shader.
 */
void QSGMaterialShader::setFlags(Flags flags)
{
    Q_D(QSGMaterialShader);
    d->flags = flags;
}

/*!
    Returns the number of elements in the combined image sampler variable at \a
    binding. This value is introspected from the shader code.  The variable may
    be an array, and may have more than one dimension.

    The count reflects the total number of combined image sampler items in the
    variable. In the following example, the count for \c{srcA} is 1, \c{srcB}
    is 4, and \c{srcC} is 6.

    \badcode
    layout (binding = 0) uniform sampler2D srcA;
    layout (binding = 1) uniform sampler2D srcB[4];
    layout (binding = 2) uniform sampler2D srcC[2][3];
    \endcode

    This count is the number of QSGTexture pointers in the texture parameter
    of \l{QSGMaterialShader::updateSampledImage}.

    \sa QSGMaterialShader::updateSampledImage
    \since 6.4
 */
int QSGMaterialShader::combinedImageSamplerCount(int binding) const
{
    Q_D(const QSGMaterialShader);

    if (binding >= 0 && binding < d->MAX_SHADER_RESOURCE_BINDINGS)
        return d->combinedImageSamplerCount[binding];

    return 0;
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
    This function is called by the scene graph to prepare use of sampled images
    in the shader, typically in the form of combined image samplers.

    \a binding is the binding number of the sampler. The function is called for
    each combined image sampler variable in the shader code associated with the
    QSGMaterialShader.

    \a{texture} is an array of QSGTexture pointers. The number of elements in
    the array matches the number of elements in the image sampler variable
    specified in the shader code. This variable may be an array, and may have
    more than one dimension.  The number of elements in the array may be
    found via \l{QSGMaterialShader::combinedImageSamplerCount}

    When an element in \a{texture} is null, it must be set to a valid
    QSGTexture pointer before returning. When non-null, it is up to the
    material to decide if a new \c{QSGTexture *} is stored to it, or if it
    updates some parameters on the already known QSGTexture. The ownership of
    the QSGTexture is not transferred.

    The current rendering \a state is passed from the scene graph. Where
    relevant, it is up to the material to trigger enqueuing texture data
    uploads via QSGTexture::commitTextureOperations().

    The subclass specific state can be extracted from \a newMaterial.

    \a oldMaterial can be used to minimize changes. When \a oldMaterial is null,
    this shader was just activated.

    \sa QSGMaterialShader::combinedImageSamplerCount
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

    \value DirtyCachedMaterialData Used to indicate that the cached material
    state has changed and must be updated.

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

    When updateGraphicsPipelineState() is called, the struct has all members
    set to a valid value to reflect the renderer's current state. Not changing
    any values (or not reimplementing the function) indicates that the material
    is fine with the defaults (which are dynamic however, depending on
    QSGMaterial flags, for example).
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
    \enum QSGMaterialShader::GraphicsPipelineState::PolygonMode
    \since 6.4
    \brief Specifies the polygon rasterization mode

    Polygon Mode (Triangle Fill Mode in Metal, Fill Mode in D3D) specifies
    the fill mode used when rasterizing polygons.  Polygons may be drawn as
    solids (Fill), or as a wire mesh (Line).

    \warning OpenGL ES does not support the \c{Line} polygon mode. OpenGL ES
    will rasterize all polygons as filled no matter what polygon mode is set.
    Using \c{Line} will make your application non-portable.

    \value Fill The interior of the polygon is filled (default)
    \value Line Boundary edges of the polygon are drawn as line segments.
 */

/*!
    \variable QSGMaterialShader::GraphicsPipelineState::blendEnable
    \since 5.14
    \brief Enables blending.

    \note Changing this flag should be done with care, and is best avoided.
    Rather, materials should always use the QSGMaterial::Blend flag to indicate
    that they wish to use blending. Changing this value from false to true for
    a material that did not declare QSGMaterial::Blend can lead to unexpected
    visual results.
 */

/*!
    \variable QSGMaterialShader::GraphicsPipelineState::srcColor
    \since 5.14
    \brief Source blending factor, either RGB or RGBA depending on separateBlendFactors.
 */

/*!
    \variable QSGMaterialShader::GraphicsPipelineState::dstColor
    \since 5.14
    \brief Destination blending factor, either RGB or RGBA depending on separateBlendFactors.
 */

/*!
    \variable QSGMaterialShader::GraphicsPipelineState::colorWrite
    \since 5.14
    \brief Color write mask.
 */

/*!
    \variable QSGMaterialShader::GraphicsPipelineState::blendConstant
    \since 5.14
    \brief Blend constant applicable when a blending factor is set to use a constant value.
 */

/*!
    \variable QSGMaterialShader::GraphicsPipelineState::cullMode
    \since 5.14
    \brief Cull mode.
 */

/*!
    \variable QSGMaterialShader::GraphicsPipelineState::polygonMode
    \since 6.4
    \brief Polygon rasterization mode.
 */

/*!
    \variable QSGMaterialShader::GraphicsPipelineState::separateBlendFactors
    \since 6.5
    \brief Indicates that alpha blending factors are specified separately.

    False by default, meaning both RGB and alpha blending factors are defined
    by srcColor and dstColor. When set to true, the alpha blending factors are
    taken from srcAlpha and dstAlpha instead, and srcColor and dstColor applies
    only to RGB.
 */

/*!
    \variable QSGMaterialShader::GraphicsPipelineState::srcAlpha
    \since 6.5
    \brief Source alpha blending factor.

    Applies only when separateBlendFactors is set to true.
 */

/*!
    \variable QSGMaterialShader::GraphicsPipelineState::dstAlpha
    \since 6.5
    \brief Destination alpha blending factor.

    Applies only when separateBlendFactors is set to true.
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
