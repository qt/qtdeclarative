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

#ifndef QQUICKBUSYINDICATOR_P_H
#define QQUICKBUSYINDICATOR_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickBusyIndicatorPrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickBusyIndicator : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged FINAL)
    QML_NAMED_ELEMENT(BusyIndicator)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickBusyIndicator(QQuickItem *parent = nullptr);

    bool isRunning() const;
    void setRunning(bool running);

Q_SIGNALS:
    void runningChanged();

protected:
#if QT_CONFIG(quicktemplates2_multitouch)
    void touchEvent(QTouchEvent *event) override;
#endif

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickBusyIndicator)
    Q_DECLARE_PRIVATE(QQuickBusyIndicator)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickBusyIndicator)

#endif // QQUICKBUSYINDICATOR_P_H
