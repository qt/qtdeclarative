// Copyright (C) 2016 Research In Motion.
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "conf.h"

#include <QCoreApplication>

#ifdef QT_GUI_LIB
#include <QGuiApplication>
#include <QWindow>
#include <QFileOpenEvent>
#include <QSurfaceFormat>
#ifdef QT_WIDGETS_LIB
#include <QApplication>
#endif // QT_WIDGETS_LIB
#endif // QT_GUI_LIB

#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QStringList>
#include <QScopedPointer>
#include <QDebug>
#include <QStandardPaths>
#include <QTranslator>
#include <QtGlobal>
#include <QLibraryInfo>
#include <qqml.h>
#include <qqmldebug.h>
#include <qqmlfileselector.h>

#include <private/qtqmlglobal_p.h>
#include <private/qqmlimport_p.h>
#if QT_CONFIG(qml_animation)
#include <private/qabstractanimation_p.h>
#endif

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <memory>

#define FILE_OPEN_EVENT_WAIT_TIME 3000 // ms

Q_LOGGING_CATEGORY(lcDeprecated, "qt.tools.qml.deprecated")

enum QmlApplicationType {
    QmlApplicationTypeUnknown
    , QmlApplicationTypeCore
#ifdef QT_GUI_LIB
    , QmlApplicationTypeGui
#ifdef QT_WIDGETS_LIB
    , QmlApplicationTypeWidget
#endif // QT_WIDGETS_LIB
#endif // QT_GUI_LIB
};

static QmlApplicationType applicationType =
#ifndef QT_GUI_LIB
    QmlApplicationTypeCore;
#else
    QmlApplicationTypeGui;
#endif // QT_GUI_LIB

static Config *conf = nullptr;
static QQmlApplicationEngine *qae = nullptr;
#if defined(Q_OS_DARWIN) || defined(QT_GUI_LIB)
static int exitTimerId = -1;
#endif
static const QString iconResourcePath(QStringLiteral(":/qt-project.org/imports/QmlRuntime/Config/resources/qml-64.png"));
static const QString confResourcePath(QStringLiteral(":/qt-project.org/imports/QmlRuntime/Config/"));
static const QString customConfFileName(QStringLiteral("configuration.qml"));
static bool verboseMode = false;
static bool quietMode = false;
static bool glShareContexts = true;
static bool disableShaderCache = true;
static bool requestAlphaChannel = false;
static bool requestMSAA = false;
static bool requestCoreProfile = false;

static void loadConf(const QString &override, bool quiet) // Terminates app on failure
{
    const QString defaultFileName = QLatin1String("default.qml");
    QUrl settingsUrl;
    bool builtIn = false; //just for keeping track of the warning
    if (override.isEmpty()) {
        QFileInfo fi;
        fi.setFile(QStandardPaths::locate(QStandardPaths::AppDataLocation, defaultFileName));
        if (fi.exists()) {
            settingsUrl = QQmlImports::urlFromLocalFileOrQrcOrUrl(fi.absoluteFilePath());
        } else {
            // ### If different built-in configs are needed per-platform, just apply QFileSelector to the qrc conf.qml path
            fi.setFile(confResourcePath + defaultFileName);
            settingsUrl = QQmlImports::urlFromLocalFileOrQrcOrUrl(fi.absoluteFilePath());
            builtIn = true;
        }
    } else {
        QFileInfo fi;
        fi.setFile(confResourcePath + override + QLatin1String(".qml"));
        if (fi.exists()) {
            settingsUrl = QQmlImports::urlFromLocalFileOrQrcOrUrl(fi.absoluteFilePath());
            builtIn = true;
        } else {
            fi.setFile(QDir(QStandardPaths::locate(QStandardPaths::AppConfigLocation, override, QStandardPaths::LocateDirectory)), customConfFileName);
            if (fi.exists())
                settingsUrl = QQmlImports::urlFromLocalFileOrQrcOrUrl(fi.absoluteFilePath());
            else
                fi.setFile(override);
            if (!fi.exists()) {
                printf("qml: Couldn't find required configuration file: %s\n",
                       qPrintable(QDir::toNativeSeparators(fi.absoluteFilePath())));
                exit(1);
            }
            settingsUrl = QQmlImports::urlFromLocalFileOrQrcOrUrl(fi.absoluteFilePath());
        }
    }

    if (!quiet) {
        printf("qml: %s\n", QLibraryInfo::build());
        if (builtIn) {
            printf("qml: Using built-in configuration: %s\n",
                   qPrintable(override.isEmpty() ? defaultFileName : override));
        } else {
            printf("qml: Using configuration: %s\n",
                    qPrintable(settingsUrl.isLocalFile()
                    ? QDir::toNativeSeparators(settingsUrl.toLocalFile())
                    : settingsUrl.toString()));
        }
    }

    // TODO: When we have better engine control, ban QtQuick* imports on this engine
    QQmlEngine e2;
    QQmlComponent c2(&e2, settingsUrl);
    conf = qobject_cast<Config*>(c2.create());

    if (!conf){
        printf("qml: Error loading configuration file: %s\n", qPrintable(c2.errorString()));
        exit(1);
    }
}

