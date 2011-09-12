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

#ifndef ULTRAPARTICLE_H
#define ULTRAPARTICLE_H
#include "qsgparticlepainter_p.h"
#include "qsgdirection_p.h"
#include <QDeclarativeListProperty>
#include <qsgsimplematerial.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class ImageMaterialData;
class QSGGeometryNode;

class QSGSprite;
class QSGStochasticEngine;

struct SimpleVertex {
    float x;
    float y;
    float t;
    float lifeSpan;
    float size;
    float endSize;
    float vx;
    float vy;
    float ax;
    float ay;
};

struct ColoredVertex {
    float x;
    float y;
    float t;
    float lifeSpan;
    float size;
    float endSize;
    float vx;
    float vy;
    float ax;
    float ay;
    Color4ub color;
};

struct DeformableVertex {
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
    Color4ub color;
    float xx;
    float xy;
    float yx;
    float yy;
    float rotation;
    float rotationSpeed;
    float autoRotate;//Assumed that GPUs prefer floats to bools
};

struct SpriteVertex {
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
    Color4ub color;
    float xx;
    float xy;
    float yx;
    float yy;
    float rotation;
    float rotationSpeed;
    float autoRotate;//Assumed that GPUs prefer floats to bools
    float animIdx;
    float frameDuration;
    float frameCount;
    float animT;
};

template <typename Vertex>
struct Vertices {
    Vertex v1;
    Vertex v2;
    Vertex v3;
    Vertex v4;
};

class QSGImageParticle : public QSGParticlePainter
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ image WRITE setImage NOTIFY imageChanged)
    Q_PROPERTY(QUrl colorTable READ colortable WRITE setColortable NOTIFY colortableChanged)
    Q_PROPERTY(QUrl sizeTable READ sizetable WRITE setSizetable NOTIFY sizetableChanged)
    Q_PROPERTY(QUrl opacityTable READ opacitytable WRITE setOpacitytable NOTIFY opacitytableChanged)

    //###Now just colorize - add a flag for 'solid' color particles(where the img is just a mask?)?
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    //Stacks (added) with individual colorVariations
    Q_PROPERTY(qreal colorVariation READ colorVariation WRITE setColorVariation NOTIFY colorVariationChanged)
    Q_PROPERTY(qreal redVariation READ redVariation WRITE setRedVariation NOTIFY redVariationChanged)
    Q_PROPERTY(qreal greenVariation READ greenVariation WRITE setGreenVariation NOTIFY greenVariationChanged)
    Q_PROPERTY(qreal blueVariation READ blueVariation WRITE setBlueVariation NOTIFY blueVariationChanged)
    //Stacks (multiplies) with the Alpha in the color, mostly here so you can use svg color names (which have full alpha)
    Q_PROPERTY(qreal alpha READ alpha WRITE setAlpha NOTIFY alphaChanged)
    Q_PROPERTY(qreal alphaVariation READ alphaVariation WRITE setAlphaVariation NOTIFY alphaVariationChanged)

    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(qreal rotationVariation READ rotationVariation WRITE setRotationVariation NOTIFY rotationVariationChanged)
    Q_PROPERTY(qreal rotationSpeed READ rotationSpeed WRITE setRotationSpeed NOTIFY rotationSpeedChanged)
    Q_PROPERTY(qreal rotationSpeedVariation READ rotationSpeedVariation WRITE setRotationSpeedVariation NOTIFY rotationSpeedVariationChanged)
    //If true, then will face the direction of motion. Stacks with rotation, e.g. setting rotation
    //to 180 will lead to facing away from the direction of motion
    Q_PROPERTY(bool autoRotation READ autoRotation WRITE setAutoRotation NOTIFY autoRotationChanged)

    //###Call i/j? Makes more sense to those with vector calculus experience, and I could even add the cirumflex in QML?
    //xVector is the vector from the top-left point to the top-right point, and is multiplied by current size
    Q_PROPERTY(QSGDirection* xVector READ xVector WRITE setXVector NOTIFY xVectorChanged)
    //yVector is the same, but top-left to bottom-left. The particle is always a parallelogram.
    Q_PROPERTY(QSGDirection* yVector READ yVector WRITE setYVector NOTIFY yVectorChanged)
    Q_PROPERTY(QDeclarativeListProperty<QSGSprite> sprites READ sprites)

    Q_PROPERTY(EntryEffect entryEffect READ entryEffect WRITE setEntryEffect NOTIFY entryEffectChanged)
    Q_PROPERTY(bool bloat READ bloat WRITE setBloat NOTIFY bloatChanged)//Just a debugging property to bypass optimizations
    Q_ENUMS(EntryEffect)
