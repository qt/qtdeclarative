/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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
**
**
**
****************************************************************************/

#ifndef QQUICKPONTHANDLER_H
#define QQUICKPONTHANDLER_H

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

#include "qquicksinglepointhandler_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QQuickPointHandler : public QQuickSinglePointHandler
{
    Q_OBJECT
    Q_PROPERTY(QVector2D translation READ translation NOTIFY translationChanged)
    QML_NAMED_ELEMENT(PointHandler)
    QML_ADDED_IN_MINOR_VERSION(12)

public:
    explicit QQuickPointHandler(QQuickItem *parent = nullptr);

    QVector2D translation() const;

Q_SIGNALS:
    void translationChanged();

protected:
    bool wantsEventPoint(QQuickEventPoint *pt) override;
    void handleEventPoint(QQuickEventPoint *point) override;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPointHandler)

#endif // QQUICKPONTHANDLER_H
