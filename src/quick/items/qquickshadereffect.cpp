// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qquickshadereffect_p_p.h>
#include <private/qsgcontextplugin_p.h>
#include <private/qsgrhisupport_p.h>
#include <private/qquickwindow_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ShaderEffect
    \instantiates QQuickShaderEffect
    \inqmlmodule QtQuick
    \inherits Item
    \ingroup qtquick-effects
    \brief Applies custom shaders to a rectangle.

    The ShaderEffect type applies a custom \l{vertexShader}{vertex} and
    \l{fragmentShader}{fragment (pixel)} shader to a rectangle. It allows
    adding effects such as drop shadow, blur, colorize and page curl into the
    QML scene.

    \note Depending on the Qt Quick scenegraph backend in use, the ShaderEffect
    type may not be supported. For example, with the \c software backend
    effects will not be rendered at all.

    \section1 Shaders

    In Qt 5, effects were provided in form of GLSL (OpenGL Shading Language)
    source code, often embedded as strings into QML. Starting with Qt 5.8,
    referring to files, either local ones or in the Qt resource system, became
    possible as well.

    In Qt 6, Qt Quick has support for graphics APIs, such as Vulkan, Metal, and
    Direct3D 11 as well. Therefore, working with GLSL source strings is no
    longer feasible. Rather, the new shader pipeline is based on compiling
    Vulkan-compatible GLSL code into \l{https://www.khronos.org/spir/}{SPIR-V},
    followed by gathering reflection information and translating into other
    shading languages, such as HLSL, the Metal Shading Language, and various
    GLSL versions. The resulting assets are packed together into a single
    package, typically stored in files with an extension of \c{.qsb}. This
    process is done offline or at application build time at latest. At run
    time, the scene graph and the underlying graphics abstraction consumes
    these \c{.qsb} files. Therefore, ShaderEffect expects file (local or qrc)
    references in Qt 6 in place of inline shader code.

    The \l vertexShader and \l fragmentShader properties are URLs in Qt 6, and
    work very similarly to \l{Image::source}{Image.source}, for example. Only
    the \c file and \c qrc schemes are supported with ShaderEffect, however. It
    is also possible to omit the \c file scheme, allowing to specify a relative
    path in a convenient way. Such a path is resolved relative to the
    component's (the \c{.qml} file's) location.

    \section1 Shader Inputs and Resources

    There are two types of input to the \l vertexShader: uniforms and vertex
    inputs.

    The following inputs are predefined:

    \list
    \li vec4 qt_Vertex with location 0 - vertex position, the top-left vertex has
       position (0, 0), the bottom-right (\l{Item::width}{width},
       \l{Item::height}{height}).
    \li vec2 qt_MultiTexCoord0 with location 1 - texture coordinate, the top-left
       coordinate is (0, 0), the bottom-right (1, 1). If \l supportsAtlasTextures
       is true, coordinates will be based on position in the atlas instead.
    \endlist

    \note It is only the vertex input location that matters in practice. The
    names are freely changeable, while the location must always be \c 0 for
    vertex position, \c 1 for texture coordinates. However, be aware that this
    applies to vertex inputs only, and is not necessarily true for output
    variables from the vertex shader that are then used as inputs in the
    fragment shader (typically, the interpolated texture coordinates).

    The following uniforms are predefined:

    \list
    \li mat4 qt_Matrix - combined transformation
       matrix, the product of the matrices from the root item to this
       ShaderEffect, and an orthogonal projection.
    \li float qt_Opacity - combined opacity, the product of the
       opacities from the root item to this ShaderEffect.
    \endlist

    \note Vulkan-style GLSL has no separate uniform variables. Instead, shaders
    must always use a uniform block with a binding point of \c 0.

    \note The uniform block layout qualifier must always be \c std140.

    \note Unlike vertex inputs, the predefined names (qt_Matrix, qt_Opacity)
    must not be changed.

    In addition, any property that can be mapped to a GLSL type can be made
    available to the shaders. The following list shows how properties are
    mapped:

    \list
    \li bool, int, qreal -> bool, int, float - If the type in the shader is not
       the same as in QML, the value is converted automatically.
    \li QColor -> vec4 - When colors are passed to the shader, they are first
       premultiplied. Thus Qt.rgba(0.2, 0.6, 1.0, 0.5) becomes
       vec4(0.1, 0.3, 0.5, 0.5) in the shader, for example.
    \li QRect, QRectF -> vec4 - Qt.rect(x, y, w, h) becomes vec4(x, y, w, h) in
       the shader.
    \li QPoint, QPointF, QSize, QSizeF -> vec2
    \li QVector3D -> vec3
    \li QVector4D -> vec4
    \li QTransform -> mat3
    \li QMatrix4x4 -> mat4
    \li QQuaternion -> vec4, scalar value is \c w.
    \li \l Image -> sampler2D - Origin is in the top-left corner, and the
        color values are premultiplied. The texture is provided as is,
        excluding the Image item's fillMode. To include fillMode, use a
        ShaderEffectSource or Image::layer::enabled.
    \li \l ShaderEffectSource -> sampler2D - Origin is in the top-left
        corner, and the color values are premultiplied.
    \endlist

    Samplers are still declared as separate uniform variables in the shader
    code. The shaders are free to choose any binding point for these, except
    for \c 0 because that is reserved for the uniform block.

    Some shading languages and APIs have a concept of separate image and
    sampler objects. Qt Quick always works with combined image sampler objects
    in shaders, as supported by SPIR-V. Therefore shaders supplied for
    ShaderEffect should always use \c{layout(binding = 1) uniform sampler2D
    tex;} style sampler declarations. The underlying abstraction layer and the
    shader pipeline takes care of making this work for all the supported APIs
    and shading languages, transparently to the applications.

    The QML scene graph back-end may choose to allocate textures in texture
    atlases. If a texture allocated in an atlas is passed to a ShaderEffect,
    it is by default copied from the texture atlas into a stand-alone texture
    so that the texture coordinates span from 0 to 1, and you get the expected
    wrap modes. However, this will increase the memory usage. To avoid the
    texture copy, set \l supportsAtlasTextures for simple shaders using
    qt_MultiTexCoord0, or for each "uniform sampler2D <name>" declare a
    "uniform vec4 qt_SubRect_<name>" which will be assigned the texture's
    normalized source rectangle. For stand-alone textures, the source rectangle
    is [0, 1]x[0, 1]. For textures in an atlas, the source rectangle corresponds
    to the part of the texture atlas where the texture is stored.
    The correct way to calculate the texture coordinate for a texture called
    "source" within a texture atlas is
    "qt_SubRect_source.xy + qt_SubRect_source.zw * qt_MultiTexCoord0".

    The output from the \l fragmentShader should be premultiplied. If
    \l blending is enabled, source-over blending is used. However, additive
    blending can be achieved by outputting zero in the alpha channel.

    \table 70%
    \row
    \li \image declarative-shadereffectitem.png
    \li \qml
        import QtQuick 2.0

        Rectangle {
            width: 200; height: 100
            Row {
                Image { id: img;
                        sourceSize { width: 100; height: 100 } source: "qt-logo.png" }
                ShaderEffect {
                    width: 100; height: 100
                    property variant src: img
                    vertexShader: "myeffect.vert.qsb"
                    fragmentShader: "myeffect.frag.qsb"
                }
            }
        }
        \endqml
    \endtable

    The example assumes \c{myeffect.vert} and \c{myeffect.frag} contain
    Vulkan-style GLSL code, processed by the \c qsb tool in order to generate
    the \c{.qsb} files.

    \badcode
        #version 440
        layout(location = 0) in vec4 qt_Vertex;
        layout(location = 1) in vec2 qt_MultiTexCoord0;
        layout(location = 0) out vec2 coord;
        layout(std140, binding = 0) uniform buf {
            mat4 qt_Matrix;
            float qt_Opacity;
        };
        void main() {
            coord = qt_MultiTexCoord0;
            gl_Position = qt_Matrix * qt_Vertex;
        }
    \endcode

    \badcode
        #version 440
        layout(location = 0) in vec2 coord;
        layout(location = 0) out vec4 fragColor;
        layout(std140, binding = 0) uniform buf {
            mat4 qt_Matrix;
            float qt_Opacity;
        };
        layout(binding = 1) uniform sampler2D src;
        void main() {
            vec4 tex = texture(src, coord);
            fragColor = vec4(vec3(dot(tex.rgb, vec3(0.344, 0.5, 0.156))), tex.a) * qt_Opacity;
        }
    \endcode

    \note Scene Graph textures have origin in the top-left corner rather than
    bottom-left which is common in OpenGL.

    \section1 Having One Shader Only

    Specifying both \l vertexShader and \l fragmentShader is not mandatory.
    Many ShaderEffect implementations will want to provide a fragment shader
    only in practice, while relying on the default, built-in vertex shader.

    The default vertex shader passes the texture coordinate along to the
    fragment shader as \c{vec2 qt_TexCoord0} at location \c 0.

    The default fragment shader expects the texture coordinate to be passed
    from the vertex shader as \c{vec2 qt_TexCoord0} at location \c 0, and it
    samples from a sampler2D named \c source at binding point \c 1.

    \warning When only one of the shaders is specified, the writer of the
    shader must be aware of the uniform block layout expected by the default
    shaders: qt_Matrix must always be at offset 0, followed by qt_Opacity at
    offset 64. Any custom uniforms must be placed after these two. This is
    mandatory even when the application-provided shader does not use the matrix
    or the opacity, because at run time there is one single uniform buffer that
    is exposed to both the vertex and fragment shader.

    \warning Unlike with vertex inputs, passing data between the vertex and
    fragment shader may, depending on the underlying graphics API, require the
    same names to be used, a matching location is not always sufficient. Most
    prominently, when specifying a fragment shader while relying on the default,
    built-in vertex shader, the texture coordinates are passed on as \c
    qt_TexCoord0 at location \c 0, and therefore it is strongly advised that the
    fragment shader declares the input with the same name
    (qt_TexCoord0). Failing to do so may lead to issues on some platforms, for
    example when running with a non-core profile OpenGL context where the
    underlying GLSL shader source code has no location qualifiers and matching
    is based on the variable names during to shader linking process.

    \section1 ShaderEffect and Item Layers

    The ShaderEffect type can be combined with \l {Item Layers} {layered items}.

    \table
    \row
      \li \b {Layer with effect disabled} \inlineimage qml-shadereffect-nolayereffect.png
      \li \b {Layer with effect enabled} \inlineimage qml-shadereffect-layereffect.png
    \row
      \li \qml
          Item {
              id: layerRoot
              layer.enabled: true
              layer.effect: ShaderEffect {
                 fragmentShader: "effect.frag.qsb"
              }
          }
          \endqml

          \badcode
          #version 440
          layout(location = 0) in vec2 qt_TexCoord0;
          layout(location = 0) out vec4 fragColor;
          layout(std140, binding = 0) uniform buf {
              mat4 qt_Matrix;
              float qt_Opacity;
          };
          layout(binding = 1) uniform sampler2D source;
          void main() {
              vec4 p = texture(source, qt_TexCoord0);
              float g = dot(p.xyz, vec3(0.344, 0.5, 0.156));
              fragColor = vec4(g, g, g, p.a) * qt_Opacity;
          }
          \endcode
    \endtable

    It is also possible to combine multiple layered items:

    \table
    \row
      \li \inlineimage qml-shadereffect-opacitymask.png
    \row
      \li \qml
          Rectangle {
              id: gradientRect;
              width: 10
              height: 10
              gradient: Gradient {
                  GradientStop { position: 0; color: "white" }
                  GradientStop { position: 1; color: "steelblue" }
              }
              visible: false; // should not be visible on screen.
              layer.enabled: true;
              layer.smooth: true
           }
           Text {
              id: textItem
              font.pixelSize: 48
              text: "Gradient Text"
              anchors.centerIn: parent
              layer.enabled: true
              // This item should be used as the 'mask'
              layer.samplerName: "maskSource"
              layer.effect: ShaderEffect {
                  property var colorSource: gradientRect;
                  fragmentShader: "mask.frag.qsb"
              }
          }
          \endqml

          \badcode
          #version 440
          layout(location = 0) in vec2 qt_TexCoord0;
          layout(location = 0) out vec4 fragColor;
          layout(std140, binding = 0) uniform buf {
              mat4 qt_Matrix;
              float qt_Opacity;
          };
          layout(binding = 1) uniform sampler2D colorSource;
          layout(binding = 2) uniform sampler2D maskSource;
          void main() {
              fragColor = texture(colorSource, qt_TexCoord0)
                              * texture(maskSource, qt_TexCoord0).a
                              * qt_Opacity;
          }
          \endcode
    \endtable

    \section1 Other Notes

    By default, the ShaderEffect consists of four vertices, one for each
    corner. For non-linear vertex transformations, like page curl, you can
    specify a fine grid of vertices by specifying a \l mesh resolution.

    \section1 Migrating From Qt 5

    For Qt 5 applications with ShaderEffect items the migration to Qt 6 involves:
    \list
    \li Moving the shader code to separate \c{.vert} and \c{.frag} files,
    \li updating the shaders to Vulkan-compatible GLSL,
    \li running the \c qsb tool on them,
    \li including the resulting \c{.qsb} files in the executable with the Qt resource system,
    \li and referencing the file in the \l vertexShader and \l fragmentShader properties.
    \endlist

    As described in the \l{Qt Shader Tools} module some of these steps can be
    automated by letting CMake invoke the \c qsb tool at build time. See \l{Qt
    Shader Tools Build System Integration} for more information and examples.

    When it comes to updating the shader code, below is an overview of the
    commonly required changes.

    \table
    \header
    \li Vertex shader in Qt 5
    \li Vertex shader in Qt 6
    \row
    \li \badcode
        attribute highp vec4 qt_Vertex;
        attribute highp vec2 qt_MultiTexCoord0;
        varying highp vec2 coord;
        uniform highp mat4 qt_Matrix;
        void main() {
            coord = qt_MultiTexCoord0;
            gl_Position = qt_Matrix * qt_Vertex;
        }
    \endcode
    \li \badcode
        #version 440
        layout(location = 0) in vec4 qt_Vertex;
        layout(location = 1) in vec2 qt_MultiTexCoord0;
        layout(location = 0) out vec2 coord;
        layout(std140, binding = 0) uniform buf {
            mat4 qt_Matrix;
            float qt_Opacity;
        };
        void main() {
            coord = qt_MultiTexCoord0;
            gl_Position = qt_Matrix * qt_Vertex;
        }
    \endcode
    \endtable

    The conversion process mostly involves updating the code to be compatible
    with
    \l{https://github.com/KhronosGroup/GLSL/blob/master/extensions/khr/GL_KHR_vulkan_glsl.txt}{GL_KHR_vulkan_glsl}.
    It is worth noting that Qt Quick uses a subset of the features provided by
    GLSL and Vulkan, and therefore the conversion process for typical
    ShaderEffect shaders is usually straightforward.

    \list

    \li The \c version directive should state \c 440 or \c 450, although
    specifying other GLSL version may work too, because the
    \l{https://github.com/KhronosGroup/GLSL/blob/master/extensions/khr/GL_KHR_vulkan_glsl.txt}{GL_KHR_vulkan_glsl}
    extension is written for GLSL 140 and higher.

    \li Inputs and outputs must use the modern GLSL \c in and \c out keywords.
    In addition, specifying a location is required. The input and output
    location namespaces are separate, and therefore assigning locations
    starting from 0 for both is safe.

    \li When it comes to vertex shader inputs, the only possibilities with
    ShaderEffect are location \c 0 for vertex position (traditionally named \c
    qt_Vertex) and location \c 1 for texture coordinates (traditionally named
    \c qt_MultiTexCoord0).

    \li The vertex shader outputs and fragment shader inputs are up to the
    shader code to define. The fragment shader must have a \c vec4 output at
    location 0 (typically called \c fragColor). For maximum portability, vertex
    outputs and fragment inputs should use both the same location number and the
    same name. When specifying only a fragment shader, the texture coordinates
    are passed in from the built-in vertex shader as \c{vec2 qt_TexCoord0} at
    location \c 0, as shown in the example snippets above.

    \li Uniform variables outside a uniform block are not legal. Rather,
    uniform data must be declared in a uniform block with binding point \c 0.

    \li The uniform block is expected to use the std140 qualifier.

    \li At run time, the vertex and fragment shader will get the same uniform
    buffer bound to binding point 0. Therefore, as a general rule, the uniform
    block declarations must be identical between the shaders. This also
    includes members that are not used in one of the shaders. The member names
    must match, because with some graphics APIs the uniform block is converted
    to a traditional struct uniform, transparently to the application.

    \li When providing one of the shaders only, watch out for the fact that the
    built-in shaders expect \c qt_Matrix and \c qt_Opacity at the top of the
    uniform block. (more precisely, at offset 0 and 64, respectively) As a
    general rule, always include these as the first and second members in the
    block.

    \li In the example the uniform block specifies the block name \c buf. This
    name can be changed freely, but must match between the shaders. Using an
    instance name, such as \c{layout(...) uniform buf { ... } instance_name;}
    is optional. When specified, all accesses to the members must be qualified
    with instance_name.

    \endlist

    \table
    \header
    \li Fragment shader in Qt 5
    \li Fragment shader in Qt 6
    \row
    \li \badcode
        varying highp vec2 coord;
        uniform lowp float qt_Opacity;
        uniform sampler2D src;
        void main() {
            lowp vec4 tex = texture2D(src, coord);
            gl_FragColor = vec4(vec3(dot(tex.rgb,
                                vec3(0.344, 0.5, 0.156))),
                                     tex.a) * qt_Opacity;
        }
    \endcode
    \li \badcode
        #version 440
        layout(location = 0) in vec2 coord;
        layout(location = 0) out vec4 fragColor;
        layout(std140, binding = 0) uniform buf {
            mat4 qt_Matrix;
            float qt_Opacity;
        };
        layout(binding = 1) uniform sampler2D src;
        void main() {
            vec4 tex = texture(src, coord);
            fragColor = vec4(vec3(dot(tex.rgb,
                             vec3(0.344, 0.5, 0.156))),
                                  tex.a) * qt_Opacity;
        }
    \endcode
    \endtable

    \list

    \li Precision qualifiers (\c lowp, \c mediump, \c highp) are not currently used.

    \li Calling built-in GLSL functions must follow the modern GLSL names, most
    prominently, \c{texture()} instead of \c{texture2D()}.

    \li Samplers must use binding points starting from 1.

    \endlist

    \sa {Item Layers}, {QSB Manual}, {Qt Shader Tools Build System Integration}
*/


namespace QtPrivate {
class EffectSlotMapper: public QtPrivate::QSlotObjectBase
{
public:
    typedef std::function<void()> PropChangedFunc;

    explicit EffectSlotMapper(PropChangedFunc func)
    : QSlotObjectBase(&impl), _signalIndex(-1), func(func)
    { ref(); }

    void setSignalIndex(int idx) { _signalIndex = idx; }
    int signalIndex() const { return _signalIndex; }

private:
    int _signalIndex;
    PropChangedFunc func;

    static void impl(int which, QSlotObjectBase *this_, QObject *, void **a, bool *ret)
    {
    auto thiz = static_cast<EffectSlotMapper*>(this_);
    switch (which) {
    case Destroy:
        delete thiz;
        break;
    case Call:
        thiz->func();
        break;
    case Compare:
        *ret = thiz == reinterpret_cast<EffectSlotMapper *>(a[0]);
        break;
    case NumOperations: ;
    }
    }
};
} // namespace QtPrivate

QQuickShaderEffect::QQuickShaderEffect(QQuickItem *parent)
    : QQuickItem(*new QQuickShaderEffectPrivate, parent)
{
    setFlag(QQuickItem::ItemHasContents);
}

QQuickShaderEffect::~QQuickShaderEffect()
{
    Q_D(QQuickShaderEffect);
    d->inDestructor = true;
}

/*!
    \qmlproperty url QtQuick::ShaderEffect::fragmentShader

    This property contains a reference to a file with the preprocessed fragment
    shader package, typically with an extension of \c{.qsb}. The value is
    treated as a \l{QUrl}{URL}, similarly to other QML types, such as Image. It
    must either be a local file or use the qrc scheme to access files embedded
    via the Qt resource system. The URL may be absolute, or relative to the URL
    of the component.

    \sa vertexShader
*/

QUrl QQuickShaderEffect::fragmentShader() const
{
    Q_D(const QQuickShaderEffect);
    return d->fragmentShader();
}

void QQuickShaderEffect::setFragmentShader(const QUrl &fileUrl)
{
    Q_D(QQuickShaderEffect);
    d->setFragmentShader(fileUrl);
}

/*!
    \qmlproperty url QtQuick::ShaderEffect::vertexShader

    This property contains a reference to a file with the preprocessed vertex
    shader package, typically with an extension of \c{.qsb}. The value is
    treated as a \l{QUrl}{URL}, similarly to other QML types, such as Image. It
    must either be a local file or use the qrc scheme to access files embedded
    via the Qt resource system. The URL may be absolute, or relative to the URL
    of the component.

    \sa fragmentShader
*/

QUrl QQuickShaderEffect::vertexShader() const
{
    Q_D(const QQuickShaderEffect);
    return d->vertexShader();
}

void QQuickShaderEffect::setVertexShader(const QUrl &fileUrl)
{
    Q_D(QQuickShaderEffect);
    d->setVertexShader(fileUrl);
}

/*!
    \qmlproperty bool QtQuick::ShaderEffect::blending

    If this property is true, the output from the \l fragmentShader is blended
    with the background using source-over blend mode. If false, the background
    is disregarded. Blending decreases the performance, so you should set this
    property to false when blending is not needed. The default value is true.
*/

bool QQuickShaderEffect::blending() const
{
    Q_D(const QQuickShaderEffect);
    return d->blending();
}

void QQuickShaderEffect::setBlending(bool enable)
{
    Q_D(QQuickShaderEffect);
    d->setBlending(enable);
}

/*!
    \qmlproperty variant QtQuick::ShaderEffect::mesh

    This property defines the mesh used to draw the ShaderEffect. It can hold
    any \l GridMesh object.
    If a size value is assigned to this property, the ShaderEffect implicitly
    uses a \l GridMesh with the value as
    \l{GridMesh::resolution}{mesh resolution}. By default, this property is
    the size 1x1.

    \sa GridMesh
*/

QVariant QQuickShaderEffect::mesh() const
{
    Q_D(const QQuickShaderEffect);
    return d->mesh();
}

void QQuickShaderEffect::setMesh(const QVariant &mesh)
{
    Q_D(QQuickShaderEffect);
    d->setMesh(mesh);
}

/*!
    \qmlproperty enumeration QtQuick::ShaderEffect::cullMode

    This property defines which sides of the item should be visible.

    \value ShaderEffect.NoCulling           Both sides are visible
    \value ShaderEffect.BackFaceCulling     only the front side is visible
    \value ShaderEffect.FrontFaceCulling    only the back side is visible

    The default is NoCulling.
*/

QQuickShaderEffect::CullMode QQuickShaderEffect::cullMode() const
{
    Q_D(const QQuickShaderEffect);
    return d->cullMode();
}

void QQuickShaderEffect::setCullMode(CullMode face)
{
    Q_D(QQuickShaderEffect);
    return d->setCullMode(face);
}

/*!
    \qmlproperty bool QtQuick::ShaderEffect::supportsAtlasTextures

    Set this property true to confirm that your shader code doesn't rely on
    qt_MultiTexCoord0 ranging from (0,0) to (1,1) relative to the mesh.
    In this case the range of qt_MultiTexCoord0 will rather be based on the position
    of the texture within the atlas. This property currently has no effect if there
    is less, or more, than one sampler uniform used as input to your shader.

    This differs from providing qt_SubRect_<name> uniforms in that the latter allows
    drawing one or more textures from the atlas in a single ShaderEffect item, while
    supportsAtlasTextures allows multiple instances of a ShaderEffect component using
    a different source image from the atlas to be batched in a single draw.
    Both prevent a texture from being copied out of the atlas when referenced by a ShaderEffect.

    The default value is false.

    \since 5.4
    \since QtQuick 2.4
*/

bool QQuickShaderEffect::supportsAtlasTextures() const
{
    Q_D(const QQuickShaderEffect);
    return d->supportsAtlasTextures();
}

void QQuickShaderEffect::setSupportsAtlasTextures(bool supports)
{
    Q_D(QQuickShaderEffect);
    d->setSupportsAtlasTextures(supports);
}

/*!
    \qmlproperty enumeration QtQuick::ShaderEffect::status

    This property tells the current status of the shaders.

    \value ShaderEffect.Compiled    the shader program was successfully compiled and linked.
    \value ShaderEffect.Uncompiled  the shader program has not yet been compiled.
    \value ShaderEffect.Error       the shader program failed to compile or link.

    When setting the fragment or vertex shader source code, the status will
    become Uncompiled. The first time the ShaderEffect is rendered with new
    shader source code, the shaders are compiled and linked, and the status is
    updated to Compiled or Error.

    When runtime compilation is not in use and the shader properties refer to
    files with bytecode, the status is always Compiled. The contents of the
    shader is not examined (apart from basic reflection to discover vertex
    input elements and constant buffer data) until later in the rendering
    pipeline so potential errors (like layout or root signature mismatches)
    will only be detected at a later point.

    \sa log
*/

/*!
    \qmlproperty string QtQuick::ShaderEffect::log

    This property holds a log of warnings and errors from the latest attempt at
    compiling the shaders. It is updated at the same time \l status is set to
    Compiled or Error.

    \note In Qt 6, the shader pipeline promotes compiling and translating the
    Vulkan-style GLSL shaders offline, or at build time at latest. This does
    not necessarily mean there is no shader compilation happening at run time,
    but even if there is, ShaderEffect is not involved in that, and syntax and
    similar errors should not occur anymore at that stage. Therefore the value
    of this property is typically empty.

    \sa status
*/

QString QQuickShaderEffect::log() const
{
    Q_D(const QQuickShaderEffect);
    return d->log();
}

QQuickShaderEffect::Status QQuickShaderEffect::status() const
{
    Q_D(const QQuickShaderEffect);
    return d->status();
}

bool QQuickShaderEffect::event(QEvent *e)
{
    Q_D(QQuickShaderEffect);
    d->handleEvent(e);
    return QQuickItem::event(e);
}

void QQuickShaderEffect::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickShaderEffect);
    d->handleGeometryChanged(newGeometry, oldGeometry);
    QQuickItem::geometryChange(newGeometry, oldGeometry);
}