void noFilesGiven()
{
    if (!quietMode)
        printf("qml: No files specified. Terminating.\n");
    exit(1);
}

static void listConfFiles()
{
    const QDir confResourceDir(confResourcePath);
    printf("%s\n", qPrintable(QCoreApplication::translate("main", "Built-in configurations:")));
    for (const QFileInfo &fi : confResourceDir.entryInfoList(QDir::Files)) {
        if (fi.completeSuffix() != QLatin1String("qml"))
            continue;

        const QString baseName = fi.baseName();
        if (baseName.isEmpty() || baseName[0].isUpper())
            continue;

        printf("  %s\n", qPrintable(baseName));
    }
    printf("%s\n", qPrintable(QCoreApplication::translate("main", "Other configurations:")));
    bool foundOther = false;
    const QStringList otherLocations = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);
    for (const auto &confDirPath : otherLocations) {
        const QDir confDir(confDirPath);
        for (const QFileInfo &fi : confDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            foundOther = true;
            if (verboseMode)
                printf("  %s\n", qPrintable(fi.absoluteFilePath()));
            else
                printf("  %s\n", qPrintable(fi.baseName()));
        }
    }
    if (!foundOther)
        printf("  %s\n", qPrintable(QCoreApplication::translate("main", "none")));
    if (verboseMode) {
        printf("%s\n", qPrintable(QCoreApplication::translate("main", "Checked in:")));
        for (const auto &confDirPath : otherLocations)
            printf("  %s\n", qPrintable(confDirPath));
    }
    exit(0);
}

#ifdef QT_GUI_LIB

// Loads qml after receiving a QFileOpenEvent
class LoaderApplication : public QGuiApplication
{
public:
    LoaderApplication(int& argc, char **argv) : QGuiApplication(argc, argv)
    {
        setWindowIcon(QIcon(iconResourcePath));
    }

    bool event(QEvent *ev) override
    {
        if (ev->type() == QEvent::FileOpen) {
            if (exitTimerId >= 0) {
                killTimer(exitTimerId);
                exitTimerId = -1;
            }
            qae->load(static_cast<QFileOpenEvent *>(ev)->url());
        }
        else
            return QGuiApplication::event(ev);
        return true;
    }

    void timerEvent(QTimerEvent *) override {
        noFilesGiven();
    }
};

#endif // QT_GUI_LIB

// Listens to the appEngine signals to determine if all files failed to load
class LoadWatcher : public QObject
{
    Q_OBJECT
public:
    LoadWatcher(QQmlApplicationEngine *e, int expected)
        : QObject(e)
        , expectedFileCount(expected)
    {
        connect(e, &QQmlApplicationEngine::objectCreated, this, &LoadWatcher::checkFinished);
        // QQmlApplicationEngine also connects quit() to QCoreApplication::quit
        // and exit() to QCoreApplication::exit but if called before exec()
        // then QCoreApplication::quit or QCoreApplication::exit does nothing
        connect(e, &QQmlEngine::quit, this, &LoadWatcher::quit);
        connect(e, &QQmlEngine::exit, this, &LoadWatcher::exit);
    }

