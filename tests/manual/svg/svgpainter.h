// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SVGPAINTER_H
#define SVGPAINTER_H

#include <QWidget>
#include <QUrl>
#include <QtSvgWidgets/QSvgWidget>

// #define SVGWIDGET

#ifdef SVGWIDGET
class SvgPainter : public QSvgWidget
#else
class SvgPainter : public QWidget
#endif
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY scaleChanged)
public:
    explicit SvgPainter(QWidget *parent = nullptr);

    QUrl source() const;
    void setSource(const QUrl &newSource);

    qreal scale() const;
    void setScale(const qreal scale);

signals:
    void sourceChanged();
    void scaleChanged();


protected:
#ifndef SVGWIDGET
    QSize sizeHint() const override;
#endif
    void paintEvent(QPaintEvent *) override;


private:
    QUrl m_source;
    QSize m_size;
    qreal m_scale;
};

#endif // SVGPAINTER_H
