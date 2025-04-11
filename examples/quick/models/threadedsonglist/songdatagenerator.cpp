// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "songdatagenerator.h"

#include <QStringLiteral>

static constexpr int s_albumCount{6};
static constexpr int s_bandNamePrefixSuffixCount{5};
static constexpr int s_songNamePrefixSuffixCount{6};
static constexpr int s_genreCount{4};

SongDataGenerator::SongDataGenerator()
{
    //classical music
    m_bandPrefixes << QStringList{QStringLiteral("Qtie's"),
                                  QStringLiteral("Kjuutburg"),
                                  QStringLiteral("Qtional"),
                                  QStringLiteral("Kjuutinese"),
                                  QStringLiteral("Qtian's")};
    m_bandSuffixes << QStringList{QStringLiteral("Philharmonic QOrchestra"),
                                  QStringLiteral("Theatre QOrchestra"),
                                  QStringLiteral("Opera QOrchestra"),
                                  QStringLiteral("City QChoir"),
                                  QStringLiteral("University QOrchestra")};
    // metal music
    m_bandPrefixes << QStringList{QStringLiteral("QtBlack"),
                                  QStringLiteral("QtDark"),
                                  QStringLiteral("QtHard"),
                                  QStringLiteral("QTest"),
                                  QStringLiteral("QtPale")};
    m_bandSuffixes << QStringList{QStringLiteral("Death"),
                                  QStringLiteral("Creature"),
                                  QStringLiteral("Devil"),
                                  QStringLiteral("Souls"),
                                  QStringLiteral("Being")};
    // pop music
    m_bandPrefixes << QStringList{QStringLiteral("Happy"),
                                  QStringLiteral("Joyful"),
                                  QStringLiteral("Pretty"),
                                  QStringLiteral("Crazy"),
                                  QStringLiteral("Wild")};
    m_bandSuffixes << QStringList{QStringLiteral("QPink"),
                                  QStringLiteral("QLife"),
                                  QStringLiteral("QGirls"),
                                  QStringLiteral("QBoys"),
                                  QStringLiteral("QMood")};
    // rock music
    m_bandPrefixes << QStringList{QStringLiteral("Grateful"),
                                  QStringLiteral("Hard"),
                                  QStringLiteral("Rolling"),
                                  QStringLiteral("Clashing"),
                                  QStringLiteral("Breaking")};
    m_bandSuffixes << QStringList{QStringLiteral("QML Thunder"),
                                  QStringLiteral("QML Slam"),
                                  QStringLiteral("QML Rocks"),
                                  QStringLiteral("QML Rally"),
                                  QStringLiteral("QML Tune")};

    // classical music
    m_songPrefixes << QStringList{"QWaltz", "QPrelude", "QDance", "QSonata", "QSymphony", "QPiano"};
    m_songSuffixes << QStringList{"in D Minor",
                                  "of the Pixies",
                                  "by Night",
                                  "on Violin",
                                  "no. 5",
                                  "in tune"};
    // metal music
    m_songPrefixes << QStringList{"QHunger", "QScreaming", "QPain", "QIllusion", "QBeauty", "QDream"};
    m_songSuffixes << QStringList{"That Never Ends",
                                  "Rising",
                                  "in My Soul",
                                  "from The Dark",
                                  "on the Night",
                                  "from Beyond"};
    // pop music
    m_songPrefixes << QStringList{"Jumping", "Running", "Playing", "Laughing", "Singing", "Joking"};
    m_songSuffixes << QStringList{"Qt Alone",
                                  "of Qt Life",
                                  "for Qt Fun",
                                  "Q-Day",
                                  "with Qties",
                                  "All the QTime"};
    // rock music
    m_songPrefixes << QStringList{"Full Steam", "Going", "Looking", "Always", "Crazy", "Drive Me"};
    m_songSuffixes << QStringList{"QWild",
                                  "Qt-wards",
                                  "to the QEnd",
                                  "for you, Qtie",
                                  "QTonight",
                                  "QHome"};
    Q_ASSERT(m_bandPrefixes.size() == s_genreCount);
    Q_ASSERT(m_bandSuffixes.size() == s_genreCount);
    Q_ASSERT(m_songPrefixes.size() == s_genreCount);
    Q_ASSERT(m_songSuffixes.size() == s_genreCount);
    for (const auto &prefixList : std::as_const(m_bandPrefixes)) {
        Q_ASSERT(prefixList.size() == s_bandNamePrefixSuffixCount);
    }
    for (const auto &suffixList : std::as_const(m_bandSuffixes)) {
        Q_ASSERT(suffixList.size() == s_bandNamePrefixSuffixCount);
    }
    for (const auto &prefixList : std::as_const(m_songPrefixes)) {
        Q_ASSERT(prefixList.size() == s_songNamePrefixSuffixCount);
    }
    for (const auto &suffixList : std::as_const(m_songSuffixes)) {
        Q_ASSERT(suffixList.size() == s_songNamePrefixSuffixCount);
    }
}

