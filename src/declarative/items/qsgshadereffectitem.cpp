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

#include <private/qsgshadereffectitem_p.h>
#include <private/qsgshadereffectnode_p.h>

#include "qsgmaterial.h"
#include "qsgitem_p.h"

#include <private/qsgcontext_p.h>
#include <private/qsgtextureprovider_p.h>
#include "qsgcanvas.h"

#include <QtCore/qsignalmapper.h>
#include <QtOpenGL/qglframebufferobject.h>

QT_BEGIN_NAMESPACE

static const char qt_default_vertex_code[] =
    "uniform highp mat4 qt_ModelViewProjectionMatrix;               \n"
    "attribute highp vec4 qt_Vertex;                                \n"
    "attribute highp vec2 qt_MultiTexCoord0;                        \n"
    "varying highp vec2 qt_TexCoord0;                               \n"
    "void main() {                                                  \n"
    "    qt_TexCoord0 = qt_MultiTexCoord0;                          \n"
    "    gl_Position = qt_ModelViewProjectionMatrix * qt_Vertex;    \n"
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
    \qmlclass ShaderEffectItem QSGShaderEffectItem
    \since 5.0
    \ingroup qml-basic-visual-elements
    \brief The ShaderEffectItem element applies custom shaders to a rectangle.
    \inherits Item

    The ShaderEffectItem element applies a custom OpenGL
    \l{vertexShader}{vertex} and \l{fragmentShader}{fragment} shader to a
    rectangle. It allows you to write effects such as drop shadow, blur,
    colorize and page curl directly in QML.

    There are two types of input to the \l vertexShader:
    uniform variables and attributes. Some are predefined:
    \list
    \o uniform mat4 qt_ModelViewProjectionMatrix - combined transformation
       matrix, the product of the matrices from the root item to this
       ShaderEffectItem, and an orthogonal projection.
    \o uniform float qt_Opacity - combined opacity, the product of the
       opacities from the root item to this ShaderEffectItem.
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
                ShaderEffectItem {
                    width: 100; height: 100
                    property variant src: img
                    vertexShader: "
                        uniform highp mat4 qt_ModelViewProjectionMatrix;
                        attribute highp vec4 qt_Vertex;
                        attribute highp vec2 qt_MultiTexCoord0;
                        varying highp vec2 coord;
                        void main() {
                            coord = qt_MultiTexCoord0;
                            gl_Position = qt_ModelViewProjectionMatrix * qt_Vertex;
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

    By default, the ShaderEffectItem consists of four vertices, one for each
    corner. For non-linear vertex transformations, like page curl, you can
    specify a fine grid of vertices by assigning a \l GridMesh to the \l mesh
    property.

    \note Scene Graph textures have origin in the top-left corner rather than
    bottom-left which is common in OpenGL.
*/

QSGShaderEffectItem::QSGShaderEffectItem(QSGItem *parent)
    : QSGItem(parent)
    , m_mesh(0)
    , m_cullMode(NoCulling)
    , m_blending(true)
    , m_dirtyData(true)
    , m_programDirty(true)
    , m_dirtyMesh(true)
    , m_dirtyGeometry(true)
{
    setFlag(QSGItem::ItemHasContents);
}

QSGShaderEffectItem::~QSGShaderEffectItem()
{
    reset();
}

void QSGShaderEffectItem::componentComplete()
{
    updateProperties();
    QSGItem::componentComplete();
}

/*!
    \qmlproperty string ShaderEffectItem::fragmentShader

    This property holds the fragment shader's GLSL source code.
    The default shader passes the texture coordinate along to the fragment
    shader as "varying highp vec2 qt_TexCoord0".
*/

void QSGShaderEffectItem::setFragmentShader(const QByteArray &code)
{
    if (m_source.fragmentCode.constData() == code.constData())
        return;
    m_source.fragmentCode = code;
    if (isComponentComplete()) {
        reset();
        updateProperties();
    }
    emit fragmentShaderChanged();
}

/*!
    \qmlproperty string ShaderEffectItem::vertexShader

    This property holds the vertex shader's GLSL source code.
    The default shader expects the texture coordinate to be passed from the
    vertex shader as "varying highp vec2 qt_TexCoord0", and it samples from a
    sampler2D named "source".
*/

void QSGShaderEffectItem::setVertexShader(const QByteArray &code)
{
    if (m_source.vertexCode.constData() == code.constData())
        return;
    m_source.vertexCode = code;
    if (isComponentComplete()) {
        reset();
        updateProperties();
    }
    emit vertexShaderChanged();
}

/*!
    \qmlproperty bool ShaderEffectItem::blending

    If this property is true, the output from the \l fragmentShader is blended
    with the background using source-over blend mode. If false, the background
    is disregarded. Blending decreases the performance, so you should set this
    property to false when blending is not needed. The default value is true.
*/

void QSGShaderEffectItem::setBlending(bool enable)
{
    if (blending() == enable)
        return;

    m_blending = enable;
    update();

    emit blendingChanged();
}

/*!
    \qmlproperty object ShaderEffectItem::mesh

    This property holds the mesh definition. If not set, a simple mesh with one
    vertex in each corner is used. Assign a \l GridMesh to this property to get
    a higher resolution grid.
*/

void QSGShaderEffectItem::setMesh(QSGShaderEffectMesh *mesh)
{
    if (mesh == m_mesh)
        return;
    if (m_mesh)
        disconnect(m_mesh, SIGNAL(geometryChanged()), this, 0);
    m_mesh = mesh;
    if (m_mesh)
        connect(m_mesh, SIGNAL(geometryChanged()), this, SLOT(updateGeometry()));
    m_dirtyMesh = true;
    update();
    emit meshChanged();
}

/*!
    \qmlproperty enumeration ShaderEffectItem::cullMode

    This property defines which sides of the element should be visible.

    \list
    \o ShaderEffectItem.NoCulling - Both sides are visible
    \o ShaderEffectItem.BackFaceCulling - only front side is visible
    \o ShaderEffectItem.FrontFaceCulling - only back side is visible
    \endlist

    The default is NoCulling.
*/

void QSGShaderEffectItem::setCullMode(CullMode face)
{
    if (face == m_cullMode)
        return;
    m_cullMode = face;
    update();
    emit cullModeChanged();
}

void QSGShaderEffectItem::changeSource(int index)
{
    Q_ASSERT(index >= 0 && index < m_sources.size());
    QVariant v = property(m_sources.at(index).name.constData());
    setSource(v, index);
}

void QSGShaderEffectItem::updateData()
{
    m_dirtyData = true;
    update();
}

void QSGShaderEffectItem::updateGeometry()
{
    m_dirtyGeometry = true;
    update();
}

void QSGShaderEffectItem::setSource(const QVariant &var, int index)
{
    Q_ASSERT(index >= 0 && index < m_sources.size());

    SourceData &source = m_sources[index];

    source.item = 0;
    if (var.isNull()) {
        return;
    } else if (!qVariantCanConvert<QObject *>(var)) {
        qWarning("Could not assign source of type '%s' to property '%s'.", var.typeName(), source.name.constData());
        return;
    }

    QObject *obj = qVariantValue<QObject *>(var);

    QSGTextureProvider *int3rface = QSGTextureProvider::from(obj);
    if (!int3rface) {
        qWarning("Could not assign property '%s', did not implement QSGTextureProvider.", source.name.constData());
    }

    source.item = qobject_cast<QSGItem *>(obj);

    // TODO: Find better solution.
    // 'source.item' needs a canvas to get a scenegraph node.
    // The easiest way to make sure it gets a canvas is to
    // make it a part of the same item tree as 'this'.
    if (source.item && source.item->parentItem() == 0) {
        source.item->setParentItem(this);
        source.item->setVisible(false);
    }
}

void QSGShaderEffectItem::disconnectPropertySignals()
{
    disconnect(this, 0, this, SLOT(updateData()));
    for (int i = 0; i < m_sources.size(); ++i) {
        SourceData &source = m_sources[i];
        disconnect(this, 0, source.mapper, 0);
        disconnect(source.mapper, 0, this, 0);
    }
}

void QSGShaderEffectItem::connectPropertySignals()
{
    QSet<QByteArray>::const_iterator it;
    for (it = m_source.uniformNames.begin(); it != m_source.uniformNames.end(); ++it) {
        int pi = metaObject()->indexOfProperty(it->constData());
        if (pi >= 0) {
            QMetaProperty mp = metaObject()->property(pi);
            if (!mp.hasNotifySignal())
                qWarning("QSGShaderEffectItem: property '%s' does not have notification method!", it->constData());
            QByteArray signalName("2");
            signalName.append(mp.notifySignal().signature());
            connect(this, signalName, this, SLOT(updateData()));
        } else {
            qWarning("QSGShaderEffectItem: '%s' does not have a matching property!", it->constData());
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
            qWarning("QSGShaderEffectItem: '%s' does not have a matching source!", source.name.constData());
        }
    }
}

void QSGShaderEffectItem::reset()
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
        if (source.item && source.item->parentItem() == this)
            source.item->setParentItem(0);
    }
    m_sources.clear();

    m_programDirty = true;
    m_dirtyMesh = true;
}