    int returnCode = 0;
    bool earlyExit = false;

public Q_SLOTS:
    void checkFinished(QObject *o, const QUrl &url)
    {
        Q_UNUSED(url);
        if (o) {
            ++createdObjects;
            if (conf && qae)
                for (PartialScene *ps : std::as_const(conf->completers))
                    if (o->inherits(ps->itemType().toUtf8().constData()))
                        contain(o, ps->container());
        }

        if (!--expectedFileCount && !createdObjects) {
            printf("qml: Did not load any objects, exiting.\n");
            exit(2);
            QCoreApplication::exit(2);
        }
    }

    void quit() {
        // Will be checked before calling exec()
        earlyExit = true;
        returnCode = 0;
    }
    void exit(int retCode) {
        earlyExit = true;
        returnCode = retCode;
    }

private:
    void contain(QObject *o, const QUrl &containPath);

private:
    int expectedFileCount;
    int createdObjects = 0;
};

void LoadWatcher::contain(QObject *o, const QUrl &containPath)
{
    QQmlComponent c(qae, containPath);
    QObject *o2 = c.create();
    if (!o2)
        return;
    o2->setParent(this);
    bool success = false;
    int idx;
    if ((idx = o2->metaObject()->indexOfProperty("containedObject")) != -1)
        success = o2->metaObject()->property(idx).write(o2, QVariant::fromValue<QObject*>(o));
    if (!success)
        o->setParent(o2); // Set QObject parent, and assume container will react as needed
}

void quietMessageHandler(QtMsgType type, const QMessageLogContext &ctxt, const QString &msg)
{
    Q_UNUSED(ctxt);
    Q_UNUSED(msg);
    // Doesn't print anything
    switch (type) {
    case QtFatalMsg:
        exit(-1);
    case QtCriticalMsg:
    case QtDebugMsg:
    case QtInfoMsg:
    case QtWarningMsg:
        ;
    }
}

// Called before application initialization
static void getAppFlags(int argc, char **argv)
{
#ifdef QT_GUI_LIB
    for (int i=0; i<argc; i++) {
        if (!strcmp(argv[i], "--apptype") || !strcmp(argv[i], "-a") || !strcmp(argv[i], "-apptype")) {
            applicationType = QmlApplicationTypeUnknown;
            if (i+1 < argc) {
                ++i;
                if (!strcmp(argv[i], "core"))
                    applicationType = QmlApplicationTypeCore;
                else if (!strcmp(argv[i], "gui"))
                    applicationType = QmlApplicationTypeGui;
#  ifdef QT_WIDGETS_LIB
                else if (!strcmp(argv[i], "widget"))
                    applicationType = QmlApplicationTypeWidget;
#  endif // QT_WIDGETS_LIB

            }
        } else if (!strcmp(argv[i], "-desktop") || !strcmp(argv[i], "--desktop")) {
            QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
        } else if (!strcmp(argv[i], "-gles") || !strcmp(argv[i], "--gles")) {
            QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
        } else if (!strcmp(argv[i], "-software") || !strcmp(argv[i], "--software")) {
            QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
        } else if (!strcmp(argv[i], "-disable-context-sharing") || !strcmp(argv[i], "--disable-context-sharing")) {
            glShareContexts = false;
        } else if (!strcmp(argv[i], "-enable-shader-cache") || !strcmp(argv[i], "--enable-shader-cache")) {
            disableShaderCache = false;
        } else if (!strcmp(argv[i], "-transparent") || !strcmp(argv[i], "--transparent")) {
            requestAlphaChannel = true;
        } else if (!strcmp(argv[i], "-multisample") || !strcmp(argv[i], "--multisample")) {
            requestMSAA = true;
        } else if (!strcmp(argv[i], "-core-profile") || !strcmp(argv[i], "--core-profile")) {
            requestCoreProfile = true;
        }
    }
#else
    Q_UNUSED(argc);
    Q_UNUSED(argv);
#endif // QT_GUI_LIB
}

#if QT_DEPRECATED_SINCE(6, 3)
static void loadDummyDataFiles(QQmlEngine &engine, const QString& directory)
{
    QDir dir(directory+"/dummydata", "*.qml");
    QStringList list = dir.entryList();
    for (int i = 0; i < list.size(); ++i) {
        QString qml = list.at(i);
        QQmlComponent comp(&engine, dir.filePath(qml));
        QObject *dummyData = comp.create();

        if (comp.isError()) {
            const QList<QQmlError> errors = comp.errors();
            for (const QQmlError &error : errors)
                qWarning() << error;
        }

        if (dummyData && !quietMode) {
            printf("qml: Loaded dummy data: %s\n",  qPrintable(dir.filePath(qml)));
            qml.truncate(qml.size()-4);
            engine.rootContext()->setContextProperty(qml, dummyData);
            dummyData->setParent(&engine);
        }
    }
}
#endif

