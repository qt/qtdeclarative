/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QDECLARATIVESCALEGRID_H
#define QDECLARATIVESCALEGRID_H

#include <QtDeclarative/qdeclarative.h>

#include <QtCore/QString>
#include <QtCore/QObject>

#include <QtQuick1/private/qdeclarativeborderimage_p.h>
#include <QtQuick1/private/qdeclarativepixmapcache_p.h>
#include <QtDeclarative/private/qdeclarativeglobal_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class Q_QTQUICK1_EXPORT QDeclarative1ScaleGrid : public QObject
{
    Q_OBJECT
    Q_ENUMS(TileRule)

    Q_PROPERTY(int left READ left WRITE setLeft NOTIFY borderChanged)
    Q_PROPERTY(int top READ top WRITE setTop NOTIFY borderChanged)
    Q_PROPERTY(int right READ right WRITE setRight NOTIFY borderChanged)
    Q_PROPERTY(int bottom READ bottom WRITE setBottom NOTIFY borderChanged)

public:
    QDeclarative1ScaleGrid(QObject *parent=0);
    ~QDeclarative1ScaleGrid();

    bool isNull() const;

    int left() const { return _left; }
    void setLeft(int);

    int top() const { return _top; }
    void setTop(int);

    int right() const { return _right; }
    void setRight(int);

    int  bottom() const { return _bottom; }
    void setBottom(int);

Q_SIGNALS:
    void borderChanged();

private:
    int _left;
    int _top;
    int _right;
    int _bottom;
};

class Q_QTQUICK1_EXPORT QDeclarative1GridScaledImage
{
public:
    QDeclarative1GridScaledImage();
    QDeclarative1GridScaledImage(const QDeclarative1GridScaledImage &);
    QDeclarative1GridScaledImage(QIODevice*);
    QDeclarative1GridScaledImage &operator=(const QDeclarative1GridScaledImage &);
    bool isValid() const;
    int gridLeft() const;
    int gridRight() const;
    int gridTop() const;
    int gridBottom() const;
    QDeclarative1BorderImage::TileMode horizontalTileRule() const { return _h; }
    QDeclarative1BorderImage::TileMode verticalTileRule() const { return _v; }

    QString pixmapUrl() const;

private:
    static QDeclarative1BorderImage::TileMode stringToRule(const QString &);

private:
    int _l;
    int _r;
    int _t;
    int _b;
    QDeclarative1BorderImage::TileMode _h;
    QDeclarative1BorderImage::TileMode _v;
    QString _pix;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarative1ScaleGrid)

QT_END_HEADER

#endif // QDECLARATIVESCALEGRID_H
