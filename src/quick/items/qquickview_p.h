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

#ifndef QQUICKVIEW_P_H
#define QQUICKVIEW_P_H

#include "qquickview.h"

#include <QtCore/qurl.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qtimer.h>
#include <QtCore/qpointer.h>
#include <QtCore/QWeakPointer>

#include <QtQml/qqmlengine.h>
#include "qquickwindow_p.h"

#include "qquickitemchangelistener_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQmlContext;
class QQmlError;
class QQuickItem;
class QQmlComponent;

class QQuickViewPrivate : public QQuickWindowPrivate,
                       public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickView)
public:
    static QQuickViewPrivate* get(QQuickView *view) { return view->d_func(); }
    static const QQuickViewPrivate* get(const QQuickView *view) { return view->d_func(); }

    QQuickViewPrivate();
    ~QQuickViewPrivate();

    void execute();
    void itemGeometryChanged(QQuickItem *item, const QRectF &newGeometry, const QRectF &oldGeometry);
    void initResize();
    void updateSize();
    void setRootObject(QObject *);

    void init(QQmlEngine* e = 0);

    QSize rootObjectSize() const;

    QPointer<QQuickItem> root;

    QUrl source;

    QPointer<QQmlEngine> engine;
    QQmlComponent *component;
    QBasicTimer resizetimer;

    QQuickView::ResizeMode resizeMode;
    QSize initialSize;
    QElapsedTimer frameTimer;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QQUICKVIEW_P_H
