/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
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

#ifndef QWIDGETPLATFORMMENU_P_H
#define QWIDGETPLATFORMMENU_P_H

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

#include <QtGui/qpa/qplatformmenu.h>

QT_BEGIN_NAMESPACE

class QMenu;
class QWidgetPlatformMenuItem;

class QWidgetPlatformMenu : public QPlatformMenu
{
    Q_OBJECT

public:
    explicit QWidgetPlatformMenu(QObject *parent = nullptr);
    ~QWidgetPlatformMenu();

    QMenu *menu() const;

    void insertMenuItem(QPlatformMenuItem *item, QPlatformMenuItem *before) override;
    void removeMenuItem(QPlatformMenuItem *item) override;
    void syncMenuItem(QPlatformMenuItem *item) override;
    void syncSeparatorsCollapsible(bool enable) override;

    void setText(const QString &text) override;
    void setIcon(const QIcon &icon) override;
    void setEnabled(bool enabled) override;
    bool isEnabled() const override;
    void setVisible(bool visible) override;
    void setMinimumWidth(int width) override;
    void setFont(const QFont &font) override;
    void setMenuType(MenuType type) override;

    void showPopup(const QWindow *window, const QRect &targetRect, const QPlatformMenuItem *item) override;
    void dismiss() override;

    QPlatformMenuItem *menuItemAt(int position) const override;
    QPlatformMenuItem *menuItemForTag(quintptr tag) const override;

    QPlatformMenuItem *createMenuItem() const override;
    QPlatformMenu *createSubMenu() const override;

private:
    QScopedPointer<QMenu> m_menu;
    QList<QWidgetPlatformMenuItem *> m_items;
};

QT_END_NAMESPACE

#endif // QWIDGETPLATFORMMENU_P_H
