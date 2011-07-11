/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
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

#include <private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>
#include <qsgnode.h>
#include <qsgtexturematerial.h>
#include <qsgtexture.h>
#include <QFile>
#include "qsgimageparticle_p.h"
#include "qsgparticleemitter_p.h"
#include "qsgsprite_p.h"
#include "qsgspriteengine_p.h"
#include <QGLFunctions>
#include <qsgengine.h>

QT_BEGIN_NAMESPACE

const float CONV = 0.017453292519943295;
class UltraMaterial : public QSGMaterial
{
public:
    UltraMaterial(bool withSprites=false)
        : timestamp(0)
        , framecount(1)
        , animcount(1)
        , usesSprites(withSprites)
    {
        setFlag(Blending, true);
    }

    ~UltraMaterial()
    {
        delete texture;
        delete colortable;
        delete sizetable;
        delete opacitytable;
    }

    virtual QSGMaterialType *type() const { static QSGMaterialType type; return &type; }
    virtual QSGMaterialShader *createShader() const;
    virtual int compare(const QSGMaterial *other) const
    {
        return this - static_cast<const UltraMaterial *>(other);
    }

    QSGTexture *texture;
    QSGTexture *colortable;
    QSGTexture *sizetable;
    QSGTexture *opacitytable;

    qreal timestamp;
    int framecount;
    int animcount;
    bool usesSprites;
};
class UltraMaterialData : public QSGMaterialShader
{
public:
    UltraMaterialData(const char *vertexFile = 0, const char *fragmentFile = 0)
    {
        QFile vf(vertexFile ? vertexFile : ":defaultshaders/ultravertex.shader");
        vf.open(QFile::ReadOnly);
        m_vertex_code = vf.readAll();

        QFile ff(fragmentFile ? fragmentFile : ":defaultshaders/ultrafragment.shader");
        ff.open(QFile::ReadOnly);
        m_fragment_code = ff.readAll();

        Q_ASSERT(!m_vertex_code.isNull());
        Q_ASSERT(!m_fragment_code.isNull());
    }

    void deactivate() {
        QSGMaterialShader::deactivate();

        for (int i=0; i<8; ++i) {
            program()->setAttributeArray(i, GL_FLOAT, chunkOfBytes, 1, 0);
        }
    }

    virtual void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *)
    {
        UltraMaterial *m = static_cast<UltraMaterial *>(newEffect);
        state.context()->functions()->glActiveTexture(GL_TEXTURE1);
        m->colortable->bind();

        state.context()->functions()->glActiveTexture(GL_TEXTURE2);
        m->sizetable->bind();

        state.context()->functions()->glActiveTexture(GL_TEXTURE3);
        m->opacitytable->bind();

        // make sure we end by setting GL_TEXTURE0 as active texture
        state.context()->functions()->glActiveTexture(GL_TEXTURE0);
        m->texture->bind();

        program()->setUniformValue(m_opacity_id, state.opacity());
        program()->setUniformValue(m_timestamp_id, (float) m->timestamp);
        program()->setUniformValue(m_framecount_id, (float) m->framecount);
        program()->setUniformValue(m_animcount_id, (float) m->animcount);

        if (state.isMatrixDirty())
            program()->setUniformValue(m_matrix_id, state.combinedMatrix());
    }

    virtual void initialize() {
        program()->bind();
        program()->setUniformValue("texture", 0);
        program()->setUniformValue("colortable", 1);
        program()->setUniformValue("sizetable", 2);
        program()->setUniformValue("opacitytable", 3);
        m_matrix_id = program()->uniformLocation("matrix");
        m_opacity_id = program()->uniformLocation("opacity");
        m_timestamp_id = program()->uniformLocation("timestamp");
        m_framecount_id = program()->uniformLocation("framecount");
        m_animcount_id = program()->uniformLocation("animcount");
    }

    virtual const char *vertexShader() const { return m_vertex_code.constData(); }
    virtual const char *fragmentShader() const { return m_fragment_code.constData(); }

    virtual char const *const *attributeNames() const {
        static const char *attr[] = {
            "vPos",
            "vTex",
            "vData",
            "vVec",
            "vColor",
            "vDeformVec",
            "vRotation",
            "vAnimData",
            0
        };
        return attr;
    }

    virtual bool isColorTable() const { return false; }

    int m_matrix_id;
    int m_opacity_id;
    int m_timestamp_id;
    int m_framecount_id;
    int m_animcount_id;

    QByteArray m_vertex_code;
    QByteArray m_fragment_code;

    static float chunkOfBytes[1024];
};
float UltraMaterialData::chunkOfBytes[1024];

