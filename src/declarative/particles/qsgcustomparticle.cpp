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

#include "qsgcustomparticle_p.h"
#include <private/qsgshadereffectmesh_p.h>
#include <cstdlib>

QT_BEGIN_NAMESPACE

//Includes comments because the code isn't self explanatory
static const char qt_particles_template_vertex_code[] =
        "attribute highp vec2 qt_ParticlePos;\n"
        "attribute highp vec2 qt_ParticleTex;\n"
        "attribute highp vec4 qt_ParticleData; //  x = time,  y = lifeSpan, z = size,  w = endSize\n"
        "attribute highp vec4 qt_ParticleVec; // x,y = constant speed,  z,w = acceleration\n"
        "attribute highp float qt_ParticleR;\n"
        "uniform highp mat4 qt_Matrix;\n"
        "uniform highp float qt_Timestamp;\n"
        "varying highp vec2 qt_TexCoord0;\n"
        "void defaultMain() {\n"
        "    qt_TexCoord0 = qt_ParticleTex;\n"
        "    highp float size = qt_ParticleData.z;\n"
        "    highp float endSize = qt_ParticleData.w;\n"
        "    highp float t = (qt_Timestamp - qt_ParticleData.x) / qt_ParticleData.y;\n"
        "    highp float currentSize = mix(size, endSize, t * t);\n"
        "    if (t < 0. || t > 1.)\n"
        "        currentSize = 0.;\n"
        "    highp vec2 pos = qt_ParticlePos\n"
        "                   - currentSize / 2. + currentSize * qt_ParticleTex   // adjust size\n"
        "                   + qt_ParticleVec.xy * t * qt_ParticleData.y         // apply speed vector..\n"
        "                   + 0.5 * qt_ParticleVec.zw * pow(t * qt_ParticleData.y, 2.);\n"
        "    gl_Position = qt_Matrix * vec4(pos.x, pos.y, 0, 1);\n"
        "}";
static const char qt_particles_default_vertex_code[] =
        "void main() {        \n"
        "    defaultMain();   \n"
        "}";

static const char qt_particles_default_fragment_code[] =
        "uniform sampler2D source;                                  \n"
        "varying highp vec2 qt_TexCoord0;                           \n"
        "uniform lowp float qt_Opacity;                             \n"
        "void main() {                                              \n"
        "    gl_FragColor = texture2D(source, fTex) * qt_Opacity;   \n"
        "}";

static QSGGeometry::Attribute PlainParticle_Attributes[] = {
    QSGGeometry::Attribute::create(0, 2, GL_FLOAT, true),       // Position
    QSGGeometry::Attribute::create(1, 2, GL_FLOAT),             // TexCoord
    QSGGeometry::Attribute::create(2, 4, GL_FLOAT),             // Data
    QSGGeometry::Attribute::create(3, 4, GL_FLOAT),             // Vectors
    QSGGeometry::Attribute::create(4, 1, GL_FLOAT)              // r
};

static QSGGeometry::AttributeSet PlainParticle_AttributeSet =
{
    5, // Attribute Count
    (2 + 2 + 4 + 4 + 1) * sizeof(float),
    PlainParticle_Attributes
};

struct PlainVertex {
    float x;
    float y;
    float tx;
    float ty;
    float t;
    float lifeSpan;
    float size;
    float endSize;
    float vx;
    float vy;
    float ax;
    float ay;
    float r;
};

struct PlainVertices {
    PlainVertex v1;
    PlainVertex v2;
    PlainVertex v3;
    PlainVertex v4;
};

/*!
    \qmlclass CustomParticle QSGCustomParticle
    \inqmlmodule QtQuick.Particles 2
    \inherits ParticlePainter
    \brief The CustomParticle element allows you to specify your own shader to paint particles.

*/

QSGCustomParticle::QSGCustomParticle(QSGItem* parent)
    : QSGParticlePainter(parent)
    , m_dirtyData(true)
    , m_material(0)
    , m_rootNode(0)
{
    setFlag(QSGItem::ItemHasContents);
}

class QSGShaderEffectMaterialObject : public QObject, public QSGShaderEffectMaterial { };

QSGCustomParticle::~QSGCustomParticle()
{
    if (m_material)
        m_material->deleteLater();
}

void QSGCustomParticle::componentComplete()
{
    reset();
    QSGParticlePainter::componentComplete();
}


