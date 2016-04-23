/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtTest/qtest.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/private/qquickstyleselector_p.h>
#include "../shared/util.h"

class tst_QQuickStyleSelector : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void select_data();
    void select();
};

void tst_QQuickStyleSelector::select_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("style");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("expected");

    QTest::newRow("empty") << "Button.qml" << "" << dataDirectory() << testFileUrl("Button.qml").toString();
    QTest::newRow("no such dir") << "Button.qml" << "Foo" << dataDirectory() << testFileUrl("Button.qml").toString();
    QTest::newRow("no such file") << "Foo.qml" << "FileSystemStyle" << dataDirectory() << testFileUrl("Foo.qml").toString();
    QTest::newRow("relative/path/to/FileSystemStyle") << "Button.qml" << "FileSystemStyle" << "data" << testFileUrl("FileSystemStyle/Button.qml").toString();
    QTest::newRow("/absolute/path/to/FileSystemStyle") << "Button.qml" << "FileSystemStyle" << dataDirectory() << testFileUrl("FileSystemStyle/Button.qml").toString();
    QTest::newRow(":/ResourceStyle") << "Button.qml" << "ResourceStyle" << ":/" << "qrc:/ResourceStyle/Button.qml";
    QTest::newRow("qrc:/ResourceStyle") << "Button.qml" << "ResourceStyle" << "qrc:/" << "qrc:/ResourceStyle/Button.qml";
}

void tst_QQuickStyleSelector::select()
{
    QFETCH(QString, file);
    QFETCH(QString, style);
    QFETCH(QString, path);
    QFETCH(QString, expected);

    QQuickStyle::setStyle(QDir(path).filePath(style));

    QQuickStyleSelector selector;
    selector.setBaseUrl(dataDirectoryUrl());
    QCOMPARE(selector.select(file), expected);
}

QTEST_MAIN(tst_QQuickStyleSelector)

#include "tst_qquickstyleselector.moc"
