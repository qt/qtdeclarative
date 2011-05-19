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
#include "ultraparticle.h"
#include "particleemitter.h"
#include "spritestate.h"
#include "spriteengine.h"
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
        QFile vf(vertexFile ? vertexFile : ":resources/ultravertex.shader");
        vf.open(QFile::ReadOnly);
        m_vertex_code = vf.readAll();

        QFile ff(fragmentFile ? fragmentFile : ":resources/ultrafragment.shader");
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
};

class SimpleMaterialData : public QSGMaterialShader
{
public:
    SimpleMaterialData(const char *vertexFile = 0, const char *fragmentFile = 0)
    {
        QFile vf(vertexFile ? vertexFile : ":resources/simplevertex.shader");
        vf.open(QFile::ReadOnly);
        m_vertex_code = vf.readAll();

        QFile ff(fragmentFile ? fragmentFile : ":resources/simplefragment.shader");
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

UltraParticle::UltraParticle(QSGItem* parent)
    : ParticleType(parent)
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
{
    setFlag(ItemHasContents);
}

QDeclarativeListProperty<SpriteState> UltraParticle::sprites()
{
    return QDeclarativeListProperty<SpriteState>(this, &m_sprites, spriteAppend, spriteCount, spriteAt, spriteClear);
}

void UltraParticle::setImage(const QUrl &image)
{
    if (image == m_image_name)
        return;
    m_image_name = image;
    emit imageChanged();
    reset();
}


void UltraParticle::setColortable(const QUrl &table)
{
    if (table == m_colortable_name)
        return;
    m_colortable_name = table;
    emit colortableChanged();
    reset();
}

void UltraParticle::setSizetable(const QUrl &table)
{
    if (table == m_sizetable_name)
        return;
    m_sizetable_name = table;
    emit sizetableChanged();
    reset();
}

void UltraParticle::setOpacitytable(const QUrl &table)
{
    if (table == m_opacitytable_name)
        return;
    m_opacitytable_name = table;
    emit opacitytableChanged();
    reset();
}

void UltraParticle::setColor(const QColor &color)
{
    if (color == m_color)
        return;
    m_color = color;
    emit colorChanged();
}

void UltraParticle::setColorVariation(qreal var)
{
    if (var == m_color_variation)
        return;
    m_color_variation = var;
    emit colorVariationChanged();
}

void UltraParticle::setCount(int c)
{
    ParticleType::setCount(c);
    m_pleaseReset = true;
}

void UltraParticle::reset()
{
    ParticleType::reset();
     m_pleaseReset = true;
}

void UltraParticle::createEngine()
{
    if(m_spriteEngine)
        delete m_spriteEngine;
    if(m_sprites.count())
        m_spriteEngine = new SpriteEngine(m_sprites, this);
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

QSGGeometryNode* UltraParticle::buildSimpleParticleNode()
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
    for (int p=0; p<m_count; ++p) {
        for (int i=0; i<4; ++i) {
            vertices[i].x = 0;
            vertices[i].y = 0;
            vertices[i].t = -1;
            vertices[i].lifeSpan = 0;
            vertices[i].size = 0;
            vertices[i].endSize = 0;
            vertices[i].sx = 0;
            vertices[i].sy = 0;
            vertices[i].ax = 0;
            vertices[i].ay = 0;
        }

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

    if (m_material) {
        delete m_material;
        m_material = 0;
    }

    m_material = new SimpleMaterial();
    m_material->texture = sceneGraphEngine()->createTextureFromImage(image);
    m_material->texture->setFiltering(QSGTexture::Linear);
    m_material->framecount = 1;
    m_node = new QSGGeometryNode();
    m_node->setGeometry(g);
    m_node->setMaterial(m_material);

    m_last_particle = 0;

    return m_node;
}

QSGGeometryNode* UltraParticle::buildParticleNode()
{
    if (m_count * 4 > 0xffff) {
        printf("UltraParticle: Too many particles... \n");//####Why is this here?
        return 0;
    }

    if(m_count <= 0) {
        printf("UltraParticle: Too few particles... \n");
        return 0;
    }

    qDebug() << m_colortable_name.isEmpty() << !m_color.isValid();
    if(!m_sprites.count() && !m_bloat
            && m_colortable_name.isEmpty()
            && m_sizetable_name.isEmpty()
            && m_opacitytable_name.isEmpty()
            && !m_rotation && !m_rotationVariation
            && !m_rotationSpeed && !m_rotationSpeedVariation
            && !m_alphaVariation && m_alpha == 1.0
            && !m_redVariation && !m_blueVariation && !m_greenVariation
            && !m_color.isValid()
            )
        return buildSimpleParticleNode();
    perfLevel = Sprites;//TODO: intermediate levels
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

    UltraVertex *vertices = (UltraVertex *) g->vertexData();
    for (int p=0; p<m_count; ++p) {

        for (int i=0; i<4; ++i) {
            vertices[i].x = 0;
            vertices[i].y = 0;
            vertices[i].t = -1;
            vertices[i].lifeSpan = 0;
            vertices[i].size = 0;
            vertices[i].endSize = 0;
            vertices[i].sx = 0;
            vertices[i].sy = 0;
            vertices[i].ax = 0;
            vertices[i].ay = 0;
            vertices[i].xx = 1;
            vertices[i].xy = 0;
            vertices[i].yx = 0;
            vertices[i].yy = 1;
            vertices[i].rotation = 0;
            vertices[i].rotationSpeed = 0;
            vertices[i].autoRotate = 0;
            vertices[i].animIdx = 0;
            vertices[i].frameDuration = 1;
            vertices[i].frameCount = 1;
            vertices[i].animT = -1;
        }

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

    if (m_material) {
        delete m_material;
        m_material = 0;
    }

    QImage colortable(m_colortable_name.toLocalFile());
    QImage sizetable(m_sizetable_name.toLocalFile());
    QImage opacitytable(m_opacitytable_name.toLocalFile());
    m_material = new UltraMaterial();
    if(colortable.isNull())
        colortable = QImage(":resources/identitytable.png");
    if(sizetable.isNull())
        sizetable = QImage(":resources/identitytable.png");
    if(opacitytable.isNull())
        opacitytable = QImage(":resources/defaultFadeInOut.png");
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

    m_node = new QSGGeometryNode();
    m_node->setGeometry(g);
    m_node->setMaterial(m_material);

    m_last_particle = 0;

    return m_node;
}

QSGNode *UltraParticle::updatePaintNode(QSGNode *, UpdatePaintNodeData *)
{
    if(m_pleaseReset){
        if(m_node)
            delete m_node;
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

void UltraParticle::prepareNextFrame()
{
    if (m_node == 0){    //TODO: Staggered loading (as emitted)
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
        m_material->animcount = m_spriteEngine->stateCount();
        UltraVertices *particles = (UltraVertices *) m_node->geometry()->vertexData();
        m_spriteEngine->updateSprites(timeStamp);
        for(int i=0; i<m_count; i++){
            UltraVertices &p = particles[i];
            int curIdx = m_spriteEngine->spriteState(i);
            if(curIdx != p.v1.animIdx){
                p.v1.animIdx = p.v2.animIdx = p.v3.animIdx = p.v4.animIdx = curIdx;
                p.v1.animT = p.v2.animT = p.v3.animT = p.v4.animT = m_spriteEngine->spriteStart(i)/1000.0;
                p.v1.frameCount = p.v2.frameCount = p.v3.frameCount = p.v4.frameCount = m_spriteEngine->state(curIdx)->frames();
                p.v1.frameDuration = p.v2.frameDuration = p.v3.frameDuration = p.v4.frameDuration = m_spriteEngine->state(curIdx)->duration();
            }
        }
    }else{
        m_material->animcount = 1;
    }
}

template <typename VT>
IntermediateVertices* transplant(IntermediateVertices* iv, VT &v)
{//Deliberate typemangling cast
    iv->v1 = (UltraVertex*)&(v.v1);
    iv->v2 = (UltraVertex*)&(v.v2);
    iv->v3 = (UltraVertex*)&(v.v3);
    iv->v4 = (UltraVertex*)&(v.v4);
    return iv;
}

IntermediateVertices* UltraParticle::fetchIntermediateVertices(int pos)
{
    //Note that this class ruins typesafety for you. Maybe even thread safety.
    //TODO: Something better, possibly with templates or inheritance
    static IntermediateVertices iv;
    SimpleVertices *sv;
    UltraVertices *uv;
    switch(perfLevel){
        case Simple:
            sv = (SimpleVertices *) m_node->geometry()->vertexData();
            return transplant(&iv, sv[pos]);
        case Coloured:
        case Deformable:
        case Tabled:
        case Sprites:
        default:
            uv = (UltraVertices *) m_node->geometry()->vertexData();
            return transplant(&iv,uv[pos]);
    }
}

void UltraParticle::reloadColor(const Color4ub &c, ParticleData* d)
{
    UltraVertices *particles = (UltraVertices *) m_node->geometry()->vertexData();
    int pos = particleTypeIndex(d);
    UltraVertices &p = particles[pos];
    p.v1.color = p.v2.color = p.v3.color = p.v4.color = c;
}

/*Repalced by superclass templated function
void UltraParticle::vertexCopy(UltraVertex &b,const ParticleVertex& a)
{
    b.x = a.x - m_systemOffset.x();
    b.y = a.y - m_systemOffset.y();
    b.t = a.t;
    b.lifeSpan = a.lifeSpan;
    b.size = a.size;
    b.endSize = a.endSize;
    b.sx = a.sx;
    b.sy = a.sy;
    b.ax = a.ax;
    b.ay = a.ay;
}
*/


void UltraParticle::reload(ParticleData *d)
{
    if (m_node == 0)
        return;

    int pos = particleTypeIndex(d);
    IntermediateVertices* p = fetchIntermediateVertices(pos);

    //Perhaps we could be more efficient?
    vertexCopy(*p->v1, d->pv);
    vertexCopy(*p->v2, d->pv);
    vertexCopy(*p->v3, d->pv);
    vertexCopy(*p->v4, d->pv);
}

void UltraParticle::load(ParticleData *d)
{
    if (m_node == 0)
        return;

    int pos = particleTypeIndex(d);
    IntermediateVertices* p = fetchIntermediateVertices(pos);//Remember this removes typesafety!
    Color4ub color;
    qreal redVariation = m_color_variation + m_redVariation;
    qreal greenVariation = m_color_variation + m_greenVariation;
    qreal blueVariation = m_color_variation + m_blueVariation;
    switch(perfLevel){//Fall-through is intended on all of them
        case Sprites:
            // Initial Sprite State
            p->v1->animT = p->v2->animT = p->v3->animT = p->v4->animT = p->v1->t;
            p->v1->animIdx = p->v2->animIdx = p->v3->animIdx = p->v4->animIdx = 0;
            if(m_spriteEngine){
                SpriteState* state = m_spriteEngine->state(0);
                p->v1->frameCount = p->v2->frameCount = p->v3->frameCount = p->v4->frameCount = state->frames();
                p->v1->frameDuration = p->v2->frameDuration = p->v3->frameDuration = p->v4->frameDuration = state->duration();
                m_spriteEngine->startSprite(pos);
            }else{
                p->v1->frameCount = p->v2->frameCount = p->v3->frameCount = p->v4->frameCount = 1;
                p->v1->frameDuration = p->v2->frameDuration = p->v3->frameDuration = p->v4->frameDuration = 9999;
            }
        case Tabled:
        case Deformable:
            //Initial Rotation
            if(m_xVector){
                const QPointF &ret = m_xVector->sample(QPointF(d->pv.x, d->pv.y));
                p->v1->xx = p->v2->xx = p->v3->xx = p->v4->xx = ret.x();
                p->v1->xy = p->v2->xy = p->v3->xy = p->v4->xy = ret.y();
            }
            if(m_yVector){
                const QPointF &ret = m_yVector->sample(QPointF(d->pv.x, d->pv.y));
                p->v1->yx = p->v2->yx = p->v3->yx = p->v4->yx = ret.x();
                p->v1->yy = p->v2->yy = p->v3->yy = p->v4->yy = ret.y();
            }
            p->v1->rotation = p->v2->rotation = p->v3->rotation = p->v4->rotation =
                    (m_rotation + (m_rotationVariation - 2*((qreal)rand()/RAND_MAX)*m_rotationVariation) ) * CONV;
            p->v1->rotationSpeed = p->v2->rotationSpeed = p->v3->rotationSpeed = p->v4->rotationSpeed =
                    (m_rotationSpeed + (m_rotationSpeedVariation - 2*((qreal)rand()/RAND_MAX)*m_rotationSpeedVariation) ) * CONV;
            p->v1->autoRotate = p->v2->autoRotate = p->v3->autoRotate = p->v4->autoRotate = m_autoRotation?1.0:0.0;
        case Coloured:
            //Color initialization
            // Particle color
            color.r = m_color.red() * (1 - redVariation) + rand() % 256 * redVariation;
            color.g = m_color.green() * (1 - greenVariation) + rand() % 256 * greenVariation;
            color.b = m_color.blue() * (1 - blueVariation) + rand() % 256 * blueVariation;
            color.a = m_alpha * m_color.alpha() * (1 - m_alphaVariation) + rand() % 256 * m_alphaVariation;
            p->v1->color = p->v2->color = p->v3->color = p->v4->color = color;
        default:
            break;
    }

    vertexCopy(*p->v1, d->pv);
    vertexCopy(*p->v2, d->pv);
    vertexCopy(*p->v3, d->pv);
    vertexCopy(*p->v4, d->pv);
}

QT_END_NAMESPACE
