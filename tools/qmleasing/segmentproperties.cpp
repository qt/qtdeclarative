// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "segmentproperties.h"
#include "splineeditor.h"

SegmentProperties::SegmentProperties(QWidget *parent) :
    QWidget(parent), m_splineEditor(nullptr), m_blockSignals(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(QMargins());
    layout->setSpacing(2);
    setLayout(layout);
    {
        QWidget *widget = new QWidget(this);
        m_ui_pane_c1.setupUi(widget);
        m_ui_pane_c1.label->setText("c1");
        m_ui_pane_c1.smooth->setVisible(false);
        layout->addWidget(widget);

        connect(m_ui_pane_c1.p1_x, &QDoubleSpinBox::valueChanged, this, &SegmentProperties::c1Updated);
        connect(m_ui_pane_c1.p1_y, &QDoubleSpinBox::valueChanged, this, &SegmentProperties::c1Updated);
    }
    {
        QWidget *widget = new QWidget(this);
        m_ui_pane_c2.setupUi(widget);
        m_ui_pane_c2.label->setText("c2");
        m_ui_pane_c2.smooth->setVisible(false);
        layout->addWidget(widget);

        connect(m_ui_pane_c2.p1_x, &QDoubleSpinBox::valueChanged, this, &SegmentProperties::c2Updated);
        connect(m_ui_pane_c2.p1_y, &QDoubleSpinBox::valueChanged, this, &SegmentProperties::c2Updated);
    }
    {
        QWidget *widget = new QWidget(this);
        m_ui_pane_p.setupUi(widget);
        m_ui_pane_p.label->setText("p1");
        layout->addWidget(widget);

        connect(m_ui_pane_p.smooth, &QCheckBox::toggled, this, &SegmentProperties::pUpdated);
        connect(m_ui_pane_p.p1_x, &QDoubleSpinBox::valueChanged, this, &SegmentProperties::pUpdated);
        connect(m_ui_pane_p.p1_y, &QDoubleSpinBox::valueChanged, this, &SegmentProperties::pUpdated);
    }
}

void SegmentProperties::c1Updated()
{
    if (m_splineEditor && !m_blockSignals) {
        QPointF c1(m_ui_pane_c1.p1_x->value(), m_ui_pane_c1.p1_y->value());
        m_splineEditor->setControlPoint(m_segment * 3, c1);
    }
}

void SegmentProperties::c2Updated()
{
    if (m_splineEditor && !m_blockSignals) {
        QPointF c2(m_ui_pane_c2.p1_x->value(), m_ui_pane_c2.p1_y->value());
        m_splineEditor->setControlPoint(m_segment * 3 + 1, c2);
    }
}

void SegmentProperties::pUpdated()
{
    if (m_splineEditor && !m_blockSignals) {
        QPointF p(m_ui_pane_p.p1_x->value(), m_ui_pane_p.p1_y->value());
        bool smooth = m_ui_pane_p.smooth->isChecked();
        m_splineEditor->setControlPoint(m_segment * 3 + 2, p);
        m_splineEditor->setSmooth(m_segment, smooth);
    }
}

void SegmentProperties::invalidate()
{
    m_blockSignals = true;

     m_ui_pane_p.label->setText(QLatin1Char('p') + QString::number(m_segment));
     m_ui_pane_p.smooth->setChecked(m_smooth);
     m_ui_pane_p.smooth->parentWidget()->setEnabled(!m_last);

     m_ui_pane_c1.p1_x->setValue(m_points.at(0).x());
     m_ui_pane_c1.p1_y->setValue(m_points.at(0).y());

     m_ui_pane_c2.p1_x->setValue(m_points.at(1).x());
     m_ui_pane_c2.p1_y->setValue(m_points.at(1).y());

     m_ui_pane_p.p1_x->setValue(m_points.at(2).x());
     m_ui_pane_p.p1_y->setValue(m_points.at(2).y());

     m_blockSignals = false;
}

#include "moc_segmentproperties.cpp"