QSGMaterialShader *UltraMaterial::createShader() const
{
    if (usesSprites)//TODO: Perhaps just swap the shaders, and don't mind the extra vector?
        return new UltraMaterialData;
    else
        return new UltraMaterialData;
}


class SimpleMaterial : public UltraMaterial
{
    virtual QSGMaterialShader *createShader() const;
    virtual QSGMaterialType *type() const { static QSGMaterialType type; return &type; }
};

class SimpleMaterialData : public QSGMaterialShader
{
public:
    SimpleMaterialData(const char *vertexFile = 0, const char *fragmentFile = 0)
    {
        QFile vf(vertexFile ? vertexFile : ":defaultshaders/simplevertex.shader");
        vf.open(QFile::ReadOnly);
        m_vertex_code = vf.readAll();

        QFile ff(fragmentFile ? fragmentFile : ":defaultshaders/simplefragment.shader");
        ff.open(QFile::ReadOnly);
        m_fragment_code = ff.readAll();

        Q_ASSERT(!m_vertex_code.isNull());
        Q_ASSERT(!m_fragment_code.isNull());
    }

    void deactivate() {
        QSGMaterialShader::deactivate();

        for (int i=0; i<8; ++i) {
            program()->setAttributeArray(i, GL_FLOAT, chunkOfBytes, 1, 0);
        }
    }

    virtual void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *)
    {
        UltraMaterial *m = static_cast<UltraMaterial *>(newEffect);
        state.context()->functions()->glActiveTexture(GL_TEXTURE0);
        m->texture->bind();

        program()->setUniformValue(m_opacity_id, state.opacity());
        program()->setUniformValue(m_timestamp_id, (float) m->timestamp);

        if (state.isMatrixDirty())
            program()->setUniformValue(m_matrix_id, state.combinedMatrix());
    }

    virtual void initialize() {
        m_matrix_id = program()->uniformLocation("matrix");
        m_opacity_id = program()->uniformLocation("opacity");
        m_timestamp_id = program()->uniformLocation("timestamp");
    }

    virtual const char *vertexShader() const { return m_vertex_code.constData(); }
    virtual const char *fragmentShader() const { return m_fragment_code.constData(); }

    virtual char const *const *attributeNames() const {
        static const char *attr[] = {
            "vPos",
            "vTex",
            "vData",
            "vVec",
            0
        };
        return attr;
    }

    virtual bool isColorTable() const { return false; }

    int m_matrix_id;
    int m_opacity_id;
    int m_timestamp_id;

    QByteArray m_vertex_code;
    QByteArray m_fragment_code;

    static float chunkOfBytes[1024];
};
float SimpleMaterialData::chunkOfBytes[1024];

QSGMaterialShader *SimpleMaterial::createShader() const {
    return new SimpleMaterialData;
}

QSGImageParticle::QSGImageParticle(QSGItem* parent)
    : QSGParticlePainter(parent)
    , m_do_reset(false)
    , m_color_variation(0.0)
    , m_rootNode(0)
    , m_material(0)
    , m_alphaVariation(0.0)
    , m_alpha(1.0)
    , m_redVariation(0.0)
    , m_greenVariation(0.0)
    , m_blueVariation(0.0)
    , m_rotation(0)
    , m_autoRotation(false)
    , m_xVector(0)
    , m_yVector(0)
    , m_rotationVariation(0)
    , m_rotationSpeed(0)
    , m_rotationSpeedVariation(0)
    , m_spriteEngine(0)
    , m_bloat(false)
    , perfLevel(Unknown)
    , m_lastLevel(Unknown)
{
    setFlag(ItemHasContents);
}

