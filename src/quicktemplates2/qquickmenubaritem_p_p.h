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

#ifndef QQUICKMENUBARITEM_P_P_H
#define QQUICKMENUBARITEM_P_P_H

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

#include <QtQuickTemplates2/private/qquickmenubaritem_p.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickMenu;
class QQuickMenuBar;

class QQuickMenuBarItemPrivate : public QQuickAbstractButtonPrivate
{
    Q_DECLARE_PUBLIC(QQuickMenuBarItem)

public:
    static QQuickMenuBarItemPrivate *get(QQuickMenuBarItem *item)
    {
        return item->d_func();
    }

    void setMenuBar(QQuickMenuBar *menuBar);

    QPalette defaultPalette() const override;

    bool highlighted = false;
    QQuickMenu *menu = nullptr;
    QQuickMenuBar *menuBar = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKMENUBARITEM_P_P_H
