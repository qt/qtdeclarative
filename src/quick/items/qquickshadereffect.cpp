/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/qquickshadereffect_p.h>
#include <private/qquickshadereffectnode_p.h>

#include <QtQuick/qsgmaterial.h>
#include "qquickitem_p.h"

#include <QtQuick/private/qsgcontext_p.h>
#include <QtQuick/qsgtextureprovider.h>
#include "qquickcanvas.h"

#include "qquickimage_p.h"
#include "qquickshadereffectsource_p.h"

#include <QtCore/qsignalmapper.h>
#include <QtGui/qopenglframebufferobject.h>

QT_BEGIN_NAMESPACE

static const char qt_default_vertex_code[] =
    "uniform highp mat4 qt_Matrix;                                  \n"
    "attribute highp vec4 qt_Vertex;                                \n"
    "attribute highp vec2 qt_MultiTexCoord0;                        \n"
    "varying highp vec2 qt_TexCoord0;                               \n"
    "void main() {                                                  \n"
    "    qt_TexCoord0 = qt_MultiTexCoord0;                          \n"
    "    gl_Position = qt_Matrix * qt_Vertex;                       \n"
    "}";

static const char qt_default_fragment_code[] =
    "varying highp vec2 qt_TexCoord0;                                   \n"
    "uniform sampler2D source;                                          \n"
    "uniform lowp float qt_Opacity;                                     \n"
    "void main() {                                                      \n"
    "    gl_FragColor = texture2D(source, qt_TexCoord0) * qt_Opacity;   \n"
    "}";

static const char qt_position_attribute_name[] = "qt_Vertex";
static const char qt_texcoord_attribute_name[] = "qt_MultiTexCoord0";

const char *qtPositionAttributeName()
{
    return qt_position_attribute_name;
}

const char *qtTexCoordAttributeName()
{
    return qt_texcoord_attribute_name;
}

/*!
    \qmlclass ShaderEffect QQuickShaderEffect
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-visual-elements
    \brief The ShaderEffect element applies custom shaders to a rectangle.
    \inherits Item

    The ShaderEffect element applies a custom OpenGL
    \l{vertexShader}{vertex} and \l{fragmentShader}{fragment} shader to a
    rectangle. It allows you to write effects such as drop shadow, blur,
    colorize and page curl directly in QML.

    There are two types of input to the \l vertexShader:
    uniform variables and attributes. Some are predefined:
    \list
    \o uniform mat4 qt_Matrix - combined transformation
       matrix, the product of the matrices from the root item to this
       ShaderEffect, and an orthogonal projection.
    \o uniform float qt_Opacity - combined opacity, the product of the
       opacities from the root item to this ShaderEffect.
    \o attribute vec4 qt_Vertex - vertex position, the top-left vertex has
       position (0, 0), the bottom-right (\l{Item::width}{width},
       \l{Item::height}{height}).
    \o attribute vec2 qt_MultiTexCoord0 - texture coordinate, the top-left
       coordinate is (0, 0), the bottom-right (1, 1).
    \endlist

    In addition, any property that can be mapped to an OpenGL Shading Language
    (GLSL) type is available as a uniform variable. The following list shows
    how properties are mapped to GLSL uniform variables:
    \list
    \o bool, int, qreal -> bool, int, float - If the type in the shader is not
       the same as in QML, the value is converted automatically.
    \o QColor -> vec4 - When colors are passed to the shader, they are first
       premultiplied. Thus Qt.rgba(0.2, 0.6, 1.0, 0.5) becomes
       vec4(0.1, 0.3, 0.5, 0.5) in the shader, for example.
    \o QRect, QRectF -> vec4 - Qt.rect(x, y, w, h) becomes vec4(x, y, w, h) in
       the shader.
    \o QPoint, QPointF, QSize, QSizeF -> vec2
    \o QVector3D -> vec3
    \o QTransform -> mat4
    \o \l Image, \l ShaderEffectSource -> sampler2D - Origin is in the top-left
       corner, and the color values are premultiplied.
    \endlist

    The output from the \l fragmentShader should be premultiplied. If
    \l blending is enabled, source-over blending is used. However, additive
    blending can be achieved by outputting zero in the alpha channel.

    \row
    \o \image declarative-shadereffectitem.png
    \o \qml
        import QtQuick 2.0

        Rectangle {
            width: 200; height: 100
            Row {
                Image { id: img; sourceSize { width: 100; height: 100 } source: "qt-logo.png" }
                ShaderEffect {
                    width: 100; height: 100
                    property variant src: img
                    vertexShader: "
                        uniform highp mat4 qt_Matrix;
                        attribute highp vec4 qt_Vertex;
                        attribute highp vec2 qt_MultiTexCoord0;
                        varying highp vec2 coord;
                        void main() {
                            coord = qt_MultiTexCoord0;
                            gl_Position = qt_Matrix * qt_Vertex;
                        }"
                    fragmentShader: "
                        varying highp vec2 coord;
                        uniform sampler2D src;
                        uniform lowp float qt_Opacity;
                        void main() {
                            lowp vec4 tex = texture2D(src, coord);
                            gl_FragColor = vec4(vec3(dot(tex.rgb, vec3(0.344, 0.5, 0.156))), tex.a) * qt_Opacity;
                        }"
                }
            }
        }
        \endqml
    \endrow

    By default, the ShaderEffect consists of four vertices, one for each
    corner. For non-linear vertex transformations, like page curl, you can
    specify a fine grid of vertices by specifying a \l mesh resolution.

    \note Scene Graph textures have origin in the top-left corner rather than
    bottom-left which is common in OpenGL.
*/

