// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MASKEDMOUSEAREA_H
#define MASKEDMOUSEAREA_H

#include <QImage>
#include <QQuickItem>


class MaskedMouseArea : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)
    Q_PROPERTY(bool containsMouse READ containsMouse NOTIFY containsMouseChanged)
    Q_PROPERTY(QUrl maskSource READ maskSource WRITE setMaskSource NOTIFY maskSourceChanged)
    Q_PROPERTY(qreal alphaThreshold READ alphaThreshold WRITE setAlphaThreshold NOTIFY alphaThresholdChanged)
    QML_ELEMENT

public:
    MaskedMouseArea(QQuickItem *parent = nullptr);

    bool contains(const QPointF &point) const override;

    bool isPressed() const { return m_pressed; }
    bool containsMouse() const { return m_containsMouse; }

    QUrl maskSource() const { return m_maskSource; }
    void setMaskSource(const QUrl &source);

    qreal alphaThreshold() const { return m_alphaThreshold; }
    void setAlphaThreshold(qreal threshold);

signals:
    void pressed();
    void released();
    void clicked();
    void canceled();
    void pressedChanged();
    void maskSourceChanged();
    void containsMouseChanged();
    void alphaThresholdChanged();

protected:
    void setPressed(bool pressed);
    void setContainsMouse(bool containsMouse);
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void mouseUngrabEvent() override;

private:
    bool m_pressed;
    QUrl m_maskSource;
    QImage m_maskImage;
    QPointF m_pressPoint;
    qreal m_alphaThreshold;
    bool m_containsMouse;
};

#endif