QSGNode *QQuickShaderEffect::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData)
{
    Q_D(QQuickShaderEffect);
    return d->handleUpdatePaintNode(oldNode, updatePaintNodeData);
}

void QQuickShaderEffect::componentComplete()
{
    Q_D(QQuickShaderEffect);
    d->maybeUpdateShaders();
    QQuickItem::componentComplete();
}

void QQuickShaderEffect::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QQuickShaderEffect);
    d->handleItemChange(change, value);
    QQuickItem::itemChange(change, value);
}

bool QQuickShaderEffect::isComponentComplete() const
{
    return QQuickItem::isComponentComplete();
}

bool QQuickShaderEffect::updateUniformValue(const QByteArray &name, const QVariant &value)
{
    auto node = static_cast<QSGShaderEffectNode *>(QQuickItemPrivate::get(this)->paintNode);
    if (!node)
        return false;

    Q_D(QQuickShaderEffect);
    return d->updateUniformValue(name, value, node);
}

void QQuickShaderEffectPrivate::updatePolish()
{
    Q_Q(QQuickShaderEffect);
    if (!qmlEngine(q))
        return;
    maybeUpdateShaders();
}

constexpr int indexToMappedId(const int shaderType, const int idx)
{
    return idx | (shaderType << 16);
}

