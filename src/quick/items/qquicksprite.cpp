/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include "qquicksprite_p.h"
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmlclass Sprite QQuickSprite
    \inqmlmodule QtQuick 2
    \brief The Sprite element represents a sprite animation

*/
/*!
    \qmlproperty int QtQuick2::Sprite::duration

    Time between frames. Use -1 to indicate one sprite frame per rendered frame.
*/
/*!
    \qmlproperty int QtQuick2::Sprite::durationVariation

    The time between frames can vary by up to this amount. Variation will never decrease the time
    between frames to less than 0.

    Default is 0.
*/

/*!
    \qmlproperty string QtQuick2::Sprite::name

    The name of this sprite, for use in the to property of other sprites.
*/
/*!
    \qmlproperty QVariantMap QtQuick2::Sprite::to

    A list of other sprites and weighted transitions to them,
    for example {"a":1, "b":2, "c":0} would specify that one-third should
    transition to sprite "a" when this sprite is done, and two-thirds should
    transition to sprite "b" when this sprite is done. As the transitions are
    chosen randomly, these proportions will not be exact. With "c":0 in the list,
    no sprites will randomly transition to "c", but it wll be a valid path if a sprite
    goal is set.

    If no list is specified, or the sum of weights in the list is zero, then the sprite
    will repeat itself after completing.
*/
/*!
    \qmlproperty int QtQuick2::Sprite::frames

    Number of frames in this sprite.
*/
/*!
    \qmlproperty int QtQuick2::Sprite::frameHeight

    Height of a single frame in this sprite.
*/
/*!
    \qmlproperty int QtQuick2::Sprite::frameWidth

    Width of a single frame in this sprite.
*/
/*!
    \qmlproperty url QtQuick2::Sprite::source

    The image source for the animation.

    If frameHeight and frameWidth are not specified, it is assumed to be a single long row of square frames.
    Otherwise, it can be multiple contiguous rows or rectangluar frames, when one row runs out the next will be used.
*/

QQuickSprite::QQuickSprite(QObject *parent) :
    QQuickStochasticState(parent)
    , m_generatedCount(0)
    , m_framesPerRow(0)
    , m_frameHeight(0)
    , m_frameWidth(0)
    , m_rowY(0)
{
}

QT_END_NAMESPACE
