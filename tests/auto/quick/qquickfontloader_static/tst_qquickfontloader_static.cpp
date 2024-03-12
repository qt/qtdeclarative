// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickView>
#include <QtQml/QQmlComponent>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <qtest.h>

QByteArray qmltemplate("import QtQuick 2.0\n"
"\n"
"Rectangle {\n"
"    width: 400\n"
"    height: 400\n"
"    color: \"red\"\n"
"    FontLoader { id: fixedFont; source: \"%1\" }\n"
"    Text {\n"
"        text: \"hello world\"\n"
"        anchors.centerIn: parent\n"
"        font.family: fixedFont.name\n"
"    }\n"
"}\n");

int actualTest(int argc, char **argv)
{
    for (int i = 0; i < 3; i++) {
        QGuiApplication app(argc, argv);
        QQmlDataTest dataTest(QT_QMLTEST_DATADIR);
        dataTest.initTestCase();
        QQuickView window;
        QQmlComponent component (window.engine());
        QUrl current = QUrl::fromLocalFile("");
        qmltemplate.replace("%1", dataTest.testFileUrl("font.ttf").toString().toLocal8Bit());
        component.setData(qmltemplate, current);
        window.setContent(current, &component, component.create());
        window.show();
        if (!QTest::qWaitForWindowActive(&window))
            return EXIT_FAILURE;
    }
    return 0;
}

// we need the QTestLib infrastructure to create a log file
struct FontLoaderStaticTester : public QObject
{
    Q_OBJECT

public:
    int result = -1;
private slots:
    void verify() {  QCOMPARE(result, 0); }
};

int main(int argc, char **argv)
{
    int result = actualTest(argc, argv);
    QCoreApplication app(argc, argv);
    FontLoaderStaticTester tester;
    tester.result = result;
    QTest::qExec(&tester, argc, argv);
}

#include "tst_qquickfontloader_static.moc"
