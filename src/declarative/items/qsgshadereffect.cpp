/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <private/qsgshadereffect_p.h>
#include <private/qsgshadereffectnode_p.h>

#include "qsgmaterial.h"
#include "qsgitem_p.h"

#include <private/qsgcontext_p.h>
#include <private/qsgtextureprovider_p.h>
#include "qsgcanvas.h"

#include <qsgimage_p.h>
#include <qsgshadereffectsource_p.h>

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

// TODO: Remove after grace period.
QSGShaderEffectItem::QSGShaderEffectItem(QSGItem *parent)
    : QSGShaderEffect(parent)
{
    qWarning("ShaderEffectItem has been deprecated. Use ShaderEffect instead.");
}


/*!
    \qmlclass ShaderEffect QSGShaderEffect
    \since 5.0
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

QSGShaderEffect::QSGShaderEffect(QSGItem *parent)
    : QSGItem(parent)
    , m_meshResolution(1, 1)
    , m_deprecatedMesh(0)
    , m_cullMode(NoCulling)
    , m_blending(true)
    , m_dirtyData(true)
    , m_programDirty(true)
    , m_dirtyMesh(true)
    , m_dirtyGeometry(true)
{
    setFlag(QSGItem::ItemHasContents);
}

QSGShaderEffect::~QSGShaderEffect()
{
    reset();
}

void QSGShaderEffect::componentComplete()
{
    updateProperties();
    QSGItem::componentComplete();
}

/*!
    \qmlproperty string ShaderEffect::fragmentShader

    This property holds the fragment shader's GLSL source code.
    The default shader passes the texture coordinate along to the fragment
    shader as "varying highp vec2 qt_TexCoord0".
*/

void QSGShaderEffect::setFragmentShader(const QByteArray &code)
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
    \qmlproperty string ShaderEffect::vertexShader

    This property holds the vertex shader's GLSL source code.
    The default shader expects the texture coordinate to be passed from the
    vertex shader as "varying highp vec2 qt_TexCoord0", and it samples from a
    sampler2D named "source".
*/

void QSGShaderEffect::setVertexShader(const QByteArray &code)
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
    \qmlproperty bool ShaderEffect::blending

    If this property is true, the output from the \l fragmentShader is blended
    with the background using source-over blend mode. If false, the background
    is disregarded. Blending decreases the performance, so you should set this
    property to false when blending is not needed. The default value is true.
*/

void QSGShaderEffect::setBlending(bool enable)
{
    if (blending() == enable)
        return;

    m_blending = enable;
    update();

    emit blendingChanged();
}

/*!
    \qmlproperty size ShaderEffect::mesh

    This property holds the mesh resolution. The default resolution is 1x1
    which is the minimum and corresponds to a mesh with four vertices.
    For non-linear vertex transformations, you probably want to set the
    resolution higher.

    \row
    \o \image declarative-gridmesh.png
    \o \qml
        import QtQuick 2.0

        ShaderEffect {
            width: 200
            height: 200
            mesh: Qt.size(20, 20)
            property variant source: Image {
                source: "qt-logo.png"
                sourceSize { width: 200; height: 200 }
                smooth: true
            }
            vertexShader: "
                uniform highp mat4 qt_Matrix;
                attribute highp vec4 qt_Vertex;
                attribute highp vec2 qt_MultiTexCoord0;
                varying highp vec2 qt_TexCoord0;
                uniform highp float width;
                void main() {
                    highp vec4 pos = qt_Vertex;
                    highp float d = .5 * smoothstep(0., 1., qt_MultiTexCoord0.y);
                    pos.x = width * mix(d, 1.0 - d, qt_MultiTexCoord0.x);
                    gl_Position = qt_Matrix * pos;
                    qt_TexCoord0 = qt_MultiTexCoord0;
                }"
        }
        \endqml
    \endrow
*/

QVariant QSGShaderEffect::mesh() const
{
    return m_deprecatedMesh ? qVariantFromValue(static_cast<QObject *>(m_deprecatedMesh))
                            : qVariantFromValue(m_meshResolution);
}

void QSGShaderEffect::setMesh(const QVariant &mesh)
{
    // TODO: Replace QVariant with QSize after grace period.
    QSGShaderEffectMesh *newMesh = qobject_cast<QSGShaderEffectMesh *>(qVariantValue<QObject *>(mesh));
    if (newMesh && newMesh == m_deprecatedMesh)
        return;
    if (m_deprecatedMesh)
        disconnect(m_deprecatedMesh, SIGNAL(geometryChanged()), this, 0);
    m_deprecatedMesh = newMesh;
    if (m_deprecatedMesh) {
        qWarning("ShaderEffect: Setting the mesh to something other than a size is deprecated.");
        connect(m_deprecatedMesh, SIGNAL(geometryChanged()), this, SLOT(updateGeometry()));
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
                qWarning("ShaderEffect: mesh resolution must be a size.");
        }
        m_defaultMesh.setResolution(m_meshResolution);
    }

    m_dirtyMesh = true;
    update();
    emit meshChanged();
}

/*!
    \qmlproperty enumeration ShaderEffect::cullMode

    This property defines which sides of the element should be visible.

    \list
    \o ShaderEffect.NoCulling - Both sides are visible
    \o ShaderEffect.BackFaceCulling - only front side is visible
    \o ShaderEffect.FrontFaceCulling - only back side is visible
    \endlist

    The default is NoCulling.
*/

void QSGShaderEffect::setCullMode(CullMode face)
{
    if (face == m_cullMode)
        return;
    m_cullMode = face;
    update();
    emit cullModeChanged();
}

void QSGShaderEffect::changeSource(int index)
{
    Q_ASSERT(index >= 0 && index < m_sources.size());
    QVariant v = property(m_sources.at(index).name.constData());
    setSource(v, index);
}

