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

#ifndef ULTRAPARTICLE_H
#define ULTRAPARTICLE_H
#include "qsgparticlepainter_p.h"
#include "qsgstochasticdirection_p.h"
#include <QDeclarativeListProperty>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class UltraMaterial;
class QSGGeometryNode;

class QSGSprite;
class QSGSpriteEngine;

struct Color4ub {
    uchar r;
    uchar g;
    uchar b;
    uchar a;
};

struct SimpleVertex {
    float x;
    float y;
    float tx;
    float ty;
    float t;
    float lifeSpan;
    float size;
    float endSize;
    float sx;
    float sy;
    float ax;
    float ay;
};

struct SimpleVertices {
    SimpleVertex v1;
    SimpleVertex v2;
    SimpleVertex v3;
    SimpleVertex v4;
};

struct UltraVertex {
    float x;
    float y;
    float tx;
    float ty;
    float t;
    float lifeSpan;
    float size;
    float endSize;
    float sx;
    float sy;
    float ax;
    float ay;
    Color4ub color;
    float xx;
    float xy;
    float yx;
    float yy;
    float rotation;
    float rotationSpeed;
    float autoRotate;//Assume that GPUs prefer floats to bools
    float animIdx;
    float frameDuration;
    float frameCount;
    float animT;
};

struct UltraVertices {
    UltraVertex v1;
    UltraVertex v2;
    UltraVertex v3;
    UltraVertex v4;
};

struct IntermediateVertices {
    UltraVertex* v1;
    UltraVertex* v2;
    UltraVertex* v3;
    UltraVertex* v4;
};

class QSGImageParticle : public QSGParticlePainter
{
    Q_OBJECT
    Q_PROPERTY(QUrl image READ image WRITE setImage NOTIFY imageChanged)
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
    Q_PROPERTY(QSGStochasticDirection* xVector READ xVector WRITE setXVector NOTIFY xVectorChanged)
    //yVector is the same, but top-left to bottom-left. The particle is always a parallelogram.
    Q_PROPERTY(QSGStochasticDirection* yVector READ yVector WRITE setYVector NOTIFY yVectorChanged)
    Q_PROPERTY(QDeclarativeListProperty<QSGSprite> sprites READ sprites)
    Q_PROPERTY(bool bloat READ bloat WRITE setBloat NOTIFY bloatChanged)//Just a debugging property to bypass optimizations
public:
    explicit QSGImageParticle(QSGItem *parent = 0);
    virtual ~QSGImageParticle(){}

    virtual void load(QSGParticleData*);
    virtual void reload(QSGParticleData*);
    virtual void setCount(int c);

    QDeclarativeListProperty<QSGSprite> sprites();
    QSGSpriteEngine* spriteEngine() {return m_spriteEngine;}

    enum PerformanceLevel{//TODO: Expose?
        Unknown = 0,
        Simple,
        Coloured,
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

    QSGStochasticDirection* xVector() const { return m_xVector; }

    QSGStochasticDirection* yVector() const { return m_yVector; }

    bool bloat() const { return m_bloat; }

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

    void xVectorChanged(QSGStochasticDirection* arg);

    void yVectorChanged(QSGStochasticDirection* arg);

    void bloatChanged(bool arg);

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

    void setXVector(QSGStochasticDirection* arg);

    void setYVector(QSGStochasticDirection* arg);

    void setBloat(bool arg);

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);
    void reset();
    void prepareNextFrame();
    QSGGeometryNode* buildParticleNode();
    QSGGeometryNode* buildSimpleParticleNode();

private slots:
    void createEngine(); //### method invoked by sprite list changing (in engine.h) - pretty nasty

private:
    //void vertexCopy(UltraVertex &b,const ParticleVertex& a);
    IntermediateVertices* fetchIntermediateVertices(int pos);
    bool m_do_reset;

    QUrl m_image_name;
    QUrl m_colortable_name;
    QUrl m_sizetable_name;
    QUrl m_opacitytable_name;


    QColor m_color;
    qreal m_color_variation;
    qreal m_particleDuration;

    QSGGeometryNode *m_node;
    UltraMaterial *m_material;

    // derived values...
    int m_last_particle;

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
    QSGStochasticDirection* m_xVector;
    QSGStochasticDirection* m_yVector;

    QList<QSGSprite*> m_sprites;
    QSGSpriteEngine* m_spriteEngine;

    bool m_bloat;
    PerformanceLevel perfLevel;

    PerformanceLevel m_lastLevel;
    void* m_lastData;
};

QT_END_NAMESPACE
QT_END_HEADER
#endif // ULTRAPARTICLE_H
