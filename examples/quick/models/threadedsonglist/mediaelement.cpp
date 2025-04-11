// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mediaelement.h"

MediaElement::MediaElement() {}

MediaElement::MediaElement(QStringView songName,
                           QStringView artistName,
                           QStringView albumName,
                           const QUrl& albumArt)
    : m_isValid{true}
    , m_song{songName}
    , m_artist{artistName}
    , m_album{albumName}
    , m_albumArtUrl{albumArt}
{}

bool MediaElement::isValid() const
{
    return m_isValid;
}

QUrl MediaElement::albumArtFile() const
{
    return m_albumArtUrl;
}

QString MediaElement::song() const
{
    return m_song;
}

QString MediaElement::artist() const
{
    return m_artist;
}

QString MediaElement::album() const
{
    return m_album;
}