constexpr int mappedIdToIndex(const int mappedId)
{
    return mappedId & 0xFFFF;
}

constexpr int mappedIdToShaderType(const int mappedId)
{
    return mappedId >> 16;
}

QQuickShaderEffectPrivate::QQuickShaderEffectPrivate()
    : m_meshResolution(1, 1)
    , m_mesh(nullptr)
    , m_cullMode(QQuickShaderEffect::NoCulling)
    , m_blending(true)
    , m_supportsAtlasTextures(false)
    , m_mgr(nullptr)
    , m_fragNeedsUpdate(true)
    , m_vertNeedsUpdate(true)
{
    qRegisterMetaType<QSGGuiThreadShaderEffectManager::ShaderInfo::Type>("ShaderInfo::Type");
    for (int i = 0; i < NShader; ++i)
        m_inProgress[i] = nullptr;
}

QQuickShaderEffectPrivate::~QQuickShaderEffectPrivate()
{
    for (int i = 0; i < NShader; ++i) {
        disconnectSignals(Shader(i));
        clearMappers(Shader(i));
    }

    delete m_mgr;
}

void QQuickShaderEffectPrivate::setFragmentShader(const QUrl &fileUrl)
{
    Q_Q(QQuickShaderEffect);
    if (m_fragShader == fileUrl)
        return;

    m_fragShader = fileUrl;

    m_fragNeedsUpdate = true;
    if (q->isComponentComplete())
        maybeUpdateShaders();

    emit q->fragmentShaderChanged();
}

