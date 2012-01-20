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

#include "qdeclarativepathinterpolator_p.h"

#include "qdeclarativepath_p.h"

QT_BEGIN_NAMESPACE

QDeclarativePathInterpolator::QDeclarativePathInterpolator(QObject *parent) :
    QObject(parent), _path(0), _x(0), _y(0), _angle(0), _progress(0)
{
}

QDeclarativePath *QDeclarativePathInterpolator::path() const
{
    return _path;
}

void QDeclarativePathInterpolator::setPath(QDeclarativePath *path)
{
    if (_path == path)
        return;
    if (_path)
        disconnect(_path, SIGNAL(changed()), this, SLOT(_q_pathUpdated()));
    _path = path;
    connect(_path, SIGNAL(changed()), this, SLOT(_q_pathUpdated()));
    emit pathChanged();
}

qreal QDeclarativePathInterpolator::progress() const
{
    return _progress;
}

void QDeclarativePathInterpolator::setProgress(qreal progress)
{
    if (progress == _progress)
        return;
    _progress = progress;
    emit progressChanged();
    _q_pathUpdated();
}

qreal QDeclarativePathInterpolator::x() const
{
    return _x;
}

qreal QDeclarativePathInterpolator::y() const
{
    return _y;
}

qreal QDeclarativePathInterpolator::angle() const
{
    return _angle;
}

void QDeclarativePathInterpolator::_q_pathUpdated()
{
    if (! _path)
        return;

    qreal angle = 0;
    const QPointF pt = _path->sequentialPointAt(_progress, &angle);

    if (_x != pt.x()) {
        _x = pt.x();
        emit xChanged();
    }

    if (_y != pt.y()) {
        _y = pt.y();
        emit yChanged();
    }

    if (angle != _angle) {
        _angle = angle;
        emit angleChanged();
    }
}

QT_END_NAMESPACE
