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

/*!
    \class QPauseAnimation2
    \brief The QPauseAnimation2 class provides a pause for QSequentialAnimationGroup2.
    \since 4.6
    \ingroup animation

    If you wish to introduce a delay between animations in a
    QSequentialAnimationGroup2, you can insert a QPauseAnimation2. This
    class does not animate anything, but does not
    \l{QAbstractAnimation2::finished()}{finish} before a specified
    number of milliseconds have elapsed from when it was started. You
    specify the duration of the pause in the constructor. It can also
    be set directly with setDuration().

    It is not necessary to construct a QPauseAnimation2 yourself.
    QSequentialAnimationGroup2 provides the convenience functions
    \l{QSequentialAnimationGroup2::}{addPause()} and
    \l{QSequentialAnimationGroup2::}{insertPause()}. These functions
    simply take the number of milliseconds the pause should last.

    \sa QSequentialAnimationGroup2
*/

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

/*!
    Constructs a QPauseAnimation2.
    \a parent is passed to QObject's constructor.
    The default duration is 0.
*/

QPauseAnimation2::QPauseAnimation2(QObject *parent) : QAbstractAnimation2(*new QPauseAnimation2Private, parent)
{
}

/*!
    Constructs a QPauseAnimation2.
    \a msecs is the duration of the pause.
    \a parent is passed to QObject's constructor.
*/

QPauseAnimation2::QPauseAnimation2(int msecs, QObject *parent) : QAbstractAnimation2(*new QPauseAnimation2Private, parent)
{
    setDuration(msecs);
}

/*!
    Destroys the pause animation.
*/
QPauseAnimation2::~QPauseAnimation2()
{
}

/*!
    \property QPauseAnimation2::duration
    \brief the duration of the pause.

    The duration of the pause. The duration should not be negative.
    The default duration is 250 milliseconds.
*/
int QPauseAnimation2::duration() const
{
    Q_D(const QPauseAnimation2);
    return d->duration;
}

void QPauseAnimation2::setDuration(int msecs)
{
    if (msecs < 0) {
        qWarning("QPauseAnimation2::setDuration: cannot set a negative duration");
        return;
    }
    Q_D(QPauseAnimation2);
    d->duration = msecs;
}

/*!
    \reimp
 */
bool QPauseAnimation2::event(QEvent *e)
{
    return QAbstractAnimation2::event(e);
}

/*!
    \reimp
 */
void QPauseAnimation2::updateCurrentTime(int)
{
}


QT_END_NAMESPACE

#include "moc_qpauseanimation2_p.cpp"


