// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qbaselinetest.h>

#include <QtCore/QDirIterator>
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QPalette>
#include <QtGui/QFont>
#include <QtQuickControls2/QQuickStyle>

#include <algorithm>

QString blockify(const QByteArray& s)
{
    const char* indent = "\n | ";
    QByteArray block = s.trimmed();
    block.replace('\n', indent);
    block.prepend(indent);
    block.append('\n');
    return block;
}

class tst_Baseline_Controls : public QObject
{
    Q_OBJECT

public:
    tst_Baseline_Controls();

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void basic_data() { setupTestSuite(); }
    void basic() { runTest("Basic"); }
    void fusion_data() { setupTestSuite(); }
    void fusion() { runTest("Fusion"); }
    void material_data() { setupTestSuite(); }
    void material() { runTest("Material"); }
    void imagine_data() { setupTestSuite(); }
    void imagine() { runTest("Imagine"); }
    void universal_data() { setupTestSuite(); }
    void universal() { runTest("Universal"); }
#if defined(Q_OS_DARWIN) // the iOS style only gets build on iOS and macOS
    void ios_data() { setupTestSuite(); }
    void ios() { runTest("iOS"); }
#endif
    void native_data() { setupTestSuite(); }
    void native() { runTest(); }

private:
    void test();

    void setupTestSuite();
    void runTest(const QString& style = QString());
    bool renderAndGrab(const QString& qmlFile, const QStringList& extraArgs, QImage *screenshot, QString *errMsg);

    QString testSuitePath;
    QString grabberPath;
    QStringList testFiles;
    int consecutiveErrors;   // Not test failures (image mismatches), but system failures (so no image at all)
    bool aborted;            // This run given up because of too many system failures
};


tst_Baseline_Controls::tst_Baseline_Controls()
    : consecutiveErrors(0), aborted(false)
{
}


void tst_Baseline_Controls::initTestCase()
{
    QString dataDir = QFINDTESTDATA("./data/.");
    if (dataDir.isEmpty())
        dataDir = QStringLiteral("data");
    QFileInfo fi(dataDir);
    if (!fi.exists() || !fi.isDir() || !fi.isReadable())
        QSKIP("Test suite data directory missing or unreadable: " + fi.canonicalFilePath().toLatin1());
    testSuitePath = fi.canonicalFilePath();

#if defined(Q_OS_WIN)
    grabberPath = QFINDTESTDATA("qmlscenegrabber.exe");
#elif defined(Q_OS_DARWIN)
    grabberPath = QFINDTESTDATA("qmlscenegrabber.app/Contents/MacOS/qmlscenegrabber");
#else
    grabberPath = QFINDTESTDATA("qmlscenegrabber");
#endif
    if (grabberPath.isEmpty())
        grabberPath = QCoreApplication::applicationDirPath() + "/../scenegraph/qmlscenegrabber";

    QBaselineTest::setProject("QuickControls");
    // Set key platform properties that are relevant for the appearance of controls
    const QString platformName = QGuiApplication::platformName() + "-" + QSysInfo::productType();
    QBaselineTest::addClientProperty("PlatformName", platformName);
    QBaselineTest::addClientProperty("OSVersion", QSysInfo::productVersion());

    const char *backendVarName = "QT_QUICK_BACKEND";
    const QString backend = qEnvironmentVariable(backendVarName, QString::fromLatin1("default"));
    QBaselineTest::addClientProperty(QString::fromLatin1(backendVarName), backend);

    QString stack = backend;
    if (stack != QLatin1String("software")) {
#if defined(Q_OS_WIN)
        const char *defaultRhiBackend = "d3d11";
#elif defined(Q_OS_DARWIN)
        const char *defaultRhiBackend = "metal";
#else
        const char *defaultRhiBackend = "opengl";
#endif
        const QString rhiBackend = qEnvironmentVariable("QSG_RHI_BACKEND", QString::fromLatin1(defaultRhiBackend));
        stack = QString::fromLatin1("RHI_%1").arg(rhiBackend);
    }

    QBaselineTest::addClientProperty(QString::fromLatin1("GraphicsStack"), stack);

    // Assume that text that's darker than the background means we run in light mode
    // See also qwidgetbaselinetest.cpp
    QPalette palette;
    QFont font;
    QByteArray colorSchemeBytes;
    {
        QDataStream colorSchemeStream(&colorSchemeBytes, QIODevice::WriteOnly);
        colorSchemeStream << palette << font;
        const qreal screenDpr = QGuiApplication::primaryScreen()->devicePixelRatio();
        if (screenDpr != 1.0) {
            qWarning() << "DPR is" << screenDpr << "- images will not be compared to 1.0 baseline!";
            colorSchemeStream << screenDpr;
        }
    }
    const quint16 colorSchemeId = qChecksum(colorSchemeBytes);

    const QColor windowColor = palette.window().color();
    const QColor textColor = palette.text().color();
    const QString colorSchemeIdStr = (windowColor.value() > textColor.value()
                                        ? QString("light-%1") : QString("dark-%1"))
                                        .arg(colorSchemeId, 0, 16);
    QBaselineTest::addClientProperty("AppearanceID", colorSchemeIdStr);

    QByteArray msg;
    if (!QBaselineTest::connectToBaselineServer(&msg))
        QSKIP(msg);

    // let users know where they can find the results
    qInfo("PlatformName computed to be  : %s", qPrintable(platformName));
    qInfo("Color Scheme computed as    : %s", qPrintable(colorSchemeIdStr));
    qInfo("Native style name is         : %s", qPrintable(QQuickStyle::name()));
}