void QQuickShaderEffectPrivate::setVertexShader(const QUrl &fileUrl)
{
    Q_Q(QQuickShaderEffect);
    if (m_vertShader == fileUrl)
        return;

    m_vertShader = fileUrl;

    m_vertNeedsUpdate = true;
    if (q->isComponentComplete())
        maybeUpdateShaders();

    emit q->vertexShaderChanged();
}

void QQuickShaderEffectPrivate::setBlending(bool enable)
{
    Q_Q(QQuickShaderEffect);
    if (m_blending == enable)
        return;

    m_blending = enable;
    q->update();
    emit q->blendingChanged();
}

QVariant QQuickShaderEffectPrivate::mesh() const
{
    return m_mesh ? QVariant::fromValue(static_cast<QObject *>(m_mesh))
                  : QVariant::fromValue(m_meshResolution);
}

void QQuickShaderEffectPrivate::setMesh(const QVariant &mesh)
{
    Q_Q(QQuickShaderEffect);
    QQuickShaderEffectMesh *newMesh = qobject_cast<QQuickShaderEffectMesh *>(qvariant_cast<QObject *>(mesh));
    if (newMesh && newMesh == m_mesh)
        return;

    if (m_mesh)
        QObject::disconnect(m_meshConnection);

    m_mesh = newMesh;

    if (m_mesh) {
        m_meshConnection = QObject::connect(m_mesh, &QQuickShaderEffectMesh::geometryChanged, q,
                                            [this] { markGeometryDirtyAndUpdate(); });
    } else {
        if (mesh.canConvert<QSize>()) {
            m_meshResolution = mesh.toSize();
        } else {
            QList<QByteArray> res = mesh.toByteArray().split('x');
            bool ok = res.size() == 2;
            if (ok) {
                int w = res.at(0).toInt(&ok);
                if (ok) {
                    int h = res.at(1).toInt(&ok);
                    if (ok)
                        m_meshResolution = QSize(w, h);
                }
            }
            if (!ok)
                qWarning("ShaderEffect: mesh property must be a size or an object deriving from QQuickShaderEffectMesh");
        }
        m_defaultMesh.setResolution(m_meshResolution);
    }

    m_dirty |= QSGShaderEffectNode::DirtyShaderMesh;
    q->update();

    emit q->meshChanged();
}