int main(int argc, char *argv[])
{
    getAppFlags(argc, argv);

    // Must set the default QSurfaceFormat before creating the app object if
    // AA_ShareOpenGLContexts is going to be set.
#if defined(QT_GUI_LIB)
    QSurfaceFormat surfaceFormat;
    surfaceFormat.setDepthBufferSize(24);
    surfaceFormat.setStencilBufferSize(8);
    if (requestMSAA)
        surfaceFormat.setSamples(4);
    if (requestAlphaChannel)
        surfaceFormat.setAlphaBufferSize(8);
    if (qEnvironmentVariableIsSet("QSG_CORE_PROFILE")
            || qEnvironmentVariableIsSet("QML_CORE_PROFILE")
            || requestCoreProfile)
    {
        // intentionally requesting 4.1 core to play nice with macOS
        surfaceFormat.setVersion(4, 1);
        surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    }
    QSurfaceFormat::setDefaultFormat(surfaceFormat);
#endif

    if (glShareContexts)
        QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    if (disableShaderCache)
        QCoreApplication::setAttribute(Qt::AA_DisableShaderDiskCache);

    std::unique_ptr<QCoreApplication> app;
    switch (applicationType) {
#ifdef QT_GUI_LIB
    case QmlApplicationTypeGui:
        app = std::make_unique<LoaderApplication>(argc, argv);
        break;
#ifdef QT_WIDGETS_LIB
    case QmlApplicationTypeWidget:
        app = std::make_unique<QApplication>(argc, argv);
        static_cast<QApplication *>(app.get())->setWindowIcon(QIcon(iconResourcePath));
        break;
#endif // QT_WIDGETS_LIB
#endif // QT_GUI_LIB
    case QmlApplicationTypeCore:
        Q_FALLTHROUGH();
    default: // QmlApplicationTypeUnknown: not allowed, but we'll exit after checking apptypeOption below
        app = std::make_unique<QCoreApplication>(argc, argv);
        break;
    }

    app->setApplicationName("Qml Runtime");
    app->setOrganizationName("QtProject");
    app->setOrganizationDomain("qt-project.org");
    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));

    QStringList files;
    QString confFile;
    QString translationFile;

    // Handle main arguments
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
    parser.addHelpOption();
    parser.addVersionOption();
#ifdef QT_GUI_LIB
    QCommandLineOption apptypeOption(QStringList() << QStringLiteral("a") << QStringLiteral("apptype"),
        QCoreApplication::translate("main", "Select which application class to use. Default is gui."),
#ifdef QT_WIDGETS_LIB
        QStringLiteral("core|gui|widget"));
#else
        QStringLiteral("core|gui"));
#endif // QT_WIDGETS_LIB
    parser.addOption(apptypeOption); // Just for the help text... we've already handled this argument above
#endif // QT_GUI_LIB
    QCommandLineOption importOption(QStringLiteral("I"),
        QCoreApplication::translate("main", "Prepend the given path to the import paths."), QStringLiteral("path"));
    parser.addOption(importOption);
    QCommandLineOption qmlFileOption(QStringLiteral("f"),
        QCoreApplication::translate("main", "Load the given file as a QML file."), QStringLiteral("file"));
    parser.addOption(qmlFileOption);
    QCommandLineOption configOption(QStringList() << QStringLiteral("c") << QStringLiteral("config"),
        QCoreApplication::translate("main", "Load the given built-in configuration or configuration file."), QStringLiteral("file"));
    parser.addOption(configOption);
    QCommandLineOption listConfOption(QStringList() << QStringLiteral("list-conf"),
                                      QCoreApplication::translate("main", "List the built-in configurations."));
    parser.addOption(listConfOption);
    QCommandLineOption translationOption(QStringLiteral("translation"),
        QCoreApplication::translate("main", "Load the given file as the translations file."), QStringLiteral("file"));
    parser.addOption(translationOption);
