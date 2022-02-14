/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickView>
#include <QtTest/QTest>

#include <QtQuickTestUtils/private/viewtestutils_p.h>

class test_qml_app_deployment : public QObject
{
    Q_OBJECT
private slots:
    void canRun();
};


void test_qml_app_deployment::canRun()
{
    QQuickView view;
#ifdef QT_STATIC
    // Need to add qrc:/// as an import path when using Qt static builds,
    // to ensure that user module qmldir files are found from the embedded resources
    // and not from the filesystem.
    view.engine()->addImportPath(QLatin1String("qrc:///"));
#endif
    QVERIFY(QQuickTest::showView(view, QUrl("qrc:///main.qml")));
}

QTEST_MAIN(test_qml_app_deployment)

#include "main.moc"
