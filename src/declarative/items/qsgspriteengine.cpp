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

#include "qsgspriteengine_p.h"
#include "qsgsprite_p.h"
#include <QDebug>
#include <QPainter>
#include <QSet>
#include <QtOpenGL>

QT_BEGIN_NAMESPACE

/* TODO: Split out image logic from stochastic state logic
   Also make sharable
   Also solve the state data initialization/transfer issue so as to not need to make friends
*/

QSGSpriteEngine::QSGSpriteEngine(QObject *parent) :
    QObject(parent), m_timeOffset(0)
{
    //Default size 1
    setCount(1);
    m_advanceTime.start();
}

QSGSpriteEngine::QSGSpriteEngine(QList<QSGSprite*> states, QObject *parent) :
    QObject(parent), m_states(states), m_timeOffset(0)
{
    //Default size 1
    setCount(1);
    m_advanceTime.start();
}

QSGSpriteEngine::~QSGSpriteEngine()
{
}

int QSGSpriteEngine::maxFrames()
{
    return m_maxFrames;
}

/* States too large to fit in one row are split into multiple rows
   This is more efficient for the implementation, but should remain an implementation detail (invisible from QML)
   Therefore the below functions abstract sprite from the viewpoint of classes that pass the details onto shaders
   But States maintain their listed index for internal structures
TODO: All these calculations should be pre-calculated and cached during initialization for a significant performance boost
*/
int QSGSpriteEngine::spriteState(int sprite)
{
    int state = m_sprites[sprite];
    if (!m_states[state]->m_generatedCount)
        return state;
    int rowDuration = m_states[state]->duration() * m_states[state]->m_framesPerRow;
    int extra = (m_timeOffset - m_startTimes[sprite])/rowDuration;
    return state + extra;
}

int QSGSpriteEngine::spriteStart(int sprite)
{
    int state = m_sprites[sprite];
    if (!m_states[state]->m_generatedCount)
        return m_startTimes[sprite];
    int rowDuration = m_states[state]->duration() * m_states[state]->m_framesPerRow;
    int extra = (m_timeOffset - m_startTimes[sprite])/rowDuration;
    return state + extra*rowDuration;
}

int QSGSpriteEngine::spriteFrames(int sprite)
{
    int state = m_sprites[sprite];
    if (!m_states[state]->m_generatedCount)
        return m_states[state]->frames();
    int rowDuration = m_states[state]->duration() * m_states[state]->m_framesPerRow;
    int extra = (m_timeOffset - m_startTimes[sprite])/rowDuration;
    if (extra == m_states[state]->m_generatedCount - 1)//last state
        return m_states[state]->frames() % m_states[state]->m_framesPerRow;
    else
        return m_states[state]->m_framesPerRow;
}

int QSGSpriteEngine::spriteDuration(int sprite)
{
    int state = m_sprites[sprite];
    if (!m_states[state]->m_generatedCount)
        return m_states[state]->duration();
    int rowDuration = m_states[state]->duration() * m_states[state]->m_framesPerRow;
    int extra = (m_timeOffset - m_startTimes[sprite])/rowDuration;
    if (extra == m_states[state]->m_generatedCount - 1)//last state
        return (m_states[state]->duration() * m_states[state]->frames()) % rowDuration;
    else
        return rowDuration;
}

int QSGSpriteEngine::spriteCount()//TODO: Actually image state count, need to rename these things to make sense together
{
    return m_imageStateCount;
}

void QSGSpriteEngine::setGoal(int state, int sprite, bool jump)
{
    if (sprite >= m_sprites.count() || state >= m_states.count())
        return;
    if (!jump){
        m_goals[sprite] = state;
        return;
    }

    if (m_sprites[sprite] == state)
        return;//Already there
    m_sprites[sprite] = state;
    m_goals[sprite] = -1;
    restartSprite(sprite);
    emit stateChanged(sprite);
    emit m_states[state]->entered();
    return;
}

