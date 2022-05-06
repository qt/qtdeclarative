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

#ifndef QQUICKTOOLSEPARATOR_P_H
#define QQUICKTOOLSEPARATOR_P_H

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

class QQuickToolSeparatorPrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickToolSeparator : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged FINAL)
    Q_PROPERTY(bool horizontal READ isHorizontal NOTIFY orientationChanged FINAL)
    Q_PROPERTY(bool vertical READ isVertical NOTIFY orientationChanged FINAL)
    QML_NAMED_ELEMENT(ToolSeparator)
    QML_ADDED_IN_VERSION(2, 1)

public:
    explicit QQuickToolSeparator(QQuickItem *parent = nullptr);

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);

    bool isHorizontal() const;
    bool isVertical() const;

Q_SIGNALS:
    void orientationChanged();

protected:
    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickToolSeparator)
    Q_DECLARE_PRIVATE(QQuickToolSeparator)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickToolSeparator)

#endif // QQUICKTOOLSEPARATOR_P_H
