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

#ifndef QQUICKSWITCH_P_H
#define QQUICKSWITCH_P_H

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

class QQuickSwitchPrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickSwitch : public QQuickAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(qreal position READ position WRITE setPosition NOTIFY positionChanged FINAL)
    Q_PROPERTY(qreal visualPosition READ visualPosition NOTIFY visualPositionChanged FINAL)
    QML_NAMED_ELEMENT(Switch)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickSwitch(QQuickItem *parent = nullptr);

    qreal position() const;
    void setPosition(qreal position);

    qreal visualPosition() const;

Q_SIGNALS:
    void positionChanged();
    void visualPositionChanged();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
#if QT_CONFIG(quicktemplates2_multitouch)
    void touchEvent(QTouchEvent *event) override;
#endif

    void mirrorChange() override;

    void nextCheckState() override;
    void buttonChange(ButtonChange change) override;

    QFont defaultFont() const override;

private:
    Q_DISABLE_COPY(QQuickSwitch)
    Q_DECLARE_PRIVATE(QQuickSwitch)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickSwitch)

#endif // QQUICKSWITCH_P_H