void QQuickShaderEffectPrivate::setCullMode(QQuickShaderEffect::CullMode face)
{
    Q_Q(QQuickShaderEffect);
    if (m_cullMode == face)
        return;

    m_cullMode = face;
    q->update();
    emit q->cullModeChanged();
}

void QQuickShaderEffectPrivate::setSupportsAtlasTextures(bool supports)
{
    Q_Q(QQuickShaderEffect);
    if (m_supportsAtlasTextures == supports)
        return;

    m_supportsAtlasTextures = supports;
    markGeometryDirtyAndUpdate();
    emit q->supportsAtlasTexturesChanged();
}

QString QQuickShaderEffectPrivate::parseLog()
{
    maybeUpdateShaders();
    return log();
}

QString QQuickShaderEffectPrivate::log() const
{
    QSGGuiThreadShaderEffectManager *mgr = shaderEffectManager();
    if (!mgr)
        return QString();

    return mgr->log();
}

QQuickShaderEffect::Status QQuickShaderEffectPrivate::status() const
{
    QSGGuiThreadShaderEffectManager *mgr = shaderEffectManager();
    if (!mgr)
        return QQuickShaderEffect::Uncompiled;

    return QQuickShaderEffect::Status(mgr->status());
}

void QQuickShaderEffectPrivate::handleEvent(QEvent *event)
{
    if (event->type() == QEvent::DynamicPropertyChange) {
        const auto propertyName = static_cast<QDynamicPropertyChangeEvent *>(event)->propertyName();
        const auto mappedId = findMappedShaderVariableId(propertyName);
        if (mappedId)
            propertyChanged(*mappedId);
    }
}

void QQuickShaderEffectPrivate::handleGeometryChanged(const QRectF &, const QRectF &)
{
    m_dirty |= QSGShaderEffectNode::DirtyShaderGeometry;
}

QSGNode *QQuickShaderEffectPrivate::handleUpdatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
    Q_Q(QQuickShaderEffect);
    QSGShaderEffectNode *node = static_cast<QSGShaderEffectNode *>(oldNode);

    if (q->width() <= 0 || q->height() <= 0) {
        delete node;
        return nullptr;
    }

    // Do not change anything while a new shader is being reflected or compiled.
    if (m_inProgress[Vertex] || m_inProgress[Fragment])
        return node;

    // The manager should be already created on the gui thread. Just take that instance.
    QSGGuiThreadShaderEffectManager *mgr = shaderEffectManager();
    if (!mgr) {
        delete node;
        return nullptr;
    }

    if (!node) {
        QSGRenderContext *rc = QQuickWindowPrivate::get(q->window())->context;
        node = rc->sceneGraphContext()->createShaderEffectNode(rc);
        if (!node) {
            qWarning("No shader effect node");
            return nullptr;
        }
        m_dirty = QSGShaderEffectNode::DirtyShaderAll;
        QObject::connect(node, &QSGShaderEffectNode::textureChanged, q, [this] { markGeometryDirtyAndUpdateIfSupportsAtlas(); });
    }

    QSGShaderEffectNode::SyncData sd;
    sd.dirty = m_dirty;
    sd.cullMode = QSGShaderEffectNode::CullMode(m_cullMode);
    sd.blending = m_blending;
    sd.vertex.shader = &m_shaders[Vertex];
    sd.vertex.dirtyConstants = &m_dirtyConstants[Vertex];
    sd.vertex.dirtyTextures = &m_dirtyTextures[Vertex];
    sd.fragment.shader = &m_shaders[Fragment];
    sd.fragment.dirtyConstants = &m_dirtyConstants[Fragment];
    sd.fragment.dirtyTextures = &m_dirtyTextures[Fragment];
    sd.materialTypeCacheKey = q->window();

    node->syncMaterial(&sd);

    if (m_dirty & QSGShaderEffectNode::DirtyShaderMesh) {
        node->setGeometry(nullptr);
        m_dirty &= ~QSGShaderEffectNode::DirtyShaderMesh;
        m_dirty |= QSGShaderEffectNode::DirtyShaderGeometry;
    }

    if (m_dirty & QSGShaderEffectNode::DirtyShaderGeometry) {
        const QRectF rect(0, 0, q->width(), q->height());
        QQuickShaderEffectMesh *mesh = m_mesh ? m_mesh : &m_defaultMesh;
        QSGGeometry *geometry = node->geometry();

        const QRectF srcRect = node->updateNormalizedTextureSubRect(m_supportsAtlasTextures);
        geometry = mesh->updateGeometry(geometry, 2, 0, srcRect, rect);

        node->setFlag(QSGNode::OwnsGeometry, false);
        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry, true);

        m_dirty &= ~QSGShaderEffectNode::DirtyShaderGeometry;
    }

    m_dirty = {};
    for (int i = 0; i < NShader; ++i) {
        m_dirtyConstants[i].clear();
        m_dirtyTextures[i].clear();
    }

    return node;
}