QImage QSGSpriteEngine::assembledImage()
{
    int frameHeight = 0;
    int frameWidth = 0;
    m_maxFrames = 0;
    m_imageStateCount = 0;

    int maxSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);

    foreach (QSGSprite* state, m_states){
        if (state->frames() > m_maxFrames)
            m_maxFrames = state->frames();

        QImage img(state->source().toLocalFile());
        if (img.isNull()) {
            qWarning() << "SpriteEngine: loading image failed..." << state->source().toLocalFile();
            return QImage();
        }

        //Check that the frame sizes are the same within one engine
        int imgWidth = state->frameWidth();
        if (!imgWidth)
            imgWidth = img.width() / state->frames();
        if (frameWidth){
            if (imgWidth != frameWidth){
                qWarning() << "SpriteEngine: Irregular frame width..." << state->source().toLocalFile();
                return QImage();
            }
        }else{
            frameWidth = imgWidth;
        }

        int imgHeight = state->frameHeight();
        if (!imgHeight)
            imgHeight = img.height();
        if (frameHeight){
            if (imgHeight!=frameHeight){
                qWarning() << "SpriteEngine: Irregular frame height..." << state->source().toLocalFile();
                return QImage();
            }
        }else{
            frameHeight = imgHeight;
        }

        if (state->frames() * frameWidth > maxSize){
            struct helper{
                static int divRoundUp(int a, int b){return (a+b-1)/b;}
            };
            int rowsNeeded = helper::divRoundUp(state->frames(), helper::divRoundUp(maxSize, frameWidth));
            if (rowsNeeded * frameHeight > maxSize){
                qWarning() << "SpriteEngine: Animation too large to fit in one texture..." << state->source().toLocalFile();
                qWarning() << "SpriteEngine: Your texture max size today is " << maxSize;
            }
            state->m_generatedCount = rowsNeeded;
            m_imageStateCount += rowsNeeded;
        }else{
            m_imageStateCount++;
        }
    }

    //maxFrames is max number in a line of the texture
    if (m_maxFrames * frameWidth > maxSize)
        m_maxFrames = maxSize/frameWidth;
    QImage image(frameWidth * m_maxFrames, frameHeight * m_imageStateCount, QImage::Format_ARGB32);
    image.fill(0);
    QPainter p(&image);
    int y = 0;
    foreach (QSGSprite* state, m_states){
        QImage img(state->source().toLocalFile());
        if (img.height() == frameHeight && img.width() <  maxSize){//Simple case
            p.drawImage(0,y,img);
            y += frameHeight;
        }else{
            state->m_framesPerRow = image.width()/frameWidth;
            int x = 0;
            int curX = 0;
            int curY = 0;
            int framesLeft = state->frames();
            while (framesLeft > 0){
                if (image.width() - x + curX <= img.width()){//finish a row in image (dest)
                    int copied = image.width() - x;
                    Q_ASSERT(!(copied % frameWidth));//XXX: Just checking
                    framesLeft -= copied/frameWidth;
                    p.drawImage(x,y,img.copy(curX,curY,copied,frameHeight));
                    y += frameHeight;
                    curX += copied;
                    x = 0;
                    if (curX == img.width()){
                        curX = 0;
                        curY += frameHeight;
                    }
                }else{//finish a row in img (src)
                    int copied = img.width() - curX;
                    Q_ASSERT(!(copied % frameWidth));//XXX: Just checking
                    framesLeft -= copied/frameWidth;
                    p.drawImage(x,y,img.copy(curX,curY,copied,frameHeight));
                    curY += frameHeight;
                    x += copied;
                    curX = 0;
                }
            }
            if (x)
                y += frameHeight;
        }
    }

    if (image.height() > maxSize){
        qWarning() << "SpriteEngine: Too many animations to fit in one texture...";
        qWarning() << "SpriteEngine: Your texture max size today is " << maxSize;
        return QImage();
    }
    return image;
}

void QSGSpriteEngine::setCount(int c)
{
    m_sprites.resize(c);
    m_goals.resize(c);
    m_startTimes.resize(c);
}

void QSGSpriteEngine::startSprite(int index, int state)
{
    if (index >= m_sprites.count())
        return;
    m_sprites[index] = state;
    m_goals[index] = -1;
    restartSprite(index);
}

void QSGSpriteEngine::stopSprite(int index)
{
    if (index >= m_sprites.count())
        return;
    //Will never change until start is called again with a new state - this is not a 'pause'
    for (int i=0; i<m_stateUpdates.count(); i++)
        m_stateUpdates[i].second.removeAll(index);
}

void QSGSpriteEngine::restartSprite(int index)
{
    m_startTimes[index] = m_timeOffset + m_advanceTime.elapsed();
    int time = m_states[m_sprites[index]]->duration() * m_states[m_sprites[index]]->frames() + m_startTimes[index];
    for (int i=0; i<m_stateUpdates.count(); i++)
        m_stateUpdates[i].second.removeAll(index);
    addToUpdateList(time, index);
}