QQuickShaderEffect::QQuickShaderEffect(QQuickItem *parent)
    : QQuickItem(parent)
    , m_meshResolution(1, 1)
    , m_mesh(0)
    , m_cullMode(NoCulling)
    , m_status(Uncompiled)
    , m_blending(true)
    , m_dirtyData(true)
    , m_programDirty(true)
    , m_dirtyMesh(true)
    , m_dirtyGeometry(true)
    , m_complete(false)
{
    setFlag(QQuickItem::ItemHasContents);
}

QQuickShaderEffect::~QQuickShaderEffect()
{
    reset();
}

/*!
    \qmlproperty string QtQuick2::ShaderEffect::fragmentShader

    This property holds the fragment shader's GLSL source code.
    The default shader passes the texture coordinate along to the fragment
    shader as "varying highp vec2 qt_TexCoord0".
*/

void QQuickShaderEffect::setFragmentShader(const QByteArray &code)
{
    if (m_source.fragmentCode.constData() == code.constData())
        return;
    m_source.fragmentCode = code;
    update();
    m_complete = false;
    if (m_status != Uncompiled) {
        m_status = Uncompiled;
        emit statusChanged();
    }
    emit fragmentShaderChanged();
}

/*!
    \qmlproperty string QtQuick2::ShaderEffect::vertexShader

    This property holds the vertex shader's GLSL source code.
    The default shader expects the texture coordinate to be passed from the
    vertex shader as "varying highp vec2 qt_TexCoord0", and it samples from a
    sampler2D named "source".
*/

void QQuickShaderEffect::setVertexShader(const QByteArray &code)
{
    if (m_source.vertexCode.constData() == code.constData())
        return;
    m_source.vertexCode = code;
    update();
    m_complete = false;
    if (m_status != Uncompiled) {
        m_status = Uncompiled;
        emit statusChanged();
    }
    emit vertexShaderChanged();
}

/*!
    \qmlproperty bool QtQuick2::ShaderEffect::blending

    If this property is true, the output from the \l fragmentShader is blended
    with the background using source-over blend mode. If false, the background
    is disregarded. Blending decreases the performance, so you should set this
    property to false when blending is not needed. The default value is true.
*/

