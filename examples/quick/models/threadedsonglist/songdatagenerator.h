// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SONGDATAGENERATOR_H
#define SONGDATAGENERATOR_H
#include "mediaelement.h"

#include <QPair>
#include <QStringList>

class SongDataGenerator
{
public:
    explicit SongDataGenerator();
    static int elementMaxCount();
    QList<MediaElement> createSongList() const;

private:
    QString bandName(int bandNamePrefixIndex, int bandNameSuffixIndex, int genreIndex) const;
    QString songName(int songNamePrefixIndex, int songNameSuffixIndex, int genreIndex) const;
    static QString albumName(int bandNamePrefixIndex, int songNamePrefixIndex, int genreIndex);
    static QUrl albumArt(int bandNamePrefixIndex, int songNamePrefixIndex, int genreIndex);
    static int albumIndex(int bandNamePrefixIndex, int songNamePrefixIndex, int genreIndex);
    QList<QStringList> m_bandPrefixes;
    QList<QStringList> m_bandSuffixes;
    QList<QStringList> m_songPrefixes;
    QList<QStringList> m_songSuffixes;
};

#endif // SONGDATAGENERATOR_H
