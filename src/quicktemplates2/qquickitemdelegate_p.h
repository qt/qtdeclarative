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

#ifndef QQUICKITEMDELEGATE_P_H
#define QQUICKITEMDELEGATE_P_H

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

class QQuickItemDelegatePrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickItemDelegate : public QQuickAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(bool highlighted READ isHighlighted WRITE setHighlighted NOTIFY highlightedChanged FINAL)
    QML_NAMED_ELEMENT(ItemDelegate)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickItemDelegate(QQuickItem *parent = nullptr);

    bool isHighlighted() const;
    void setHighlighted(bool highlighted);

Q_SIGNALS:
    void highlightedChanged();

protected:
    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

protected:
    QQuickItemDelegate(QQuickItemDelegatePrivate &dd, QQuickItem *parent);

private:
    Q_DISABLE_COPY(QQuickItemDelegate)
    Q_DECLARE_PRIVATE(QQuickItemDelegate)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickItemDelegate)

#endif // QQUICKITEMDELEGATE_P_H