void QQuickShaderEffect::setBlending(bool enable)
{
    if (blending() == enable)
        return;

    m_blending = enable;
    update();

    emit blendingChanged();
}

/*!
    \qmlproperty variant QtQuick2::ShaderEffect::mesh

    This property defines the mesh used to draw the ShaderEffect. It can hold
    any mesh object deriving from \l QQuickShaderEffectMesh, such as \l GridMesh.
    If a size value is assigned to this property, the ShaderEffect implicitly
    uses a \l GridMesh with the value as
    \l{GridMesh::resolution}{mesh resolution}. By default, this property is
    the size 1x1.

    \sa GridMesh
*/

QVariant QQuickShaderEffect::mesh() const
{
    return m_mesh ? qVariantFromValue(static_cast<QObject *>(m_mesh))
                  : qVariantFromValue(m_meshResolution);
}

void QQuickShaderEffect::setMesh(const QVariant &mesh)
{
    QQuickShaderEffectMesh *newMesh = qobject_cast<QQuickShaderEffectMesh *>(qVariantValue<QObject *>(mesh));
    if (newMesh && newMesh == m_mesh)
        return;
    if (m_mesh)
        disconnect(m_mesh, SIGNAL(geometryChanged()), this, 0);
    m_mesh = newMesh;
    if (m_mesh) {
        connect(m_mesh, SIGNAL(geometryChanged()), this, SLOT(updateGeometry()));
    } else {
        if (qVariantCanConvert<QSize>(mesh)) {
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
                qWarning("ShaderEffect: mesh property must be size or object deriving from QQuickShaderEffectMesh.");
        }
        m_defaultMesh.setResolution(m_meshResolution);
    }

    m_dirtyMesh = true;
    update();
    emit meshChanged();
}

/*!
    \qmlproperty enumeration QtQuick2::ShaderEffect::cullMode

    This property defines which sides of the element should be visible.

    \list
    \o ShaderEffect.NoCulling - Both sides are visible
    \o ShaderEffect.BackFaceCulling - only front side is visible
    \o ShaderEffect.FrontFaceCulling - only back side is visible
    \endlist

    The default is NoCulling.
*/

void QQuickShaderEffect::setCullMode(CullMode face)
{
    if (face == m_cullMode)
        return;
    m_cullMode = face;
    update();
    emit cullModeChanged();
}

/*!
    \qmlproperty enumeration QtQuick2::ShaderEffect::status

    This property tells the current status of the OpenGL shader program.

    \list
    \o ShaderEffect.Compiled - the shader program was successfully compiled and linked.
    \o ShaderEffect.Uncompiled - the shader program has not yet been compiled.
    \o ShaderEffect.Error - the shader program failed to compile or link.
    \endlist

    When setting the fragment or vertex shader source code, the status will become Uncompiled.
    The first time the ShaderEffect is rendered with new shader source code, the shaders are
    compiled and linked, and the status is updated to Compiled or Error.

    \sa log
*/

/*!
    \qmlproperty string QtQuick2::ShaderEffect::log

    This property holds a log of warnings and errors from the latest attempt at compiling and
    linking the OpenGL shader program. It is updated at the same time \l status is set to Compiled
    or Error.

    \sa status
*/

void QQuickShaderEffect::changeSource(int index)
{
    Q_ASSERT(index >= 0 && index < m_sources.size());
    QVariant v = property(m_sources.at(index).name.constData());
    setSource(v, index);
}

void QQuickShaderEffect::updateData()
{
    m_dirtyData = true;
    update();
}

void QQuickShaderEffect::updateGeometry()
{
    m_dirtyGeometry = true;
    update();
}

void QQuickShaderEffect::updateLogAndStatus(const QString &log, int status)
{
    m_log = m_parseLog + log;
    m_status = Status(status);
    emit logChanged();
    emit statusChanged();
}