void QQuickShaderEffectPrivate::maybeUpdateShaders()
{
    Q_Q(QQuickShaderEffect);
    if (m_vertNeedsUpdate)
        m_vertNeedsUpdate = !updateShader(Vertex, m_vertShader);
    if (m_fragNeedsUpdate)
        m_fragNeedsUpdate = !updateShader(Fragment, m_fragShader);
    if (m_vertNeedsUpdate || m_fragNeedsUpdate) {
        // This function is invoked either from componentComplete or in a
        // response to a previous invocation's polish() request. If this is
        // case #1 then updateShader can fail due to not having a window or
        // scenegraph ready. Schedule the polish to try again later. In case #2
        // the backend probably does not have shadereffect support so there is
        // nothing to do for us here.
        if (!q->window() || !q->window()->isSceneGraphInitialized())
            q->polish();
    }
}

bool QQuickShaderEffectPrivate::updateUniformValue(const QByteArray &name, const QVariant &value,
                                                QSGShaderEffectNode *node)
{
    Q_Q(QQuickShaderEffect);
    const auto mappedId = findMappedShaderVariableId(name);
    if (!mappedId)
        return false;

    const Shader type = Shader(mappedIdToShaderType(*mappedId));
    const int idx = mappedIdToIndex(*mappedId);

    // Update value
    m_shaders[type].varData[idx].value = value;

    // Insert dirty uniform
    QSet<int> dirtyConstants[NShader];
    dirtyConstants[type].insert(idx);

    // Sync material change
    QSGShaderEffectNode::SyncData sd;
    sd.dirty = QSGShaderEffectNode::DirtyShaderConstant;
    sd.cullMode = QSGShaderEffectNode::CullMode(m_cullMode);
    sd.blending = m_blending;
    sd.vertex.shader = &m_shaders[Vertex];
    sd.vertex.dirtyConstants = &dirtyConstants[Vertex];
    sd.vertex.dirtyTextures = {};
    sd.fragment.shader = &m_shaders[Fragment];
    sd.fragment.dirtyConstants = &dirtyConstants[Fragment];
    sd.fragment.dirtyTextures = {};
    sd.materialTypeCacheKey = q->window();

    node->syncMaterial(&sd);

    return true;
}

void QQuickShaderEffectPrivate::handleItemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    if (inDestructor)
        return;

    // Move the window ref.
    if (change == QQuickItem::ItemSceneChange) {
        for (int shaderType = 0; shaderType < NShader; ++shaderType) {
            for (const auto &vd : std::as_const(m_shaders[shaderType].varData)) {
                if (vd.specialType == QSGShaderEffectNode::VariableData::Source) {
                    QQuickItem *source = qobject_cast<QQuickItem *>(qvariant_cast<QObject *>(vd.value));
                    if (source) {
                        if (value.window)
                            QQuickItemPrivate::get(source)->refWindow(value.window);
                        else
                            QQuickItemPrivate::get(source)->derefWindow();
                    }
                }
            }
        }
    }
}

QSGGuiThreadShaderEffectManager *QQuickShaderEffectPrivate::shaderEffectManager() const
{
    Q_Q(const QQuickShaderEffect);
    if (!m_mgr) {
        // return null if this is not the gui thread and not already created
        if (QThread::currentThread() != q->thread())
            return m_mgr;
        QQuickWindow *w = q->window();
        if (w) { // note: just the window, don't care about isSceneGraphInitialized() here
            m_mgr = QQuickWindowPrivate::get(w)->context->sceneGraphContext()->createGuiThreadShaderEffectManager();
            if (m_mgr) {
                QObject::connect(m_mgr, &QSGGuiThreadShaderEffectManager::logAndStatusChanged, q, &QQuickShaderEffect::logChanged);
                QObject::connect(m_mgr, &QSGGuiThreadShaderEffectManager::logAndStatusChanged, q, &QQuickShaderEffect::statusChanged);
                QObject::connect(m_mgr, &QSGGuiThreadShaderEffectManager::shaderCodePrepared, q,
                                 [this](bool ok, QSGGuiThreadShaderEffectManager::ShaderInfo::Type typeHint,
                                 const QUrl &loadUrl, QSGGuiThreadShaderEffectManager::ShaderInfo *result)
                { const_cast<QQuickShaderEffectPrivate *>(this)->shaderCodePrepared(ok, typeHint, loadUrl, result); });
            }
        }
    }
    return m_mgr;
}

void QQuickShaderEffectPrivate::disconnectSignals(Shader shaderType)
{
    Q_Q(QQuickShaderEffect);
    for (auto *mapper : m_mappers[shaderType]) {
        void *a = mapper;
        if (mapper)
            QObjectPrivate::disconnect(q, mapper->signalIndex(), &a);
    }
    for (const auto &vd : std::as_const(m_shaders[shaderType].varData)) {
        if (vd.specialType == QSGShaderEffectNode::VariableData::Source) {
            QQuickItem *source = qobject_cast<QQuickItem *>(qvariant_cast<QObject *>(vd.value));
            if (source) {
                if (q->window())
                    QQuickItemPrivate::get(source)->derefWindow();
                auto it = m_destroyedConnections.constFind(source);
                if (it != m_destroyedConnections.constEnd()) {
                    QObject::disconnect(*it);
                    m_destroyedConnections.erase(it);
                }
            }
        }
    }
}

void QQuickShaderEffectPrivate::clearMappers(QQuickShaderEffectPrivate::Shader shaderType)
{
    for (auto *mapper : std::as_const(m_mappers[shaderType])) {
        if (mapper)
            mapper->destroyIfLastRef();
    }
    m_mappers[shaderType].clear();
}