//Trying to keep the shader conventions the same as in qsgshadereffectitem
/*!
    \qmlproperty string QtQuick.Particles2::CustomParticle::fragmentShader

    This property holds the fragment shader's GLSL source code.
    The default shader expects the texture coordinate to be passed from the
    vertex shader as "varying highp vec2 qt_TexCoord0", and it samples from a
    sampler2D named "source".
*/

void QSGCustomParticle::setFragmentShader(const QByteArray &code)
{
    if (m_source.fragmentCode.constData() == code.constData())
        return;
    m_source.fragmentCode = code;
    if (isComponentComplete()) {
        reset();
    }
    emit fragmentShaderChanged();
}

/*!
    \qmlproperty string QtQuick.Particles2::CustomParticle::vertexShader

    This property holds the vertex shader's GLSL source code.

    The default shader passes the texture coordinate along to the fragment
    shader as "varying highp vec2 qt_TexCoord0".

    To aid writing a particle vertex shader, the following GLSL code is prepended
    to your vertex shader:
    \code
        attribute highp vec2 qt_ParticlePos;
        attribute highp vec2 qt_ParticleTex;
        attribute highp vec4 qt_ParticleData; //  x = time,  y = lifeSpan, z = size,  w = endSize
        attribute highp vec4 qt_ParticleVec; // x,y = constant speed,  z,w = acceleration
        attribute highp float qt_ParticleR;
        uniform highp mat4 qt_Matrix;
        uniform highp float qt_Timestamp;
        varying highp vec2 qt_TexCoord0;
        void defaultMain() {
            qt_TexCoord0 = qt_ParticleTex;
            highp float size = qt_ParticleData.z;
            highp float endSize = qt_ParticleData.w;
            highp float t = (qt_Timestamp - qt_ParticleData.x) / qt_ParticleData.y;
            highp float currentSize = mix(size, endSize, t * t);
            if (t < 0. || t > 1.)
                currentSize = 0.;
            highp vec2 pos = qt_ParticlePos
                           - currentSize / 2. + currentSize * qt_ParticleTex   // adjust size
                           + qt_ParticleVec.xy * t * qt_ParticleData.y         // apply speed vector..
                           + 0.5 * qt_ParticleVec.zw * pow(t * qt_ParticleData.y, 2.);
            gl_Position = qt_Matrix * vec4(pos.x, pos.y, 0, 1);
        }
    \endcode

    defaultMain() is the same code as in the default shader, you can call this for basic
    particle functions and then add additional variables for custom effects. Note that
    the vertex shader for particles is responsible for simulating the movement of particles
    over time, the particle data itself only has the starting position and spawn time.
*/

void QSGCustomParticle::setVertexShader(const QByteArray &code)
{
    if (m_source.vertexCode.constData() == code.constData())
        return;
    m_source.vertexCode = code;
    if (isComponentComplete()) {
        reset();
    }
    emit vertexShaderChanged();
}

void QSGCustomParticle::reset()
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

    QSGParticlePainter::reset();
    m_pleaseReset = true;
    update();
}


void QSGCustomParticle::changeSource(int index)
{
    Q_ASSERT(index >= 0 && index < m_sources.size());
    QVariant v = property(m_sources.at(index).name.constData());
    setSource(v, index);
}

void QSGCustomParticle::updateData()
{
    m_dirtyData = true;
    update();
}

void QSGCustomParticle::setSource(const QVariant &var, int index)
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
    source.item = qobject_cast<QSGItem *>(obj);
    if (!source.item || !source.item->isTextureProvider()) {
        qWarning("ShaderEffect: source uniform [%s] is not assigned a valid texture provider: %s [%s]",
                 source.name.constData(), qPrintable(obj->objectName()), obj->metaObject()->className());
        return;
    }

    // TODO: Copy better solution in QSGShaderEffect when they find it.
    // 'source.item' needs a canvas to get a scenegraph node.
    // The easiest way to make sure it gets a canvas is to
    // make it a part of the same item tree as 'this'.
    if (source.item && source.item->parentItem() == 0) {
        source.item->setParentItem(this);
        source.item->setVisible(false);
    }
}

void QSGCustomParticle::disconnectPropertySignals()
{
    disconnect(this, 0, this, SLOT(updateData()));
    for (int i = 0; i < m_sources.size(); ++i) {
        SourceData &source = m_sources[i];
        disconnect(this, 0, source.mapper, 0);
        disconnect(source.mapper, 0, this, 0);
    }
}