public:
    explicit QSGImageParticle(QSGItem *parent = 0);
    virtual ~QSGImageParticle();


    QDeclarativeListProperty<QSGSprite> sprites();
    QSGStochasticEngine* spriteEngine() {return m_spriteEngine;}

    enum EntryEffect {
        None = 0,
        Fade = 1,
        Scale = 2
    };

    enum PerformanceLevel{//TODO: Expose?
        Unknown = 0,
        Simple,
        Colored,
        Deformable,
        Tabled,
        Sprites
    };

    QUrl image() const { return m_image_name; }
    void setImage(const QUrl &image);

    QUrl colortable() const { return m_colortable_name; }
    void setColortable(const QUrl &table);

    QUrl sizetable() const { return m_sizetable_name; }
    void setSizetable (const QUrl &table);

    QUrl opacitytable() const { return m_opacitytable_name; }
    void setOpacitytable(const QUrl &table);

    QColor color() const { return m_color; }
    void setColor(const QColor &color);

    qreal colorVariation() const { return m_color_variation; }
    void setColorVariation(qreal var);

    qreal renderOpacity() const { return m_render_opacity; }

    qreal alphaVariation() const { return m_alphaVariation; }

    qreal alpha() const { return m_alpha; }

    qreal redVariation() const { return m_redVariation; }

    qreal greenVariation() const { return m_greenVariation; }

    qreal blueVariation() const { return m_blueVariation; }

    qreal rotation() const { return m_rotation; }

    qreal rotationVariation() const { return m_rotationVariation; }

    qreal rotationSpeed() const { return m_rotationSpeed; }

    qreal rotationSpeedVariation() const { return m_rotationSpeedVariation; }

    bool autoRotation() const { return m_autoRotation; }

    QSGDirection* xVector() const { return m_xVector; }

    QSGDirection* yVector() const { return m_yVector; }

    bool bloat() const { return m_bloat; }

    EntryEffect entryEffect() const { return m_entryEffect; }

signals:

    void imageChanged();
    void colortableChanged();
    void sizetableChanged();
    void opacitytableChanged();

    void colorChanged();
    void colorVariationChanged();

    void particleDurationChanged();
    void alphaVariationChanged(qreal arg);

    void alphaChanged(qreal arg);

    void redVariationChanged(qreal arg);

    void greenVariationChanged(qreal arg);

    void blueVariationChanged(qreal arg);

    void rotationChanged(qreal arg);

    void rotationVariationChanged(qreal arg);

    void rotationSpeedChanged(qreal arg);

    void rotationSpeedVariationChanged(qreal arg);

    void autoRotationChanged(bool arg);

    void xVectorChanged(QSGDirection* arg);

    void yVectorChanged(QSGDirection* arg);

    void bloatChanged(bool arg);

    void entryEffectChanged(EntryEffect arg);

public slots:
    void reloadColor(const Color4ub &c, QSGParticleData* d);
    void setAlphaVariation(qreal arg);

    void setAlpha(qreal arg);

    void setRedVariation(qreal arg);

    void setGreenVariation(qreal arg);

    void setBlueVariation(qreal arg);

    void setRotation(qreal arg);

    void setRotationVariation(qreal arg);

    void setRotationSpeed(qreal arg);

    void setRotationSpeedVariation(qreal arg);

    void setAutoRotation(bool arg);

    void setXVector(QSGDirection* arg);

    void setYVector(QSGDirection* arg);

    void setBloat(bool arg);

    void setEntryEffect(EntryEffect arg);

protected:
    void reset();
    virtual void initialize(int gIdx, int pIdx);
    virtual void commit(int gIdx, int pIdx);

    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);
    void prepareNextFrame();
    QSGGeometryNode* buildParticleNodes();

private slots:
    void createEngine(); //### method invoked by sprite list changing (in engine.h) - pretty nasty

private:
    bool m_do_reset;

    QUrl m_image_name;
    QUrl m_colortable_name;
    QUrl m_sizetable_name;
    QUrl m_opacitytable_name;


    QColor m_color;
    qreal m_color_variation;
    qreal m_particleDuration;

    QSGGeometryNode *m_rootNode;
    QHash<int, QSGGeometryNode *> m_nodes;
    QHash<int, int> m_idxStarts;//TODO: Proper resizing will lead to needing a spriteEngine per particle - do this after sprite engine gains transparent sharing?
    int m_lastIdxStart;
    QSGMaterial *m_material;

    // derived values...

    qreal m_render_opacity;
    qreal m_alphaVariation;
    qreal m_alpha;
    qreal m_redVariation;
    qreal m_greenVariation;
    qreal m_blueVariation;
    qreal m_rotation;
    qreal m_rotationVariation;
    qreal m_rotationSpeed;
    qreal m_rotationSpeedVariation;
    bool m_autoRotation;
    QSGDirection* m_xVector;
    QSGDirection* m_yVector;

    QList<QSGSprite*> m_sprites;
    QSGSpriteEngine* m_spriteEngine;

    bool m_bloat;
    PerformanceLevel perfLevel;

    PerformanceLevel m_lastLevel;
    bool m_debugMode;

    template<class Vertex>
    void initTexCoords(Vertex* v, int count){
        Vertex* end = v + count;
        while (v < end){
            v[0].tx = 0;
            v[0].ty = 0;

            v[1].tx = 1;
            v[1].ty = 0;

            v[2].tx = 0;
            v[2].ty = 1;

            v[3].tx = 1;
            v[3].ty = 1;

            v += 4;
        }
    }

    template<class MaterialData>
    MaterialData* getState(QSGMaterial* m){
        return static_cast<QSGSimpleMaterial<MaterialData> *>(m)->state();
    }
    EntryEffect m_entryEffect;
};

QT_END_NAMESPACE
QT_END_HEADER
#endif // ULTRAPARTICLE_H