QDeclarativeListProperty<QSGSprite> QSGImageParticle::sprites()
{
    return QDeclarativeListProperty<QSGSprite>(this, &m_sprites, spriteAppend, spriteCount, spriteAt, spriteClear);
}

void QSGImageParticle::setImage(const QUrl &image)
{
    if (image == m_image_name)
        return;
    m_image_name = image;
    emit imageChanged();
    reset();
}


void QSGImageParticle::setColortable(const QUrl &table)
{
    if (table == m_colortable_name)
        return;
    m_colortable_name = table;
    emit colortableChanged();
    reset();
}

void QSGImageParticle::setSizetable(const QUrl &table)
{
    if (table == m_sizetable_name)
        return;
    m_sizetable_name = table;
    emit sizetableChanged();
    reset();
}

void QSGImageParticle::setOpacitytable(const QUrl &table)
{
    if (table == m_opacitytable_name)
        return;
    m_opacitytable_name = table;
    emit opacitytableChanged();
    reset();
}

void QSGImageParticle::setColor(const QColor &color)
{
    if (color == m_color)
        return;
    m_color = color;
    emit colorChanged();
    if (perfLevel < Colored)
        reset();
}

void QSGImageParticle::setColorVariation(qreal var)
{
    if (var == m_color_variation)
        return;
    m_color_variation = var;
    emit colorVariationChanged();
    if (perfLevel < Colored)
        reset();
}

void QSGImageParticle::setAlphaVariation(qreal arg)
{
    if (m_alphaVariation != arg) {
        m_alphaVariation = arg;
        emit alphaVariationChanged(arg);
    }
    if (perfLevel < Colored)
        reset();
}

void QSGImageParticle::setAlpha(qreal arg)
{
    if (m_alpha != arg) {
        m_alpha = arg;
        emit alphaChanged(arg);
    }
    if (perfLevel < Colored)
        reset();
}

void QSGImageParticle::setRedVariation(qreal arg)
{
    if (m_redVariation != arg) {
        m_redVariation = arg;
        emit redVariationChanged(arg);
    }
    if (perfLevel < Colored)
        reset();
}

void QSGImageParticle::setGreenVariation(qreal arg)
{
    if (m_greenVariation != arg) {
        m_greenVariation = arg;
        emit greenVariationChanged(arg);
    }
    if (perfLevel < Colored)
        reset();
}

void QSGImageParticle::setBlueVariation(qreal arg)
{
    if (m_blueVariation != arg) {
        m_blueVariation = arg;
        emit blueVariationChanged(arg);
    }
    if (perfLevel < Colored)
        reset();
}

void QSGImageParticle::setRotation(qreal arg)
{
    if (m_rotation != arg) {
        m_rotation = arg;
        emit rotationChanged(arg);
    }
    if (perfLevel < Deformable)
        reset();
}

void QSGImageParticle::setRotationVariation(qreal arg)
{
    if (m_rotationVariation != arg) {
        m_rotationVariation = arg;
        emit rotationVariationChanged(arg);
    }
    if (perfLevel < Deformable)
        reset();
}

void QSGImageParticle::setRotationSpeed(qreal arg)
{
    if (m_rotationSpeed != arg) {
        m_rotationSpeed = arg;
        emit rotationSpeedChanged(arg);
    }
    if (perfLevel < Deformable)
        reset();
}

void QSGImageParticle::setRotationSpeedVariation(qreal arg)
{
    if (m_rotationSpeedVariation != arg) {
        m_rotationSpeedVariation = arg;
        emit rotationSpeedVariationChanged(arg);
    }
    if (perfLevel < Deformable)
        reset();
}

void QSGImageParticle::setAutoRotation(bool arg)
{
    if (m_autoRotation != arg) {
        m_autoRotation = arg;
        emit autoRotationChanged(arg);
    }
    if (perfLevel < Deformable)
        reset();
}