#if QT_DEPRECATED_SINCE(6, 3)
    QCommandLineOption dummyDataOption(QStringLiteral("dummy-data"),
        QCoreApplication::translate("main", "Load QML files from the given directory as context properties. (deprecated)"), QStringLiteral("file"));
    parser.addOption(dummyDataOption);
#endif
#ifdef QT_GUI_LIB
    // OpenGL options
    QCommandLineOption glDesktopOption(QStringLiteral("desktop"),
        QCoreApplication::translate("main", "Force use of desktop OpenGL (AA_UseDesktopOpenGL)."));
    parser.addOption(glDesktopOption); // Just for the help text... we've already handled this argument above
    QCommandLineOption glEsOption(QStringLiteral("gles"),
        QCoreApplication::translate("main", "Force use of GLES (AA_UseOpenGLES)."));
    parser.addOption(glEsOption); // Just for the help text... we've already handled this argument above
    QCommandLineOption glSoftwareOption(QStringLiteral("software"),
        QCoreApplication::translate("main", "Force use of software rendering (AA_UseSoftwareOpenGL)."));
    parser.addOption(glSoftwareOption); // Just for the help text... we've already handled this argument above
    QCommandLineOption glCoreProfile(QStringLiteral("core-profile"),
        QCoreApplication::translate("main", "Force use of OpenGL Core Profile."));
    parser.addOption(glCoreProfile); // Just for the help text... we've already handled this argument above
    QCommandLineOption glContextSharing(QStringLiteral("disable-context-sharing"),
        QCoreApplication::translate("main", "Disable the use of a shared GL context for QtQuick Windows"));
    parser.addOption(glContextSharing); // Just for the help text... we've already handled this argument above
    // Options relevant for other 3D APIs as well
    QCommandLineOption shaderCaching(QStringLiteral("enable-shader-cache"),
        QCoreApplication::translate("main", "Enable persistent caching of generated shaders"));
    parser.addOption(shaderCaching); // Just for the help text... we've already handled this argument above
    QCommandLineOption transparentOption(QStringLiteral("transparent"),
        QCoreApplication::translate("main", "Requests an alpha channel in order to enable semi-transparent windows."));
    parser.addOption(transparentOption); // Just for the help text... we've already handled this argument above
    QCommandLineOption multisampleOption(QStringLiteral("multisample"),
        QCoreApplication::translate("main", "Requests 4x multisample antialiasing."));
    parser.addOption(multisampleOption); // Just for the help text... we've already handled this argument above
#endif // QT_GUI_LIB

    // Debugging and verbosity options
    QCommandLineOption quietOption(QStringLiteral("quiet"),
        QCoreApplication::translate("main", "Suppress all output."));
    parser.addOption(quietOption);
    QCommandLineOption verboseOption(QStringLiteral("verbose"),
        QCoreApplication::translate("main", "Print information about what qml is doing, like specific file URLs being loaded."));
    parser.addOption(verboseOption);
    QCommandLineOption slowAnimationsOption(QStringLiteral("slow-animations"),
        QCoreApplication::translate("main", "Run all animations in slow motion."));
    parser.addOption(slowAnimationsOption);
    QCommandLineOption fixedAnimationsOption(QStringLiteral("fixed-animations"),
        QCoreApplication::translate("main", "Run animations off animation tick rather than wall time."));
    parser.addOption(fixedAnimationsOption);
    QCommandLineOption rhiOption(QStringList() << QStringLiteral("r") << QStringLiteral("rhi"),
        QCoreApplication::translate("main", "Set the backend for the Qt graphics abstraction (RHI). "
                                    "Backend is one of: default, vulkan, metal, d3d11, gl"),
                                 QStringLiteral("backend"));
    parser.addOption(rhiOption);
    QCommandLineOption selectorOption(QStringLiteral("S"), QCoreApplication::translate("main",
        "Add selector to the list of QQmlFileSelectors."), QStringLiteral("selector"));
    parser.addOption(selectorOption);

    // Positional arguments
    parser.addPositionalArgument("files",
        QCoreApplication::translate("main", "Any number of QML files can be loaded. They will share the same engine."), "[files...]");
    parser.addPositionalArgument("args",
        QCoreApplication::translate("main", "Arguments after '--' are ignored, but passed through to the application.arguments variable in QML."), "[-- args...]");

    parser.process(*app);
    if (parser.isSet(verboseOption))
        verboseMode = true;
    if (parser.isSet(quietOption)) {
        quietMode = true;
        verboseMode = false;
    }
    if (parser.isSet(listConfOption))
        listConfFiles();
    if (applicationType == QmlApplicationTypeUnknown) {
#ifdef QT_WIDGETS_LIB
        qWarning() << QCoreApplication::translate("main", "--apptype must be followed by one of the following: core gui widget\n");
#else
        qWarning() << QCoreApplication::translate("main", "--apptype must be followed by one of the following: core gui\n");
#endif // QT_WIDGETS_LIB
        parser.showHelp();
    }
