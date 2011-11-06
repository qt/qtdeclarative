/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QANIMATIONGROUP2_P_H
#define QANIMATIONGROUP2_P_H

#include <QtCore/qlist.h>
#include "private/qabstractanimation2_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class Q_DECLARATIVE_EXPORT QAnimationGroup2 : public QAbstractAnimation2
{
public:
    QAnimationGroup2(QDeclarativeAbstractAnimation* animation = 0);
    QAnimationGroup2(const QAnimationGroup2& other);
    ~QAnimationGroup2();

    //unused
    int animationCount() const;
    void clear();
    QAbstractAnimation2Pointer animationAt(int index) const;
    int indexOfAnimation(QAbstractAnimation2Pointer animation) const;

    //can be removed after refactor
    //the parameter needs to be "QAbstractAnimation2 *" here, as it will be
    //called in the QAbstractAnimation2's dtor
    void removeAnimation(QAbstractAnimation2 *animation);
    QAbstractAnimation2Pointer takeAnimation(int index);

    void addAnimation(QAbstractAnimation2Pointer animation);
    void insertAnimation(int index, QAbstractAnimation2Pointer animation);
    virtual void uncontrolledAnimationFinished(QAbstractAnimation2Pointer animation);

protected:
    void topLevelAnimationLoopChanged();

private:
    //can likely be removed after refactor
    virtual void animationInsertedAt(int) { }
    virtual void animationRemoved(int, QAbstractAnimation2Pointer);

    void connectUncontrolledAnimations();
    void disconnectUncontrolledAnimations();
    void connectUncontrolledAnimation(QAbstractAnimation2Pointer anim);
    void disconnectUncontrolledAnimation(QAbstractAnimation2Pointer anim);
    bool isAnimationConnected(QAbstractAnimation2Pointer anim) const;
    bool isUncontrolledAnimationFinished(QAbstractAnimation2Pointer anim) const;

    friend class QParallelAnimationGroup2;
    friend class QSequentialAnimationGroup2;
private:
    //definition
    QList<QAbstractAnimation2Pointer> m_animations;
    //state
    QHash<QAbstractAnimation2Pointer, int> m_uncontrolledFinishTime;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif //QANIMATIONGROUP2_P_H