void QQuickShaderEffect::setSource(const QVariant &var, int index)
{
    Q_ASSERT(index >= 0 && index < m_sources.size());

    SourceData &source = m_sources[index];

    source.sourceObject = 0;
    if (var.isNull()) {
        return;
    } else if (!qVariantCanConvert<QObject *>(var)) {
        qWarning("Could not assign source of type '%s' to property '%s'.", var.typeName(), source.name.constData());
        return;
    }

    QObject *obj = qVariantValue<QObject *>(var);
    QQuickItem *item = qobject_cast<QQuickItem *>(obj);
    if (!item || !item->isTextureProvider()) {
        qWarning("ShaderEffect: source uniform [%s] is not assigned a valid texture provider: %s [%s]",
                 source.name.constData(), qPrintable(obj->objectName()), obj->metaObject()->className());
        return;
    }

    source.sourceObject = item;

    if (item) {
        QQuickItemPrivate *d = QQuickItemPrivate::get(item);
        // 'item' needs a canvas to get a scene graph node. It usually gets one through its
        // parent, but if the source item is "inline" rather than a reference -- i.e.
        // "property variant source: Image { }" instead of "property variant source: foo" -- it
        // will not get a parent. In those cases, 'item' should get the canvas from 'this'.
        if (!d->parentItem && canvas() && !d->canvas) {
            QQuickItemPrivate::InitializationState initState;
            initState.clear();
            d->initCanvas(&initState, canvas());
        }
    }
}

void QQuickShaderEffect::disconnectPropertySignals()
{
    disconnect(this, 0, this, SLOT(updateData()));
    for (int i = 0; i < m_sources.size(); ++i) {
        SourceData &source = m_sources[i];
        disconnect(this, 0, source.mapper, 0);
        disconnect(source.mapper, 0, this, 0);
    }
}

void QQuickShaderEffect::connectPropertySignals()
{
    QSet<QByteArray>::const_iterator it;
    for (it = m_source.uniformNames.begin(); it != m_source.uniformNames.end(); ++it) {
        int pi = metaObject()->indexOfProperty(it->constData());
        if (pi >= 0) {
            QMetaProperty mp = metaObject()->property(pi);
            if (!mp.hasNotifySignal())
                qWarning("QQuickShaderEffect: property '%s' does not have notification method!", it->constData());
            QByteArray signalName("2");
            signalName.append(mp.notifySignal().signature());
            connect(this, signalName, this, SLOT(updateData()));
        } else {
            // If the source is set via a dynamic property, like the layer is, then we need this check
            // to disable the warning.
            if (property(it->constData()).isValid())
                continue;
            qWarning("QQuickShaderEffect: '%s' does not have a matching property!", it->constData());
        }
    }
    for (int i = 0; i < m_sources.size(); ++i) {
        SourceData &source = m_sources[i];
        int pi = metaObject()->indexOfProperty(source.name.constData());
        if (pi >= 0) {
            QMetaProperty mp = metaObject()->property(pi);
            QByteArray signalName("2");
            signalName.append(mp.notifySignal().signature());
            connect(this, signalName, source.mapper, SLOT(map()));
            source.mapper->setMapping(this, i);
            connect(source.mapper, SIGNAL(mapped(int)), this, SLOT(changeSource(int)));
        } else {
            // If the source is set via a dynamic property, like the layer is, then we need this check
            // to disable the warning.
            if (property(source.name.constData()).isValid())
                continue;
            qWarning("QQuickShaderEffect: '%s' does not have a matching source!", source.name.constData());
        }
    }
}


void QQuickShaderEffect::ensureCompleted()
{
    if (!m_complete) {
        reset();
        updateProperties();
        m_complete = true;
    }
}


void QQuickShaderEffect::reset()
{
    disconnectPropertySignals();

    m_source.attributeNames.clear();
    m_source.uniformNames.clear();
    m_source.respectsOpacity = false;
    m_source.respectsMatrix = false;
    m_source.className = metaObject()->className();

    for (int i = 0; i < m_sources.size(); ++i) {
        const SourceData &source = m_sources.at(i);
        delete source.mapper;
    }
    m_sources.clear();
    m_log.clear();
    m_parseLog.clear();
    m_programDirty = true;
    m_dirtyMesh = true;
}

