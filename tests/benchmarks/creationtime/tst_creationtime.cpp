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

    void universal();
    void universal_data();

    void calendar();
    void calendar_data();

private:
    QQmlEngine engine;
};

void tst_CreationTime::init()
{
    engine.clearComponentCache();
}

static void addTestRows(const QString &path)
{
    QFileInfoList entries = QDir(path).entryInfoList(QStringList("*.qml"), QDir::Files);
    foreach (const QFileInfo &entry, entries)
        QTest::newRow(qPrintable(entry.baseName())) << QUrl::fromLocalFile(entry.absoluteFilePath());
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
    QFETCH(QUrl, url);
    QQmlComponent component(&engine);
    component.loadUrl(url);
    doBenchmark(&component);
}

void tst_CreationTime::controls_data()
{
    QTest::addColumn<QUrl>("url");
    addTestRows(QQC2_IMPORT_PATH "/controls");
}

void tst_CreationTime::universal()
{
    QFETCH(QUrl, url);
    QQmlComponent component(&engine);
    component.loadUrl(url);
    doBenchmark(&component);
}

void tst_CreationTime::universal_data()
{
    QTest::addColumn<QUrl>("url");
    addTestRows(QQC2_IMPORT_PATH "/controls/universal");
}

void tst_CreationTime::calendar()
{
    QFETCH(QUrl, url);
    QQmlComponent component(&engine);
    component.loadUrl(url);
    doBenchmark(&component);
}

void tst_CreationTime::calendar_data()
{
    QTest::addColumn<QUrl>("url");
    addTestRows(QQC2_IMPORT_PATH "/calendar");
}

QTEST_MAIN(tst_CreationTime)

#include "tst_creationtime.moc"
