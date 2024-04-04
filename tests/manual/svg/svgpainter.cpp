// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "svgpainter.h"
#include "svgmanager.h"

#include <QtSvg>

SvgPainter::SvgPainter(QWidget *parent)
#ifdef SVGWIDGET
    : QSvgWidget{parent}
#else
    : QWidget{parent}
#endif
    , m_scale(10)
{
#ifndef SVGWIDGET
    connect(this, SIGNAL(sourceChanged()), this, SLOT(update()));
#endif

    connect(this, SIGNAL(scaleChanged()), this, SLOT(update()));
}

QUrl SvgPainter::source() const
{
    return m_source;
}

void SvgPainter::setSource(const QUrl &newSource)
{
    if (m_source == newSource)
        return;
    m_source = newSource;
#ifdef SVGWIDGET
    load(m_source.toLocalFile());
#endif
    emit sourceChanged();
}

qreal SvgPainter::scale() const
{
    return m_scale;
}

void SvgPainter::setScale(const qreal scale)
{
    if (m_scale == scale)
        return;
    m_scale = scale;
    emit scaleChanged();
}

#ifndef SVGWIDGET
QSize SvgPainter::sizeHint() const
{
    return !m_source.isEmpty() ? m_size * m_scale / 10.0 : QSize(1, 1);
}
#endif

void SvgPainter::paintEvent(QPaintEvent *event)
{
#ifndef SVGWIDGET
    Q_UNUSED(event)
    if (!m_source.isEmpty()) {
        QPainter p(this);
        p.fillRect(rect(), Qt::white);
        QSvgRenderer renderer(m_source.toLocalFile());

        renderer.setAspectRatioMode(Qt::KeepAspectRatio);
        renderer.render(&p);
        m_size = renderer.defaultSize();
        setFixedSize(m_size * m_scale / 10.0);
    }
#else
    m_size = renderer()->defaultSize();
    setFixedSize(m_size * m_scale / 10.0);
    QSvgWidget::paintEvent(event);
#endif

}

