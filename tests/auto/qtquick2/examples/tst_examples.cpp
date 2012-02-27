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
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QDeclarativeComponent>
#include <QDeclarativeEngine>
#include <QDeclarativeError>

static QtMsgHandler testlibMsgHandler = 0;
void msgHandlerFilter(QtMsgType type, const char *msg)
{
    if (type == QtCriticalMsg || type == QtFatalMsg)
        (*testlibMsgHandler)(type, msg);
}

class tst_examples : public QObject
{
    Q_OBJECT
public:
    tst_examples();

private slots:
    void init();
    void cleanup();

    void sgexamples_data();
    void sgexamples();
    void sgsnippets_data();
    void sgsnippets();

    void namingConvention();
private:
    QStringList excludedDirs;
    QStringList excludedFiles;

    void namingConvention(const QDir &);
    QStringList findQmlFiles(const QDir &);

    QDeclarativeEngine engine;
};

tst_examples::tst_examples()
{
    // Add files to exclude here
    excludedFiles << "doc/src/snippets/declarative/listmodel.qml"; //Just a ListModel, no root QQuickItem

    // Add directories you want excluded here (don't add examples/, because they install to examples/qtdeclarative/)
    excludedDirs << "shared"; //Not an example
    excludedDirs << "qtquick/text/fonts"; // QTBUG-21415
    excludedDirs << "doc/src/snippets/declarative/path"; //No root QQuickItem
    excludedDirs << "tutorials/gettingStartedQml"; //C++ example, but no cpp files in root dir

    // These snippets are not expected to run on their own.
    excludedDirs << "doc/src/snippets/declarative/visualdatamodel_rootindex";
    excludedDirs << "doc/src/snippets/declarative/qtbinding";
    excludedDirs << "doc/src/snippets/declarative/imports";

#ifdef QT_NO_WEBKIT
    excludedDirs << "qtquick/modelviews/webview";
    excludedDirs << "demos/webbrowser";
    excludedDirs << "doc/src/snippets/declarative/webview";
#endif

#ifdef QT_NO_XMLPATTERNS
    excludedDirs << "demos/twitter";
    excludedDirs << "demos/flickr";
    excludedDirs << "demos/photoviewer";
#endif
}

void tst_examples::init()
{
    if (!qstrcmp(QTest::currentTestFunction(), "sgsnippets"))
        testlibMsgHandler = qInstallMsgHandler(msgHandlerFilter);
}

void tst_examples::cleanup()
{
    if (!qstrcmp(QTest::currentTestFunction(), "sgsnippets"))
        qInstallMsgHandler(testlibMsgHandler);
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
    } else if(!seenLowercase) {
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
                bool superContinue = false;
                for (int ii = 0; ii < excludedFiles.count(); ++ii) {
                    QString e = excludedFiles.at(ii);
                    if (d.absoluteFilePath(file).endsWith(e)) {
                        superContinue = true;
                        break;
                    }
                }
                if (superContinue)
                    continue;
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
void tst_examples::sgexamples_data()
{
    QTest::addColumn<QString>("file");

    QString examples = QLatin1String(SRCDIR) + "/../../../../examples/";

    QStringList files;
    files << findQmlFiles(QDir(examples));

    foreach (const QString &file, files)
        QTest::newRow(qPrintable(file)) << file;
}

void tst_examples::sgexamples()
{
    QFETCH(QString, file);

    QDeclarativeComponent component(&engine, QUrl::fromLocalFile(file));
    if (component.status() == QDeclarativeComponent::Error)
        qWarning() << component.errors();
    QCOMPARE(component.status(), QDeclarativeComponent::Ready);

    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QQuickItem *root = qobject_cast<QQuickItem *>(object.data());
    if (!root)
        component.completeCreate();
    QVERIFY(root);

    QQuickCanvas canvas;
    root->setParentItem(canvas.rootItem());
    component.completeCreate();
    canvas.show();

    QTest::qWaitForWindowShown(&canvas);

}

void tst_examples::sgsnippets_data()
{
    QTest::addColumn<QString>("file");

    QString snippets = QLatin1String(SRCDIR) + "/../../../../doc/src/snippets/declarative";

    QStringList files;
    files << findQmlFiles(QDir(snippets));

    foreach (const QString &file, files)
        QTest::newRow(qPrintable(file)) << file;
}

void tst_examples::sgsnippets()
{
    QFETCH(QString, file);

    QDeclarativeComponent component(&engine, QUrl::fromLocalFile(file));
    if (component.status() == QDeclarativeComponent::Error)
        qWarning() << component.errors();
    QCOMPARE(component.status(), QDeclarativeComponent::Ready);

    QScopedPointer<QObject> object(component.beginCreate(engine.rootContext()));
    QQuickItem *root = qobject_cast<QQuickItem *>(object.data());
    if (!root)
        component.completeCreate();
    QVERIFY(root);

    QQuickCanvas canvas;
    root->setParentItem(canvas.rootItem());
    component.completeCreate();
    canvas.show();

    QTest::qWaitForWindowShown(&canvas);

}

QTEST_MAIN(tst_examples)

#include "tst_examples.moc"
