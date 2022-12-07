// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SPLINEEDITOR_H
#define SPLINEEDITOR_H

#include <QWidget>
#include <QMenu>
#include <QAction>
#include <QScrollArea>

#include <QEasingCurve>
#include <QHash>

class SegmentProperties;

class SplineEditor : public QWidget
{
    Q_OBJECT

     Q_PROPERTY(QEasingCurve easingCurve READ easingCurve WRITE setEasingCurve NOTIFY easingCurveChanged);

public:
    explicit SplineEditor(QWidget *parent = nullptr);
    QString generateCode();
    QStringList presetNames() const;
    QWidget *pointListWidget();

    void setControlPoint(int index, const QPointF &point)
    {
        m_controlPoints[index] = point;
        update();
    }

    void setSmooth(int index, bool smooth)
    {
        m_smoothAction->setChecked(smooth);
        smoothPoint(index * 3 + 2);
        //update();
    }

Q_SIGNALS:
    void easingCurveChanged();
    void easingCurveCodeChanged(const QString &code);


public Q_SLOTS:
    void setEasingCurve(const QEasingCurve &easingCurve);
    void setPreset(const QString &name);
    void setEasingCurve(const QString &code);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
#if QT_CONFIG(contextmenu)
    void contextMenuEvent(QContextMenuEvent *) override;
#endif // contextmenu

    void invalidate();
    void invalidateSmoothList();
    void invalidateSegmentProperties();

    QEasingCurve easingCurve() const
    { return m_easingCurve; }

    QHash<QString, QEasingCurve> presets() const;

private:
    int findControlPoint(const QPoint &point);
    bool isSmooth(int i) const;

    void smoothPoint( int index);
    void cornerPoint( int index);
    void deletePoint(int index);
    void addPoint(const QPointF point);

    void initPresets();

    void setupPointListWidget();

    bool isControlPointSmooth(int i) const;

    QEasingCurve m_easingCurve;
    QVector<QPointF> m_controlPoints;
    QVector<bool> m_smoothList;
    int m_numberOfSegments;
    int m_activeControlPoint;
    bool m_mouseDrag;
    QPoint m_mousePress;
    QHash<QString, QEasingCurve> m_presets;

    QMenu *m_pointContextMenu;
    QMenu *m_curveContextMenu;
    QAction *m_deleteAction;
    QAction *m_smoothAction;
    QAction *m_cornerAction;
    QAction *m_addPoint;

    QScrollArea *m_pointListWidget;

    QList<SegmentProperties*> m_segmentProperties;
    bool m_block;
};

#endif // SPLINEEDITOR_H
