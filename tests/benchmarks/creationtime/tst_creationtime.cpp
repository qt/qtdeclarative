/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtQml>
#include <QtTest>

class tst_CreationTime : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void controls();
    void controls_data();

    void calendar();
    void calendar_data();

private:
    QQmlEngine engine;
};

void tst_CreationTime::init()
{
    engine.clearComponentCache();
}

static QStringList listControls(const QDir &dir)
{
    QStringList controls;
    foreach (const QFileInfo &entry, dir.entryInfoList(QStringList("*.qml"), QDir::Files))
        controls += entry.baseName();
    return controls;
}

static void addTestRows(const QStringList &importPaths, const QString &importPath)
{
    QStringList controls;
    foreach (const QString &path, importPaths) {
        QDir dir(path);
        if (dir.cd(importPath)) {
            foreach (const QString &control, listControls(dir)) {
                if (!controls.contains(control))
                    controls += control;
            }
        }
    }

    foreach (const QString &control, controls)
        QTest::newRow(qPrintable(control)) << control.toUtf8();
}

static void doBenchmark(QQmlComponent *component)
{
    QObjectList objects;
    objects.reserve(4096);
    QBENCHMARK {
        QObject *object = component->create();
        if (!object)
            qFatal("%s", qPrintable(component->errorString()));
        objects += object;
    }
    qDeleteAll(objects);
}

void tst_CreationTime::controls()
{
    QFETCH(QByteArray, control);
    QQmlComponent component(&engine);
    component.setData("import Qt.labs.controls 1.0;" + control + "{}", QUrl());
    doBenchmark(&component);
}

void tst_CreationTime::controls_data()
{
    QTest::addColumn<QByteArray>("control");
    addTestRows(engine.importPathList(), "Qt/labs/controls");
}

void tst_CreationTime::calendar()
{
    QFETCH(QByteArray, control);
    QQmlComponent component(&engine);
    component.setData("import Qt.labs.calendar 1.0;" + control + "{}", QUrl());
    doBenchmark(&component);
}

void tst_CreationTime::calendar_data()
{
    QTest::addColumn<QByteArray>("control");
    addTestRows(engine.importPathList(), "Qt/labs/calendar");
}

QTEST_MAIN(tst_CreationTime)

#include "tst_creationtime.moc"
