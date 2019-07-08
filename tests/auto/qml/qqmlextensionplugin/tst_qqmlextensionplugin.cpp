/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    for (QDirIterator it(QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath), QDirIterator::Subdirectories); it.hasNext(); ) {
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
