// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
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
