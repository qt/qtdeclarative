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

#ifndef QDECLARATIVEPATHINTERPOLATOR_P_H
#define QDECLARATIVEPATHINTERPOLATOR_P_H

#include <qdeclarative.h>
#include <QObject>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QDeclarativePath;
class Q_AUTOTEST_EXPORT QDeclarativePathInterpolator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDeclarativePath *path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged)
    Q_PROPERTY(qreal x READ x NOTIFY xChanged)
    Q_PROPERTY(qreal y READ y NOTIFY yChanged)
    Q_PROPERTY(qreal angle READ angle NOTIFY angleChanged)
public:
    explicit QDeclarativePathInterpolator(QObject *parent = 0);

    QDeclarativePath *path() const;
    void setPath(QDeclarativePath *path);

    qreal progress() const;
    void setProgress(qreal progress);

    qreal x() const;
    qreal y() const;
    qreal angle() const;

Q_SIGNALS:
    void pathChanged();
    void progressChanged();
    void xChanged();
    void yChanged();
    void angleChanged();

private Q_SLOTS:
    void _q_pathUpdated();

private:
    QDeclarativePath *_path;
    qreal _x;
    qreal _y;
    qreal _angle;
    qreal _progress;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativePathInterpolator)

QT_END_HEADER

#endif // QDECLARATIVEPATHINTERPOLATOR_P_H
