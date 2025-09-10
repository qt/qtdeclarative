// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgmaterial.h"
#include "qsgrenderer_p.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQsgLeak)

/*!
    \group qtquick-scenegraph-materials
    \title Qt Quick Scene Graph Material Classes
    \brief classes used to define materials in the Qt Quick Scene Graph.

    This page lists the material classes in \l {Qt Quick}'s
    \l {scene graph}{Qt Quick Scene Graph}.
 */

#ifndef QT_NO_DEBUG
static int qt_material_count = 0;

static void qt_print_material_count()
{
    qCDebug(lcQsgLeak, "Number of leaked materials: %i", qt_material_count);
    qt_material_count = -1;
}
#endif

/*!
    \class QSGMaterialType
    \brief The QSGMaterialType class is used as a unique type token in combination with QSGMaterial.
    \inmodule QtQuick
    \ingroup qtquick-scenegraph-materials

    It serves no purpose outside the QSGMaterial::type() function.

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.
 */

/*!
    \class QSGMaterial
    \brief The QSGMaterial class encapsulates rendering state for a shader program.
    \inmodule QtQuick
    \ingroup qtquick-scenegraph-materials

    QSGMaterial and QSGMaterialShader subclasses form a tight relationship. For
    one scene graph (including nested graphs), there is one unique
    QSGMaterialShader instance which encapsulates the shaders the scene graph
    uses to render that material, such as a shader to flat coloring of
    geometry. Each QSGGeometryNode can have a unique QSGMaterial containing the
    how the shader should be configured when drawing that node, such as the
    actual color to used to render the geometry.

    QSGMaterial has two virtual functions that both need to be implemented. The
    function type() should return a unique instance for all instances of a
    specific subclass. The createShader() function should return a new instance
    of QSGMaterialShader, specific to that subclass of QSGMaterial.

    A minimal QSGMaterial implementation could look like this:
    \code
        class Material : public QSGMaterial
        {
        public:
            QSGMaterialType *type() const override { static QSGMaterialType type; return &type; }
            QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override { return new Shader; }
        };
    \endcode

    See the \l{Scene Graph - Custom Material}{Custom Material example} for an introduction
    on implementing a QQuickItem subclass backed by a QSGGeometryNode and a custom
    material.

    \note createShader() is called only once per QSGMaterialType, to reduce
    redundant work with shader preparation. If a QSGMaterial is backed by
    multiple sets of vertex and fragment shader combinations, the implementation
    of type() must return a different, unique QSGMaterialType pointer for each
    combination of shaders.

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.

    \sa QSGMaterialShader, {Scene Graph - Custom Material}, {Scene Graph - Two Texture Providers}, {Scene Graph - Graph}
 */

/*!
    \internal
 */

QSGMaterial::QSGMaterial()
{
    Q_UNUSED(m_reserved);
#ifndef QT_NO_DEBUG
    if (lcQsgLeak().isDebugEnabled()) {
        ++qt_material_count;
        static bool atexit_registered = false;
        if (!atexit_registered) {
            atexit(qt_print_material_count);
            atexit_registered = true;
        }
    }
#endif
}


/*!
    \internal
 */

QSGMaterial::~QSGMaterial()
{
#ifndef QT_NO_DEBUG
    if (lcQsgLeak().isDebugEnabled()) {
        --qt_material_count;
        if (qt_material_count < 0)
            qCDebug(lcQsgLeak, "Material destroyed after qt_print_material_count() was called.");
    }
#endif
}



/*!
    \enum QSGMaterial::Flag

    \value Blending Set this flag to true if the material requires blending to be
    enabled during rendering.

    \value RequiresDeterminant Set this flag to true if the material relies on
    the determinant of the matrix of the geometry nodes for rendering.

    \value RequiresFullMatrixExceptTranslate Set this flag to true if the material
    relies on the full matrix of the geometry nodes for rendering, except the translation part.

    \value RequiresFullMatrix Set this flag to true if the material relies on
    the full matrix of the geometry nodes for rendering.

    \value NoBatching Set this flag to true if the material uses shaders that are
    incompatible with the \l{Qt Quick Scene Graph Default Renderer}{scene graph's batching
    mechanism}. This is relevant in certain advanced usages, such as, directly
    manipulating \c{gl_Position.z} in the vertex shader. Such solutions are often tied to
    a specific scene structure, and are likely not safe to use with arbitrary contents in
    a scene. Thus this flag should only be set after appropriate investigation, and will
    never be needed for the vast majority of materials. Setting this flag can lead to
    reduced performance due to having to issue more draw calls. This flag was introduced
    in Qt 6.3.

    \value CustomCompileStep In Qt 6 this flag is identical to NoBatching. Prefer using
    NoBatching instead.

    \omitvalue MultiView2
    \omitvalue MultiView3
    \omitvalue MultiView4
 */

