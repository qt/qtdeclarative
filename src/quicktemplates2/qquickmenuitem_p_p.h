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

#ifndef QQUICKMENUITEM_P_P_H
#define QQUICKMENUITEM_P_P_H

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

#include <QtQuickTemplates2/private/qquickmenuitem_p.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickMenu;

class QQuickMenuItemPrivate : public QQuickAbstractButtonPrivate
{
    Q_DECLARE_PUBLIC(QQuickMenuItem)

public:
    static QQuickMenuItemPrivate *get(QQuickMenuItem *item)
    {
        return item->d_func();
    }

    void setMenu(QQuickMenu *menu);
    void setSubMenu(QQuickMenu *subMenu);

    void updateEnabled();

    void cancelArrow();
    void executeArrow(bool complete = false);

    bool acceptKeyClick(Qt::Key key) const override;

    QPalette defaultPalette() const override;

    bool highlighted = false;
    QQuickDeferredPointer<QQuickItem> arrow;
    QQuickMenu *menu = nullptr;
    QQuickMenu *subMenu = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKMENUITEM_P_P_H