static inline QVariant getValueFromProperty(QObject *item, const QMetaObject *itemMetaObject,
                                            const QByteArray &name, int propertyIndex)
{
    QVariant value;
    if (propertyIndex == -1) {
        value = item->property(name);
    } else {
        value = itemMetaObject->property(propertyIndex).read(item);
    }
    return value;
}

using QQuickShaderInfoCache = QHash<QUrl, QSGGuiThreadShaderEffectManager::ShaderInfo>;
Q_GLOBAL_STATIC(QQuickShaderInfoCache, shaderInfoCache)

void qtquick_shadereffect_purge_gui_thread_shader_cache()
{
    shaderInfoCache()->clear();
}

bool QQuickShaderEffectPrivate::updateShader(Shader shaderType, const QUrl &fileUrl)
{
    Q_Q(QQuickShaderEffect);
    QSGGuiThreadShaderEffectManager *mgr = shaderEffectManager();
    if (!mgr)
        return false;

    const bool texturesSeparate = mgr->hasSeparateSamplerAndTextureObjects();

    disconnectSignals(shaderType);

    m_shaders[shaderType].shaderInfo.variables.clear();
    m_shaders[shaderType].varData.clear();

    if (!fileUrl.isEmpty()) {
        const QQmlContext *context = qmlContext(q);
        const QUrl loadUrl = context ? context->resolvedUrl(fileUrl) : fileUrl;
        auto it = shaderInfoCache()->constFind(loadUrl);
        if (it != shaderInfoCache()->cend()) {
            m_shaders[shaderType].shaderInfo = *it;
            m_shaders[shaderType].hasShaderCode = true;
        } else {
            // Each prepareShaderCode call needs its own work area, hence the
            // dynamic alloc. If there are calls in progress, let those run to
            // finish, their results can then simply be ignored because
            // m_inProgress indicates what we care about.
            m_inProgress[shaderType] = new QSGGuiThreadShaderEffectManager::ShaderInfo;
            const QSGGuiThreadShaderEffectManager::ShaderInfo::Type typeHint =
                    shaderType == Vertex ? QSGGuiThreadShaderEffectManager::ShaderInfo::TypeVertex
                                         : QSGGuiThreadShaderEffectManager::ShaderInfo::TypeFragment;
            // Figure out what input parameters and variables are used in the
            // shader. This is where the data is pulled in from the file.
            // (however, if there is compilation involved, that happens at a
            // later stage, up to the QRhi backend)
            mgr->prepareShaderCode(typeHint, loadUrl, m_inProgress[shaderType]);
            // the rest is handled in shaderCodePrepared()
            return true;
        }
    } else {
        m_shaders[shaderType].hasShaderCode = false;
        if (shaderType == Fragment) {
            // With built-in shaders hasShaderCode is set to false and all
            // metadata is empty, as it is left up to the node to provide a
            // built-in default shader and its metadata. However, in case of
            // the built-in fragment shader the value for 'source' has to be
            // provided and monitored like with an application-provided shader.
            QSGGuiThreadShaderEffectManager::ShaderInfo::Variable v;
            v.name = QByteArrayLiteral("source");
            v.bindPoint = 1; // fake, must match the default source bindPoint in qquickshadereffectnode.cpp
            v.type = texturesSeparate ? QSGGuiThreadShaderEffectManager::ShaderInfo::Texture
                                      : QSGGuiThreadShaderEffectManager::ShaderInfo::Sampler;
            m_shaders[shaderType].shaderInfo.variables.append(v);
        }
    }

    updateShaderVars(shaderType);
    m_dirty |= QSGShaderEffectNode::DirtyShaders;
    q->update();
    return true;
}

void QQuickShaderEffectPrivate::shaderCodePrepared(bool ok, QSGGuiThreadShaderEffectManager::ShaderInfo::Type typeHint,
                                                const QUrl &loadUrl, QSGGuiThreadShaderEffectManager::ShaderInfo *result)
{
    Q_Q(QQuickShaderEffect);
    const Shader shaderType = typeHint == QSGGuiThreadShaderEffectManager::ShaderInfo::TypeVertex ? Vertex : Fragment;

    // If another call was made to updateShader() for the same shader type in
    // the meantime then our results are useless, just drop them.
    if (result != m_inProgress[shaderType]) {
        delete result;
        return;
    }

    m_shaders[shaderType].shaderInfo = *result;
    delete result;
    m_inProgress[shaderType] = nullptr;

    if (!ok) {
        qWarning("ShaderEffect: shader preparation failed for %s\n%s\n",
                 qPrintable(loadUrl.toString()), qPrintable(log()));
        m_shaders[shaderType].hasShaderCode = false;
        return;
    }

    m_shaders[shaderType].hasShaderCode = true;
    shaderInfoCache()->insert(loadUrl, m_shaders[shaderType].shaderInfo);
    updateShaderVars(shaderType);
    m_dirty |= QSGShaderEffectNode::DirtyShaders;
    q->update();
}