/*!
    \fn QSGMaterial::Flags QSGMaterial::flags() const

    Returns the material's flags.
 */



/*!
    Sets the flags \a flags on this material if \a on is true;
    otherwise clears the attribute.
*/

void QSGMaterial::setFlag(Flags flags, bool on)
{
    if (on)
        m_flags |= flags;
    else
        m_flags &= ~flags;
}



/*!
    Compares this material to \a other and returns 0 if they are equal; -1 if
    this material should sort before \a other and 1 if \a other should sort
    before.

    The scene graph can reorder geometry nodes to minimize state changes.
    The compare function is called during the sorting process so that
    the materials can be sorted to minimize state changes in each
    call to QSGMaterialShader::updateState().

    The this pointer and \a other is guaranteed to have the same type().
 */

int QSGMaterial::compare(const QSGMaterial *other) const
{
    Q_ASSERT(other && type() == other->type());
    const qintptr diff = qintptr(this) - qintptr(other);
    return diff < 0 ? -1 : (diff > 0 ? 1 : 0);
}



/*!
    \fn QSGMaterialType *QSGMaterial::type() const

    This function is called by the scene graph to query an identifier that is
    unique to the QSGMaterialShader instantiated by createShader().

    For many materials, the typical approach will be to return a pointer to a
    static, and so globally available, QSGMaterialType instance. The
    QSGMaterialType is an opaque object. Its purpose is only to serve as a
    type-safe, simple way to generate unique material identifiers.
    \code
        QSGMaterialType *type() const override
        {
            static QSGMaterialType type;
            return &type;
        }
    \endcode
 */



/*!
    \fn QSGMaterialShader *QSGMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const

    This function returns a new instance of a the QSGMaterialShader
    implementation used to render geometry for a specific implementation
    of QSGMaterial.

    The function will be called only once for each combination of material type and \a renderMode
    and will be cached internally.

    For most materials, the \a renderMode can be ignored. A few materials may need
    custom handling for specific render modes. For instance if the material implements
    antialiasing in a way that needs to account for perspective transformations when
    RenderMode3D is in use.
*/