#if QT_CONFIG(qml_animation)
    if (parser.isSet(slowAnimationsOption))
        QUnifiedTimer::instance()->setSlowModeEnabled(true);
    if (parser.isSet(fixedAnimationsOption))
        QUnifiedTimer::instance()->setConsistentTiming(true);
#endif

    QQmlApplicationEngine e;

    for (const QString &importPath : parser.values(importOption))
        e.addImportPath(importPath);

    QStringList customSelectors;
    for (const QString &selector : parser.values(selectorOption))
        customSelectors.append(selector);

    if (!customSelectors.isEmpty())
        e.setExtraFileSelectors(customSelectors);

    files << parser.values(qmlFileOption);
    if (parser.isSet(configOption))
        confFile = parser.value(configOption);
    if (parser.isSet(translationOption))
        translationFile = parser.value(translationOption);
    if (parser.isSet(rhiOption)) {
        const QString rhiBackend = parser.value(rhiOption);
        if (rhiBackend == QLatin1String("default"))
            qunsetenv("QSG_RHI_BACKEND");
        else
            qputenv("QSG_RHI_BACKEND", rhiBackend.toLatin1());
    }
    for (QString posArg : parser.positionalArguments()) {
        if (posArg == QLatin1String("--"))
            break;
        else
            files << posArg;
    }

#if QT_CONFIG(translation)
    // Need to be installed before QQmlApplicationEngine's automatic translation loading
    // (qt_ translations are loaded there)
    QTranslator translator;
    if (!translationFile.isEmpty()) {
        if (translator.load(translationFile)) {
            app->installTranslator(&translator);
            if (verboseMode)
                printf("qml: Loaded translation file %s\n", qPrintable(QDir::toNativeSeparators(translationFile)));
        } else {
            if (!quietMode)
                printf("qml: Could not load the translation file %s\n", qPrintable(QDir::toNativeSeparators(translationFile)));
        }
    }
#else
    if (!translationFile.isEmpty() && !quietMode)
        printf("qml: Translation file specified, but Qt built without translation support.\n");
#endif

    if (quietMode) {
        qInstallMessageHandler(quietMessageHandler);
        QLoggingCategory::setFilterRules(QStringLiteral("*=false"));
    }

    if (files.size() <= 0) {
#if defined(Q_OS_DARWIN) && defined(QT_GUI_LIB)
        if (applicationType == QmlApplicationTypeGui)
            exitTimerId = static_cast<LoaderApplication *>(app.get())->startTimer(FILE_OPEN_EVENT_WAIT_TIME);
        else
#endif
        noFilesGiven();
    }

    qae = &e;
    loadConf(confFile, !verboseMode);

    // Load files
    QScopedPointer<LoadWatcher> lw(new LoadWatcher(&e, files.size()));

#if QT_DEPRECATED_SINCE(6, 3)
    QString dummyDir;
    if (parser.isSet(dummyDataOption))
        dummyDir = parser.value(dummyDataOption);
    // Load dummy data before loading QML-files
    if (!dummyDir.isEmpty() && QFileInfo (dummyDir).isDir()) {
        qCWarning(lcDeprecated()) << "Warning: the qml --dummy-data option is deprecated and will be removed in a future version of Qt.";
        loadDummyDataFiles(e, dummyDir);
    }
#endif

    for (const QString &path : std::as_const(files)) {
        QUrl url = QUrl::fromUserInput(path, QDir::currentPath(), QUrl::AssumeLocalFile);
        if (verboseMode)
            printf("qml: loading %s\n", qPrintable(url.toString()));
        e.load(url);
    }

    if (lw->earlyExit)
        return lw->returnCode;

    return app->exec();
}

#include "main.moc"
