// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>
#include <QtQuick>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/private/qquickstyle_p.h>

typedef QPair<QString, QString> QStringPair;

class tst_Snippets : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void verify();
    void verify_data();

private:
    void loadSnippet(const QString &source);

    bool takeScreenshots;
    QMap<QString, QStringPair> snippetPaths;
};

static QMap<QString, QStringPair> findSnippets(const QDir &inputDir, const QDir &outputDir = QDir())
{
    QMap<QString, QStringPair> snippetPaths;
    QDirIterator it(inputDir.path(), QStringList() << "qtquick*.qml" << "qtlabs*.qml", QDir::Files | QDir::Readable);
    while (it.hasNext()) {
        QFileInfo fi(it.next());
        const QString outDirPath = !outputDir.path().isEmpty() ? outputDir.filePath(fi.baseName() + ".png") : QString();
        snippetPaths.insert(fi.baseName(), qMakePair(fi.filePath(), outDirPath));
    }
    return snippetPaths;
}

void tst_Snippets::initTestCase()
{
    qInfo() << "Snippets are taken from" << QQC2_SNIPPETS_PATH;

    QDir snippetsDir(QQC2_SNIPPETS_PATH);
    QVERIFY(!snippetsDir.path().isEmpty());

    QDir screenshotsDir(QDir::current().filePath("screenshots"));

    takeScreenshots = qEnvironmentVariableIntValue("SCREENSHOTS");
    if (takeScreenshots)
        QVERIFY(screenshotsDir.exists() || QDir::current().mkpath("screenshots"));

    snippetPaths = findSnippets(snippetsDir, screenshotsDir);
    QVERIFY(!snippetPaths.isEmpty());
}

Q_DECLARE_METATYPE(QList<QQmlError>)

void tst_Snippets::verify()
{
    QFETCH(QString, input);
    QFETCH(QString, output);

    QQmlEngine engine;
    QQmlComponent component(&engine);

    qRegisterMetaType<QList<QQmlError> >();
    QSignalSpy warnings(&engine, SIGNAL(warnings(QList<QQmlError>)));
    QVERIFY(warnings.isValid());

    QUrl url = QUrl::fromLocalFile(input);
    component.loadUrl(url);

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(!root.isNull(), qPrintable(component.errorString()));

    QCOMPARE(component.status(), QQmlComponent::Ready);
    QVERIFY(component.errors().isEmpty());

    QVERIFY(warnings.isEmpty());

    if (takeScreenshots) {
        const QString currentDataTag = QLatin1String(QTest::currentDataTag());
        static const QString applicationStyle = QQuickStyle::name().isEmpty() ? "Basic" : QQuickStyle::name();
        static const QStringList builtInStyles = QQuickStylePrivate::builtInStyles();

        bool isStyledSnippet = false;
        const QString snippetStyle = currentDataTag.section("-", 1, 1);
        for (const QString &style : builtInStyles) {
            if (!snippetStyle.compare(style, Qt::CaseInsensitive)) {
                if (applicationStyle != style)
                    QSKIP(qPrintable(QString("%1 style specific snippet. Running with the %2 style.").arg(style, applicationStyle)));
                isStyledSnippet = true;
            }
        }

        if (!isStyledSnippet && !applicationStyle.isEmpty()) {
            int index = output.indexOf("-", output.lastIndexOf("/"));
            if (index != -1)
                output.insert(index, "-" + applicationStyle.toLower());
        }

        QQuickWindow *window = qobject_cast<QQuickWindow *>(root.data());
        if (!window) {
            QQuickView *view = new QQuickView;
            view->setContent(url, &component, root.data());
            window = view;
        }

        window->show();
        window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(window));

        QSharedPointer<QQuickItemGrabResult> result = window->contentItem()->grabToImage();
        QSignalSpy spy(result.data(), SIGNAL(ready()));
        QVERIFY(spy.isValid());
        QVERIFY(spy.wait());
        QVERIFY(result->saveToFile(output));

        window->close();
    }
}

void tst_Snippets::verify_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("output");

    QMap<QString, QStringPair>::const_iterator it;
    for (it = snippetPaths.constBegin(); it != snippetPaths.constEnd(); ++it)
        QTest::newRow(qPrintable(it.key())) << it.value().first << it.value().second;
}

QTEST_MAIN(tst_Snippets)

#include "tst_snippets.moc"