void QQuickShaderEffect::updateProperties()
{
    if (m_source.vertexCode.isEmpty()) {
        m_source.attributeNames.append(QByteArray(qt_position_attribute_name));
        m_source.attributeNames.append(QByteArray(qt_texcoord_attribute_name));
        m_source.respectsMatrix = true;
    } else {
        lookThroughShaderCode(m_source.vertexCode);
    }
    if (m_source.fragmentCode.isEmpty()) {
        m_source.respectsOpacity = true;
        QByteArray name("source");
        m_source.uniformNames.insert(name);
        SourceData d;
        d.mapper = new QSignalMapper;
        d.name = name;
        d.sourceObject = 0;
        m_sources.append(d);
    } else {
        lookThroughShaderCode(m_source.fragmentCode);
    }

    if (!m_mesh && !m_source.attributeNames.contains(qt_position_attribute_name)) {
        m_parseLog += QLatin1String("Warning: Missing reference to \'");
        m_parseLog += QLatin1String(qt_position_attribute_name);
        m_parseLog += QLatin1String("\'.\n");
    }
    if (!m_mesh && !m_source.attributeNames.contains(qt_texcoord_attribute_name)) {
        m_parseLog += QLatin1String("Warning: Missing reference to \'");
        m_parseLog += QLatin1String(qt_texcoord_attribute_name);
        m_parseLog += QLatin1String("\'.\n");
    }
    if (!m_source.respectsMatrix) {
        m_parseLog += QLatin1String("Warning: Missing reference to \'qt_Matrix\'.\n");
    }
    if (!m_source.respectsOpacity) {
        m_parseLog += QLatin1String("Warning: Missing reference to \'qt_Opacity\'.\n");
    }

    for (int i = 0; i < m_sources.size(); ++i) {
        QVariant v = property(m_sources.at(i).name);
        setSource(v, i);
    }

    connectPropertySignals();
}

namespace {

    enum VariableQualifier {
        AttributeQualifier,
        UniformQualifier
    };

    inline bool qt_isalpha(char c)
    {
        char ch = c | 0x20;
        return (ch >= 'a' && ch <= 'z') || c == '_';
    }

    inline bool qt_isalnum(char c)
    {
        return qt_isalpha(c) || (c >= '0' && c <= '9');
    }

    inline bool qt_isspace(char c)
    {
        return c == ' ' || (c >= 0x09 && c <= 0x0d);
    }

