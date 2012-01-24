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
#include "qmlruntime.h"
#include <QDeclarativeView>
#include <QDeclarativeError>

class tst_examples : public QObject
{
    Q_OBJECT
public:
    tst_examples();

private slots:
    void examples_data();
    void examples();

    void namingConvention();
private:
    QStringList excludedDirs;

    void namingConvention(const QDir &);
    QStringList findQmlFiles(const QDir &);
};

tst_examples::tst_examples()
{
    // Add directories you want excluded here

#ifdef QT_NO_WEBKIT
    excludedDirs << "examples/declarative/qtquick1/modelviews/webview";
    excludedDirs << "examples/declarative/qtquick1/webbrowser";
    excludedDirs << "doc/src/snippets/declarative/qtquick1/webview";
    excludedDirs << "doc/src/snippets/qtquick1/qtquick1/webview";
#endif

#ifdef QT_NO_XMLPATTERNS
    excludedDirs << "examples/declarative/qtquick1/xml/xmldata";
    excludedDirs << "examples/declarative/qtquick1/twitter";
    excludedDirs << "examples/declarative/qtquick1/flickr";
    excludedDirs << "examples/declarative/qtquick1/photoviewer";
#endif
}

/*
This tests that the examples follow the naming convention required
to have them tested by the examples() test.
*/
void tst_examples::namingConvention(const QDir &d)
{
    for (int ii = 0; ii < excludedDirs.count(); ++ii) {
        QString s = excludedDirs.at(ii);
        if (d.absolutePath().endsWith(s))
            return;
    }

    QStringList files = d.entryList(QStringList() << QLatin1String("*.qml"),
                                    QDir::Files);

    bool seenQml = !files.isEmpty();
    bool seenLowercase = false;

    foreach (const QString &file, files) {
        if (file.at(0).isLower())
            seenLowercase = true;
    }

    if (!seenQml) {
        QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot |
                QDir::NoSymLinks);
        foreach (const QString &dir, dirs) {
            QDir sub = d;
            sub.cd(dir);
            namingConvention(sub);
        }
    } else if (!seenLowercase) {
        QFAIL(qPrintable(QString(
            "Directory %1 violates naming convention; expected at least one qml file "
            "starting with lower case, got: %2"
        ).arg(d.absolutePath()).arg(files.join(","))));
    }
}

void tst_examples::namingConvention()
{
    QString examples = QLibraryInfo::location(QLibraryInfo::ExamplesPath);

    namingConvention(QDir(examples));
}

QStringList tst_examples::findQmlFiles(const QDir &d)
{
    for (int ii = 0; ii < excludedDirs.count(); ++ii) {
        QString s = excludedDirs.at(ii);
        if (d.absolutePath().endsWith(s))
            return QStringList();
    }

    QStringList rv;

    QStringList cppfiles = d.entryList(QStringList() << QLatin1String("*.cpp"), QDir::Files);
    if (cppfiles.isEmpty()) {
        QStringList files = d.entryList(QStringList() << QLatin1String("*.qml"),
                                        QDir::Files);
        foreach (const QString &file, files) {
            if (file.at(0).isLower()) {
                rv << d.absoluteFilePath(file);
            }
        }
    }

    QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot |
                                   QDir::NoSymLinks);
    foreach (const QString &dir, dirs) {
        QDir sub = d;
        sub.cd(dir);
        rv << findQmlFiles(sub);
    }

    return rv;
}

/*
This test runs all the examples in the declarative UI source tree and ensures
that they start and exit cleanly.

Examples are any .qml files under the examples/ directory that start
with a lower case letter.
*/
static void silentErrorsMsgHandler(QtMsgType, const char *)
{
}


void tst_examples::examples_data()
{
    QTest::addColumn<QString>("file");

    QString examples = QLatin1String(SRCDIR) + "/../../../../examples/declarative/qtquick1";

    QStringList files;
    files << findQmlFiles(QDir(examples));

    foreach (const QString &file, files)
        QTest::newRow(qPrintable(file)) << file;
}

void tst_examples::examples()
{
    QFETCH(QString, file);

    QDeclarativeViewer viewer;

    QtMsgHandler old = qInstallMsgHandler(silentErrorsMsgHandler);
    QVERIFY(viewer.open(file));
    qInstallMsgHandler(old);

    if (viewer.view()->status() == QDeclarativeView::Error)
        qWarning() << viewer.view()->errors();

    QCOMPARE(viewer.view()->status(), QDeclarativeView::Ready);
    viewer.show();

    QTest::qWaitForWindowShown(&viewer);
}

QTEST_MAIN(tst_examples)

#include "tst_examples.moc"
