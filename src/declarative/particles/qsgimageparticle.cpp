/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
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
        program()->setUniformValue(m_colortable_id, 1);

        state.context()->functions()->glActiveTexture(GL_TEXTURE2);
        m->sizetable->bind();
        program()->setUniformValue(m_sizetable_id, 2);

        state.context()->functions()->glActiveTexture(GL_TEXTURE3);
        m->opacitytable->bind();
        program()->setUniformValue(m_opacitytable_id, 3);

        state.context()->functions()->glActiveTexture(GL_TEXTURE0);//Investigate why this screws up Text{} if placed before 1
        m->texture->bind();

        program()->setUniformValue(m_opacity_id, state.opacity());
        program()->setUniformValue(m_timestamp_id, (float) m->timestamp);
        program()->setUniformValue(m_framecount_id, (float) m->framecount);
        program()->setUniformValue(m_animcount_id, (float) m->animcount);

        if (state.isMatrixDirty())
            program()->setUniformValue(m_matrix_id, state.combinedMatrix());
    }

    virtual void initialize() {
        m_colortable_id = program()->uniformLocation("colortable");
        m_sizetable_id = program()->uniformLocation("sizetable");
        m_opacitytable_id = program()->uniformLocation("opacitytable");
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
    int m_colortable_id;
    int m_sizetable_id;
    int m_opacitytable_id;
    int m_framecount_id;
    int m_animcount_id;

    QByteArray m_vertex_code;
    QByteArray m_fragment_code;

    static float chunkOfBytes[1024];
};
float UltraMaterialData::chunkOfBytes[1024];

QSGMaterialShader *UltraMaterial::createShader() const
{
    if(usesSprites)//TODO: Perhaps just swap the shaders, and don't mind the extra vector?
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
    , m_node(0)
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
    if(perfLevel < Colored)
        reset();
}

void QSGImageParticle::setColorVariation(qreal var)
{
    if (var == m_color_variation)
        return;
    m_color_variation = var;
    emit colorVariationChanged();
    if(perfLevel < Colored)
        reset();
}

void QSGImageParticle::setAlphaVariation(qreal arg)
{
    if (m_alphaVariation != arg) {
        m_alphaVariation = arg;
        emit alphaVariationChanged(arg);
    }
    if(perfLevel < Colored)
        reset();
}

void QSGImageParticle::setAlpha(qreal arg)
{
    if (m_alpha != arg) {
        m_alpha = arg;
        emit alphaChanged(arg);
    }
    if(perfLevel < Colored)
        reset();
}

void QSGImageParticle::setRedVariation(qreal arg)
{
    if (m_redVariation != arg) {
        m_redVariation = arg;
        emit redVariationChanged(arg);
    }
    if(perfLevel < Colored)
        reset();
}

void QSGImageParticle::setGreenVariation(qreal arg)
{
    if (m_greenVariation != arg) {
        m_greenVariation = arg;
        emit greenVariationChanged(arg);
    }
    if(perfLevel < Colored)
        reset();
}

void QSGImageParticle::setBlueVariation(qreal arg)
{
    if (m_blueVariation != arg) {
        m_blueVariation = arg;
        emit blueVariationChanged(arg);
    }
    if(perfLevel < Colored)
        reset();
}

void QSGImageParticle::setRotation(qreal arg)
{
    if (m_rotation != arg) {
        m_rotation = arg;
        emit rotationChanged(arg);
    }
    if(perfLevel < Deformable)
        reset();
}

void QSGImageParticle::setRotationVariation(qreal arg)
{
    if (m_rotationVariation != arg) {
        m_rotationVariation = arg;
        emit rotationVariationChanged(arg);
    }
    if(perfLevel < Deformable)
        reset();
}

void QSGImageParticle::setRotationSpeed(qreal arg)
{
    if (m_rotationSpeed != arg) {
        m_rotationSpeed = arg;
        emit rotationSpeedChanged(arg);
    }
    if(perfLevel < Deformable)
        reset();
}

void QSGImageParticle::setRotationSpeedVariation(qreal arg)
{
    if (m_rotationSpeedVariation != arg) {
        m_rotationSpeedVariation = arg;
        emit rotationSpeedVariationChanged(arg);
    }
    if(perfLevel < Deformable)
        reset();
}

void QSGImageParticle::setAutoRotation(bool arg)
{
    if (m_autoRotation != arg) {
        m_autoRotation = arg;
        emit autoRotationChanged(arg);
    }
    if(perfLevel < Deformable)
        reset();
}

