/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEIMAGE_H
#define QDECLARATIVEIMAGE_H

#include "private/qdeclarativeimagebase_p.h"

#include <QtNetwork/qnetworkreply.h>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE


class QDeclarative1ImagePrivate;
class Q_AUTOTEST_EXPORT QDeclarative1Image : public QDeclarative1ImageBase
{
    Q_OBJECT
    Q_ENUMS(FillMode)

    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_PROPERTY(qreal paintedWidth READ paintedWidth NOTIFY paintedGeometryChanged)
    Q_PROPERTY(qreal paintedHeight READ paintedHeight NOTIFY paintedGeometryChanged)

public:
    QDeclarative1Image(QDeclarativeItem *parent=0);
    ~QDeclarative1Image();

    enum FillMode { Stretch, PreserveAspectFit, PreserveAspectCrop, Tile, TileVertically, TileHorizontally };
    FillMode fillMode() const;
    void setFillMode(FillMode);

    QPixmap pixmap() const;
    void setPixmap(const QPixmap &);

    qreal paintedWidth() const;
    qreal paintedHeight() const;

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
    QRectF boundingRect() const;

Q_SIGNALS:
    void fillModeChanged();
    void paintedGeometryChanged();

protected:
    QDeclarative1Image(QDeclarative1ImagePrivate &dd, QDeclarativeItem *parent);
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    void pixmapChange();
    void updatePaintedGeometry();

private:
    Q_DISABLE_COPY(QDeclarative1Image)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarative1Image)
};

QT_END_NAMESPACE
QML_DECLARE_TYPE(QDeclarative1Image)
QT_END_HEADER

#endif // QDECLARATIVEIMAGE_H
