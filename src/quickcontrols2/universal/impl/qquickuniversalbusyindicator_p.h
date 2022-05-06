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

#ifndef QQUICKUNIVERSALBUSYINDICATOR_P_H
#define QQUICKUNIVERSALBUSYINDICATOR_P_H

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

#include <QtGui/qcolor.h>
#include <QtQuick/qquickitem.h>

QT_BEGIN_NAMESPACE

class QQuickUniversalBusyIndicator : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(int count READ count WRITE setCount FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor FINAL)
    QML_NAMED_ELEMENT(BusyIndicatorImpl)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickUniversalBusyIndicator(QQuickItem *parent = nullptr);

    int count() const;
    void setCount(int count);

    QColor color() const;
    void setColor(const QColor &color);

    int elapsed() const;

protected:
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private:
    int m_count = 5;
    int m_elapsed = 0;
    QColor m_color = Qt::black;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickUniversalBusyIndicator)

#endif // QQUICKUNIVERSALBUSYINDICATOR_P_H
