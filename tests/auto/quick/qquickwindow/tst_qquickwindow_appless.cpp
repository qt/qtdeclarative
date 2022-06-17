// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTemporaryFile>
#include <QtQml/QQmlApplicationEngine>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <private/qquickwindow_p.h>

using namespace Qt::StringLiterals;

class tst_qquickwindow_appless: public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickwindow_appless()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {}

private slots:
    void screenBindingsAreUpdatedAfterAScreenChange();
};

void tst_qquickwindow_appless::screenBindingsAreUpdatedAfterAScreenChange()
{
    QJsonDocument configuration{
        QJsonObject {
            {
                "screens", QJsonArray {
                    QJsonObject {
                        {"name", "firstScreen"},
                        {"x", 0},
                        {"y", 0},
                        {"width", 640},
                        {"height", 480},
                        {"logicalDpi", 96},
                        {"logicalBaseDpi", 96},
                        {"dpr", 1}
                    },
                    QJsonObject {
                        {"name", "secondScreen"},
                        {"x", 640},
                        {"y", 0},
                        {"width", 640},
                        {"height", 480},
                        {"logicalDpi", 96},
                        {"logicalBaseDpi", 96},
                        {"dpr", 1}
                }
            }
        }
    }};

    // We would generally use a QTemporaryFile here.
    // At the time of writing the argument parser for the platform plugin, which
    // we are forcing to offscreen with the following file used for
    // configuration, doesn't correctly handle colons (https://bugreports.qt.io/browse/QTBUG-130346).
    // The usage of QTemporaryFile and its filename implementation showed to
    // generally produce a path with a drive in it on Windows platforms in CI
    // checks, which clashs with the current behavior of the argument parser.
    // The following code is used as a substitute as it avoids the issue.
    // The `QFile` code was taken from `tst_highdpi` in qtbase, which performs the same
    // operation of configuring custom screens to the offscreen plugins.
    QFile configurationFile(QLatin1String("qt-offscreen-test-config.json"));
    if (!configurationFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        configurationFile.resize(0); // truncate
    if (configurationFile.write(configuration.toJson()) == -1)
        qFatal("Could not write config file: %s", qPrintable(configurationFile.errorString()));
    configurationFile.close();

    static QByteArray nameArgument = QByteArray("tst_qquickwindow_appless");
    static QByteArray platformArgument = QByteArray("-platform");
    static QByteArray offscreenAndConfigurationArgument;
    offscreenAndConfigurationArgument = QByteArray("offscreen:configfile=") + configurationFile.fileName().toUtf8();

    static int argc = 3;
    static char *argv[3];

    argv[0] = nameArgument.data();
    argv[1] = platformArgument.data();
    argv[2] = offscreenAndConfigurationArgument.data();

    QGuiApplication app(argc, argv);
    configurationFile.remove(); // config file is needed during QGuiApplication construction only.

    QQmlApplicationEngine engine(testFileUrl("screenBindingsAreUpdatedAfterAScreenChange.qml"));

    QVERIFY(!engine.rootObjects().isEmpty());

    QQuickWindow *window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    QVERIFY(window);

    // Ensure that we show the window so that screen changes based on
    // position can be applied.
    window->show();

    QCOMPARE(window->property("screenName").toString(), u"secondScreen"_s);
    QCOMPARE(window->property("attachedScreenName").toString(), u"secondScreen"_s);
}

#include "tst_qquickwindow_appless.moc"
QTEST_APPLESS_MAIN(tst_qquickwindow_appless);
