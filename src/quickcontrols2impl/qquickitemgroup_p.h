/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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

#ifndef QQUICKITEMGROUP_P_H
#define QQUICKITEMGROUP_P_H

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

#include <QtQuick/private/qquickimplicitsizeitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2_PRIVATE_EXPORT QQuickItemGroup : public QQuickImplicitSizeItem, protected QQuickItemChangeListener
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ItemGroup)
    QML_ADDED_IN_VERSION(2, 2)

public:
    explicit QQuickItemGroup(QQuickItem *parent = nullptr);
    ~QQuickItemGroup();

protected:
    void watch(QQuickItem *item);
    void unwatch(QQuickItem *item);

    QSizeF calculateImplicitSize() const;
    void updateImplicitSize();

    void itemChange(ItemChange change, const ItemChangeData &data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickItemGroup)

#endif // QQUICKITEMGROUP_P_H