    // Returns -1 if not found, returns index to first character after the name if found.
    int qt_search_for_variable(const char *s, int length, int index, VariableQualifier &decl,
                               int &typeIndex, int &typeLength,
                               int &nameIndex, int &nameLength)
    {
        enum Identifier {
            QualifierIdentifier, // Base state
            PrecisionIdentifier,
            TypeIdentifier,
            NameIdentifier
        };
        Identifier expected = QualifierIdentifier;
        bool compilerDirectiveExpected = index == 0;

        while (index < length) {
            // Skip whitespace.
            while (qt_isspace(s[index])) {
                compilerDirectiveExpected |= s[index] == '\n';
                ++index;
            }

            if (qt_isalpha(s[index])) {
                // Read identifier.
                int idIndex = index;
                ++index;
                while (qt_isalnum(s[index]))
                    ++index;
                int idLength = index - idIndex;

                const int attrLen = sizeof("attribute") - 1;
                const int uniLen = sizeof("uniform") - 1;
                const int loLen = sizeof("lowp") - 1;
                const int medLen = sizeof("mediump") - 1;
                const int hiLen = sizeof("highp") - 1;

                switch (expected) {
                case QualifierIdentifier:
                    if (idLength == attrLen && qstrncmp("attribute", s + idIndex, attrLen) == 0) {
                        decl = AttributeQualifier;
                        expected = PrecisionIdentifier;
                    } else if (idLength == uniLen && qstrncmp("uniform", s + idIndex, uniLen) == 0) {
                        decl = UniformQualifier;
                        expected = PrecisionIdentifier;
                    }
                    break;
                case PrecisionIdentifier:
                    if ((idLength == loLen && qstrncmp("lowp", s + idIndex, loLen) == 0)
                            || (idLength == medLen && qstrncmp("mediump", s + idIndex, medLen) == 0)
                            || (idLength == hiLen && qstrncmp("highp", s + idIndex, hiLen) == 0))
                    {
                        expected = TypeIdentifier;
                        break;
                    }
                    // Fall through.
                case TypeIdentifier:
                    typeIndex = idIndex;
                    typeLength = idLength;
                    expected = NameIdentifier;
                    break;
                case NameIdentifier:
                    nameIndex = idIndex;
                    nameLength = idLength;
                    return index; // Attribute or uniform declaration found. Return result.
                default:
                    break;
                }
            } else if (s[index] == '#' && compilerDirectiveExpected) {
                // Skip compiler directives.
                ++index;
                while (index < length && (s[index] != '\n' || s[index - 1] == '\\'))
                    ++index;
            } else if (s[index] == '/' && s[index + 1] == '/') {
                // Skip comments.
                index += 2;
                while (index < length && s[index] != '\n')
                    ++index;
            } else if (s[index] == '/' && s[index + 1] == '*') {
                // Skip comments.
                index += 2;
                while (index < length && (s[index] != '*' || s[index + 1] != '/'))
                    ++index;
                if (index < length)
                    index += 2; // Skip star-slash.
            } else {
                expected = QualifierIdentifier;
                ++index;
            }
            compilerDirectiveExpected = false;
        }
        return -1;
    }
}

void QQuickShaderEffect::lookThroughShaderCode(const QByteArray &code)
{
    int index = 0;
    int typeIndex, typeLength, nameIndex, nameLength;
    const char *s = code.constData();
    VariableQualifier decl;
    while ((index = qt_search_for_variable(s, code.size(), index, decl, typeIndex, typeLength,
                                           nameIndex, nameLength)) != -1)
    {
        if (decl == AttributeQualifier) {
            m_source.attributeNames.append(QByteArray(s + nameIndex, nameLength));
        } else {
            Q_ASSERT(decl == UniformQualifier);

            const int matLen = sizeof("qt_Matrix") - 1;
            const int opLen = sizeof("qt_Opacity") - 1;
            const int sampLen = sizeof("sampler2D") - 1;

            if (nameLength == matLen && qstrncmp("qt_Matrix", s + nameIndex, matLen) == 0) {
                m_source.respectsMatrix = true;
            } else if (nameLength == opLen && qstrncmp("qt_Opacity", s + nameIndex, opLen) == 0) {
                m_source.respectsOpacity = true;
            } else {
                QByteArray name(s + nameIndex, nameLength);
                m_source.uniformNames.insert(name);
                if (typeLength == sampLen && qstrncmp("sampler2D", s + typeIndex, sampLen) == 0) {
                    SourceData d;
                    d.mapper = new QSignalMapper;
                    d.name = name;
                    d.sourceObject = 0;
                    m_sources.append(d);
                }
            }
        }
    }
}

void QQuickShaderEffect::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_dirtyGeometry = true;
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
}

