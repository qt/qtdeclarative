/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QQUICKSPRITEENGINE_P_H
#define QQUICKSPRITEENGINE_P_H

#include <QObject>
#include <QVector>
#include <QTimer>
#include <QTime>
#include <QList>
#include <QDeclarativeListProperty>
#include <QImage>
#include <QPair>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQuickSprite;
class Q_AUTOTEST_EXPORT QQuickStochasticState : public QObject //For internal use
{
    Q_OBJECT
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(int durationVariation READ durationVariance WRITE setDurationVariance NOTIFY durationVarianceChanged)
    Q_PROPERTY(QVariantMap to READ to WRITE setTo NOTIFY toChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(qreal speedModifiesDuration READ speedModifer WRITE setSpeedModifier NOTIFY speedModifierChanged)
    Q_PROPERTY(int frames READ frames WRITE setFrames NOTIFY framesChanged)

public:
    QQuickStochasticState(QObject* parent = 0)
        : QObject(parent)
        , m_frames(1)
        , m_duration(1000)
    {
    }

    int duration() const
    {
        return m_duration;
    }

    QString name() const
    {
        return m_name;
    }

    QVariantMap to() const
    {
        return m_to;
    }

    qreal speedModifer() const
    {
        return m_speedModifier;
    }

    int durationVariance() const
    {
        return m_durationVariance;
    }


    int variedDuration() const
    {
        return m_duration
                + (m_durationVariance * ((qreal)qrand()/RAND_MAX) * 2)
                - m_durationVariance;
    }

    int frames() const
    {
        return m_frames;
    }

signals:
    void durationChanged(int arg);

    void nameChanged(QString arg);

    void toChanged(QVariantMap arg);

    void speedModifierChanged(qreal arg);

    void durationVarianceChanged(int arg);

    void entered();//### Just playing around - don't expect full state API
    void framesChanged(int arg);

public slots:
    void setDuration(int arg)
    {
        if (m_duration != arg) {
            m_duration = arg;
            emit durationChanged(arg);
        }
    }

    void setName(QString arg)
    {
        if (m_name != arg) {
            m_name = arg;
            emit nameChanged(arg);
        }
    }

    void setTo(QVariantMap arg)
    {
        if (m_to != arg) {
            m_to = arg;
            emit toChanged(arg);
        }
    }

    void setSpeedModifier(qreal arg)
    {
        if (m_speedModifier != arg) {
            m_speedModifier = arg;
            emit speedModifierChanged(arg);
        }
    }

    void setDurationVariance(int arg)
    {
        if (m_durationVariance != arg) {
            m_durationVariance = arg;
            emit durationVarianceChanged(arg);
        }
    }

    void setFrames(int arg)
    {
        if (m_frames != arg) {
            m_frames = arg;
            emit framesChanged(arg);
        }
    }

private:
    QString m_name;
    int m_frames;
    QVariantMap m_to;
    int m_duration;
    qreal m_speedModifier;
    int m_durationVariance;

    friend class QQuickStochasticEngine;
};

class Q_AUTOTEST_EXPORT QQuickStochasticEngine : public QObject
{
    Q_OBJECT
    //TODO: Optimize single state case?
    Q_PROPERTY(QString globalGoal READ globalGoal WRITE setGlobalGoal NOTIFY globalGoalChanged)
    Q_PROPERTY(QDeclarativeListProperty<QQuickStochasticState> states READ states)
public:
    explicit QQuickStochasticEngine(QObject *parent = 0);
    QQuickStochasticEngine(QList<QQuickStochasticState*> states, QObject *parent=0);
    ~QQuickStochasticEngine();

    QDeclarativeListProperty<QQuickStochasticState> states()
    {
        return QDeclarativeListProperty<QQuickStochasticState>(this, m_states);
    }

    QString globalGoal() const
    {
        return m_globalGoal;
    }

    int count() const {return m_things.count();}
    void setCount(int c);

    void setGoal(int state, int sprite=0, bool jump=false);
    void start(int index=0, int state=0);
    void stop(int index=0);
    int curState(int index=0) {return m_things[index];}

    QQuickStochasticState* state(int idx){return m_states[idx];}
    int stateIndex(QQuickStochasticState* s){return m_states.indexOf(s);}
    int stateIndex(const QString& s) {
        for (int i=0; i<m_states.count(); i++)
            if (m_states[i]->name() == s)
                return i;
        return -1;
    }

    int stateCount() {return m_states.count();}
private:
signals:

    void globalGoalChanged(QString arg);
    void stateChanged(int idx);

public slots:
    void setGlobalGoal(QString arg)
    {
        if (m_globalGoal != arg) {
            m_globalGoal = arg;
            emit globalGoalChanged(arg);
        }
    }

    uint updateSprites(uint time);

protected:
    friend class QQuickParticleSystem;
    void restart(int index);
    void addToUpdateList(uint t, int idx);
    int goalSeek(int curState, int idx, int dist=-1);
    QList<QQuickStochasticState*> m_states;
    //### Consider struct or class for the four data variables?
    QVector<int> m_things;//int is the index in m_states of the current state
    QVector<int> m_goals;
    QVector<int> m_duration;
    QVector<int> m_startTimes;
    QList<QPair<uint, QList<int> > > m_stateUpdates;//### This could be done faster - priority queue?

    QTime m_advanceTime;
    uint m_timeOffset;
    QString m_globalGoal;
    int m_maxFrames;
    int m_imageStateCount;
};

class QQuickSpriteEngine : public QQuickStochasticEngine
{
    Q_OBJECT
    Q_PROPERTY(QDeclarativeListProperty<QQuickSprite> sprites READ sprites)
public:
    explicit QQuickSpriteEngine(QObject *parent = 0);
    QQuickSpriteEngine(QList<QQuickSprite*> sprites, QObject *parent=0);
    ~QQuickSpriteEngine();
    QDeclarativeListProperty<QQuickSprite> sprites()
    {
        return QDeclarativeListProperty<QQuickSprite>(this, m_sprites);
    }


    int spriteState(int sprite=0);
    int spriteStart(int sprite=0);
    int spriteFrames(int sprite=0);
    int spriteDuration(int sprite=0);
    int spriteX(int /* sprite */ = 0) { return 0; }//Currently all rows are 0 aligned, if we get more space efficient we might change this
    int spriteY(int sprite=0);
    int spriteWidth(int sprite=0);
    int spriteHeight(int sprite=0);
    int spriteCount();//Like state count, but for the image states
    int maxFrames();
    QImage assembledImage();
private:
    QList<QQuickSprite*> m_sprites;
};

//Common use is to have your own list property which is transparently an engine
inline void spriteAppend(QDeclarativeListProperty<QQuickSprite> *p, QQuickSprite* s)
{
    reinterpret_cast<QList<QQuickSprite *> *>(p->data)->append(s);
    p->object->metaObject()->invokeMethod(p->object, "createEngine");
}

inline QQuickSprite* spriteAt(QDeclarativeListProperty<QQuickSprite> *p, int idx)
{
    return reinterpret_cast<QList<QQuickSprite *> *>(p->data)->at(idx);
}

inline void spriteClear(QDeclarativeListProperty<QQuickSprite> *p)
{
    reinterpret_cast<QList<QQuickSprite *> *>(p->data)->clear();
    p->object->metaObject()->invokeMethod(p->object, "createEngine");
}

inline int spriteCount(QDeclarativeListProperty<QQuickSprite> *p)
{
    return reinterpret_cast<QList<QQuickSprite *> *>(p->data)->count();
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QQUICKSPRITEENGINE_P_H
