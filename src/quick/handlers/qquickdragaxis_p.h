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

#ifndef QQUICKDRAGAXIS_P_H
#define QQUICKDRAGAXIS_P_H

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

#include <QtQml/qqml.h>
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QQuickDragAxis : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
    Q_PROPERTY(qreal maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    QML_NAMED_ELEMENT(DragAxis)
    QML_ADDED_IN_MINOR_VERSION(12)
    QML_UNCREATABLE("DragAxis is only available as a grouped property of DragHandler.")

public:
    QQuickDragAxis();

    qreal minimum() const { return m_minimum; }
    void setMinimum(qreal minimum);

    qreal maximum() const { return m_maximum; }
    void setMaximum(qreal maximum);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

signals:
    void minimumChanged();
    void maximumChanged();
    void enabledChanged();

private:
    qreal m_minimum;
    qreal m_maximum;
    bool m_enabled;
};

QT_END_NAMESPACE

#endif // QQUICKDRAGAXIS_P_H
