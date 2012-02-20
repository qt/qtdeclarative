/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKITEMCHANGELISTENER_P_H
#define QQUICKITEMCHANGELISTENER_P_H

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

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

class QRectF;
class QQuickItem;
class QQuickAnchorsPrivate;
class QQuickItemChangeListener
{
public:
    virtual ~QQuickItemChangeListener() {};

    virtual void itemGeometryChanged(QQuickItem *, const QRectF & /* new */, const QRectF & /* old */ ) {}
    virtual void itemSiblingOrderChanged(QQuickItem *) {}
    virtual void itemVisibilityChanged(QQuickItem *) {}
    virtual void itemOpacityChanged(QQuickItem *) {}
    virtual void itemDestroyed(QQuickItem *) {}
    virtual void itemChildAdded(QQuickItem *, QQuickItem *) {}
    virtual void itemChildRemoved(QQuickItem *, QQuickItem *) {}
    virtual void itemParentChanged(QQuickItem *, QQuickItem *) {}
    virtual void itemRotationChanged(QQuickItem *) {}

    virtual QQuickAnchorsPrivate *anchorPrivate() { return 0; }
};

QT_END_NAMESPACE

#endif // QQUICKITEMCHANGELISTENER_P_H
