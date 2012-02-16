/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPARALLELANIMATIONGROUPJOB_P_H
#define QPARALLELANIMATIONGROUPJOB_P_H

#include "private/qanimationgroupjob_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Qml)

class Q_QML_EXPORT QParallelAnimationGroupJob : public QAnimationGroupJob
{
    Q_DISABLE_COPY(QParallelAnimationGroupJob)
public:
    QParallelAnimationGroupJob();
    ~QParallelAnimationGroupJob();

    int duration() const;

protected:
    void updateCurrentTime(int currentTime);
    void updateState(QAbstractAnimationJob::State newState, QAbstractAnimationJob::State oldState);
    void updateDirection(QAbstractAnimationJob::Direction direction);
    void uncontrolledAnimationFinished(QAbstractAnimationJob *animation);

private:
    bool shouldAnimationStart(QAbstractAnimationJob *animation, bool startIfAtEnd) const;
    void applyGroupState(QAbstractAnimationJob *animation);

    //state
    int m_previousLoop;
    int m_previousCurrentTime;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPARALLELANIMATIONGROUPJOB_P_H