void QSGImageParticle::setXVector(QSGStochasticDirection* arg)
{
    if (m_xVector != arg) {
        m_xVector = arg;
        emit xVectorChanged(arg);
    }
    if (perfLevel < Deformable)
        reset();
}

void QSGImageParticle::setYVector(QSGStochasticDirection* arg)
{
    if (m_yVector != arg) {
        m_yVector = arg;
        emit yVectorChanged(arg);
    }
    if (perfLevel < Deformable)
        reset();
}

void QSGImageParticle::setBloat(bool arg)
{
    if (m_bloat != arg) {
        m_bloat = arg;
        emit bloatChanged(arg);
    }
    if (perfLevel < 9999)
        reset();
}

void QSGImageParticle::reset()
{
    QSGParticlePainter::reset();
    m_pleaseReset = true;
    update();
}

void QSGImageParticle::createEngine()
{
    if (m_spriteEngine)
        delete m_spriteEngine;
    if (m_sprites.count())
        m_spriteEngine = new QSGSpriteEngine(m_sprites, this);
    else
        m_spriteEngine = 0;
    reset();
}

static QSGGeometry::Attribute SimpleParticle_Attributes[] = {
    { 0, 2, GL_FLOAT },             // Position
    { 1, 2, GL_FLOAT },             // TexCoord
    { 2, 4, GL_FLOAT },             // Data
    { 3, 4, GL_FLOAT }             // Vectors
};

static QSGGeometry::AttributeSet SimpleParticle_AttributeSet =
{
    4, // Attribute Count
    (2 + 2 + 4 + 4 ) * sizeof(float),
    SimpleParticle_Attributes
};

static QSGGeometry::Attribute UltraParticle_Attributes[] = {
    { 0, 2, GL_FLOAT },             // Position
    { 1, 2, GL_FLOAT },             // TexCoord
    { 2, 4, GL_FLOAT },             // Data
    { 3, 4, GL_FLOAT },             // Vectors
    { 4, 4, GL_UNSIGNED_BYTE },     // Colors
    { 5, 4, GL_FLOAT },             // DeformationVectors
    { 6, 3, GL_FLOAT },             // Rotation
    { 7, 4, GL_FLOAT }              // Anim Data
};

static QSGGeometry::AttributeSet UltraParticle_AttributeSet =
{
    8, // Attribute Count
    (2 + 2 + 4 + 4 + 4 + 4 + 3) * sizeof(float) + 4 * sizeof(uchar),
    UltraParticle_Attributes
};

