// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlExpression>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtCore/qstandardpaths.h>

class tst_qqmlstandardpaths : public QQmlDataTest
{
    Q_OBJECT

public:
    explicit tst_qqmlstandardpaths() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private Q_SLOTS:
    void standardPaths();
    void standardLocations();
};

static const QList<QStandardPaths::StandardLocation> allStandardLocations = {
    QStandardPaths::DesktopLocation,
    QStandardPaths::DocumentsLocation,
    QStandardPaths::FontsLocation,
    QStandardPaths::ApplicationsLocation,
    QStandardPaths::MusicLocation,
    QStandardPaths::MoviesLocation,
    QStandardPaths::PicturesLocation,
    QStandardPaths::TempLocation,
    QStandardPaths::HomeLocation,
    QStandardPaths::AppLocalDataLocation,
    QStandardPaths::CacheLocation,
    QStandardPaths::GenericDataLocation,
    QStandardPaths::RuntimeLocation,
    QStandardPaths::ConfigLocation,
    QStandardPaths::DownloadLocation,
    QStandardPaths::GenericCacheLocation,
    QStandardPaths::GenericConfigLocation,
    QStandardPaths::AppDataLocation,
    QStandardPaths::AppConfigLocation,
    QStandardPaths::PublicShareLocation,
    QStandardPaths::TemplatesLocation
};

void tst_qqmlstandardpaths::standardPaths()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("tst_standardpaths.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("desktop").toInt(), QStandardPaths::DesktopLocation);
    QCOMPARE(object->property("documents").toInt(), QStandardPaths::DocumentsLocation);
    QCOMPARE(object->property("fonts").toInt(), QStandardPaths::FontsLocation);
    QCOMPARE(object->property("applications").toInt(), QStandardPaths::ApplicationsLocation);
    QCOMPARE(object->property("music").toInt(), QStandardPaths::MusicLocation);
    QCOMPARE(object->property("movies").toInt(), QStandardPaths::MoviesLocation);
    QCOMPARE(object->property("pictures").toInt(), QStandardPaths::PicturesLocation);
    QCOMPARE(object->property("temp").toInt(), QStandardPaths::TempLocation);
    QCOMPARE(object->property("home").toInt(), QStandardPaths::HomeLocation);
    QCOMPARE(object->property("appLocalData").toInt(), QStandardPaths::AppLocalDataLocation);
    QCOMPARE(object->property("cache").toInt(), QStandardPaths::CacheLocation);
    QCOMPARE(object->property("genericData").toInt(), QStandardPaths::GenericDataLocation);
    QCOMPARE(object->property("runtime").toInt(), QStandardPaths::RuntimeLocation);
    QCOMPARE(object->property("config").toInt(), QStandardPaths::ConfigLocation);
    QCOMPARE(object->property("download").toInt(), QStandardPaths::DownloadLocation);
    QCOMPARE(object->property("genericCache").toInt(), QStandardPaths::GenericCacheLocation);
    QCOMPARE(object->property("genericConfig").toInt(), QStandardPaths::GenericConfigLocation);
    QCOMPARE(object->property("appData").toInt(), QStandardPaths::AppDataLocation);
    QCOMPARE(object->property("appConfig").toInt(), QStandardPaths::AppConfigLocation);
    QCOMPARE(object->property("locateFile").toInt(), QStandardPaths::LocateFile);
    QCOMPARE(object->property("locateDirectory").toInt(), QStandardPaths::LocateDirectory);
}

/*
    E.g.

        ENUM_VALUE_TO_STRING(QStandardPaths, StandardLocation, QStandardPaths::DesktopLocation)

    would result in "DesktopLocation".
*/
#define ENUM_VALUE_TO_STRING(ClassType, UnqualifiedEnumType, enumValue) \
    ClassType::staticMetaObject.enumerator(ClassType::staticMetaObject.indexOfEnumerator( \
        #UnqualifiedEnumType)).valueToKey(enumValue)

// Tests that the QML implementation of standardLocations() matches the C++ implementation.
void tst_qqmlstandardpaths::standardLocations()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("tst_standardpaths.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    for (const QStandardPaths::StandardLocation standardLocation : allStandardLocations) {
        // First, check that the QStandardPaths doesn't give us any empty paths.
        // While we're at it, convert the strings to URLs to allow us to compare later on.
        const QString locationName = ENUM_VALUE_TO_STRING(QStandardPaths, StandardLocation, standardLocation);
        const QList<QString> standardLocationPaths = QStandardPaths::standardLocations(standardLocation);
        QList<QUrl> standardLocationUrls;
        bool skipLocation = false;
        for (const auto &path : standardLocationPaths) {
#ifdef Q_OS_ANDROID
            // These paths are empty on Android (see QTBUG-108057 for related doc task).
            if (standardLocation == QStandardPaths::ApplicationsLocation
                    || standardLocation == QStandardPaths::PublicShareLocation
                    || standardLocation == QStandardPaths::TemplatesLocation) {
                skipLocation = true;
                continue;
            }
#endif
            QVERIFY2(!path.isEmpty(), qPrintable(QString::fromLatin1(
                "Path for %1 received from QStandardPaths::standardLocations is empty").arg(locationName)));
            QUrl url = QUrl::fromLocalFile(path);
            QVERIFY(!url.isEmpty());
            QVERIFY(url.isValid());
            standardLocationUrls.append(url);
        }
        if (skipLocation)
            continue;

        const QString qml = QLatin1String("StandardPaths.standardLocations(StandardPaths.") + locationName + QLatin1Char(')');
        QQmlExpression qmlExpression(qmlContext(object.data()), object.data(), qml);
        const QVariant evaluationResult = qmlExpression.evaluate();
        QVERIFY2(!qmlExpression.hasError(), qPrintable(qmlExpression.error().toString()));
        const auto actualValue = evaluationResult.value<QList<QUrl>>();
        // Give some extra context to the failure message since we're not using data rows.
        if (actualValue != standardLocationUrls)
            qWarning() << "Comparison failed for" << locationName;
        QCOMPARE(actualValue, standardLocationUrls);
    }
}

QTEST_MAIN(tst_qqmlstandardpaths)

#include "tst_qqmlstandardpaths.moc"
