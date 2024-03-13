// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SVGMANAGER_H
#define SVGMANAGER_H

#include <QObject>
#include <QUrl>
#include <QQmlEngine>

class SvgManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QUrl currentSource READ currentSource NOTIFY currentSourceChanged)
    Q_PROPERTY(QString qmlSource READ qmlSource NOTIFY currentSourceChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int sourceCount READ sourceCount NOTIFY sourcesChanged)
    Q_PROPERTY(QString currentDirectory READ currentDirectory WRITE setCurrentDirectory NOTIFY currentDirectoryChanged)
    Q_PROPERTY(QList<QUrl> sources READ sources NOTIFY sourcesChanged)
    Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY scaleChanged)
public:
    SvgManager(QObject *parent);
    ~SvgManager() override;

    static SvgManager *create(QQmlEngine *, QJSEngine *)
    {
        return g_manager;
    }

    QUrl currentSource() const
    {
        if (m_currentIndex < 0)
            return QUrl{};

        return m_sources.at(m_currentIndex);
    }

    int currentIndex() const
    {
        return m_currentIndex;
    }
    void setCurrentIndex(int newCurrentIndex);

    int sourceCount() const
    {
        return m_sources.size();
    }

    QList<QUrl> sources() const;

    QString currentDirectory() const;
    void setCurrentDirectory(const QString &newCurrentDirectory);

    QString qmlSource() const;

    qreal scale() const;

public slots:
    void setScale(int newScale);

signals:
    void currentSourceChanged();
    void currentIndexChanged();

    void sourcesChanged();

    void currentDirectoryChanged();

    void scaleChanged();

private:
    static SvgManager *g_manager;
    int m_currentIndex = -1;
    QList<QUrl> m_sources;
    QString m_currentDirectory;
    QString m_qmlSource;
    qreal m_scale = 10.0;
};

#endif // SVGMANAGER_H
