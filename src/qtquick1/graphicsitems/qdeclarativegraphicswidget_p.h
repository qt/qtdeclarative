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

#ifndef QDECLARATIVEGRAPHICSWIDGET_P_H
#define QDECLARATIVEGRAPHICSWIDGET_P_H

#include <QObject>
#include <QtDeclarative/qdeclarativecomponent.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QGraphicsObject;
class QDeclarative1AnchorLine;
class QDeclarative1Anchors;
class QDeclarative1GraphicsWidgetPrivate;

// ### TODO can the extension object be the anchor directly? We save one allocation -> awesome.
class QDeclarative1GraphicsWidget : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDeclarative1Anchors * anchors READ anchors DESIGNABLE false CONSTANT FINAL)
    Q_PROPERTY(QDeclarative1AnchorLine left READ left CONSTANT FINAL)
    Q_PROPERTY(QDeclarative1AnchorLine right READ right CONSTANT FINAL)
    Q_PROPERTY(QDeclarative1AnchorLine horizontalCenter READ horizontalCenter CONSTANT FINAL)
    Q_PROPERTY(QDeclarative1AnchorLine top READ top CONSTANT FINAL)
    Q_PROPERTY(QDeclarative1AnchorLine bottom READ bottom CONSTANT FINAL)
    Q_PROPERTY(QDeclarative1AnchorLine verticalCenter READ verticalCenter CONSTANT FINAL)
    // ### TODO : QGraphicsWidget don't have a baseline concept yet.
    //Q_PROPERTY(QDeclarative1AnchorLine baseline READ baseline CONSTANT FINAL)
public:
    QDeclarative1GraphicsWidget(QObject *parent = 0);
    ~QDeclarative1GraphicsWidget();
    QDeclarative1Anchors *anchors();
    QDeclarative1AnchorLine left() const;
    QDeclarative1AnchorLine right() const;
    QDeclarative1AnchorLine horizontalCenter() const;
    QDeclarative1AnchorLine top() const;
    QDeclarative1AnchorLine bottom() const;
    QDeclarative1AnchorLine verticalCenter() const;
    Q_DISABLE_COPY(QDeclarative1GraphicsWidget)
    Q_DECLARE_PRIVATE(QDeclarative1GraphicsWidget)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDECLARATIVEGRAPHICSWIDGET_P_H
