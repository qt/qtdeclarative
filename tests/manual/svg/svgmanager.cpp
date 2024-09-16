// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "svgmanager.h"

#include <QDir>
#include <QtQuickVectorImageGenerator/private/qquickqmlgenerator_p.h>
#include <QTemporaryFile>

SvgManager *SvgManager::g_manager = nullptr;

SvgManager::SvgManager(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(g_manager == nullptr);
    g_manager = this;
    connect(this, &SvgManager::currentIndexChanged, this, &SvgManager::currentSourceChanged, Qt::QueuedConnection);
}

SvgManager::~SvgManager()
{
    Q_ASSERT(g_manager == this);
    g_manager = nullptr;
}

void SvgManager::setCurrentIndex(int newCurrentIndex)
{
    if (m_currentIndex == newCurrentIndex)
        return;
    m_currentIndex = newCurrentIndex;
    emit currentIndexChanged();
}

QList<QUrl> SvgManager::sources() const
{
    return m_sources;
}

QString SvgManager::currentDirectory() const
{
    return m_currentDirectory;
}

void SvgManager::setCurrentDirectory(const QString &newCurrentDirectory)
{
    if (m_currentDirectory == newCurrentDirectory)
        return;
    m_currentDirectory = newCurrentDirectory;
    emit currentDirectoryChanged();

    m_sources.clear();
    if (!m_currentDirectory.isEmpty()) {
        QDir dir(m_currentDirectory);
        QList<QFileInfo> infos = dir.entryInfoList(QStringList() << QStringLiteral("*.svg"));

        for (const QFileInfo &info : infos)
            m_sources.append(QUrl::fromLocalFile(info.absoluteFilePath()));
    }
    m_currentIndex = m_sources.isEmpty() ? -1 : 0;
    emit sourcesChanged();
    emit currentIndexChanged();
}

QString SvgManager::qmlSource() const
{
    QTemporaryFile tempFile;
    if (tempFile.open()) {
        QString name = tempFile.fileName();
        {
            QQuickQmlGenerator generator(currentSource().toLocalFile(), QQuickVectorImageGenerator::CurveRenderer, tempFile.fileName());
            generator.setCommentString(QStringLiteral("Generated"));
            generator.generate();
        }
        tempFile.close();

        QFile file(name);
        if (file.open(QIODevice::ReadOnly))
            return QString::fromUtf8(file.readAll());

    }

    return QStringLiteral("import QtQuick\nRectangle { width: 100; height: 100; color: \"red\" }");;
}

qreal SvgManager::scale() const
{
    return m_scale;
}

void SvgManager::setScale(int newScale)
{
    if (qFuzzyCompare(m_scale, newScale))
        return;
    m_scale = newScale;
    emit scaleChanged();
}