void QSGCustomParticle::connectPropertySignals()
{
    QSet<QByteArray>::const_iterator it;
    for (it = m_source.uniformNames.begin(); it != m_source.uniformNames.end(); ++it) {
        int pi = metaObject()->indexOfProperty(it->constData());
        if (pi >= 0) {
            QMetaProperty mp = metaObject()->property(pi);
            if (!mp.hasNotifySignal())
                qWarning("QSGCustomParticle: property '%s' does not have notification method!", it->constData());
            QByteArray signalName("2");
            signalName.append(mp.notifySignal().signature());
            connect(this, signalName, this, SLOT(updateData()));
        } else {
            qWarning("QSGCustomParticle: '%s' does not have a matching property!", it->constData());
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
            qWarning("QSGCustomParticle: '%s' does not have a matching source!", source.name.constData());
        }
    }
}

void QSGCustomParticle::updateProperties()
{
    QByteArray vertexCode = m_source.vertexCode;
    QByteArray fragmentCode = m_source.fragmentCode;
    if (vertexCode.isEmpty())
        vertexCode = qt_particles_default_vertex_code;
    if (fragmentCode.isEmpty())
        fragmentCode = qt_particles_default_fragment_code;
    vertexCode = qt_particles_template_vertex_code + vertexCode;

    m_source.attributeNames.clear();
    m_source.attributeNames << "qt_ParticlePos"
                            << "qt_ParticleTex"
                            << "qt_ParticleData"
                            << "qt_ParticleVec"
                            << "qt_ParticleR";

    lookThroughShaderCode(vertexCode);
    lookThroughShaderCode(fragmentCode);

    if (!m_source.respectsMatrix)
        qWarning("QSGCustomParticle: Missing reference to \'qt_Matrix\'.");
    if (!m_source.respectsOpacity)
        qWarning("QSGCustomParticle: Missing reference to \'qt_Opacity\'.");

    for (int i = 0; i < m_sources.size(); ++i) {
        QVariant v = property(m_sources.at(i).name);
        setSource(v, i);
    }

    connectPropertySignals();
}

