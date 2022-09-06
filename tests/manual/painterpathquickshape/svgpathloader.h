// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SVGPATHLOADER_H
#define SVGPATHLOADER_H

#include <QObject>
#include <QUrl>
#include <QPainterPath>
#include <QDebug>

class SvgPathLoader : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QStringList paths READ paths NOTIFY pathsChanged)
    Q_PROPERTY(QStringList fillColors READ fillColors NOTIFY pathsChanged)
public:
    SvgPathLoader();

    QUrl source() const
    {
        return m_source;
    }
    void setSource(const QUrl &url)
    {
        if (url == m_source)
            return;
        m_source = url;
        qDebug() << "Set source" << url;
        emit sourceChanged();
    }

    QStringList paths() const
    {
        return m_paths;
    }

    QStringList fillColors() const
    {
        return m_fillColors;
    }

private slots:
    void loadPaths();

signals:
    void sourceChanged();
    void pathsChanged();

private:
    QUrl m_source;
    QStringList m_paths;
    QStringList m_fillColors;
};

#endif // SVGPATHLOADER_H
