/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QACCESSIBLEQUICKWIDGET_H
#define QACCESSIBLEQUICKWIDGET_H

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

#include "qquickwidget.h"
#include <QtWidgets/qaccessiblewidget.h>

#include <private/qaccessiblequickview_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

// These classes implement the QQuickWiget accessibility switcharoo,
// where the child items of the QQuickWidgetOffscreenWindow are reported
// as child accessible interfaces of the QAccessibleQuickWidget.
class QAccessibleQuickWidget: public QAccessibleWidget
{
public:
    QAccessibleQuickWidget(QQuickWidget* widget);

    QAccessibleInterface *child(int index) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *iface) const override;
    QAccessibleInterface *childAt(int x, int y) const override;

private:
    QAccessibleQuickWindow m_accessibleWindow;
    Q_DISABLE_COPY(QAccessibleQuickWidget)
};

class QAccessibleQuickWidgetOffscreenWindow: public QAccessibleQuickWindow
{
public:
    QAccessibleQuickWidgetOffscreenWindow(QQuickWindow *window);
    QAccessibleInterface *child(int index) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *iface) const override;
    QAccessibleInterface *childAt(int x, int y) const override;
};

#endif // accessibility

QT_END_NAMESPACE

#endif
