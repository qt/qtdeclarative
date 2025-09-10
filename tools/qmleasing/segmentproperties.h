// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SEGMENTPROPERTIES_H
#define SEGMENTPROPERTIES_H

#include <QWidget>
#include <ui_pane.h>

class SplineEditor;

class SegmentProperties : public QWidget
{
    Q_OBJECT
public:
    explicit SegmentProperties(QWidget *parent = nullptr);
    void setSplineEditor(SplineEditor *splineEditor)
    {
        m_splineEditor = splineEditor;
    }

    void setSegment(int segment, QVector<QPointF> points, bool smooth, bool last)
    {
        m_segment = segment;
        m_points = points;
        m_smooth = smooth;
        m_last = last;
        invalidate();
    }

private Q_SLOTS:
    void c1Updated();
    void c2Updated();
    void pUpdated();

private:
    void invalidate();

    Ui_Pane m_ui_pane_c1;
    Ui_Pane m_ui_pane_c2;
    Ui_Pane m_ui_pane_p;

    SplineEditor *m_splineEditor;
    QVector<QPointF> m_points;
    int m_segment;
    bool m_smooth;
    bool m_last;

    bool m_blockSignals;
};

#endif // SEGMENTPROPERTIES_H