void QSGImageParticle::setXVector(QSGStochasticDirection* arg)
{
    if (m_xVector != arg) {
        m_xVector = arg;
        emit xVectorChanged(arg);
    }
    if(perfLevel < Deformable)
        reset();
}

void QSGImageParticle::setYVector(QSGStochasticDirection* arg)
{
    if (m_yVector != arg) {
        m_yVector = arg;
        emit yVectorChanged(arg);
    }
    if(perfLevel < Deformable)
        reset();
}

void QSGImageParticle::setBloat(bool arg)
{
    if (m_bloat != arg) {
        m_bloat = arg;
        emit bloatChanged(arg);
    }
    if(perfLevel < 9999)
        reset();
}

void QSGImageParticle::reset()
{
    QSGParticlePainter::reset();
     m_pleaseReset = true;
}

void QSGImageParticle::createEngine()
{
    if(m_spriteEngine)
        delete m_spriteEngine;
    if(m_sprites.count())
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

QSGGeometryNode* QSGImageParticle::buildSimpleParticleNode()
{
    perfLevel = Simple;//TODO: Intermediate levels
    QImage image = QImage(m_image_name.toLocalFile());
    if (image.isNull()) {
        printf("UltraParticle: loading image failed... '%s'\n", qPrintable(m_image_name.toLocalFile()));
        return 0;
    }
    int vCount = m_count * 4;
    int iCount = m_count * 6;
    qDebug() << "Simple Case";

    QSGGeometry *g = new QSGGeometry(SimpleParticle_AttributeSet, vCount, iCount);
    g->setDrawingMode(GL_TRIANGLES);

    SimpleVertex *vertices = (SimpleVertex *) g->vertexData();
    for (int p=0; p<m_count; ++p){
        for(int i=0; i<4; i++){
            vertices[i].x = m_data[p]->x;
            vertices[i].y = m_data[p]->y;
            vertices[i].t = m_data[p]->t;
            vertices[i].size = m_data[p]->size;
            vertices[i].endSize = m_data[p]->endSize;
            vertices[i].sx = m_data[p]->sx;
            vertices[i].sy = m_data[p]->sy;
            vertices[i].ax = m_data[p]->ax;
            vertices[i].ay = m_data[p]->ay;
        }
        //reload(p);
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
    for (int i=0; i<m_count; ++i) {
        int o = i * 4;
        indices[0] = o;
        indices[1] = o + 1;
        indices[2] = o + 2;
        indices[3] = o + 1;
        indices[4] = o + 3;
        indices[5] = o + 2;
        indices += 6;
    }

    m_node = new QSGGeometryNode();
    m_node->setGeometry(g);

    if (m_material) {
        delete m_material;
        m_material = 0;
    }

    m_material = new SimpleMaterial();
    m_material->texture = sceneGraphEngine()->createTextureFromImage(image);
    m_material->texture->setFiltering(QSGTexture::Linear);
    m_material->framecount = 1;
    m_node->setMaterial(m_material);

    m_last_particle = 0;
    return m_node;
}

QSGGeometryNode* QSGImageParticle::buildParticleNode()
{
    if (m_count * 4 > 0xffff) {
        printf("UltraParticle: Too many particles... \n");//### Why is this here?
        return 0;
    }

    if(m_count <= 0) {
        qDebug() << "UltraParticle: Too few particles... \n";//XXX: Is now a vaild intermediate state...
        return 0;
    }

    if(!m_sprites.count() && !m_bloat
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
        return buildSimpleParticleNode();
    perfLevel = Sprites;//TODO: intermediate levels
    if(!m_color.isValid())//But we're in colored level (or higher)
        m_color = QColor(Qt::white);
    qDebug() << "Complex Case";

    QImage image;
    if(m_sprites.count()){
        if (!m_spriteEngine) {
            qWarning() << "UltraParticle: No sprite engine...";
            return 0;
        }
        image = m_spriteEngine->assembledImage();
        if(image.isNull())//Warning is printed in engine
            return 0;
    }else{
        image = QImage(m_image_name.toLocalFile());
        if (image.isNull()) {
            printf("UltraParticle: loading image failed... '%s'\n", qPrintable(m_image_name.toLocalFile()));
            return 0;
        }
    }

    int vCount = m_count * 4;
    int iCount = m_count * 6;

    QSGGeometry *g = new QSGGeometry(UltraParticle_AttributeSet, vCount, iCount);
    g->setDrawingMode(GL_TRIANGLES);
    m_node = new QSGGeometryNode();
    m_node->setGeometry(g);

    UltraVertex *vertices = (UltraVertex *) g->vertexData();
    for (int p=0; p<m_count; ++p) {
        reload(p);//reload gets geometry from node

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
    for (int i=0; i<m_count; ++i) {
        int o = i * 4;
        indices[0] = o;
        indices[1] = o + 1;
        indices[2] = o + 2;
        indices[3] = o + 1;
        indices[4] = o + 3;
        indices[5] = o + 2;
        indices += 6;
    }

    qFree(m_lastData);
    if (m_material) {
        delete m_material;
        m_material = 0;
    }

    QImage colortable(m_colortable_name.toLocalFile());
    QImage sizetable(m_sizetable_name.toLocalFile());
    QImage opacitytable(m_opacitytable_name.toLocalFile());
    m_material = new UltraMaterial();
    if(colortable.isNull())
        colortable = QImage(":defaultshaders/identitytable.png");
    if(sizetable.isNull())
        sizetable = QImage(":defaultshaders/identitytable.png");
    if(opacitytable.isNull())
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
    if(m_spriteEngine){
        m_material->framecount = m_spriteEngine->maxFrames();
        m_spriteEngine->setCount(m_count);
    }

    m_node->setMaterial(m_material);

    m_last_particle = 0;

    return m_node;
}

QSGNode *QSGImageParticle::updatePaintNode(QSGNode *, UpdatePaintNodeData *)
{
    if(m_pleaseReset){
        if(m_node){
            if(perfLevel == 1){
                m_lastCount = m_node->geometry()->vertexCount() / 4;
                m_lastData = qMalloc(m_lastCount*sizeof(SimpleVertices));
                memcpy(m_lastData, m_node->geometry()->vertexData(), m_lastCount * sizeof(SimpleVertices));//TODO: Multiple levels
            }
            m_lastLevel = perfLevel;
            delete m_node;
        }
        if(m_material)
            delete m_material;

        m_node = 0;
        m_material = 0;
        m_pleaseReset = false;
    }

    if(m_system && m_system->isRunning())
        prepareNextFrame();
    if (m_node){
        update();
        m_node->markDirty(QSGNode::DirtyMaterial);
    }

    return m_node;
}

void QSGImageParticle::prepareNextFrame()
{
    if (m_node == 0){//TODO: Staggered loading (as emitted)
        m_node = buildParticleNode();
        if(m_node == 0)
            return;
        qDebug() << "Feature level: " << perfLevel;
    }
    qint64 timeStamp = m_system->systemSync(this);

    qreal time = timeStamp / 1000.;
    m_material->timestamp = time;

    //Advance State
    if(m_spriteEngine){//perfLevel == Sprites?
        m_material->animcount = m_spriteEngine->spriteCount();
        UltraVertices *particles = (UltraVertices *) m_node->geometry()->vertexData();
        m_spriteEngine->updateSprites(timeStamp);
        for(int i=0; i<m_count; i++){
            UltraVertices &p = particles[i];
            int curIdx = m_spriteEngine->spriteState(i);
            if(curIdx != p.v1.animIdx){
                p.v1.animIdx = p.v2.animIdx = p.v3.animIdx = p.v4.animIdx = curIdx;
                p.v1.animT = p.v2.animT = p.v3.animT = p.v4.animT = m_spriteEngine->spriteStart(i)/1000.0;
                p.v1.frameCount = p.v2.frameCount = p.v3.frameCount = p.v4.frameCount = m_spriteEngine->spriteFrames(i);
                p.v1.frameDuration = p.v2.frameDuration = p.v3.frameDuration = p.v4.frameDuration = m_spriteEngine->spriteDuration(i);
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

void QSGImageParticle::initialize(int idx)
{
    Color4ub color;
    qreal redVariation = m_color_variation + m_redVariation;
    qreal greenVariation = m_color_variation + m_greenVariation;
    qreal blueVariation = m_color_variation + m_blueVariation;
    switch(perfLevel){//Fall-through is intended on all of them
        case Sprites:
            // Initial Sprite State
            m_data[idx]->animT = m_data[idx]->t;
            m_data[idx]->animIdx = 0;
            if(m_spriteEngine){
                m_spriteEngine->startSprite(idx);
                m_data[idx]->frameCount = m_spriteEngine->spriteFrames(idx);
                m_data[idx]->frameDuration = m_spriteEngine->spriteDuration(idx);
            }else{
                m_data[idx]->frameCount = 1;
                m_data[idx]->frameDuration = 9999;
            }
        case Tabled:
        case Deformable:
            //Initial Rotation
            if(m_xVector){
                const QPointF &ret = m_xVector->sample(QPointF(m_data[idx]->x, m_data[idx]->y));
                m_data[idx]->xx = ret.x();
                m_data[idx]->xy = ret.y();
            }
            if(m_yVector){
                const QPointF &ret = m_yVector->sample(QPointF(m_data[idx]->x, m_data[idx]->y));
                m_data[idx]->yx = ret.x();
                m_data[idx]->yy = ret.y();
            }
            m_data[idx]->rotation =
                    (m_rotation + (m_rotationVariation - 2*((qreal)rand()/RAND_MAX)*m_rotationVariation) ) * CONV;
            m_data[idx]->rotationSpeed =
                    (m_rotationSpeed + (m_rotationSpeedVariation - 2*((qreal)rand()/RAND_MAX)*m_rotationSpeedVariation) ) * CONV;
            m_data[idx]->autoRotate = m_autoRotation?1.0:0.0;
        case Colored:
            //Color initialization
            // Particle color
            color.r = m_color.red() * (1 - redVariation) + rand() % 256 * redVariation;
            color.g = m_color.green() * (1 - greenVariation) + rand() % 256 * greenVariation;
            color.b = m_color.blue() * (1 - blueVariation) + rand() % 256 * blueVariation;
            color.a = m_alpha * m_color.alpha() * (1 - m_alphaVariation) + rand() % 256 * m_alphaVariation;
            m_data[idx]->color = color;
        default:
            break;
    }
}

void QSGImageParticle::reload(int idx)
{
    if(!m_node)
        return;

    m_node->setFlag(QSGNode::OwnsGeometry, false);
    UltraVertex *ultraVertices = (UltraVertex *) m_node->geometry()->vertexData();
    SimpleVertex *simpleVertices = (SimpleVertex *) m_node->geometry()->vertexData();
    switch(perfLevel){
    case Sprites:
        ultraVertices += idx*4;
        for(int i=0; i<4; i++){
            ultraVertices[i].x = m_data[idx]->x  - m_systemOffset.x();
            ultraVertices[i].y = m_data[idx]->y  - m_systemOffset.y();
            ultraVertices[i].t = m_data[idx]->t;
            ultraVertices[i].lifeSpan = m_data[idx]->lifeSpan;
            ultraVertices[i].size = m_data[idx]->size;
            ultraVertices[i].endSize = m_data[idx]->endSize;
            ultraVertices[i].sx = m_data[idx]->sx;
            ultraVertices[i].sy = m_data[idx]->sy;
            ultraVertices[i].ax = m_data[idx]->ax;
            ultraVertices[i].ay = m_data[idx]->ay;
            ultraVertices[i].xx = m_data[idx]->xx;
            ultraVertices[i].xy = m_data[idx]->xy;
            ultraVertices[i].yx = m_data[idx]->yx;
            ultraVertices[i].yy = m_data[idx]->yy;
            ultraVertices[i].rotation = m_data[idx]->rotation;
            ultraVertices[i].rotationSpeed = m_data[idx]->rotationSpeed;
            ultraVertices[i].autoRotate = m_data[idx]->autoRotate;
            ultraVertices[i].animIdx = m_data[idx]->animIdx;
            ultraVertices[i].frameDuration = m_data[idx]->frameDuration;
            ultraVertices[i].frameCount = m_data[idx]->frameCount;
            ultraVertices[i].animT = m_data[idx]->animT;
            ultraVertices[i].color.r = m_data[idx]->color.r;
            ultraVertices[i].color.g = m_data[idx]->color.g;
            ultraVertices[i].color.b = m_data[idx]->color.b;
            ultraVertices[i].color.a = m_data[idx]->color.a;
        }
        break;
    case Tabled://TODO: Us
    case Deformable:
    case Colored:
    case Simple:
        simpleVertices += idx*4;
        for(int i=0; i<4; i++){
            simpleVertices[i].x = m_data[idx]->x - m_systemOffset.x();
            simpleVertices[i].y = m_data[idx]->y - m_systemOffset.y();
            simpleVertices[i].t = m_data[idx]->t;
            simpleVertices[i].lifeSpan = m_data[idx]->lifeSpan;
            simpleVertices[i].size = m_data[idx]->size;
            simpleVertices[i].endSize = m_data[idx]->endSize;
            simpleVertices[i].sx = m_data[idx]->sx;
            simpleVertices[i].sy = m_data[idx]->sy;
            simpleVertices[i].ax = m_data[idx]->ax;
            simpleVertices[i].ay = m_data[idx]->ay;
        }
        break;
    default:
        break;
    }

    m_node->setFlag(QSGNode::OwnsGeometry, true);
}



QT_END_NAMESPACE
