// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SVGPAINTER_H
#define SVGPAINTER_H

#include <QWidget>
#include <QUrl>
#include <QtSvgWidgets/QSvgWidget>
#include <QtSvg>

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
    Q_PROPERTY(bool looping READ looping WRITE setLooping NOTIFY loopingChanged)
    Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY playingChanged)
public:
    explicit SvgPainter(QWidget *parent = nullptr);

    QUrl source() const;
    void setSource(const QUrl &newSource);

    qreal scale() const;
    void setScale(const qreal scale);

    bool looping() const
    {
        return m_looping;
    }

    void setLooping(bool looping)
    {
        if (m_looping == looping)
            return;
        m_looping = looping;
        emit loopingChanged();
    }

    bool playing() const
    {
        return m_playing;
    }

    void setPlaying(bool playing)
    {
        if (m_playing == playing)
            return;
        m_playing = playing;
        update();
        emit playingChanged();
    }

signals:
    void sourceChanged();
    void scaleChanged();
    void loopingChanged();
    void playingChanged();

protected:
#ifndef SVGWIDGET
    QSize sizeHint() const override;
#endif
    void paintEvent(QPaintEvent *) override;


private:
    QUrl m_source;
    QSize m_size;
    qreal m_scale;
    bool m_looping = false;
    bool m_playing = false;
#ifndef SVGWIDGET
    QSvgRenderer m_renderer;
#endif
};

#endif // SVGPAINTER_H
