// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BEZIERCURVE_H
#define BEZIERCURVE_H

//! [1]
#include <QtQuick/QQuickItem>

class BezierCurve : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QPointF p1 READ p1 WRITE setP1 NOTIFY p1Changed)
    Q_PROPERTY(QPointF p2 READ p2 WRITE setP2 NOTIFY p2Changed)
    Q_PROPERTY(QPointF p3 READ p3 WRITE setP3 NOTIFY p3Changed)
    Q_PROPERTY(QPointF p4 READ p4 WRITE setP4 NOTIFY p4Changed)

    Q_PROPERTY(int segmentCount READ segmentCount WRITE setSegmentCount NOTIFY segmentCountChanged)
//! [3]
    QML_ELEMENT
//! [3]

public:
    BezierCurve(QQuickItem *parent = nullptr);
    ~BezierCurve();

//! [2]
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
//! [2]

    QPointF p1() const { return m_p1; }
    QPointF p2() const { return m_p2; }
    QPointF p3() const { return m_p3; }
    QPointF p4() const { return m_p4; }

    int segmentCount() const { return m_segmentCount; }

    void setP1(const QPointF &p);
    void setP2(const QPointF &p);
    void setP3(const QPointF &p);
    void setP4(const QPointF &p);

    void setSegmentCount(int count);

signals:
    void p1Changed(const QPointF &p);
    void p2Changed(const QPointF &p);
    void p3Changed(const QPointF &p);
    void p4Changed(const QPointF &p);

    void segmentCountChanged(int count);

private:
    QPointF m_p1;
    QPointF m_p2;
    QPointF m_p3;
    QPointF m_p4;

    int m_segmentCount;
};
//! [1]

#endif