QSGNode *QQuickShaderEffect::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QQuickShaderEffectNode *node = static_cast<QQuickShaderEffectNode *>(oldNode);

    ensureCompleted();

    // In the case of a bad vertex shader, don't try to create a node...
    if (m_source.attributeNames.isEmpty()) {
        if (node)
            delete node;
        return 0;
    }

    if (!node) {
        node = new QQuickShaderEffectNode;
        m_programDirty = true;
        m_dirtyData = true;
        m_dirtyGeometry = true;
        connect(node, SIGNAL(logAndStatusChanged(QString,int)), this, SLOT(updateLogAndStatus(QString,int)));
    }

    QQuickShaderEffectMaterial *material = node->shaderMaterial();

    if (m_dirtyMesh) {
        node->setGeometry(0);
        m_dirtyMesh = false;
        m_dirtyGeometry = true;
    }

    if (m_dirtyGeometry) {
        node->setFlag(QSGNode::OwnsGeometry, false);
        QSGGeometry *geometry = node->geometry();
        QRectF rect(0, 0, width(), height());
        QQuickShaderEffectMesh *mesh = m_mesh ? m_mesh : &m_defaultMesh;

        geometry = mesh->updateGeometry(geometry, m_source.attributeNames, rect);
        if (!geometry) {
            QString log = mesh->log();
            if (!log.isNull()) {
                m_log = m_parseLog;
                m_log += QLatin1String("*** Mesh ***\n");
                m_log += log;
                m_status = Error;
                emit logChanged();
                emit statusChanged();
            }
            delete node;
            return 0;
        }

        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry, true);

        m_dirtyGeometry = false;
    }

    if (m_programDirty) {
        QQuickShaderEffectProgram s = m_source;
        if (s.fragmentCode.isEmpty())
            s.fragmentCode = qt_default_fragment_code;
        if (s.vertexCode.isEmpty())
            s.vertexCode = qt_default_vertex_code;
        s.className = metaObject()->className();

        material->setProgramSource(s);
        node->markDirty(QSGNode::DirtyMaterial);
        m_programDirty = false;
    }

    // Update blending
    if (bool(material->flags() & QSGMaterial::Blending) != m_blending) {
        material->setFlag(QSGMaterial::Blending, m_blending);
        node->markDirty(QSGNode::DirtyMaterial);
    }

    if (int(material->cullMode()) != int(m_cullMode)) {
        material->setCullMode(QQuickShaderEffectMaterial::CullMode(m_cullMode));
        node->markDirty(QSGNode::DirtyMaterial);
    }

    if (m_dirtyData) {
        QVector<QPair<QByteArray, QVariant> > values;
        QVector<QPair<QByteArray, QSGTextureProvider *> > textures;
        const QVector<QPair<QByteArray, QSGTextureProvider *> > &oldTextures = material->textureProviders();

        for (QSet<QByteArray>::const_iterator it = m_source.uniformNames.begin();
             it != m_source.uniformNames.end(); ++it) {
            values.append(qMakePair(*it, property(*it)));
        }
        for (int i = 0; i < oldTextures.size(); ++i) {
            QSGTextureProvider *t = oldTextures.at(i).second;
            if (t)
                disconnect(t, SIGNAL(textureChanged()), node, SLOT(markDirtyTexture()));
        }
        for (int i = 0; i < m_sources.size(); ++i) {
            const SourceData &source = m_sources.at(i);
            QSGTextureProvider *t = source.sourceObject ? source.sourceObject->textureProvider() : 0;
            textures.append(qMakePair(source.name, t));
            if (t)
                connect(t, SIGNAL(textureChanged()), node, SLOT(markDirtyTexture()), Qt::DirectConnection);
        }
        material->setUniforms(values);
        material->setTextureProviders(textures);
        node->markDirty(QSGNode::DirtyMaterial);
        m_dirtyData = false;
    }

    return node;
}

void QQuickShaderEffect::itemChange(ItemChange change, const ItemChangeData &value)
{
    if (change == QQuickItem::ItemSceneChange) {
        // See comment in QQuickShaderEffect::setSource().
        for (int i = 0; i < m_sources.size(); ++i) {
            QQuickItemPrivate *d = QQuickItemPrivate::get(m_sources.at(i).sourceObject);
            if (!d->parentItem && value.canvas != d->canvas) {
                QQuickItemPrivate::InitializationState initState;
                initState.clear();
                d->initCanvas(&initState, value.canvas);
            }
        }
    }
    QQuickItem::itemChange(change, value);
}


QT_END_NAMESPACE
