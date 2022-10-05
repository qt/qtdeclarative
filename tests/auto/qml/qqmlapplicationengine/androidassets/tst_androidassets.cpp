// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtQml/qqmlapplicationengine.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

class tst_AndroidAssets : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void loadsFromAssetsPath();
    void loadsFromAssetsUrl();

private:

    static QString pathPrefix()
    {
#ifdef Q_OS_ANDROID
        return QStringLiteral("assets:");
#else
        // Even when not running on android we can check that the copying to build dir worked.
        return QCoreApplication::applicationDirPath() + QStringLiteral("/android-build/assets");
#endif
    }

    static QString urlPrefix() {
#ifdef Q_OS_ANDROID
        return pathPrefix();
#else
        return QStringLiteral("file:") + pathPrefix();
#endif
    }
};


void tst_AndroidAssets::loadsFromAssetsPath()
{
    QQmlApplicationEngine engine;
    QSignalSpy failureSpy(&engine, &QQmlApplicationEngine::objectCreationFailed);

    // load QML file from assets, by path:
    engine.load(pathPrefix() + QStringLiteral("/qml/main.qml"));
    QTRY_VERIFY(engine.rootObjects().size() == 1);
    QVERIFY(failureSpy.isEmpty());
}

void tst_AndroidAssets::loadsFromAssetsUrl()
{
    QQmlApplicationEngine engine;
    QSignalSpy failureSpy(&engine, &QQmlApplicationEngine::objectCreationFailed);

    // load QML file from assets, by URL:
    engine.load(QUrl(urlPrefix() + QStringLiteral("/qml/main.qml")));
    QTRY_VERIFY(engine.rootObjects().size() == 1);
    QVERIFY(failureSpy.isEmpty());
}

QTEST_MAIN(tst_AndroidAssets)

#include "tst_androidassets.moc"
