// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
    Q_PROPERTY(QStringList strokeColors READ strokeColors NOTIFY pathsChanged)
    Q_PROPERTY(QStringList strokeWidths READ strokeWidths NOTIFY pathsChanged)
    Q_PROPERTY(QStringList transforms READ transforms NOTIFY pathsChanged)
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

    QStringList strokeColors() const
    {
        return m_strokeColors;
    }

    QStringList strokeWidths() const
    {
        return m_strokeWidths;
    }

    QStringList transforms() const
    {
        return m_transforms;
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
    QStringList m_strokeColors;
    QStringList m_strokeWidths;
    QStringList m_transforms;
};

#endif // SVGPATHLOADER_H
