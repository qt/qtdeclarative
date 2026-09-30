// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQml/qqmlabstracturlinterceptor.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlfileselector.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qfileselector.h>
#include <QtCore/qresource.h>
#include <QtCore/qset.h>
#include <QtTest/qtest.h>

#include <memory>
#include <optional>

using namespace Qt::StringLiterals;

class CountingInterceptor : public QQmlAbstractUrlInterceptor
{
public:
    QUrl intercept(const QUrl &path, DataType type) override
    {
        Q_UNUSED(type);
        ++calls;
        urls.insert(path);
        return path;
    }

    qint64 calls = 0;
    QSet<QUrl> urls;
};

struct Location
{
    const char *name;
    QString importPath;
    QUrl url;
    QString moduleUrl;

    // Most libraries register their resources at "/". Every qrc lookup has to go through all
    // of them. A real application links many such libraries.
    int extraResourceRoots = 0;
};

static QList<Location> locations()
{
    const QString diskModule =
            QUrl::fromLocalFile(QStringLiteral(GENERATED_DIR "/App/")).toString();
    return {
        { "qrc", u":/qt/qml"_s, QUrl(u"qrc:/qt/qml/Main.qml"_s), u"qrc:/qt/qml/App/"_s },
        { "qrc, 50 extra resource roots", u":/qt/qml"_s, QUrl(u"qrc:/qt/qml/Main.qml"_s),
          u"qrc:/qt/qml/App/"_s, 50 },
        { "disk", QStringLiteral(GENERATED_DIR),
          QUrl::fromLocalFile(QStringLiteral(GENERATED_DIR "/Main.qml")), diskModule },
    };
}

class ExtraResourceRoots
{
    Q_DISABLE_COPY_MOVE(ExtraResourceRoots)
public:
    explicit ExtraResourceRoots(int count)
    {
        if (count == 0)
            return;

        QFile file(QStringLiteral(OTHER_RCC));
        if (!file.open(QIODevice::ReadOnly))
            qFatal("Cannot open %s", OTHER_RCC);
        m_data = file.readAll();
        for (int i = 0; i < count; ++i) {
            if (!QResource::registerResource(data(), u"/"_s))
                qFatal("Cannot register %s", OTHER_RCC);
            ++m_count;
        }
    }

    ~ExtraResourceRoots()
    {
        for (int i = 0; i < m_count; ++i)
            QResource::unregisterResource(data(), u"/"_s);
    }

private:
    const uchar *data() const { return reinterpret_cast<const uchar *>(m_data.constData()); }

    QByteArray m_data;
    int m_count = 0;
};

struct InterceptionFixture
{
    explicit InterceptionFixture(const Location &location)
    {
        {
            QQmlEngine selectorEngine;
            QQmlFileSelector fileSelector{ &selectorEngine };
            selectors = fileSelector.selector()->allSelectors();
        }

        for (int i = 0; i < TYPE_COUNT; ++i) {
            const QUrl url(location.moduleUrl + u"Type%1.qml"_s.arg(i));
            paths.append(url.isLocalFile() ? url.toLocalFile() : u':' + url.path());
        }
    }

    QStringList paths;
    QStringList selectors;
};

class tst_urlinterception : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void load_data();
    void load();
    void interceptions_data();
    void interceptions();
    void selectorCount_data();
    void selectorCount();
};

static std::unique_ptr<QObject> doLoad(QQmlEngine *engine, const Location &location)
{
    engine->addImportPath(location.importPath);
    QQmlComponent component(engine, location.url);
    std::unique_ptr<QObject> object(component.create());
    if (!object)
        qWarning().noquote() << component.errorString();
    return object;
}

void tst_urlinterception::initTestCase()
{
    QQmlEngine engine;
    QQmlFileSelector selector(&engine);

    // QFileSelector probes one directory per selector for each URL.
    qInfo() << "File selectors:" << selector.selector()->allSelectors();
}

void tst_urlinterception::load_data()
{
    QTest::addColumn<Location>("location");
    QTest::addColumn<bool>("fileSelector");

    for (const Location &location : locations()) {
        QTest::addRow("%s", location.name) << location << false;

        // QQmlApplicationEngine installs a QQmlFileSelector by default.
        QTest::addRow("%s, file selector", location.name) << location << true;
    }
}

void tst_urlinterception::load()
{
    QFETCH(Location, location);
    QFETCH(bool, fileSelector);

    ExtraResourceRoots extraRoots(location.extraResourceRoots);
    QBENCHMARK {
        QQmlEngine engine;
        std::optional<QQmlFileSelector> selector;
        if (fileSelector)
            selector.emplace(&engine);
        QVERIFY(doLoad(&engine, location));
    }
}

void tst_urlinterception::interceptions_data()
{
    QTest::addColumn<Location>("location");
    for (const Location &location : locations())
        QTest::addRow("%s", location.name) << location;
}

void tst_urlinterception::interceptions()
{
    QFETCH(Location, location);

    ExtraResourceRoots extraRoots(location.extraResourceRoots);
    CountingInterceptor interceptor;
    {
        QQmlEngine engine;
        engine.addUrlInterceptor(&interceptor);
        QVERIFY(doLoad(&engine, location));
    }

    qInfo() << interceptor.calls << "calls for" << interceptor.urls.size() << "distinct URLs";
    QTest::setBenchmarkResult(interceptor.calls, QTest::Events);
}

void tst_urlinterception::selectorCount_data()
{
    QTest::addColumn<Location>("location");
    QTest::addColumn<int>("extraSelectors");

    // Runs the real QFileSelector::select() with additional selectors that match no directory.
    // Each one adds one directory probe. The growth per selector is the actual cost of a probe,
    // and extrapolating to zero selectors gives the actual cost of everything else. This checks
    // the breakdown in interceptionSteps(), which can only replicate what QFileSelector does.
    for (const Location &location : locations()) {
        for (int extraSelectors : { 0, 4, 8, 16 }) {
            QTest::addRow("%s: %d extra selectors", location.name, extraSelectors)
                    << location << extraSelectors;
        }
    }
}

void tst_urlinterception::selectorCount()
{
    QFETCH(Location, location);
    QFETCH(int, extraSelectors);

    ExtraResourceRoots extraRoots(location.extraResourceRoots);
    const InterceptionFixture fixture(location);

    QFileSelector selector;
    QStringList extras;
    for (int i = 0; i < extraSelectors; ++i)
        extras.append(u"benchmark%1"_s.arg(i));
    selector.setExtraSelectors(extras);
    QCOMPARE(selector.allSelectors().size(), fixture.selectors.size() + extraSelectors);

    qsizetype result = 0;
    QBENCHMARK {
        for (const QString &path : fixture.paths)
            result += selector.select(path).size();
    }
    QVERIFY(result > 0);
}

QTEST_MAIN(tst_urlinterception)

#include "tst_urlinterception.moc"
