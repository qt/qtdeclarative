// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "remotemedia.h"
#include "songdatagenerator.h"

#include <QThread>

static constexpr int s_fetchMilliSecondsPerSong{100};

RemoteMedia::RemoteMedia()
{
    SongDataGenerator dataGenerator;
    m_items = dataGenerator.createSongList();
}

int RemoteMedia::count()
{
    return SongDataGenerator::elementMaxCount();
}

QList<MediaElement> RemoteMedia::getElements(int amount, int offset) const
{
    if (amount < 1)
        return {};
    QList<MediaElement> returnValue{m_items.cbegin() + offset, m_items.cbegin() + offset + amount};
    QThread::sleep(std::chrono::milliseconds{s_fetchMilliSecondsPerSong * amount});
    return returnValue;
}