void QQuickShaderEffectPrivate::updateShaderVars(Shader shaderType)
{
    Q_Q(QQuickShaderEffect);
    QSGGuiThreadShaderEffectManager *mgr = shaderEffectManager();
    if (!mgr)
        return;

    const bool texturesSeparate = mgr->hasSeparateSamplerAndTextureObjects();

    const int varCount = m_shaders[shaderType].shaderInfo.variables.size();
    m_shaders[shaderType].varData.resize(varCount);

    // Recreate signal mappers when the shader has changed.
    clearMappers(shaderType);

    QQmlPropertyCache::ConstPtr propCache = QQmlData::ensurePropertyCache(q);

    if (!m_itemMetaObject)
        m_itemMetaObject = q->metaObject();

    // Hook up the signals to get notified about changes for properties that
    // correspond to variables in the shader. Store also the values.
    for (int i = 0; i < varCount; ++i) {
        const auto &v(m_shaders[shaderType].shaderInfo.variables.at(i));
        QSGShaderEffectNode::VariableData &vd(m_shaders[shaderType].varData[i]);
        const bool isSpecial = v.name.startsWith("qt_"); // special names not mapped to properties
        if (isSpecial) {
            if (v.name == "qt_Opacity")
                vd.specialType = QSGShaderEffectNode::VariableData::Opacity;
            else if (v.name == "qt_Matrix")
                vd.specialType = QSGShaderEffectNode::VariableData::Matrix;
            else if (v.name.startsWith("qt_SubRect_"))
                vd.specialType = QSGShaderEffectNode::VariableData::SubRect;
            continue;
        }

        // The value of a property corresponding to a sampler is the source
        // item ref, unless there are separate texture objects in which case
        // the sampler is ignored (here).
        if (v.type == QSGGuiThreadShaderEffectManager::ShaderInfo::Sampler) {
            if (texturesSeparate) {
                vd.specialType = QSGShaderEffectNode::VariableData::Unused;
                continue;
            } else {
                vd.specialType = QSGShaderEffectNode::VariableData::Source;
            }
        } else if (v.type == QSGGuiThreadShaderEffectManager::ShaderInfo::Texture) {
            Q_ASSERT(texturesSeparate);
            vd.specialType = QSGShaderEffectNode::VariableData::Source;
        } else {
            vd.specialType = QSGShaderEffectNode::VariableData::None;
        }

        // Find the property on the ShaderEffect item.
        int propIdx = -1;
        const QQmlPropertyData *pd = nullptr;
        if (propCache) {
            pd = propCache->property(QLatin1String(v.name), nullptr, nullptr);
            if (pd) {
                if (!pd->isFunction())
                    propIdx = pd->coreIndex();
            }
        }
        if (propIdx >= 0) {
            if (pd && !pd->isFunction()) {
                if (pd->notifyIndex() == -1) {
                    qWarning("QQuickShaderEffect: property '%s' does not have notification method!", v.name.constData());
                } else {
                    const int mappedId = indexToMappedId(shaderType, i);
                    auto mapper = new QtPrivate::EffectSlotMapper([this, mappedId](){
                        this->propertyChanged(mappedId);
                    });
                    m_mappers[shaderType].append(mapper);
                    mapper->setSignalIndex(m_itemMetaObject->property(propIdx).notifySignal().methodIndex());
                    Q_ASSERT(q->metaObject() == m_itemMetaObject);
                    bool ok = QObjectPrivate::connectImpl(q, pd->notifyIndex(), q, nullptr, mapper,
                                                          Qt::AutoConnection, nullptr, m_itemMetaObject);
                    if (!ok)
                        qWarning() << "Failed to connect to property" << m_itemMetaObject->property(propIdx).name()
                                   << "(" << propIdx << ", signal index" << pd->notifyIndex()
                                   << ") of item" << q;
                }
            }
        } else {
            // Do not warn for dynamic properties.
            if (!q->property(v.name.constData()).isValid())
                qWarning("ShaderEffect: '%s' does not have a matching property", v.name.constData());
        }


        vd.propertyIndex = propIdx;
        vd.value = getValueFromProperty(q, m_itemMetaObject, v.name, vd.propertyIndex);
        if (vd.specialType == QSGShaderEffectNode::VariableData::Source) {
            QQuickItem *source = qobject_cast<QQuickItem *>(qvariant_cast<QObject *>(vd.value));
            if (source) {
                if (q->window())
                    QQuickItemPrivate::get(source)->refWindow(q->window());

                // Cannot just pass q as the 'context' for the connect(). The
                // order of destruction is...complicated. Having an inline
                // source (e.g. source: ShaderEffectSource { ... } in QML would
                // emit destroyed() after the connection was already gone. To
                // work that around, store the Connection and manually
                // disconnect instead.
                if (!m_destroyedConnections.contains(source))
                    m_destroyedConnections.insert(source, QObject::connect(source, &QObject::destroyed, [this](QObject *obj) { sourceDestroyed(obj); }));
            }
        }
    }
}

std::optional<int> QQuickShaderEffectPrivate::findMappedShaderVariableId(const QByteArray &name) const
{
    for (int shaderType = 0; shaderType < NShader; ++shaderType) {
        const auto &vars = m_shaders[shaderType].shaderInfo.variables;
        for (int idx = 0; idx < vars.size(); ++idx) {
            if (vars[idx].name == name)
                return indexToMappedId(shaderType, idx);
        }
    }

    return {};
}

bool QQuickShaderEffectPrivate::sourceIsUnique(QQuickItem *source, Shader typeToSkip, int indexToSkip) const
{
    for (int shaderType = 0; shaderType < NShader; ++shaderType) {
        for (int idx = 0; idx < m_shaders[shaderType].varData.size(); ++idx) {
            if (shaderType != typeToSkip || idx != indexToSkip) {
                const auto &vd(m_shaders[shaderType].varData[idx]);
                if (vd.specialType == QSGShaderEffectNode::VariableData::Source && qvariant_cast<QObject *>(vd.value) == source)
                    return false;
            }
        }
    }
    return true;
}

void QQuickShaderEffectPrivate::propertyChanged(int mappedId)
{
    Q_Q(QQuickShaderEffect);
    const Shader type = Shader(mappedIdToShaderType(mappedId));
    const int idx = mappedIdToIndex(mappedId);
    const auto &v(m_shaders[type].shaderInfo.variables[idx]);
    auto &vd(m_shaders[type].varData[idx]);

    QVariant oldValue = vd.value;
    vd.value = getValueFromProperty(q, m_itemMetaObject, v.name, vd.propertyIndex);

    if (vd.specialType == QSGShaderEffectNode::VariableData::Source) {
        QQuickItem *source = qobject_cast<QQuickItem *>(qvariant_cast<QObject *>(oldValue));
        if (source) {
            if (q->window())
                QQuickItemPrivate::get(source)->derefWindow();
            // If the same source has been attached to two separate
            // textures/samplers, then changing one of them would trigger both
            // to be disconnected. So check first.
            if (sourceIsUnique(source, type, idx)) {
                auto it = m_destroyedConnections.constFind(source);
                if (it != m_destroyedConnections.constEnd()) {
                    QObject::disconnect(*it);
                    m_destroyedConnections.erase(it);
                }
            }
        }

        source = qobject_cast<QQuickItem *>(qvariant_cast<QObject *>(vd.value));
        if (source) {
            // 'source' needs a window to get a scene graph node. It usually gets one through its
            // parent, but if the source item is "inline" rather than a reference -- i.e.
            // "property variant source: Image { }" instead of "property variant source: foo" -- it
            // will not get a parent. In those cases, 'source' should get the window from 'item'.
            if (q->window())
                QQuickItemPrivate::get(source)->refWindow(q->window());
            if (!m_destroyedConnections.contains(source))
                m_destroyedConnections.insert(source, QObject::connect(source, &QObject::destroyed, [this](QObject *obj) { sourceDestroyed(obj); }));
        }

        m_dirty |= QSGShaderEffectNode::DirtyShaderTexture;
        m_dirtyTextures[type].insert(idx);

     } else {
        m_dirty |= QSGShaderEffectNode::DirtyShaderConstant;
        m_dirtyConstants[type].insert(idx);
    }

    q->update();
}

void QQuickShaderEffectPrivate::sourceDestroyed(QObject *object)
{
    for (int shaderType = 0; shaderType < NShader; ++shaderType) {
        for (auto &vd : m_shaders[shaderType].varData) {
            if (vd.specialType == QSGShaderEffectNode::VariableData::Source && vd.value.canConvert<QObject *>()) {
                if (qvariant_cast<QObject *>(vd.value) == object)
                    vd.value = QVariant();
            }
        }
    }
}

void QQuickShaderEffectPrivate::markGeometryDirtyAndUpdate()
{
    Q_Q(QQuickShaderEffect);
    m_dirty |= QSGShaderEffectNode::DirtyShaderGeometry;
    q->update();
}

void QQuickShaderEffectPrivate::markGeometryDirtyAndUpdateIfSupportsAtlas()
{
    if (m_supportsAtlasTextures)
        markGeometryDirtyAndUpdate();
}

QT_END_NAMESPACE

#include "moc_qquickshadereffect_p.cpp"
