// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef VECTORIMAGEMANAGER_H
#define VECTORIMAGEMANAGER_H

#include <QObject>
#include <QUrl>
#include <QQmlEngine>

class VectorImageManager : public QObject
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
    Q_PROPERTY(bool looping READ looping WRITE setLooping NOTIFY loopingChanged)
    Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(qreal currentTime READ currentTime WRITE setCurrentTime NOTIFY currentTimeChanged)
public:
    VectorImageManager(QObject *parent);
    ~VectorImageManager() override;

    static VectorImageManager *create(QQmlEngine *, QJSEngine *)
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

    bool looping() const
    {
        return m_looping;
    }

    bool playing() const
    {
        return m_playing;
    }

    void setPlaying(bool on)
    {
        if (m_playing == on)
            return;

        m_playing = on;
        emit playingChanged(on);
    }

    void seek(qreal pos)
    {
        emit seekRequested(pos);
    }

    qreal currentTime() const
    {
        return m_currentTime;
    }

public slots:
    void setScale(int newScale);
    void setLooping(bool looping)
    {
        if (m_looping == looping)
            return;

        m_looping = looping;
        emit loopingChanged();
    }

    void setCurrentTime(qreal time)
    {
        if (qFuzzyCompare(time, m_currentTime))
            return;

        m_currentTime = time;
        emit currentTimeChanged(time);
    }

signals:
    void currentSourceChanged();
    void currentIndexChanged();

    void sourcesChanged();

    void currentDirectoryChanged();

    void scaleChanged();
    void loopingChanged();

    void playingChanged(bool play);
    void seekRequested(qreal time);
    void currentTimeChanged(qreal time);

private:
    static VectorImageManager *g_manager;
    int m_currentIndex = -1;
    QList<QUrl> m_sources;
    QString m_currentDirectory;
    QString m_qmlSource;
    qreal m_scale = 10.0;
    bool m_looping = false;
    bool m_playing = false;
    qreal m_currentTime = 0.0;
};

#endif // VECTORIMAGEMANAGER_H