void QSGCustomParticle::lookThroughShaderCode(const QByteArray &code)
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
            if (!m_source.attributeNames.contains(name))
                qWarning() << "Custom Particle: Unknown attribute " << name;
        } else {
            Q_ASSERT(decl == "uniform");//TODO: Shouldn't assert

            if (name == "qt_Matrix") {
                m_source.respectsMatrix = true;
            } else if (name == "qt_Opacity") {
                m_source.respectsOpacity = true;
            } else if (name == "qt_Timestamp") {
                //Not strictly necessary
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

QSGNode *QSGCustomParticle::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    Q_UNUSED(oldNode);
    if (m_pleaseReset){

        //delete m_material;//Shader effect item doesn't regen material?

        delete m_rootNode;//Automatically deletes children
        m_rootNode = 0;
        m_nodes.clear();
        m_pleaseReset = false;
        m_dirtyData = false;
    }

    if (m_system && m_system->isRunning() && !m_system->isPaused()){
        prepareNextFrame();
        if (m_rootNode) {
            update();
            //### Should I be using dirty geometry too/instead?
            foreach (QSGGeometryNode* node, m_nodes)
                node->markDirty(QSGNode::DirtyMaterial);//done in buildData?
        }
    }

    return m_rootNode;
}

void QSGCustomParticle::prepareNextFrame(){
    if (!m_rootNode)
        m_rootNode = buildCustomNodes();
    if (!m_rootNode)
        return;

    m_lastTime = m_system->systemSync(this) / 1000.;
    if (m_dirtyData || true)//Currently this is how we update timestamp... potentially over expensive.
        buildData();
}

QSGShaderEffectNode* QSGCustomParticle::buildCustomNodes()
{
#ifdef QT_OPENGL_ES_2
    if (m_count * 4 > 0xffff) {
        printf("CustomParticle: Too many particles... \n");
        return 0;
    }
#endif

    if (m_count <= 0) {
        printf("CustomParticle: Too few particles... \n");
        return 0;
    }

    updateProperties();

    QSGShaderEffectProgram s = m_source;
    if (s.fragmentCode.isEmpty())
        s.fragmentCode = qt_particles_default_fragment_code;
    if (s.vertexCode.isEmpty())
        s.vertexCode = qt_particles_default_vertex_code;

    if (!m_material) {
        m_material = new QSGShaderEffectMaterialObject;
    }

    s.vertexCode = qt_particles_template_vertex_code + s.vertexCode;
    m_material->setProgramSource(s);
    foreach (const QString &str, m_groups){
        int gIdx = m_system->m_groupIds[str];
        int count = m_system->m_groupData[gIdx]->size();
        //Create Particle Geometry
        int vCount = count * 4;
        int iCount = count * 6;
        QSGGeometry *g = new QSGGeometry(PlainParticle_AttributeSet, vCount, iCount);
        g->setDrawingMode(GL_TRIANGLES);
        PlainVertex *vertices = (PlainVertex *) g->vertexData();
        for (int p=0; p < count; ++p) {
            commit(gIdx, p);
            vertices[0].tx = 0;
            vertices[0].ty = 0;

            vertices[1].tx = 1;
            vertices[1].ty = 0;

            vertices[2].tx = 0;
            vertices[2].ty = 1;

            vertices[3].tx = 1;
            vertices[3].ty = 1;
            vertices += 4;
        }
        quint16 *indices = g->indexDataAsUShort();
        for (int i=0; i < count; ++i) {
            int o = i * 4;
            indices[0] = o;
            indices[1] = o + 1;
            indices[2] = o + 2;
            indices[3] = o + 1;
            indices[4] = o + 3;
            indices[5] = o + 2;
            indices += 6;
        }

        QSGShaderEffectNode* node = new QSGShaderEffectNode();

        node->setGeometry(g);
        node->setMaterial(m_material);
        node->markDirty(QSGNode::DirtyMaterial);

        m_nodes.insert(gIdx, node);
    }
    foreach (QSGShaderEffectNode* node, m_nodes){
        if (node == *(m_nodes.begin()))
                continue;
        (*(m_nodes.begin()))->appendChildNode(node);
    }

    return *(m_nodes.begin());
}


void QSGCustomParticle::buildData()
{
    if (!m_rootNode)
        return;
    const QByteArray timestampName("qt_Timestamp");
    QVector<QPair<QByteArray, QVariant> > values;
    QVector<QPair<QByteArray, QSGTextureProvider *> > textures;
    const QVector<QPair<QByteArray, QSGTextureProvider *> > &oldTextures = m_material->textureProviders();
    for (int i = 0; i < oldTextures.size(); ++i) {
        QSGTextureProvider *t = oldTextures.at(i).second;
        if (t)
            foreach (QSGShaderEffectNode* node, m_nodes)
                disconnect(t, SIGNAL(textureChanged()), node, SLOT(markDirtyTexture()));
    }
    for (int i = 0; i < m_sources.size(); ++i) {
        const SourceData &source = m_sources.at(i);
        QSGTextureProvider *t = source.item->textureProvider();
        textures.append(qMakePair(source.name, t));
        if (t)
            foreach (QSGShaderEffectNode* node, m_nodes)
                connect(t, SIGNAL(textureChanged()), node, SLOT(markDirtyTexture()), Qt::DirectConnection);
    }
    for (QSet<QByteArray>::const_iterator it = m_source.uniformNames.begin();
         it != m_source.uniformNames.end(); ++it) {
        values.append(qMakePair(*it, property(*it)));
    }
    values.append(qMakePair(timestampName, QVariant(m_lastTime)));
    m_material->setUniforms(values);
    m_material->setTextureProviders(textures);
    m_dirtyData = false;
    foreach (QSGShaderEffectNode* node, m_nodes)
        node->markDirty(QSGNode::DirtyMaterial);
}

void QSGCustomParticle::initialize(int gIdx, int pIdx)
{
    QSGParticleData* datum = m_system->m_groupData[gIdx]->data[pIdx];
    datum->r = rand()/(qreal)RAND_MAX;
}

void QSGCustomParticle::commit(int gIdx, int pIdx)
{
    if (m_nodes[gIdx] == 0)
        return;

    QSGParticleData* datum = m_system->m_groupData[gIdx]->data[pIdx];
    PlainVertices *particles = (PlainVertices *) m_nodes[gIdx]->geometry()->vertexData();
    PlainVertex *vertices = (PlainVertex *)&particles[pIdx];
    for (int i=0; i<4; ++i) {
        vertices[i].x = datum->x - m_systemOffset.x();
        vertices[i].y = datum->y - m_systemOffset.y();
        vertices[i].t = datum->t;
        vertices[i].lifeSpan = datum->lifeSpan;
        vertices[i].size = datum->size;
        vertices[i].endSize = datum->endSize;
        vertices[i].vx = datum->vx;
        vertices[i].vy = datum->vy;
        vertices[i].ax = datum->ax;
        vertices[i].ay = datum->ay;
        vertices[i].r = datum->r;
    }
}

QT_END_NAMESPACE
