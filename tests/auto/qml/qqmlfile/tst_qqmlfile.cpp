// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore>
#include <QtTest>
#include <QQmlFile>

class tst_qqmlfile : public QObject
{
    Q_OBJECT

public:
    tst_qqmlfile() {}

private Q_SLOTS:
    void isLocalFile_data();
    void isLocalFile();

    void urlToLocalFileOrQrcOverloads_data();
    void urlToLocalFileOrQrcOverloads();

private:
    void urlData();
};


void tst_qqmlfile::urlData()
{
    QTest::addColumn<QString>("urlString");
    QTest::addColumn<bool>("isLocal");
    QTest::addColumn<QString>("localPath");

    const QString invalid;
    const QString relative = QStringLiteral("foo/bar");
    const QString absolute = QStringLiteral("/foo/bar");

    QTest::addRow("plain empty")     << QStringLiteral("") << false << invalid;
    QTest::addRow("plain no slash")  << QStringLiteral("foo/bar") << false << invalid;
    QTest::addRow("plain 1 slash")   << QStringLiteral("/foo/bar") << false << invalid;
    QTest::addRow("plain 2 slashes") << QStringLiteral("//foo/bar") << false << invalid;
    QTest::addRow("plain 3 slashes") << QStringLiteral("///foo/bar") << false << invalid;

    QTest::addRow(": empty")     << QStringLiteral(":") << false << invalid;
    QTest::addRow(": no slash")  << QStringLiteral(":foo/bar") << false << invalid;
    QTest::addRow(": 1 slash")   << QStringLiteral(":/foo/bar") << false << invalid;
    QTest::addRow(": 2 slashes") << QStringLiteral("://foo/bar") << false << invalid;
    QTest::addRow(": 3 slashes") << QStringLiteral(":///foo/bar") << false << invalid;

    QTest::addRow("C empty")     << QStringLiteral("C:") << false << invalid;
    QTest::addRow("C no slash")  << QStringLiteral("C:foo/bar") << false << invalid;
    QTest::addRow("C 1 slash")   << QStringLiteral("C:/foo/bar") << false << invalid;
    QTest::addRow("C 2 slashes") << QStringLiteral("C://foo/bar") << false << invalid;
    QTest::addRow("C 3 slashes") << QStringLiteral("C:///foo/bar") << false << invalid;

    QTest::addRow("file empty")     << QStringLiteral("file:") << true << QString();
    QTest::addRow("file no slash")  << QStringLiteral("file:foo/bar") << true << relative;
    QTest::addRow("file 1 slash")   << QStringLiteral("file:/foo/bar") << true << absolute;
    QTest::addRow("file 2 slashes") << QStringLiteral("file://foo/bar") << true << QStringLiteral("//foo/bar");
    QTest::addRow("file 3 slashes") << QStringLiteral("file:///foo/bar") << true << absolute;

    QTest::addRow("qrc empty")     << QStringLiteral("qrc:") << true << QStringLiteral(":");
    QTest::addRow("qrc no slash")  << QStringLiteral("qrc:foo/bar") << true << u':' + relative;
    QTest::addRow("qrc 1 slash")   << QStringLiteral("qrc:/foo/bar") << true << u':' + absolute;
    QTest::addRow("qrc 2 slashes") << QStringLiteral("qrc://foo/bar") << false << invalid;
    QTest::addRow("qrc 3 slashes") << QStringLiteral("qrc:///foo/bar") << true << u':' + absolute;

    QTest::addRow("file+stuff empty")     << QStringLiteral("file+stuff:") << false << invalid;
    QTest::addRow("file+stuff no slash")  << QStringLiteral("file+stuff:foo/bar") << false << invalid;
    QTest::addRow("file+stuff 1 slash")   << QStringLiteral("file+stuff:/foo/bar") << false << invalid;
    QTest::addRow("file+stuff 2 slashes") << QStringLiteral("file+stuff://foo/bar") << false << invalid;
    QTest::addRow("file+stuff 3 slashes") << QStringLiteral("file+stuff:///foo/bar") << false << invalid;

    // "assets:" and "content:" URLs are only treated as local files on android. In contrast to
    // "qrc:" and "file:" we're not trying to be clever about multiple slashes. Two slashes are
    // prohibited as that says part of what we would recognize as path is actually a URL authority.
    // Everything else is android's problem.

#ifdef Q_OS_ANDROID
    const bool hasAssetsAndContent = true;
#else
    const bool hasAssetsAndContent = false;
#endif

    const QString assetsEmpty = hasAssetsAndContent ? QStringLiteral("assets:") : invalid;
    const QString assetsRelative = hasAssetsAndContent ? (QStringLiteral("assets:") + relative) : invalid;
    const QString assetsAbsolute = hasAssetsAndContent ? (QStringLiteral("assets:") + absolute) : invalid;
    const QString assetsThreeSlashes = hasAssetsAndContent ? (QStringLiteral("assets://") + absolute) : invalid;

    QTest::addRow("assets empty")     << QStringLiteral("assets:") << hasAssetsAndContent << assetsEmpty;
    QTest::addRow("assets no slash")  << QStringLiteral("assets:foo/bar") << hasAssetsAndContent << assetsRelative;
    QTest::addRow("assets 1 slash")   << QStringLiteral("assets:/foo/bar") << hasAssetsAndContent << assetsAbsolute;
    QTest::addRow("assets 2 slashes") << QStringLiteral("assets://foo/bar") << false << invalid;
    QTest::addRow("assets 3 slashes") << QStringLiteral("assets:///foo/bar") << hasAssetsAndContent << assetsThreeSlashes;

    const QString contentEmpty = hasAssetsAndContent ? QStringLiteral("content:") : invalid;
    const QString contentRelative = hasAssetsAndContent ? (QStringLiteral("content:") + relative) : invalid;
    const QString contentAbsolute = hasAssetsAndContent ? (QStringLiteral("content:") + absolute) : invalid;
    const QString contentThreeSlashes = hasAssetsAndContent ? (QStringLiteral("content://") + absolute) : invalid;

    QTest::addRow("content empty")     << QStringLiteral("content:") << hasAssetsAndContent << contentEmpty;
    QTest::addRow("content no slash")  << QStringLiteral("content:foo/bar") << hasAssetsAndContent << contentRelative;
    QTest::addRow("content 1 slash")   << QStringLiteral("content:/foo/bar") << hasAssetsAndContent << contentAbsolute;
    QTest::addRow("content 2 slashes") << QStringLiteral("content://foo/bar") << false << invalid;
    QTest::addRow("content 3 slashes") << QStringLiteral("content:///foo/bar") << hasAssetsAndContent << contentThreeSlashes;


    // These are local files everywhere. Their paths are only meaningful on android, though.
    // The inner slashes of the path do not influence the URL parsing.

    QTest::addRow("file:assets empty")     << QStringLiteral("file:assets:") << true << QStringLiteral("assets:");
    QTest::addRow("file:assets no slash")  << QStringLiteral("file:assets:foo/bar") << true << QStringLiteral("assets:foo/bar");
    QTest::addRow("file:assets 1 slash")   << QStringLiteral("file:assets:/foo/bar") << true << QStringLiteral("assets:/foo/bar");
    QTest::addRow("file:assets 2 slashes") << QStringLiteral("file:assets://foo/bar") << true << QStringLiteral("assets://foo/bar");
    QTest::addRow("file:assets 3 slashes") << QStringLiteral("file:assets:///foo/bar") << true << QStringLiteral("assets:///foo/bar");

    QTest::addRow("file:content empty")     << QStringLiteral("file:content:") << true << QStringLiteral("content:");
    QTest::addRow("file:content no slash")  << QStringLiteral("file:content:foo/bar") << true << QStringLiteral("content:foo/bar");
    QTest::addRow("file:content 1 slash")   << QStringLiteral("file:content:/foo/bar") << true << QStringLiteral("content:/foo/bar");
    QTest::addRow("file:content 2 slashes") << QStringLiteral("file:content://foo/bar") << true << QStringLiteral("content://foo/bar");
    QTest::addRow("file:content 3 slashes") << QStringLiteral("file:content:///foo/bar") << true << QStringLiteral("content:///foo/bar");

    const QString contentExternalstoragePath = hasAssetsAndContent ?
                QStringLiteral("content://com.android.externalstorage.documents/foo") : invalid;
    const QString contentDownloadsPath = hasAssetsAndContent ?
                QStringLiteral("content://com.android.providers.downloads.documents/foo") : invalid;
    const QString contentMediaPath = hasAssetsAndContent ?
                QStringLiteral("content://com.android.providers.media.documents") : invalid;

    QTest::addRow("content externalstorage")     << QStringLiteral("content://com.android.externalstorage.documents/foo")
                                                 << hasAssetsAndContent << contentExternalstoragePath;
    QTest::addRow("content downloads documents") << QStringLiteral("content://com.android.providers.downloads.documents/foo")
                                                 << hasAssetsAndContent << contentDownloadsPath;
    QTest::addRow("content media documents")     << QStringLiteral("content://com.android.providers.media.documents")
                                                 << hasAssetsAndContent << contentMediaPath;

    QTest::addRow("assets externalstorage")      << QStringLiteral("assets://com.android.externalstorage.documents/foo")
                                                 << false << invalid;
    QTest::addRow("assets downloads documents")  << QStringLiteral("assets://com.android.providers.downloads.documents/foo")
                                                 << false << invalid;
    QTest::addRow("assets media documents")      << QStringLiteral("assets://com.android.providers.media.documents")
                                                 << false << invalid;
}

void tst_qqmlfile::isLocalFile_data()
{
    urlData();
}

void tst_qqmlfile::isLocalFile()
{
    QFETCH(QString, urlString);
    QFETCH(bool, isLocal);

    const QUrl url(urlString);

    QCOMPARE(QQmlFile::isLocalFile(urlString), isLocal);
    QCOMPARE(QQmlFile::isLocalFile(url), isLocal);
}

void tst_qqmlfile::urlToLocalFileOrQrcOverloads_data()
{
    urlData();
}

void tst_qqmlfile::urlToLocalFileOrQrcOverloads()
{
    QFETCH(QString, urlString);
    QFETCH(QString, localPath);

    const QUrl url(urlString);
    const QString pathForUrlString = QQmlFile::urlToLocalFileOrQrc(urlString);
    const QString pathForUrl = QQmlFile::urlToLocalFileOrQrc(url);

    QCOMPARE(pathForUrlString, localPath);
    QCOMPARE(pathForUrl, localPath);
}

QTEST_GUILESS_MAIN(tst_qqmlfile)

#include "tst_qqmlfile.moc"
