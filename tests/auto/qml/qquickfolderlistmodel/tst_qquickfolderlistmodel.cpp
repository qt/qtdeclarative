/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qabstractitemmodel.h>
#include <QDebug>
#include "../../shared/util.h"

#if defined (Q_OS_WIN)
#include <qt_windows.h>
#endif

// From qquickfolderlistmodel.h
const int FileNameRole = Qt::UserRole+1;
const int FilePathRole = Qt::UserRole+2;
enum SortField { Unsorted, Name, Time, Size, Type };

class tst_qquickfolderlistmodel : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickfolderlistmodel() : removeStart(0), removeEnd(0) {}

public slots:
    void removed(const QModelIndex &, int start, int end) {
        removeStart = start;
        removeEnd = end;
    }

private slots:
    void basicProperties();
    void resetFiltering();
    void refresh();
#if defined (Q_OS_WIN) && !defined (Q_OS_WINCE)
    // WinCE does not have drive concept, so lets execute this test only on desktop Windows.
    void changeDrive();
#endif

private:
    void checkNoErrors(const QQmlComponent& component);
    QQmlEngine engine;

    int removeStart;
    int removeEnd;
};

void tst_qquickfolderlistmodel::checkNoErrors(const QQmlComponent& component)
{
    // Wait until the component is ready
    QTRY_VERIFY(component.isReady() || component.isError());

    if (component.isError()) {
        QList<QQmlError> errors = component.errors();
        for (int ii = 0; ii < errors.count(); ++ii) {
            const QQmlError &error = errors.at(ii);
            QByteArray errorStr = QByteArray::number(error.line()) + ":" +
                                  QByteArray::number(error.column()) + ":" +
                                  error.description().toUtf8();
            qWarning() << errorStr;
        }
    }
    QVERIFY(!component.isError());
}

void tst_qquickfolderlistmodel::basicProperties()
{
    QQmlComponent component(&engine, testFileUrl("basic.qml"));
    checkNoErrors(component);

    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(flm != 0);

    flm->setProperty("folder", dataDirectoryUrl());
    QTRY_COMPARE(flm->property("count").toInt(),4); // wait for refresh
    QCOMPARE(flm->property("folder").toUrl(), dataDirectoryUrl());
    QCOMPARE(flm->property("parentFolder").toUrl(), QUrl::fromLocalFile(QDir(directory()).canonicalPath()));
    QCOMPARE(flm->property("sortField").toInt(), int(Name));
    QCOMPARE(flm->property("nameFilters").toStringList(), QStringList() << "*.qml");
    QCOMPARE(flm->property("sortReversed").toBool(), false);
    QCOMPARE(flm->property("showDirs").toBool(), true);
    QCOMPARE(flm->property("showDotAndDotDot").toBool(), false);
    QCOMPARE(flm->property("showOnlyReadable").toBool(), false);
    QCOMPARE(flm->data(flm->index(0),FileNameRole).toString(), QLatin1String("basic.qml"));
    QCOMPARE(flm->data(flm->index(1),FileNameRole).toString(), QLatin1String("dummy.qml"));    
    
    flm->setProperty("folder",QUrl::fromLocalFile(""));
    QCOMPARE(flm->property("folder").toUrl(), QUrl::fromLocalFile(""));
}

void tst_qquickfolderlistmodel::resetFiltering()
{
    // see QTBUG-17837
    QQmlComponent component(&engine, testFileUrl("resetFiltering.qml"));
    checkNoErrors(component);

    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(flm != 0);

    connect(flm, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
            this, SLOT(removed(const QModelIndex&,int,int)));

    flm->setProperty("folder", testFileUrl("resetfiltering"));
    QTRY_COMPARE(flm->property("count").toInt(),1); // should just be "test.txt" visible
    int count = flm->rowCount();
    QCOMPARE(removeStart, 0);
    QCOMPARE(removeEnd, count-1);

    flm->setProperty("folder", testFileUrl("resetfiltering/innerdir"));
    QTRY_COMPARE(flm->property("count").toInt(),1); // should just be "test2.txt" visible
    count = flm->rowCount();
    QCOMPARE(removeStart, 0);
    QCOMPARE(removeEnd, count-1);

    flm->setProperty("folder", testFileUrl("resetfiltering"));
    QTRY_COMPARE(flm->property("count").toInt(),1); // should just be "test.txt" visible
    count = flm->rowCount();
    QCOMPARE(removeStart, 0);
    QCOMPARE(removeEnd, count-1);
}

void tst_qquickfolderlistmodel::refresh()
{
    QQmlComponent component(&engine, testFileUrl("basic.qml"));
    checkNoErrors(component);

    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(flm != 0);

    flm->setProperty("folder", dataDirectoryUrl());
    QTRY_COMPARE(flm->property("count").toInt(),4); // wait for refresh

    int count = flm->rowCount();

    connect(flm, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
            this, SLOT(removed(const QModelIndex&,int,int)));

    flm->setProperty("sortReversed", true);

    QTRY_COMPARE(removeStart, 0);
    QTRY_COMPARE(removeEnd, count-1); // wait for refresh
}

#if defined (Q_OS_WIN) && !defined (Q_OS_WINCE)
void tst_qquickfolderlistmodel::changeDrive()
{
    QSKIP("QTBUG-26728");
    class DriveMapper
    {
    public:
        DriveMapper(const QString &dataDir)
        {
            size_t stringLen = dataDir.length();
            targetPath = new wchar_t[stringLen+1];
            dataDir.toWCharArray(targetPath);
            targetPath[stringLen] = 0;

            DefineDosDevice(DDD_NO_BROADCAST_SYSTEM, L"X:", targetPath);
        }

        ~DriveMapper()
        {
            DefineDosDevice(DDD_EXACT_MATCH_ON_REMOVE | DDD_NO_BROADCAST_SYSTEM | DDD_REMOVE_DEFINITION, L"X:", targetPath);
            delete [] targetPath;
        }

    private:
        wchar_t *targetPath;
    };

    QString dataDir = testFile(0);
    DriveMapper dm(dataDir);
    QQmlComponent component(&engine, testFileUrl("basic.qml"));

    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(flm != 0);

    QSignalSpy folderChangeSpy(flm, SIGNAL(folderChanged()));

    flm->setProperty("folder",QUrl::fromLocalFile(dataDir));
    QCOMPARE(flm->property("folder").toUrl(), QUrl::fromLocalFile(dataDir));
    QTRY_VERIFY(folderChangeSpy.count() == 1);

    flm->setProperty("folder",QUrl::fromLocalFile("X:/resetfiltering/"));
    QCOMPARE(flm->property("folder").toUrl(), QUrl::fromLocalFile("X:/resetfiltering/"));
    QTRY_VERIFY(folderChangeSpy.count() == 2);
}
#endif

QTEST_MAIN(tst_qquickfolderlistmodel)

#include "tst_qquickfolderlistmodel.moc"
