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

#ifndef SPRITEENGINE_H
#define SPRITEENGINE_H

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

QT_MODULE(Declarative)

class QSGSprite;

class QSGSpriteEngine : public QObject
{
    Q_OBJECT
    //TODO: Optimize single sprite case
    Q_PROPERTY(QDeclarativeListProperty<QSGSprite> sprites READ sprites)
    Q_PROPERTY(QString globalGoal READ globalGoal WRITE setGlobalGoal NOTIFY globalGoalChanged)
public:
    explicit QSGSpriteEngine(QObject *parent = 0);
    QSGSpriteEngine(QList<QSGSprite*> sprites, QObject *parent=0);
    ~QSGSpriteEngine();

    QDeclarativeListProperty<QSGSprite> sprites()
    {
        return QDeclarativeListProperty<QSGSprite>(this, m_states);
    }
    QString globalGoal() const
    {
        return m_globalGoal;
    }

    int count() const {return m_sprites.count();}
    void setCount(int c);

    int spriteState(int sprite=0);// {return m_sprites[sprite];}
    int spriteStart(int sprite=0);// {return m_startTimes[sprite];}
    int spriteFrames(int sprite=0);
    int spriteDuration(int sprite=0);
    int spriteCount();//Like state count, but for the image states
    int maxFrames();

    void setGoal(int state, int sprite=0, bool jump=false);
    QImage assembledImage();

    void startSprite(int index=0, int state=0);
    void stopSprite(int index=0);

private://Nothing outside should use this?
    friend class QSGSpriteGoalAffector;//XXX: Fix interface
    int stateCount() {return m_states.count();}
    int stateIndex(QSGSprite* s){return m_states.indexOf(s);}//TODO: Does this need to be hidden?
    QSGSprite* state(int idx){return m_states[idx];}//Used by spritegoal affector
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

private:
    friend class QSGParticleSystem;
    void restartSprite(int sprite);
    void addToUpdateList(uint t, int idx);
    int goalSeek(int curState, int spriteIdx, int dist=-1);
    QList<QSGSprite*> m_states;
    QVector<int> m_sprites;//int is the index in m_states of the current state
    QVector<int> m_goals;
    QVector<int> m_startTimes;
    QList<QPair<uint, QList<int> > > m_stateUpdates;//### This could be done faster - priority queue?

    QTime m_advanceTime;
    uint m_timeOffset;
    QString m_globalGoal;
    int m_maxFrames;
    int m_imageStateCount;
};

//Common use is to have your own list property which is transparently an engine
inline void spriteAppend(QDeclarativeListProperty<QSGSprite> *p, QSGSprite* s)
{
    reinterpret_cast<QList<QSGSprite *> *>(p->data)->append(s);
    p->object->metaObject()->invokeMethod(p->object, "createEngine");
}

inline QSGSprite* spriteAt(QDeclarativeListProperty<QSGSprite> *p, int idx)
{
    return reinterpret_cast<QList<QSGSprite *> *>(p->data)->at(idx);
}

inline void spriteClear(QDeclarativeListProperty<QSGSprite> *p)
{
    reinterpret_cast<QList<QSGSprite *> *>(p->data)->clear();
    p->object->metaObject()->invokeMethod(p->object, "createEngine");
}

inline int spriteCount(QDeclarativeListProperty<QSGSprite> *p)
{
    return reinterpret_cast<QList<QSGSprite *> *>(p->data)->count();
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // SPRITEENGINE_H
