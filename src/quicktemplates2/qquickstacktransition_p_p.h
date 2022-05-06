/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQUICKSTACKTRANSITION_P_P_H
#define QQUICKSTACKTRANSITION_P_P_H

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

#include <QtQuickTemplates2/private/qquickstackview_p.h>
#include <QtQuick/private/qquickitemviewtransition_p.h>

QT_BEGIN_NAMESPACE

class QQuickStackElement;

struct QQuickStackTransition
{
    static QQuickStackTransition popExit(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view);
    static QQuickStackTransition popEnter(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view);

    static QQuickStackTransition pushExit(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view);
    static QQuickStackTransition pushEnter(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view);

    static QQuickStackTransition replaceExit(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view);
    static QQuickStackTransition replaceEnter(QQuickStackView::Operation operation, QQuickStackElement *element, QQuickStackView *view);

    bool target = false;
    QQuickStackView::Status status = QQuickStackView::Inactive;
    QQuickItemViewTransitioner::TransitionType type = QQuickItemViewTransitioner::NoTransition;
    QRectF viewBounds;
    QQuickStackElement *element = nullptr;
    QQuickTransition *transition = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKSTACKTRANSITION_P_P_H
