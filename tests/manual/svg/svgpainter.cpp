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
{
#ifndef SVGWIDGET
    connect(this, SIGNAL(sourceChanged()), this, SLOT(update()));
#endif

    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
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

#ifndef SVGWIDGET
QSize SvgPainter::sizeHint() const
{
    return !m_source.isEmpty() ? m_size : QSize(1, 1);
}

void SvgPainter::paintEvent(QPaintEvent *)
{
    if (!m_source.isEmpty()) {
        QPainter p(this);
        p.fillRect(rect(), Qt::white);
        QSvgRenderer renderer(m_source.toLocalFile());

        renderer.setAspectRatioMode(Qt::KeepAspectRatio);
        renderer.render(&p);
        m_size = renderer.defaultSize();
        setMaximumSize(m_size);
        setMinimumSize(m_size);
    }
}
#endif