int SongDataGenerator::elementMaxCount()
{
    return s_bandNamePrefixSuffixCount * s_songNamePrefixSuffixCount * s_bandNamePrefixSuffixCount
           * s_songNamePrefixSuffixCount * s_genreCount;
}

QList<MediaElement> SongDataGenerator::createSongList() const
{
    QList<MediaElement> returnList;
    returnList.reserve(elementMaxCount());
    for (int bandSuffix = 0; bandSuffix < s_bandNamePrefixSuffixCount; ++bandSuffix) {
        for (int songSuffix = 0; songSuffix < s_songNamePrefixSuffixCount; ++songSuffix) {
            for (int bandPrefix = 0; bandPrefix < s_bandNamePrefixSuffixCount; ++bandPrefix) {
                for (int songPrefix = 0; songPrefix < s_songNamePrefixSuffixCount; ++songPrefix) {
                    for (int genre = 0; genre < s_genreCount; ++genre) {
                        returnList.append(
                            MediaElement{songName(songPrefix, songSuffix, genre),
                                         bandName(bandPrefix, bandSuffix, genre),
                                         SongDataGenerator::albumName(bandPrefix, songPrefix, genre),
                                         SongDataGenerator::albumArt(bandPrefix, songPrefix, genre)});
                    }
                }
            }
        }
    }
    Q_ASSERT(returnList.size() == elementMaxCount());
    return returnList;
}

QString SongDataGenerator::bandName(int bandNamePrefixIndex,
                                    int bandNameSuffixIndex,
                                    int genreIndex) const
{
    return QStringLiteral("%1 %2").arg(m_bandPrefixes.at(genreIndex).at(bandNamePrefixIndex),
                                       m_bandSuffixes.at(genreIndex).at(bandNameSuffixIndex));
}

QString SongDataGenerator::songName(int songNamePrefixIndex,
                                    int songNameSuffixIndex,
                                    int genreIndex) const
{
    return QStringLiteral("%1 %2").arg(m_songPrefixes.at(genreIndex).at(songNamePrefixIndex),
                                       m_songSuffixes.at(genreIndex).at(songNameSuffixIndex));
}

QString SongDataGenerator::albumName(int bandNamePrefixIndex,
                                     int songNamePrefixIndex,
                                     int genreIndex)
{
    static const QStringList albums{"First QAlbum",
                                    "The QBest Of",
                                    "World QtTour",
                                    "Live at Qt Arena",
                                    "QEnchanted",
                                    "QFarewell"};
    Q_ASSERT(albums.size() == s_albumCount);
    return albums.at(albumIndex(bandNamePrefixIndex, songNamePrefixIndex, genreIndex));
}

QUrl SongDataGenerator::albumArt(int bandNamePrefixIndex, int songNamePrefixIndex, int genreIndex)
{
    static const QStringList genres{"classical", "metal", "pop", "rock"};
    return QUrl{QStringLiteral("qrc:/qt/qml/threadedsonglist/images/album_covers/%1%2.jpeg")
                    .arg(genres.at(genreIndex))
                    .arg(albumIndex(bandNamePrefixIndex, songNamePrefixIndex, genreIndex) + 1)};
}

int SongDataGenerator::albumIndex(int bandNamePrefixIndex, int songNamePrefixIndex, int genreIndex)
{
    return ((bandNamePrefixIndex * s_songNamePrefixSuffixCount) + songNamePrefixIndex
            + (genreIndex % s_genreCount))
           % s_albumCount;
}