void tst_Baseline_Controls::init()
{
    consecutiveErrors = 0;
    aborted = false;
}

void tst_Baseline_Controls::cleanup()
{
    // Allow subsystems time to settle
    if (!aborted)
        QTest::qWait(20);
}

void tst_Baseline_Controls::setupTestSuite()
{
    QTest::addColumn<QString>("qmlFile");

    const QStringView qmlExt(u".qml");
    if (testFiles.isEmpty()) {
        QDirIterator it(testSuitePath, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString fp = it.next().toLatin1();
            if (fp.endsWith(qmlExt))
                testFiles.append(it.filePath());
        }
        std::sort(testFiles.begin(), testFiles.end());
    }

    if (testFiles.isEmpty())
        QSKIP("No .qml test files found in " + testSuitePath.toLatin1());

    for (const auto &filePath : std::as_const(testFiles)) {
        QString itemName = filePath.sliced(testSuitePath.size() + 1);
        itemName = itemName.left(itemName.size() - qmlExt.size());
        QBaselineTest::newRow(itemName.toLatin1()) << filePath;
    }
}


void tst_Baseline_Controls::runTest(const QString& style)
{
    if (aborted)
        QSKIP("System too unstable.");

    QFETCH(QString, qmlFile);

    QImage screenShot;
    QString errorMessage;
    QStringList args;
    if (!style.isEmpty())
        args.append({"-style", style});
    if (renderAndGrab(qmlFile, args, &screenShot, &errorMessage)) {
        consecutiveErrors = 0;
    } else {
        if (++consecutiveErrors >= 3)
            aborted = true;                   // Just give up if screen grabbing fails 3 times in a row
        QFAIL(qPrintable("QuickView grabbing failed: " + errorMessage));
    }

    QBASELINE_TEST(screenShot);
}


bool tst_Baseline_Controls::renderAndGrab(const QString& qmlFile, const QStringList& extraArgs, QImage *screenshot, QString *errMsg)
{
    bool usePipe = true;  // Whether to transport the grabbed image using temp. file or pipe. TBD: cmdline option
    QProcess grabber;
    grabber.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    QStringList args = extraArgs;
    QString tmpfile = usePipe ? QString("-") : QString("/tmp/qmlscenegrabber-%1-out.ppm").arg(QCoreApplication::applicationPid());
    args << qmlFile << "-o" << tmpfile;
    grabber.start(grabberPath, args, QIODevice::ReadOnly);
    grabber.waitForFinished(17000);         //### hardcoded, must be larger than the scene timeout in qmlscenegrabber
    if (grabber.state() != QProcess::NotRunning) {
        grabber.terminate();
        grabber.waitForFinished(3000);
    }
    QImage img;
    bool res = usePipe ? img.load(&grabber, "ppm") : img.load(tmpfile);
    if (!res || img.isNull()) {
        if (errMsg) {
            QString s("Failed to grab screen. qmlscenegrabber exitcode: %1. Process error: %2. Stderr:%3");
            *errMsg = s.arg(grabber.exitCode()).arg(grabber.errorString()).arg(blockify(grabber.readAllStandardError()));
        }
        if (!usePipe)
            QFile::remove(tmpfile);
        return false;
    }
    if (screenshot)
        *screenshot = img;
    if (!usePipe)
        QFile::remove(tmpfile);
    return true;
}

#define main _realmain
QTEST_MAIN(tst_Baseline_Controls)
#undef main

int main(int argc, char *argv[])
{
    QBaselineTest::handleCmdLineArgs(&argc, &argv);
    return _realmain(argc, argv);
}

#include "tst_baseline_controls.moc"