QSGGeometryNode* QSGImageParticle::buildSimpleParticleNodes()
{
    perfLevel = Simple;//TODO: Intermediate levels
    QImage image = QImage(m_image_name.toLocalFile());
    if (image.isNull()) {
        printf("UltraParticle: loading image failed... '%s'\n", qPrintable(m_image_name.toLocalFile()));
        return 0;
    }

    if (m_material) {
        delete m_material;
        m_material = 0;
    }

    m_material = new SimpleMaterial();
    m_material->texture = sceneGraphEngine()->createTextureFromImage(image);
    m_material->texture->setFiltering(QSGTexture::Linear);
    m_material->framecount = 1;

    foreach (const QString &str, m_particles){
        int gIdx = m_system->m_groupIds[str];
        int count = m_system->m_groupData[gIdx]->size();

        QSGGeometryNode* node = new QSGGeometryNode();
        m_nodes.insert(gIdx, node);
        node->setMaterial(m_material);

        int vCount = count * 4;
        int iCount = count * 6;

        QSGGeometry *g = new QSGGeometry(SimpleParticle_AttributeSet, vCount, iCount);
        node->setGeometry(g);
        g->setDrawingMode(GL_TRIANGLES);

        SimpleVertex *vertices = (SimpleVertex *) g->vertexData();
        for (int p=0; p < count; ++p){
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
    }

    foreach (QSGGeometryNode* node, m_nodes){
        if (node == *(m_nodes.begin()))
                continue;
        (*(m_nodes.begin()))->appendChildNode(node);
    }

    return *(m_nodes.begin());
}

QSGGeometryNode* QSGImageParticle::buildParticleNodes()
{
    if (m_count * 4 > 0xffff) {
        printf("UltraParticle: Too many particles... \n");//### Why is this here?
        return 0;
    }

    if (count() <= 0)
        return 0;

    if (!m_sprites.count() && !m_bloat
            && m_colortable_name.isEmpty()
            && m_sizetable_name.isEmpty()
            && m_opacitytable_name.isEmpty()
            && !m_autoRotation
            && !m_rotation && !m_rotationVariation
            && !m_rotationSpeed && !m_rotationSpeedVariation
            && !m_alphaVariation && m_alpha == 1.0
            && !m_redVariation && !m_blueVariation && !m_greenVariation
            && !m_color.isValid()
            )
        return buildSimpleParticleNodes();
    perfLevel = Sprites;//TODO: intermediate levels
    if (!m_color.isValid())//But we're in colored level (or higher)
        m_color = QColor(Qt::white);

    QImage image;
    if (m_sprites.count()){
        if (!m_spriteEngine) {
            qWarning() << "UltraParticle: No sprite engine...";
            return 0;
        }
        image = m_spriteEngine->assembledImage();
        if (image.isNull())//Warning is printed in engine
            return 0;
    }else{
        image = QImage(m_image_name.toLocalFile());
        if (image.isNull()) {
            printf("UltraParticle: loading image failed... '%s'\n", qPrintable(m_image_name.toLocalFile()));
            return 0;
        }
    }

    if (m_material) {
        delete m_material;
        m_material = 0;
    }

    QImage colortable(m_colortable_name.toLocalFile());
    QImage sizetable(m_sizetable_name.toLocalFile());
    QImage opacitytable(m_opacitytable_name.toLocalFile());
    m_material = new UltraMaterial();
    if (colortable.isNull())
        colortable = QImage(":defaultshaders/identitytable.png");
    if (sizetable.isNull())
        sizetable = QImage(":defaultshaders/identitytable.png");
    if (opacitytable.isNull())
        opacitytable = QImage(":defaultshaders/defaultFadeInOut.png");
    Q_ASSERT(!colortable.isNull());
    Q_ASSERT(!sizetable.isNull());
    Q_ASSERT(!opacitytable.isNull());
    m_material->colortable = sceneGraphEngine()->createTextureFromImage(colortable);
    m_material->sizetable = sceneGraphEngine()->createTextureFromImage(sizetable);
    m_material->opacitytable = sceneGraphEngine()->createTextureFromImage(opacitytable);

    m_material->texture = sceneGraphEngine()->createTextureFromImage(image);
    m_material->texture->setFiltering(QSGTexture::Linear);

    m_material->framecount = 1;
    if (m_spriteEngine){
        m_material->framecount = m_spriteEngine->maxFrames();
        m_spriteEngine->setCount(m_count);
    }

    foreach (const QString &str, m_particles){
        int gIdx = m_system->m_groupIds[str];
        int count = m_system->m_groupData[gIdx]->size();
        QSGGeometryNode* node = new QSGGeometryNode();
        node->setMaterial(m_material);

        m_nodes.insert(gIdx, node);
        m_idxStarts.insert(gIdx, m_lastIdxStart);
        m_lastIdxStart += count;

        //Create Particle Geometry
        int vCount = count * 4;
        int iCount = count * 6;

        QSGGeometry *g = new QSGGeometry(UltraParticle_AttributeSet, vCount, iCount);
        node->setGeometry(g);
        g->setDrawingMode(GL_TRIANGLES);

        UltraVertex *vertices = (UltraVertex *) g->vertexData();
        for (int p=0; p < count; ++p) {
            commit(gIdx, p);//commit sets geometry for the node

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

    }

    foreach (QSGGeometryNode* node, m_nodes){
        if (node == *(m_nodes.begin()))
            continue;
        (*(m_nodes.begin()))->appendChildNode(node);
    }

    return *(m_nodes.begin());
}

QSGNode *QSGImageParticle::updatePaintNode(QSGNode *, UpdatePaintNodeData *)
{
    if (m_pleaseReset){
        m_lastLevel = perfLevel;

        delete m_rootNode;//Automatically deletes children
        m_rootNode = 0;
        m_nodes.clear();

        m_idxStarts.clear();
        m_lastIdxStart = 0;

        if (m_material)
            delete m_material;
        m_material = 0;

        m_pleaseReset = false;
    }

    if (m_system && m_system->isRunning())
        prepareNextFrame();
    if (m_rootNode){
        update();
        //### Should I be using dirty geometry too/instead?
        foreach (QSGGeometryNode* node, m_nodes)
            node->markDirty(QSGNode::DirtyMaterial);
    }

    return m_rootNode;
}

void QSGImageParticle::prepareNextFrame()
{
    if (m_rootNode == 0){//TODO: Staggered loading (as emitted)
        m_rootNode = buildParticleNodes();
        if (m_rootNode == 0)
            return;
        //qDebug() << "Feature level: " << perfLevel;
    }
    qint64 timeStamp = m_system->systemSync(this);

    qreal time = timeStamp / 1000.;
    m_material->timestamp = time;

    //Advance State
    if (m_spriteEngine){//perfLevel == Sprites?//TODO: use signals?

        m_material->animcount = m_spriteEngine->spriteCount();
        m_spriteEngine->updateSprites(timeStamp);
        foreach (const QString &str, m_particles){
            int gIdx = m_system->m_groupIds[str];
            int count = m_system->m_groupData[gIdx]->size();

            UltraVertices *particles = (UltraVertices *) m_nodes[gIdx]->geometry()->vertexData();
            for (int i=0; i < count; i++){
                int spriteIdx = m_idxStarts[gIdx] + i;
                UltraVertices &p = particles[i];
                int curIdx = m_spriteEngine->spriteState(spriteIdx);
                if (curIdx != p.v1.animIdx){
                    p.v1.animIdx = p.v2.animIdx = p.v3.animIdx = p.v4.animIdx = curIdx;
                    p.v1.animT = p.v2.animT = p.v3.animT = p.v4.animT = m_spriteEngine->spriteStart(spriteIdx)/1000.0;
                    p.v1.frameCount = p.v2.frameCount = p.v3.frameCount = p.v4.frameCount = m_spriteEngine->spriteFrames(spriteIdx);
                    p.v1.frameDuration = p.v2.frameDuration = p.v3.frameDuration = p.v4.frameDuration = m_spriteEngine->spriteDuration(spriteIdx);
                }
            }
        }
    }else{
        m_material->animcount = 1;
    }
}

void QSGImageParticle::reloadColor(const Color4ub &c, QSGParticleData* d)
{
    d->color = c;
    //TODO: get index for reload - or make function take an index
}

void QSGImageParticle::initialize(int gIdx, int pIdx)
{
    Color4ub color;
    QSGParticleData* datum = m_system->m_groupData[gIdx]->data[pIdx];
    qreal redVariation = m_color_variation + m_redVariation;
    qreal greenVariation = m_color_variation + m_greenVariation;
    qreal blueVariation = m_color_variation + m_blueVariation;
    int spriteIdx = m_idxStarts[gIdx] + datum->index;
    switch (perfLevel){//Fall-through is intended on all of them
        case Sprites:
            // Initial Sprite State
            datum->animT = datum->t;
            datum->animIdx = 0;
            if (m_spriteEngine){
                m_spriteEngine->startSprite(spriteIdx);
                datum->frameCount = m_spriteEngine->spriteFrames(spriteIdx);
                datum->frameDuration = m_spriteEngine->spriteDuration(spriteIdx);
            }else{
                datum->frameCount = 1;
                datum->frameDuration = 9999;
            }
        case Tabled:
        case Deformable:
            //Initial Rotation
            if (m_xVector){
                const QPointF &ret = m_xVector->sample(QPointF(datum->x, datum->y));
                datum->xx = ret.x();
                datum->xy = ret.y();
            }
            if (m_yVector){
                const QPointF &ret = m_yVector->sample(QPointF(datum->x, datum->y));
                datum->yx = ret.x();
                datum->yy = ret.y();
            }
            datum->rotation =
                    (m_rotation + (m_rotationVariation - 2*((qreal)rand()/RAND_MAX)*m_rotationVariation) ) * CONV;
            datum->rotationSpeed =
                    (m_rotationSpeed + (m_rotationSpeedVariation - 2*((qreal)rand()/RAND_MAX)*m_rotationSpeedVariation) ) * CONV;
            datum->autoRotate = m_autoRotation?1.0:0.0;
        case Colored:
            //Color initialization
            // Particle color
            color.r = m_color.red() * (1 - redVariation) + rand() % 256 * redVariation;
            color.g = m_color.green() * (1 - greenVariation) + rand() % 256 * greenVariation;
            color.b = m_color.blue() * (1 - blueVariation) + rand() % 256 * blueVariation;
            color.a = m_alpha * m_color.alpha() * (1 - m_alphaVariation) + rand() % 256 * m_alphaVariation;
            datum->color = color;
        default:
            break;
    }
}

void QSGImageParticle::commit(int gIdx, int pIdx)
{
    if (m_pleaseReset)
        return;
    QSGGeometryNode *node = m_nodes[gIdx];
    if (!node)
        return;
    QSGParticleData* datum = m_system->m_groupData[gIdx]->data[pIdx];

    node->setFlag(QSGNode::OwnsGeometry, false);
    UltraVertex *ultraVertices = (UltraVertex *) node->geometry()->vertexData();
    SimpleVertex *simpleVertices = (SimpleVertex *) node->geometry()->vertexData();
    switch (perfLevel){
    case Sprites:
        ultraVertices += pIdx*4;
        for (int i=0; i<4; i++){
            ultraVertices[i].x = datum->x  - m_systemOffset.x();
            ultraVertices[i].y = datum->y  - m_systemOffset.y();
            ultraVertices[i].t = datum->t;
            ultraVertices[i].lifeSpan = datum->lifeSpan;
            ultraVertices[i].size = datum->size;
            ultraVertices[i].endSize = datum->endSize;
            ultraVertices[i].sx = datum->sx;
            ultraVertices[i].sy = datum->sy;
            ultraVertices[i].ax = datum->ax;
            ultraVertices[i].ay = datum->ay;
            ultraVertices[i].xx = datum->xx;
            ultraVertices[i].xy = datum->xy;
            ultraVertices[i].yx = datum->yx;
            ultraVertices[i].yy = datum->yy;
            ultraVertices[i].rotation = datum->rotation;
            ultraVertices[i].rotationSpeed = datum->rotationSpeed;
            ultraVertices[i].autoRotate = datum->autoRotate;
            ultraVertices[i].animIdx = datum->animIdx;
            ultraVertices[i].frameDuration = datum->frameDuration;
            ultraVertices[i].frameCount = datum->frameCount;
            ultraVertices[i].animT = datum->animT;
            ultraVertices[i].color.r = datum->color.r;
            ultraVertices[i].color.g = datum->color.g;
            ultraVertices[i].color.b = datum->color.b;
            ultraVertices[i].color.a = datum->color.a;
        }
        break;
    case Tabled://TODO: Us
    case Deformable:
    case Colored:
    case Simple:
        simpleVertices += pIdx*4;
        for (int i=0; i<4; i++){
            simpleVertices[i].x = datum->x - m_systemOffset.x();
            simpleVertices[i].y = datum->y - m_systemOffset.y();
            simpleVertices[i].t = datum->t;
            simpleVertices[i].lifeSpan = datum->lifeSpan;
            simpleVertices[i].size = datum->size;
            simpleVertices[i].endSize = datum->endSize;
            simpleVertices[i].sx = datum->sx;
            simpleVertices[i].sy = datum->sy;
            simpleVertices[i].ax = datum->ax;
            simpleVertices[i].ay = datum->ay;
        }
        break;
    default:
        break;
    }

    node->setFlag(QSGNode::OwnsGeometry, true);
}



QT_END_NAMESPACE
