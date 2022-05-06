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

#ifndef QQUICKDEFAULTBUSYINDICATOR_P_H
#define QQUICKDEFAULTBUSYINDICATOR_P_H

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

#include <QtQuick/qquickitem.h>
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class QQuickBasicBusyIndicator : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor pen READ pen WRITE setPen FINAL)
    Q_PROPERTY(QColor fill READ fill WRITE setFill FINAL)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning)
    QML_NAMED_ELEMENT(BusyIndicatorImpl)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickBasicBusyIndicator(QQuickItem *parent = nullptr);

    QColor pen() const;
    void setPen(const QColor &pen);

    QColor fill() const;
    void setFill(const QColor &fill);

    bool isRunning() const;
    void setRunning(bool running);

    int elapsed() const;

protected:
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private:
    int m_elapsed = 0;
    QColor m_pen;
    QColor m_fill;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickBasicBusyIndicator)

#endif // QQUICKDEFAULTBUSYINDICATOR_P_H
