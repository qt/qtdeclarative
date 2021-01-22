/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQUICKVIEW_P_H
#define QQUICKVIEW_P_H

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

#include "qquickview.h"

#include <QtCore/qurl.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qtimer.h>
#include <QtCore/qpointer.h>
#include <QtCore/QWeakPointer>

#include <QtQml/qqmlengine.h>
#include <private/qv4object_p.h>
#include "qquickwindow_p.h"

#include "qquickitemchangelistener_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {
struct ExecutionEngine;
}

class QQmlContext;
class QQmlError;
class QQuickItem;
class QQmlComponent;

class Q_QUICK_PRIVATE_EXPORT QQuickViewPrivate : public QQuickWindowPrivate,
                       public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickView)
public:
    static QQuickViewPrivate* get(QQuickView *view) { return view->d_func(); }
    static const QQuickViewPrivate* get(const QQuickView *view) { return view->d_func(); }

    QQuickViewPrivate();
    ~QQuickViewPrivate();

    void execute();
    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &) override;
    void initResize();
    void updateSize();
    bool setRootObject(QObject *);

    void init(QQmlEngine* e = nullptr);

    QSize rootObjectSize() const;

    QPointer<QQuickItem> root;

    QUrl source;

    QPointer<QQmlEngine> engine;
    QQmlComponent *component;
    QBasicTimer resizetimer;

    QQuickView::ResizeMode resizeMode;
    QSize initialSize;
    QElapsedTimer frameTimer;

    QVariantMap initialProperties;
};

QT_END_NAMESPACE

#endif // QQUICKVIEW_P_H