/*!
    \return The number of views in case of the material is used in multiview
    rendering.

    \note The return value is valid only when called from createShader(), and
    afterwards. The value is not necessarily up-to-date before createShader()
    is invokved by the scene graph.

    Normally the return value is \c 1. A view count greater than 2 implies a
    \e{multiview render pass}. Materials that support multiview are expected to
    query viewCount() in createShader(), or in their QSGMaterialShader
    constructor, and ensure the appropriate shaders are picked. The vertex
    shader is then expected to use
    \c{gl_ViewIndex} to index the modelview-projection matrix array as there
    are multiple matrices in multiview mode. (one for each view)

    As an example, take the following simple vertex shader:

    \badcode
    #version 440

    layout(location = 0) in vec4 vertexCoord;
    layout(location = 1) in vec4 vertexColor;

    layout(location = 0) out vec4 color;

    layout(std140, binding = 0) uniform buf {
        mat4 matrix[2];
        float opacity;
    };

    void main()
    {
        gl_Position = matrix[gl_ViewIndex] * vertexCoord;
        color = vertexColor * opacity;
    }
    \endcode

    This shader is prepared to handle 2 views, and 2 views only. It is not
    compatible with other view counts. When conditioning the shader, the \c qsb
    tool has to be invoked with \c{--view-count 2} or, if using the CMake
    integration,
    \c{VIEW_COUNT 2} must be specified in the \c{qt_add_shaders()} command.

    \note A line with \c{#extension GL_EXT_multiview : require} is injected
    automatically by \c qsb whenever a view count of 2 or greater is set.

    Developers are encouraged to use the automatically injected preprocessor
    variable \c{QSHADER_VIEW_COUNT} to simplify the handling of the different
    number of views. For example, if there is a need to support both
    non-multiview and multiview with a view count of 2 in the same source file,
    the following could be done:

    \badcode
    #version 440

    layout(location = 0) in vec4 vertexCoord;
    layout(location = 1) in vec4 vertexColor;

    layout(location = 0) out vec4 color;

    layout(std140, binding = 0) uniform buf {
    #if QSHADER_VIEW_COUNT >= 2
        mat4 matrix[QSHADER_VIEW_COUNT];
    #else
        mat4 matrix;
    #endif
        float opacity;
    };

    void main()
    {
    #if QSHADER_VIEW_COUNT >= 2
        gl_Position = matrix[gl_ViewIndex] * vertexCoord;
    #else
        gl_Position = matrix * vertexCoord;
    #endif
        color = vertexColor * opacity;
    }
    \endcode

    The same source file can now be run through \c qsb or \c{qt_add_shaders()}
    twice, once without specify the view count, and once with the view count
    set to 2. The material can then pick the appropriate .qsb file based on
    viewCount() at run time.

    With CMake, this could looks similar to the following. With this example
    the corresponding QSGMaterialShader is expected to choose between
    \c{:/shaders/example.vert.qsb} and \c{:/shaders/multiview/example.vert.qsb}
    based on the value of viewCount(). (same goes for the fragment shader)

    \badcode
    qt_add_shaders(application "application_shaders"
        PREFIX
            /
        FILES
            shaders/example.vert
            shaders/example.frag
    )

    qt_add_shaders(application "application_multiview_shaders"
        GLSL
            330,300es
        HLSL
            61
        MSL
            12
        VIEW_COUNT
            2
        PREFIX
            /
        FILES
            shaders/example.vert
            shaders/example.frag
        OUTPUTS
            shaders/multiview/example.vert
            shaders/multiview/example.frag
    )
    \endcode

    \note The fragment shader should be treated the same way the vertex shader
    is, even though the fragment shader code cannot have any dependency on the
    view count (\c{gl_ViewIndex}), for maximum portability. There are two
    reasons for including fragment shaders too in the multiview set. One is that
    mixing different shader versions within the same graphics pipeline can be
    problematic, depending on the underlying graphics API: with D3D12 for
    example, mixing HLSL shaders for shader model 5.0 and 6.1 would generate an
    error. The other is that having \c QSHADER_VIEW_COUNT defined in fragment
    shaders can be very useful, for example when sharing a uniform buffer layout
    between the vertex and fragment stages.

    \note For OpenGL the minimum GLSL version for vertex shaders relying on
    \c{gl_ViewIndex} is \c 330. Lower versions may be accepted at build time,
    but may lead to an error at run time, depending on the OpenGL implementation.

    As a convenience, there is also a \c MULTIVIEW option for qt_add_shaders().
    This first runs the \c qsb tool normally, then overrides \c VIEW_COUNT to
    \c 2, sets \c GLSL, \c HLSL, \c MSL to some suitable defaults, and runs \c
    qsb again, this time outputting .qsb files with a suffix added. The material
    implementation can then use the \l QSGMaterialShader::setShaderFileName()
    overload taking a \c viewCount argument, that automatically picks the
    correct .qsb file.

    The following is therefore mostly equivalent to the example call shown
    above, except that no manually managed output files need to be specified.
    Note that there can be cases when the automatically chosen shading language
    versions are not sufficient, in which case applications should continue
    specify everything explicitly.

    \badcode
    qt_add_shaders(application "application_multiview_shaders"
        MULTIVIEW
        PREFIX
            /
        FILES
            shaders/example.vert
            shaders/example.frag
    )
    \endcode

    See \l QRhi::MultiView, \l QRhiColorAttachment::setMultiViewCount(), and
    \l QRhiGraphicsPipeline::setMultiViewCount() for further, lower-level details
    on multiview support in Qt. The Qt Quick scene graph renderer is prepared to
    recognize multiview render targets, when specified via \l
    QQuickRenderTarget::fromRhiRenderTarget() or the 3D API specific functions,
    such as \l{QQuickRenderTarget::}{fromVulkanImage()} with an \c arraySize
    argument greater than 1. The renderer will then propagate the view count to
    graphics pipelines and the materials.

    \since 6.8
 */
int QSGMaterial::viewCount() const
{
    if (m_flags.testFlag(MultiView4))
        return 4;
    if (m_flags.testFlag(MultiView3))
        return 3;
    if (m_flags.testFlag(MultiView2))
        return 2;
    return 1;
}

QT_END_NAMESPACE
