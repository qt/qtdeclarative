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

#ifndef QQUICKPANE_P_P_H
#define QQUICKPANE_P_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickPane;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickPanePrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickPane)

public:
    void init();

    virtual QQmlListProperty<QObject> contentData();
    virtual QQmlListProperty<QQuickItem> contentChildren();
    virtual QList<QQuickItem *> contentChildItems() const;

    QQuickItem *getContentItem() override;

    qreal getContentWidth() const override;
    qreal getContentHeight() const override;

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;

    void contentChildrenChange();

    void updateContentWidth();
    void updateContentHeight();

    bool hasContentWidth = false;
    bool hasContentHeight = false;
    qreal contentWidth = 0;
    qreal contentHeight = 0;
    QQuickItem *firstChild = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKPANE_P_P_H
