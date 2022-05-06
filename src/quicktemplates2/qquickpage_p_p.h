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

#ifndef QQUICKPAGE_P_P_H
#define QQUICKPAGE_P_P_H

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

#include <QtQuickTemplates2/private/qquickpane_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickPane;

class QQuickPagePrivate : public QQuickPanePrivate
{
    Q_DECLARE_PUBLIC(QQuickPage)

public:
    void relayout();
    void resizeContent() override;

    void itemVisibilityChanged(QQuickItem *item) override;
    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF & diff) override;
    void itemDestroyed(QQuickItem *item) override;

    QString title;
    QQuickItem *header = nullptr;
    QQuickItem *footer = nullptr;
    bool emittingImplicitSizeChangedSignals = false;
};

QT_END_NAMESPACE

#endif // QQUICKPAGE_P_P_H
