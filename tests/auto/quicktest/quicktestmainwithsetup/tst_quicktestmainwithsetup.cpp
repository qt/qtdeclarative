// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtQuickTest/quicktest.h>

class QmlRegisterTypeCppType : public QObject
{
    Q_OBJECT

public:
    QmlRegisterTypeCppType() {}
};

class CustomTestSetup : public QObject
{
    Q_OBJECT

public:
    CustomTestSetup() {}

public slots:
    void qmlEngineAvailable(QQmlEngine *qmlEngine)
    {
        // Test that modules are successfully imported by the TestCaseCollector that
        // parses the QML files (but doesn't run them). For that to happen, qmlEngineAvailable()
        // must be called before TestCaseCollector does its thing.
        qmlRegisterType<QmlRegisterTypeCppType>("QmlRegisterTypeCppModule", 1, 0, "QmlRegisterTypeCppType");
        qmlEngine->addImportPath(QString::fromUtf8(QT_QMLTEST_DATADIR) + "/../imports");

        qmlEngine->rootContext()->setContextProperty("qmlEngineAvailableCalled", true);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(qquicktestsetup, CustomTestSetup)

#include "tst_quicktestmainwithsetup.moc"