void QSGShaderEffectItem::updateProperties()
{
    QByteArray vertexCode = m_source.vertexCode;
    QByteArray fragmentCode = m_source.fragmentCode;
    if (vertexCode.isEmpty())
        vertexCode = qt_default_vertex_code;
    if (fragmentCode.isEmpty())
        fragmentCode = qt_default_fragment_code;

    lookThroughShaderCode(vertexCode);
    lookThroughShaderCode(fragmentCode);

    if (!m_mesh && !m_source.attributeNames.contains(qt_position_attribute_name))
        qWarning("QSGShaderEffectItem: Missing reference to \'%s\'.", qt_position_attribute_name);
    if (!m_mesh && !m_source.attributeNames.contains(qt_texcoord_attribute_name))
        qWarning("QSGShaderEffectItem: Missing reference to \'%s\'.", qt_texcoord_attribute_name);
    if (!m_source.respectsMatrix)
        qWarning("QSGShaderEffectItem: Missing reference to \'qt_ModelViewProjectionMatrix\'.");
    if (!m_source.respectsOpacity)
        qWarning("QSGShaderEffectItem: Missing reference to \'qt_Opacity\'.");

    for (int i = 0; i < m_sources.size(); ++i) {
        QVariant v = property(m_sources.at(i).name);
        setSource(v, i);
    }

    connectPropertySignals();
}

