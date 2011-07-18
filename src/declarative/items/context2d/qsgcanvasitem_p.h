/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QSGCANVASITEM_P_H
#define QSGCANVASITEM_P_H

#include "qsgpainteditem.h"
#include <private/qv8engine_p.h>

#define QSGCANVASITEM_DEBUG //enable this for just DEBUG purpose!

#ifdef QSGCANVASITEM_DEBUG
#include <QElapsedTimer>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)
class QSGContext2D;
class QSGCanvasItemPrivate;
class QSGCanvasItem : public QSGPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QPointF canvasPos READ canvasPos FINAL)
    Q_PROPERTY(qreal canvasX READ canvasX WRITE setCanvasX NOTIFY canvasXChanged FINAL)
    Q_PROPERTY(qreal canvasY READ canvasY WRITE setCanvasY NOTIFY canvasYChanged FINAL)
public:
    QSGCanvasItem(QSGItem *parent = 0);
    ~QSGCanvasItem();
    void setCanvasX(qreal x);
    void setCanvasY(qreal y);
    qreal canvasX() const;
    qreal canvasY() const;
    QPointF canvasPos() const;
Q_SIGNALS:
    void painted();
    void paint(QDeclarativeV8Handle context, const QRect &region);
    void canvasXChanged();
    void canvasYChanged();
public Q_SLOTS:
    QString toDataURL(const QString& type = QLatin1String("image/png")) const;
    QDeclarativeV8Handle getContext(const QString & = QLatin1String("2d"));
    void requestPaint(const QRect& region = QRect());

    // Save current canvas to disk
    bool save(const QString& filename) const;

protected:
    void updatePolish();
    void paint(QPainter *painter);
    virtual void componentComplete();
private:
    void createContext();
    Q_DECLARE_PRIVATE(QSGCanvasItem)
    friend class QSGContext2D;
};
QT_END_NAMESPACE

QML_DECLARE_TYPE(QSGCanvasItem)

QT_END_HEADER

#endif //QSGCANVASITEM_P_H
