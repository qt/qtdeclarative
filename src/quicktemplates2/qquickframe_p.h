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

#ifndef QQUICKFRAME_P_H
#define QQUICKFRAME_P_H

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

#include <QtQuickTemplates2/private/qquickpane_p.h>

QT_BEGIN_NAMESPACE

class QQuickFramePrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickFrame : public QQuickPane
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Frame)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickFrame(QQuickItem *parent = nullptr);

protected:
    QQuickFrame(QQuickFramePrivate &dd, QQuickItem *parent);

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickFrame)
    Q_DECLARE_PRIVATE(QQuickFrame)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickFrame)

#endif // QQUICKFRAME_P_H