void QSGShaderEffectItem::lookThroughShaderCode(const QByteArray &code)
{
    // Regexp for matching attributes and uniforms.
    // In human readable form: attribute|uniform [lowp|mediump|highp] <type> <name>
    static QRegExp re(QLatin1String("\\b(attribute|uniform)\\b\\s*\\b(?:lowp|mediump|highp)?\\b\\s*\\b(\\w+)\\b\\s*\\b(\\w+)"));
    Q_ASSERT(re.isValid());

    int pos = -1;

    QString wideCode = QString::fromLatin1(code.constData(), code.size());

    while ((pos = re.indexIn(wideCode, pos + 1)) != -1) {
        QByteArray decl = re.cap(1).toLatin1(); // uniform or attribute
        QByteArray type = re.cap(2).toLatin1(); // type
        QByteArray name = re.cap(3).toLatin1(); // variable name

        if (decl == "attribute") {
            m_source.attributeNames.append(name);
        } else {
            Q_ASSERT(decl == "uniform");

            if (name == "qt_ModelViewProjectionMatrix") {
                m_source.respectsMatrix = true;
            } else if (name == "qt_Opacity") {
                m_source.respectsOpacity = true;
            } else {
                m_source.uniformNames.insert(name);
                if (type == "sampler2D") {
                    SourceData d;
                    d.mapper = new QSignalMapper;
                    d.name = name;
                    d.item = 0;
                    m_sources.append(d);
                }
            }
        }
    }
}

void QSGShaderEffectItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_dirtyGeometry = true;
    QSGItem::geometryChanged(newGeometry, oldGeometry);
}

QSGNode *QSGShaderEffectItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGShaderEffectNode *node = static_cast<QSGShaderEffectNode *>(oldNode);

    if (!node) {
        node = new QSGShaderEffectNode;
        node->setMaterial(&m_material);
        m_programDirty = true;
        m_dirtyData = true;
        m_dirtyGeometry = true;
    }

    if (m_dirtyMesh) {
        node->setGeometry(0);
        m_dirtyMesh = false;
        m_dirtyGeometry = true;
    }

    if (m_dirtyGeometry) {
        node->setFlag(QSGNode::OwnsGeometry, false);
        QSGGeometry *geometry = node->geometry();
        QRectF rect(0, 0, width(), height());
        QSGShaderEffectMesh *mesh = m_mesh ? m_mesh : &m_defaultMesh;

        geometry = mesh->updateGeometry(geometry, m_source.attributeNames, rect);
        if (!geometry) {
            delete node;
            return 0;
        }

        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry, true);

        m_dirtyGeometry = false;
    }

    if (m_programDirty) {
        QSGShaderEffectProgram s = m_source;
        if (s.fragmentCode.isEmpty())
            s.fragmentCode = qt_default_fragment_code;
        if (s.vertexCode.isEmpty())
            s.vertexCode = qt_default_vertex_code;
        s.className = metaObject()->className();

        m_material.setProgramSource(s);
        node->markDirty(QSGNode::DirtyMaterial);
        m_programDirty = false;
    }

    // Update blending
    if (bool(m_material.flags() & QSGMaterial::Blending) != m_blending) {
        m_material.setFlag(QSGMaterial::Blending, m_blending);
        node->markDirty(QSGNode::DirtyMaterial);
    }

    if (int(m_material.cullMode()) != int(m_cullMode)) {
        m_material.setCullMode(QSGShaderEffectMaterial::CullMode(m_cullMode));
        node->markDirty(QSGNode::DirtyMaterial);
    }

    if (m_dirtyData) {
        QVector<QPair<QByteArray, QVariant> > values;
        QVector<QPair<QByteArray, QPointer<QSGItem> > > textures;
        const QVector<QPair<QByteArray, QPointer<QSGItem> > > &oldTextures = m_material.textureProviders();

        for (QSet<QByteArray>::const_iterator it = m_source.uniformNames.begin(); 
             it != m_source.uniformNames.end(); ++it) {
            values.append(qMakePair(*it, property(*it)));
        }
        for (int i = 0; i < oldTextures.size(); ++i) {
            QSGTextureProvider *oldSource = QSGTextureProvider::from(oldTextures.at(i).second);
            if (oldSource && oldSource->textureChangedSignal())
                disconnect(oldTextures.at(i).second, oldSource->textureChangedSignal(), node, SLOT(markDirtyTexture()));
        }
        for (int i = 0; i < m_sources.size(); ++i) {
            const SourceData &source = m_sources.at(i);
            textures.append(qMakePair(source.name, source.item));
            QSGTextureProvider *t = QSGTextureProvider::from(source.item);
            if (t && t->textureChangedSignal())
                connect(source.item, t->textureChangedSignal(), node, SLOT(markDirtyTexture()), Qt::DirectConnection);
        }
        m_material.setUniforms(values);
        m_material.setTextureProviders(textures);
        node->markDirty(QSGNode::DirtyMaterial);
        m_dirtyData = false;
    }

    return node;
}

QT_END_NAMESPACE