uint QSGSpriteEngine::updateSprites(uint time)//### would returning a list of changed idxs be faster than signals?
{
    //Sprite State Update;
    QSet<int> changedIndexes;
    while (!m_stateUpdates.isEmpty() && time >= m_stateUpdates.first().first){
        foreach (int idx, m_stateUpdates.first().second){
            if (idx >= m_sprites.count())
                continue;//TODO: Proper fix(because this does happen and I'm just ignoring it)
            int stateIdx = m_sprites[idx];
            int nextIdx = -1;
            int goalPath = goalSeek(stateIdx, idx);
            if (goalPath == -1){//Random
                qreal r =(qreal) qrand() / (qreal) RAND_MAX;
                qreal total = 0.0;
                for (QVariantMap::const_iterator iter=m_states[stateIdx]->m_to.constBegin();
                    iter!=m_states[stateIdx]->m_to.constEnd(); iter++)
                    total += (*iter).toReal();
                r*=total;
                for (QVariantMap::const_iterator iter= m_states[stateIdx]->m_to.constBegin();
                        iter!=m_states[stateIdx]->m_to.constEnd(); iter++){
                    if (r < (*iter).toReal()){
                        bool superBreak = false;
                        for (int i=0; i<m_states.count(); i++){
                            if (m_states[i]->name() == iter.key()){
                                nextIdx = i;
                                superBreak = true;
                                break;
                            }
                        }
                        if (superBreak)
                            break;
                    }
                    r -= (*iter).toReal();
                }
            }else{//Random out of shortest paths to goal
                nextIdx = goalPath;
            }
            if (nextIdx == -1)//No to states means stay here
                nextIdx = stateIdx;

            m_sprites[idx] = nextIdx;
            m_startTimes[idx] = time;
            if (nextIdx != stateIdx){
                changedIndexes << idx;
                emit m_states[nextIdx]->entered();
            }
            addToUpdateList((m_states[nextIdx]->duration() * m_states[nextIdx]->frames()) + time, idx);
        }
        m_stateUpdates.pop_front();
    }

    m_timeOffset = time;
    m_advanceTime.start();
    //TODO: emit this when a psuedostate changes too
    foreach (int idx, changedIndexes){//Batched so that update list doesn't change midway
        emit stateChanged(idx);
    }
    if (m_stateUpdates.isEmpty())
        return -1;
    return m_stateUpdates.first().first;
}

int QSGSpriteEngine::goalSeek(int curIdx, int spriteIdx, int dist)
{
    QString goalName;
    if (m_goals[spriteIdx] != -1)
        goalName = m_states[m_goals[spriteIdx]]->name();
    else
        goalName = m_globalGoal;
    if (goalName.isEmpty())
        return -1;
    //TODO: caching instead of excessively redoing iterative deepening (which was chosen arbitarily anyways)
    // Paraphrased - implement in an *efficient* manner
    for (int i=0; i<m_states.count(); i++)
        if (m_states[curIdx]->name() == goalName)
            return curIdx;
    if (dist < 0)
        dist = m_states.count();
    QSGSprite* curState = m_states[curIdx];
    for (QVariantMap::const_iterator iter = curState->m_to.constBegin();
        iter!=curState->m_to.constEnd(); iter++){
        if (iter.key() == goalName)
            for (int i=0; i<m_states.count(); i++)
                if (m_states[i]->name() == goalName)
                    return i;
    }
    QSet<int> options;
    for (int i=1; i<dist; i++){
        for (QVariantMap::const_iterator iter = curState->m_to.constBegin();
            iter!=curState->m_to.constEnd(); iter++){
            int option = -1;
            for (int j=0; j<m_states.count(); j++)//One place that could be a lot more efficient...
                if (m_states[j]->name() == iter.key())
                    if (goalSeek(j, spriteIdx, i) != -1)
                        option = j;
            if (option != -1)
                options << option;
        }
        if (!options.isEmpty()){
            if (options.count()==1)
                return *(options.begin());
            int option = -1;
            qreal r =(qreal) qrand() / (qreal) RAND_MAX;
            qreal total;
            for (QSet<int>::const_iterator iter=options.constBegin();
                iter!=options.constEnd(); iter++)
                total += curState->m_to.value(m_states[(*iter)]->name()).toReal();
            r *= total;
            for (QVariantMap::const_iterator iter = curState->m_to.constBegin();
                iter!=curState->m_to.constEnd(); iter++){
                bool superContinue = true;
                for (int j=0; j<m_states.count(); j++)
                    if (m_states[j]->name() == iter.key())
                        if (options.contains(j))
                            superContinue = false;
                if (superContinue)
                    continue;
                if (r < (*iter).toReal()){
                    bool superBreak = false;
                    for (int j=0; j<m_states.count(); j++){
                        if (m_states[j]->name() == iter.key()){
                            option = j;
                            superBreak = true;
                            break;
                        }
                    }
                    if (superBreak)
                        break;
                }
                r-=(*iter).toReal();
            }
            return option;
        }
    }
    return -1;
}

void QSGSpriteEngine::addToUpdateList(uint t, int idx)
{
    for (int i=0; i<m_stateUpdates.count(); i++){
        if (m_stateUpdates[i].first==t){
            m_stateUpdates[i].second << idx;
            return;
        }else if (m_stateUpdates[i].first > t){
            QList<int> tmpList;
            tmpList << idx;
            m_stateUpdates.insert(i, qMakePair(t, tmpList));
            return;
        }
    }
    QList<int> tmpList;
    tmpList << idx;
    m_stateUpdates << qMakePair(t, tmpList);
}

QT_END_NAMESPACE
