// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef METALSQUIRCLE_H
#define METALSQUIRCLE_H

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

class SquircleRenderer;

class MetalSquircle : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(qreal t READ t WRITE setT NOTIFY tChanged)
    QML_ELEMENT

public:
    MetalSquircle();

    qreal t() const { return m_t; }
    void setT(qreal t);

signals:
    void tChanged();

public slots:
    void sync();
    void cleanup();

private slots:
    void handleWindowChanged(QQuickWindow *win);

private:
    void releaseResources() override;

    qreal m_t;
    SquircleRenderer *m_renderer = nullptr;
};

#endif
