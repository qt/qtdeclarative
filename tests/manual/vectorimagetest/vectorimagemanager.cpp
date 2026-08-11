// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "vectorimagemanager.h"

#include <QDir>
#include <QtQuickVectorImageGenerator/private/qquickqmlgenerator_p.h>
#include <QTemporaryFile>

VectorImageManager *VectorImageManager::g_manager = nullptr;

VectorImageManager::VectorImageManager(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(g_manager == nullptr);
    g_manager = this;
    connect(this, &VectorImageManager::currentIndexChanged, this, &VectorImageManager::currentSourceChanged, Qt::QueuedConnection);
}

VectorImageManager::~VectorImageManager()
{
    Q_ASSERT(g_manager == this);
    g_manager = nullptr;
}

void VectorImageManager::setCurrentIndex(int newCurrentIndex)
{
    if (m_currentIndex == newCurrentIndex)
        return;
    m_currentIndex = newCurrentIndex;
    emit currentIndexChanged();
}

QList<QUrl> VectorImageManager::sources() const
{
    return m_sources;
}

QString VectorImageManager::currentDirectory() const
{
    return m_currentDirectory;
}

void VectorImageManager::setCurrentDirectory(const QString &newCurrentDirectory)
{
    if (m_currentDirectory == newCurrentDirectory)
        return;
    m_currentDirectory = newCurrentDirectory;
    emit currentDirectoryChanged();

    m_sources.clear();
    if (!m_currentDirectory.isEmpty()) {
        QDir dir(m_currentDirectory);
        QList<QFileInfo> infos = dir.entryInfoList(QStringList() << QStringLiteral("*.svg") << QStringLiteral("*.svgz") << QStringLiteral("*.json"));

        for (const QFileInfo &info : infos) {
            if (info.isFile())
                m_sources.append(QUrl::fromLocalFile(info.absoluteFilePath()));
        }
    }

    m_currentIndex = m_sources.isEmpty() ? -1 : 0;
    emit sourcesChanged();
    emit currentIndexChanged();
}

QString VectorImageManager::qmlSource() const
{
    QTemporaryFile tempFile;
    if (tempFile.open()) {
        QString name = tempFile.fileName();
        {
            QQuickQmlGenerator generator(QQuickVectorImageSource(currentSource().toLocalFile()),
                                         QQuickVectorImageGenerator::CurveRenderer,
                                         tempFile.fileName());
            generator.setCommentString(QStringLiteral("Generated"));
            generator.generate();
            generator.save();
        }
        tempFile.close();

        QFile file(name);
        if (file.open(QIODevice::ReadOnly))
            return QString::fromUtf8(file.readAll());

    }

    return QStringLiteral("import QtQuick\nRectangle { width: 100; height: 100; color: \"red\" }");;
}

qreal VectorImageManager::scale() const
{
    return m_scale;
}

void VectorImageManager::setScale(int newScale)
{
    if (qFuzzyCompare(m_scale, newScale))
        return;
    m_scale = newScale;
    emit scaleChanged();
}
