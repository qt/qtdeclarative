// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MEDIAELEMENT_H
#define MEDIAELEMENT_H

#include <QUrl>

class MediaElement
{
public:
    explicit MediaElement();
    MediaElement(QStringView songName,
                 QStringView artistName,
                 QStringView albumName,
                 const QUrl &albumArt);
    bool isValid() const;
    QUrl albumArtFile() const;
    QString song() const;
    QString artist() const;
    QString album() const;

private:
    bool m_isValid{false};
    QString m_song;
    QString m_artist;
    QString m_album;
    QUrl m_albumArtUrl;
};

#endif // MEDIAELEMENT_H
