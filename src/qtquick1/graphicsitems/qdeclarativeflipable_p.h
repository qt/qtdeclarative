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

#ifndef QDECLARATIVEFLIPABLE_H
#define QDECLARATIVEFLIPABLE_H

#include "qdeclarativeitem.h"

#include <QtCore/QObject>
#include <QtGui/QTransform>
#include <QtGui/qvector3d.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QDeclarative1FlipablePrivate;
class Q_AUTOTEST_EXPORT QDeclarative1Flipable : public QDeclarativeItem
{
    Q_OBJECT

    Q_ENUMS(Side)
    Q_PROPERTY(QGraphicsObject *front READ front WRITE setFront NOTIFY frontChanged)
    Q_PROPERTY(QGraphicsObject *back READ back WRITE setBack NOTIFY backChanged)
    Q_PROPERTY(Side side READ side NOTIFY sideChanged)
    //### flipAxis
    //### flipRotation
public:
    QDeclarative1Flipable(QDeclarativeItem *parent=0);
    ~QDeclarative1Flipable();

    QGraphicsObject *front();
    void setFront(QGraphicsObject *);

    QGraphicsObject *back();
    void setBack(QGraphicsObject *);

    enum Side { Front, Back };
    Side side() const;

Q_SIGNALS:
    void frontChanged();
    void backChanged();
    void sideChanged();

private Q_SLOTS:
    void retransformBack();

private:
    Q_DISABLE_COPY(QDeclarative1Flipable)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QDeclarative1Flipable)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarative1Flipable)

QT_END_HEADER

#endif // QDECLARATIVEFLIPABLE_H
