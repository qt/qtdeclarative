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

#ifndef QQUICKCHECKDELEGATE_P_H
#define QQUICKCHECKDELEGATE_P_H

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

#include <QtQuickTemplates2/private/qquickitemdelegate_p.h>

QT_BEGIN_NAMESPACE

class QQuickCheckDelegatePrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickCheckDelegate : public QQuickItemDelegate
{
    Q_OBJECT
    Q_PROPERTY(bool tristate READ isTristate WRITE setTristate NOTIFY tristateChanged FINAL)
    Q_PROPERTY(Qt::CheckState checkState READ checkState WRITE setCheckState NOTIFY checkStateChanged FINAL)
    // 2.4 (Qt 5.11)
    Q_PRIVATE_PROPERTY(QQuickCheckDelegate::d_func(), QJSValue nextCheckState MEMBER nextCheckState WRITE setNextCheckState NOTIFY nextCheckStateChanged FINAL REVISION(2, 4))
    QML_NAMED_ELEMENT(CheckDelegate)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickCheckDelegate(QQuickItem *parent = nullptr);

    bool isTristate() const;
    void setTristate(bool tristate);

    Qt::CheckState checkState() const;
    void setCheckState(Qt::CheckState state);

Q_SIGNALS:
    void tristateChanged();
    void checkStateChanged();
    // 2.4 (Qt 5.11)
    Q_REVISION(2, 4) void nextCheckStateChanged();

protected:
    QFont defaultFont() const override;

    void buttonChange(ButtonChange change) override;
    void nextCheckState() override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickCheckDelegate)
    Q_DECLARE_PRIVATE(QQuickCheckDelegate)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickCheckDelegate)

#endif // QQUICKCHECKDELEGATE_P_H
