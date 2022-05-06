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

#ifndef QQUICKMENUBARITEM_P_H
#define QQUICKMENUBARITEM_P_H

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

#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickMenu;
class QQuickMenuBar;
class QQuickMenuBarItemPrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickMenuBarItem : public QQuickAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(QQuickMenuBar *menuBar READ menuBar NOTIFY menuBarChanged FINAL)
    Q_PROPERTY(QQuickMenu *menu READ menu WRITE setMenu NOTIFY menuChanged FINAL)
    Q_PROPERTY(bool highlighted READ isHighlighted WRITE setHighlighted NOTIFY highlightedChanged FINAL)
    QML_NAMED_ELEMENT(MenuBarItem)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickMenuBarItem(QQuickItem *parent = nullptr);

    QQuickMenuBar *menuBar() const;

    QQuickMenu *menu() const;
    void setMenu(QQuickMenu *menu);

    bool isHighlighted() const;
    void setHighlighted(bool highlighted);

Q_SIGNALS:
    void triggered();
    void menuBarChanged();
    void menuChanged();
    void highlightedChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickMenuBarItem)
    Q_DECLARE_PRIVATE(QQuickMenuBarItem)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickMenuBarItem)

#endif // QQUICKMENUBARITEM_P_H
