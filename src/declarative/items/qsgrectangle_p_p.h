// Commit: ac5c099cc3c5b8c7eec7a49fdeb8a21037230350
/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

#ifndef QSGRECTANGLE_P_P_H
#define QSGRECTANGLE_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qsgitem_p.h"

QT_BEGIN_NAMESPACE

class QSGGradient;
class QSGRectangle;
class QSGRectanglePrivate : public QSGItemPrivate
{
    Q_DECLARE_PUBLIC(QSGRectangle)

public:
    QSGRectanglePrivate() :
    color(Qt::white), gradient(0), pen(0), radius(0), penMargin(0), penOffset(0)
    {
    }

    ~QSGRectanglePrivate()
    {
        delete pen;
    }

    QColor color;
    QSGGradient *gradient;
    QSGPen *pen;
    qreal radius;
    qreal penMargin;
    qreal penOffset;
    static int doUpdateSlotIdx;

    QSGPen *getPen() {
        if (!pen) {
            Q_Q(QSGRectangle);
            pen = new QSGPen;
            static int penChangedSignalIdx = -1;
            if (penChangedSignalIdx < 0)
                penChangedSignalIdx = QSGPen::staticMetaObject.indexOfSignal("penChanged()");
            if (doUpdateSlotIdx < 0)
                doUpdateSlotIdx = QSGRectangle::staticMetaObject.indexOfSlot("doUpdate()");
            QMetaObject::connect(pen, penChangedSignalIdx, q, doUpdateSlotIdx);
        }
        return pen;
    }
};

QT_END_NAMESPACE

#endif // QSGRECTANGLE_P_P_H
