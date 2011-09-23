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

#include "private/qpauseanimation2_p.h"
#include "private/qabstractanimation2_p_p.h"

QT_BEGIN_NAMESPACE

class QPauseAnimation2Private : public QAbstractAnimation2Private
{
public:
    QPauseAnimation2Private() : QAbstractAnimation2Private(), duration(250)
    {
        isPause = true;
    }

    int duration;
};

QPauseAnimation2::QPauseAnimation2(QDeclarativeAbstractAnimation *animation)
    : QAbstractAnimation2(new QPauseAnimation2Private, animation)
{
}

QPauseAnimation2::QPauseAnimation2(int msecs, QDeclarativeAbstractAnimation *animation)
    : QAbstractAnimation2(new QPauseAnimation2Private, animation)
{
    setDuration(msecs);
}

QPauseAnimation2::QPauseAnimation2(QPauseAnimation2Private* dd, QDeclarativeAbstractAnimation *animation)
    :QAbstractAnimation2(dd, animation)
{
}

QPauseAnimation2::~QPauseAnimation2()
{
}

int QPauseAnimation2::duration() const
{
    return d_func()->duration;
}

void QPauseAnimation2::setDuration(int msecs)
{
    if (msecs < 0) {
        qWarning("QPauseAnimation2::setDuration: cannot set a negative duration");
        return;
    }
    d_func()->duration = msecs;
}

void QPauseAnimation2::updateCurrentTime(int)
{
}


QT_END_NAMESPACE
