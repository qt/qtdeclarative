/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QLibraryInfo>
#include <QDir>
#include <QProcess>
#include <QDebug>
#include <QDeclarativeError>
#include <cstdlib>

class tst_qmlmin : public QObject
{
    Q_OBJECT
public:
    tst_qmlmin();

private slots:
    void initTestCase();
    void qmlMinify_data();
    void qmlMinify();

private:
    QString qmlminPath;
    QStringList excludedDirs;
    QStringList invalidFiles;

    QStringList findFiles(const QDir &);
    bool isInvalidFile(const QFileInfo &fileName) const;
};

tst_qmlmin::tst_qmlmin()
{
}

void tst_qmlmin::initTestCase()
{
    qmlminPath = QLibraryInfo::location(QLibraryInfo::BinariesPath) + QLatin1String("/qmlmin");
#ifdef Q_OS_WIN
    qmlminPath += QLatin1String(".exe");
#endif
    if (!QFileInfo(qmlminPath).exists()) {
        QString message = QString::fromLatin1("qmlmin executable not found (looked for %0)")
                .arg(qmlminPath);
        QFAIL(qPrintable(message));
    }

    // Add directories you want excluded here

    // These snippets are not expected to run on their own.
    excludedDirs << "doc/src/snippets/declarative/visualdatamodel_rootindex";
    excludedDirs << "doc/src/snippets/declarative/qtbinding";
    excludedDirs << "doc/src/snippets/declarative/imports";
    excludedDirs << "doc/src/snippets/qtquick1/visualdatamodel_rootindex";
    excludedDirs << "doc/src/snippets/qtquick1/qtbinding";
    excludedDirs << "doc/src/snippets/qtquick1/imports";

    // Add invalid files (i.e. files with syntax errors)
    invalidFiles << "tests/auto/qtquick2/qquickloader/data/InvalidSourceComponent.qml";
    invalidFiles << "tests/auto/declarative/qdeclarativelanguage/data/dynamicObjectProperties.2.qml";
    invalidFiles << "tests/auto/declarative/qdeclarativelanguage/data/signal.3.qml";
    invalidFiles << "tests/auto/declarative/qdeclarativelanguage/data/property.4.qml";
    invalidFiles << "tests/auto/declarative/qdeclarativelanguage/data/empty.qml";
    invalidFiles << "tests/auto/declarative/qdeclarativelanguage/data/signal.2.qml";
    invalidFiles << "tests/auto/declarative/qdeclarativelanguage/data/missingObject.qml";
    invalidFiles << "tests/auto/declarative/qdeclarativelanguage/data/insertedSemicolon.1.qml";
    invalidFiles << "tests/auto/declarative/qdeclarativelanguage/data/nonexistantProperty.5.qml";
    invalidFiles << "tests/auto/declarative/qdeclarativelanguage/data/invalidRoot.1.qml";
    invalidFiles << "tests/auto/declarative/qdeclarativefolderlistmodel/data/dummy.qml";
    invalidFiles << "tests/auto/declarative/qdeclarativeecmascript/data/qtbug_22843.js";
    invalidFiles << "tests/auto/declarative/qdeclarativeecmascript/data/qtbug_22843.library.js";
    invalidFiles << "tests/auto/declarative/qdeclarativeworkerscript/data/script_error_onLoad.js";
    invalidFiles << "tests/auto/declarative/parserstress/tests/ecma_3/Unicode/regress-352044-02-n.js";
}

QStringList tst_qmlmin::findFiles(const QDir &d)
{
    for (int ii = 0; ii < excludedDirs.count(); ++ii) {
        QString s = excludedDirs.at(ii);
        if (d.absolutePath().endsWith(s))
            return QStringList();
    }

    QStringList rv;

    QStringList files = d.entryList(QStringList() << QLatin1String("*.qml") << QLatin1String("*.js"),
                                    QDir::Files);
    foreach (const QString &file, files) {
        rv << d.absoluteFilePath(file);
    }

    QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot |
                                   QDir::NoSymLinks);
    foreach (const QString &dir, dirs) {
        QDir sub = d;
        sub.cd(dir);
        rv << findFiles(sub);
    }

    return rv;
}

bool tst_qmlmin::isInvalidFile(const QFileInfo &fileName) const
{
    foreach (const QString &invalidFile, invalidFiles) {
        if (fileName.absoluteFilePath().endsWith(invalidFile))
            return true;
    }
    return false;
}

/*
This test runs all the examples in the declarative UI source tree and ensures
that they start and exit cleanly.

Examples are any .qml files under the examples/ directory that start
with a lower case letter.
*/

void tst_qmlmin::qmlMinify_data()
{
    QTest::addColumn<QString>("file");

    QString examples = QLatin1String(SRCDIR) + "/../../../../examples/";
    QString tests = QLatin1String(SRCDIR) + "/../../../../tests/";

    QStringList files;
    files << findFiles(QDir(examples));
    files << findFiles(QDir(tests));

    foreach (const QString &file, files)
        QTest::newRow(qPrintable(file)) << file;
}

void tst_qmlmin::qmlMinify()
{
    QFETCH(QString, file);

    QProcess qmlminify;
    qmlminify.start(qmlminPath, QStringList() << QLatin1String("--verify-only") << file);
    qmlminify.waitForFinished();

    QCOMPARE(qmlminify.error(), QProcess::UnknownError);
    QCOMPARE(qmlminify.exitStatus(), QProcess::NormalExit);

    if (isInvalidFile(file))
        QCOMPARE(qmlminify.exitCode(), EXIT_FAILURE); // cannot minify files with syntax errors
    else
        QCOMPARE(qmlminify.exitCode(), 0);
}

QTEST_MAIN(tst_qmlmin)

#include "tst_qmlmin.moc"
