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
public:
    explicit SvgPainter(QWidget *parent = nullptr);

    QUrl source() const;
    void setSource(const QUrl &newSource);


signals:
    void sourceChanged();

#ifndef SVGWIDGET
protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *) override;
#endif

private:
    QUrl m_source;
    QSize m_size;
};

#endif // SVGPAINTER_H
