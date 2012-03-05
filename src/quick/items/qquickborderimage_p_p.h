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

#ifndef QQUICKBORDERIMAGE_P_P_H
#define QQUICKBORDERIMAGE_P_P_H

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

#include "qquickimagebase_p_p.h"
#include "qquickscalegrid_p_p.h"

#include <private/qqmlglobal_p.h>

QT_BEGIN_NAMESPACE

class QNetworkReply;
class QQuickBorderImagePrivate : public QQuickImageBasePrivate
{
    Q_DECLARE_PUBLIC(QQuickBorderImage)

public:
    QQuickBorderImagePrivate()
      : border(0), sciReply(0),
        horizontalTileMode(QQuickBorderImage::Stretch),
        verticalTileMode(QQuickBorderImage::Stretch),
        redirectCount(0), pixmapChanged(false)
    {
    }

    ~QQuickBorderImagePrivate()
    {
    }


    QQuickScaleGrid *getScaleGrid()
    {
        Q_Q(QQuickBorderImage);
        if (!border) {
            border = new QQuickScaleGrid(q);
            FAST_CONNECT(border, SIGNAL(borderChanged()), q, SLOT(doUpdate()))
        }
        return border;
    }

    QQuickScaleGrid *border;
    QUrl sciurl;
    QNetworkReply *sciReply;
    QQuickBorderImage::TileMode horizontalTileMode;
    QQuickBorderImage::TileMode verticalTileMode;
    int redirectCount;

    bool pixmapChanged : 1;
};

QT_END_NAMESPACE

#endif // QQUICKBORDERIMAGE_P_P_H