void QSGShaderEffect::updateData()
{
    m_dirtyData = true;
    update();
}

void QSGShaderEffect::updateGeometry()
{
    m_dirtyGeometry = true;
    update();
}

void QSGShaderEffect::setSource(const QVariant &var, int index)
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
    QSGItem *item = qobject_cast<QSGItem *>(obj);
    if (!item || !item->isTextureProvider()) {
        qWarning("ShaderEffect: source uniform [%s] is not assigned a valid texture provider: %s [%s]",
                 qPrintable(source.name), qPrintable(obj->objectName()), obj->metaObject()->className());
        return;
    }

    source.sourceObject = item;


    // TODO: Find better solution.
    // 'item' needs a canvas to get a scenegraph node.
    // The easiest way to make sure it gets a canvas is to
    // make it a part of the same item tree as 'this'.
    if (item && item->parentItem() == 0) {
        item->setParentItem(this);
        item->setVisible(false);
    }
}

void QSGShaderEffect::disconnectPropertySignals()
{
    disconnect(this, 0, this, SLOT(updateData()));
    for (int i = 0; i < m_sources.size(); ++i) {
        SourceData &source = m_sources[i];
        disconnect(this, 0, source.mapper, 0);
        disconnect(source.mapper, 0, this, 0);
    }
}

void QSGShaderEffect::connectPropertySignals()
{
    QSet<QByteArray>::const_iterator it;
    for (it = m_source.uniformNames.begin(); it != m_source.uniformNames.end(); ++it) {
        int pi = metaObject()->indexOfProperty(it->constData());
        if (pi >= 0) {
            QMetaProperty mp = metaObject()->property(pi);
            if (!mp.hasNotifySignal())
                qWarning("QSGShaderEffect: property '%s' does not have notification method!", it->constData());
            QByteArray signalName("2");
            signalName.append(mp.notifySignal().signature());
            connect(this, signalName, this, SLOT(updateData()));
        } else {
            qWarning("QSGShaderEffect: '%s' does not have a matching property!", it->constData());
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
            qWarning("QSGShaderEffect: '%s' does not have a matching source!", source.name.constData());
        }
    }
}

void QSGShaderEffect::reset()
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
        QSGItem *item = qobject_cast<QSGItem *>(source.sourceObject);
        if (item && item->parentItem() == this)
            item->setParentItem(0);
    }
    m_sources.clear();

    m_programDirty = true;
    m_dirtyMesh = true;
}

void QSGShaderEffect::updateProperties()
{
    QByteArray vertexCode = m_source.vertexCode;
    QByteArray fragmentCode = m_source.fragmentCode;
    if (vertexCode.isEmpty())
        vertexCode = qt_default_vertex_code;
    if (fragmentCode.isEmpty())
        fragmentCode = qt_default_fragment_code;

    lookThroughShaderCode(vertexCode);
    lookThroughShaderCode(fragmentCode);

    // TODO: Remove !m_deprecatedMesh check after grace period.
    if (!m_deprecatedMesh && !m_source.attributeNames.contains(qt_position_attribute_name))
        qWarning("QSGShaderEffect: Missing reference to \'%s\'.", qt_position_attribute_name);
    if (!m_deprecatedMesh && !m_source.attributeNames.contains(qt_texcoord_attribute_name))
        qWarning("QSGShaderEffect: Missing reference to \'%s\'.", qt_texcoord_attribute_name);
    if (!m_source.respectsMatrix)
        qWarning("QSGShaderEffect: Missing reference to \'qt_Matrix\'.");
    if (!m_source.respectsOpacity)
        qWarning("QSGShaderEffect: Missing reference to \'qt_Opacity\'.");

    for (int i = 0; i < m_sources.size(); ++i) {
        QVariant v = property(m_sources.at(i).name);
        setSource(v, i);
    }

    connectPropertySignals();
}

void QSGShaderEffect::lookThroughShaderCode(const QByteArray &code)
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

            if (name == "qt_Matrix") {
                m_source.respectsMatrix = true;
            } else if (name == "qt_ModelViewProjectionMatrix") {
                // TODO: Remove after grace period.
                qWarning("ShaderEffect: qt_ModelViewProjectionMatrix is deprecated. Use qt_Matrix instead.");
                m_source.respectsMatrix = true;
            } else if (name == "qt_Opacity") {
                m_source.respectsOpacity = true;
            } else {
                m_source.uniformNames.insert(name);
                if (type == "sampler2D") {
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

void QSGShaderEffect::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_dirtyGeometry = true;
    QSGItem::geometryChanged(newGeometry, oldGeometry);
}

QSGNode *QSGShaderEffect::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGShaderEffectNode *node = static_cast<QSGShaderEffectNode *>(oldNode);

    if (!node) {
        node = new QSGShaderEffectNode;
        m_programDirty = true;
        m_dirtyData = true;
        m_dirtyGeometry = true;
    }

    QSGShaderEffectMaterial *material = node->shaderMaterial();

    if (m_dirtyMesh) {
        node->setGeometry(0);
        m_dirtyMesh = false;
        m_dirtyGeometry = true;
    }

    if (m_dirtyGeometry) {
        node->setFlag(QSGNode::OwnsGeometry, false);
        QSGGeometry *geometry = node->geometry();
        QRectF rect(0, 0, width(), height());
        QSGShaderEffectMesh *mesh = m_deprecatedMesh ? m_deprecatedMesh : &m_defaultMesh;

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
        material->setCullMode(QSGShaderEffectMaterial::CullMode(m_cullMode));
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
            QSGTextureProvider *t = source.sourceObject->textureProvider();
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

QT_END_NAMESPACE
