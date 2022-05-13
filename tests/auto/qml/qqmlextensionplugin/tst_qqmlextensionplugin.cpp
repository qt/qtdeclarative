// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore>
#include <QtTest>
#include <QtQml>

#if defined(Q_OS_WIN)
# define SUFFIX QLatin1String(".dll")
# define DEBUG_SUFFIX QLatin1String("d.dll")

#elif defined(Q_OS_DARWIN)
# define SUFFIX QLatin1String(".dylib")
# define DEBUG_SUFFIX QLatin1String("_debug.dylib")

# else  // Unix
# define SUFFIX QLatin1String(".so")
#endif


class tst_qqmlextensionplugin : public QObject
{
    Q_OBJECT

    static QStringList removeDuplicates(QStringList files) {
#ifdef DEBUG_SUFFIX
        const auto isDuplicate = [files] (QString file) {
# ifdef QT_DEBUG
            return !file.endsWith(DEBUG_SUFFIX) && files.contains(file.replace(SUFFIX, DEBUG_SUFFIX));
# else
            return file.endsWith(DEBUG_SUFFIX) && files.contains(file.replace(DEBUG_SUFFIX, SUFFIX));
# endif
        };

        files.erase(std::remove_if(files.begin(), files.end(), isDuplicate),
                    files.end());

#endif
        return files;
    }

public:
    tst_qqmlextensionplugin() {}

private Q_SLOTS:
    void iidCheck_data();
    void iidCheck();
};


void tst_qqmlextensionplugin::iidCheck_data()
{
    QList<QString> files;
    // On Android the plugins are located in the APK's libs subdir. They can
    // be distinguished by the name, which starts from "libqml_" and ends with
    // "plugin_${ARCH}.so"
#ifdef Q_OS_ANDROID
    const QStringList libraryPaths = QCoreApplication::libraryPaths();
    QVERIFY(!libraryPaths.isEmpty());
    const QLatin1String nameFilters("libqml_*plugin_" ANDROID_ARCH "*");
    for (QDirIterator it(libraryPaths.front(), { nameFilters }, QDir::Files); it.hasNext(); ) {
#else
    for (QDirIterator it(QLibraryInfo::path(QLibraryInfo::QmlImportsPath), QDirIterator::Subdirectories); it.hasNext(); ) {
#endif
        QString file = it.next();
#if defined(Q_OS_DARWIN)
        if (file.contains(QLatin1String(".dSYM/")))
            continue;
#endif
        if (file.endsWith(SUFFIX)) {
            files << file;
        }
    }

    files = removeDuplicates(std::move(files));

    QTest::addColumn<QString>("filePath");
    foreach (const QString &file, files) {
        QFileInfo fileInfo(file);
        QTest::newRow(fileInfo.baseName().toLatin1().data()) << fileInfo.absoluteFilePath();
    }
}


void tst_qqmlextensionplugin::iidCheck()
{
    QFETCH(QString, filePath);

    QPluginLoader loader(filePath);
    QVERIFY2(loader.load(), qPrintable(loader.errorString()));
    QVERIFY(loader.instance() != nullptr);

    if (qobject_cast<QQmlExtensionPlugin *>(loader.instance())) {
        QString iid = loader.metaData().value(QStringLiteral("IID")).toString();
        if (iid == QLatin1String(QQmlExtensionInterface_iid_old))
            qWarning() << "Old extension plugin found. Update the IID" << loader.metaData();
        else
            QCOMPARE(iid, QLatin1String(QQmlExtensionInterface_iid));
    }
}


QTEST_APPLESS_MAIN(tst_qqmlextensionplugin)
#include "tst_qqmlextensionplugin.moc"
