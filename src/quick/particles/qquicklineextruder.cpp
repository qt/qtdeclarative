/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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
#include "qquicklineextruder_p.h"
#include <stdlib.h>
#include <cmath>

/*!
    \qmlclass LineShape QQuickLineExtruder
    \inqmlmodule QtQuick.Particles 2
    \inherits Shape
    \brief The LineShape represents a line to Affectors and Emitter

*/

/*!
    \qmlproperty bool QtQuick.Particles2::LineShape::mirrored

    By default, the line goes from (0,0) to (width, height) of the item that
    this shape is being applied to.

    If mirrored is set to true, this will be mirrored along the y axis.
    The line will then go from (0,height) to (width, 0).
*/

QQuickLineExtruder::QQuickLineExtruder(QObject *parent) :
    QQuickParticleExtruder(parent), m_mirrored(false)
{
}

QPointF QQuickLineExtruder::extrude(const QRectF &r)
{
    qreal x,y;
    if (!r.height()){
        x = r.width() * ((qreal)rand())/RAND_MAX;
        y = 0;
    }else{
        y = r.height() * ((qreal)rand())/RAND_MAX;
        if (!r.width()){
            x = 0;
        }else{
            x = r.width()/r.height() * y;
            if (m_mirrored)
                x = r.width() - x;
        }
    }
    return QPointF(x,y);
}
