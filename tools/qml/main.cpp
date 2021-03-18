/****************************************************************************
**
** Copyright (C) 2016 Research In Motion.
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "conf.h"

#include <QCoreApplication>

#ifdef QT_GUI_LIB
#include <QGuiApplication>
#include <QWindow>
#include <QFileOpenEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
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

#include <private/qmemory_p.h>
#include <private/qtqmlglobal_p.h>
#if QT_CONFIG(qml_animation)
#include <private/qabstractanimation_p.h>
#endif

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <memory>

#define FILE_OPEN_EVENT_WAIT_TIME 3000 // ms

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
static const QString iconResourcePath(QStringLiteral(":/qt-project.org/QmlRuntime/resources/qml-64.png"));
static const QString confResourcePath(QStringLiteral(":/qt-project.org/QmlRuntime/conf/"));
static bool verboseMode = false;
static bool quietMode = false;

static void loadConf(const QString &override, bool quiet) // Terminates app on failure
{
    const QString defaultFileName = QLatin1String("default.qml");
    QUrl settingsUrl;
    bool builtIn = false; //just for keeping track of the warning
    if (override.isEmpty()) {
        QFileInfo fi;
        fi.setFile(QStandardPaths::locate(QStandardPaths::DataLocation, defaultFileName));
        if (fi.exists()) {
            settingsUrl = QUrl::fromLocalFile(fi.absoluteFilePath());
        } else {
            // ### If different built-in configs are needed per-platform, just apply QFileSelector to the qrc conf.qml path
            fi.setFile(confResourcePath + defaultFileName);
            settingsUrl = QUrl::fromLocalFile(fi.absoluteFilePath());
            builtIn = true;
        }
    } else {
        QFileInfo fi;
        fi.setFile(confResourcePath + override + QLatin1String(".qml"));
        if (fi.exists()) {
            settingsUrl = QUrl::fromLocalFile(fi.absoluteFilePath());
            builtIn = true;
        } else {
            fi.setFile(override);
            if (!fi.exists()) {
                printf("qml: Couldn't find required configuration file: %s\n",
                       qPrintable(QDir::toNativeSeparators(fi.absoluteFilePath())));
                exit(1);
            }
            settingsUrl = QUrl::fromLocalFile(fi.absoluteFilePath());
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
    QDir confResourceDir(confResourcePath);
    printf("%s\n", qPrintable(QCoreApplication::translate("main", "Built-in configurations:")));
    for (const QFileInfo &fi : confResourceDir.entryInfoList(QDir::Files))
        printf("  %s\n", qPrintable(fi.baseName()));
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
        Q_UNUSED(url)
        if (o) {
            checkForWindow(o);
            if (conf && qae)
                for (PartialScene *ps : qAsConst(conf->completers))
                    if (o->inherits(ps->itemType().toUtf8().constData()))
                        contain(o, ps->container());
        }
        if (haveWindow)
            return;

        if (! --expectedFileCount) {
            printf("qml: Did not load any objects, exiting.\n");
            std::exit(2); // Different return code from qFatal
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

#if defined(QT_GUI_LIB) && QT_CONFIG(opengl)
    void onOpenGlContextCreated(QOpenGLContext *context);
#endif

private:
    void contain(QObject *o, const QUrl &containPath);
    void checkForWindow(QObject *o);

private:
    bool haveWindow = false;
    int expectedFileCount;
};

void LoadWatcher::contain(QObject *o, const QUrl &containPath)
{
    QQmlComponent c(qae, containPath);
    QObject *o2 = c.create();
    if (!o2)
        return;
    checkForWindow(o2);
    bool success = false;
    int idx;
    if ((idx = o2->metaObject()->indexOfProperty("containedObject")) != -1)
        success = o2->metaObject()->property(idx).write(o2, QVariant::fromValue<QObject*>(o));
    if (!success)
        o->setParent(o2); // Set QObject parent, and assume container will react as needed
}

void LoadWatcher::checkForWindow(QObject *o)
{
#if defined(QT_GUI_LIB) && QT_CONFIG(opengl)
    if (o->isWindowType() && o->inherits("QQuickWindow")) {
        haveWindow = true;
        if (verboseMode)
            connect(o, SIGNAL(openglContextCreated(QOpenGLContext*)),
                    this, SLOT(onOpenGlContextCreated(QOpenGLContext*)));
    }
#else
    Q_UNUSED(o)
#endif // QT_GUI_LIB && !QT_NO_OPENGL
}

#if defined(QT_GUI_LIB) && QT_CONFIG(opengl)
void LoadWatcher::onOpenGlContextCreated(QOpenGLContext *context)
{
    context->makeCurrent(qobject_cast<QWindow *>(sender()));
    QOpenGLFunctions functions(context);
    QByteArray output = "Vendor  : ";
    output += reinterpret_cast<const char *>(functions.glGetString(GL_VENDOR));
    output += "\nRenderer: ";
    output += reinterpret_cast<const char *>(functions.glGetString(GL_RENDERER));
    output += "\nVersion : ";
    output += reinterpret_cast<const char *>(functions.glGetString(GL_VERSION));
    output += "\nLanguage: ";
    output += reinterpret_cast<const char *>(functions.glGetString(GL_SHADING_LANGUAGE_VERSION));
    puts(output.constData());
    context->doneCurrent();
}
#endif // QT_GUI_LIB && !QT_NO_OPENGL

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
        } else if (!strcmp(argv[i], "-scaling") || !strcmp(argv[i], "--scaling")) {
            QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        } else if (!strcmp(argv[i], "-no-scaling") || !strcmp(argv[i], "--no-scaling")) {
            QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
        }
    }
#else
    Q_UNUSED(argc)
    Q_UNUSED(argv)
#endif // QT_GUI_LIB
}

bool getFileSansBangLine(const QString &path, QByteArray &output)
{
    QFile f(path);
    if (!f.open(QFile::ReadOnly | QFile::Text))
        return false;
    output = f.readAll();
    if (output.startsWith("#!")) {//Remove first line in this case (except \n, to avoid disturbing line count)
        output.remove(0, output.indexOf('\n'));
        return true;
    }
    return false;
}

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
            qml.truncate(qml.length()-4);
            engine.rootContext()->setContextProperty(qml, dummyData);
            dummyData->setParent(&engine);
        }
    }
}

int main(int argc, char *argv[])
{
    getAppFlags(argc, argv);
    std::unique_ptr<QCoreApplication> app;
    switch (applicationType) {
#ifdef QT_GUI_LIB
    case QmlApplicationTypeGui:
        app = qt_make_unique<LoaderApplication>(argc, argv);
        break;
#ifdef QT_WIDGETS_LIB
    case QmlApplicationTypeWidget:
        app = qt_make_unique<QApplication>(argc, argv);
        static_cast<QApplication *>(app.get())->setWindowIcon(QIcon(iconResourcePath));
        break;
#endif // QT_WIDGETS_LIB
#endif // QT_GUI_LIB
    case QmlApplicationTypeCore:
        Q_FALLTHROUGH();
    default: // QmlApplicationTypeUnknown: not allowed, but we'll exit after checking apptypeOption below
        app = qt_make_unique<QCoreApplication>(argc, argv);
        break;
    }

    app->setApplicationName("Qml Runtime");
    app->setOrganizationName("QtProject");
    app->setOrganizationDomain("qt-project.org");
    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));

    QQmlApplicationEngine e;
    QStringList files;
    QString confFile;
    QString translationFile;
    QString dummyDir;

    // Handle main arguments
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();
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
    QCommandLineOption dummyDataOption(QStringLiteral("dummy-data"),
        QCoreApplication::translate("main", "Load QML files from the given directory as context properties."), QStringLiteral("file"));
    parser.addOption(dummyDataOption);
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
    QCommandLineOption scalingOption(QStringLiteral("scaling"),
        QCoreApplication::translate("main", "Enable High DPI scaling (AA_EnableHighDpiScaling)."));
    parser.addOption(scalingOption); // Just for the help text... we've already handled this argument above
    QCommandLineOption noScalingOption(QStringLiteral("no-scaling"),
        QCoreApplication::translate("main", "Disable High DPI scaling (AA_DisableHighDpiScaling)."));
    parser.addOption(noScalingOption); // Just for the help text... we've already handled this argument above
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
        QCoreApplication::translate("main", "Use the Qt graphics abstraction (RHI) instead of OpenGL directly. "
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

    if (!parser.parse(QCoreApplication::arguments())) {
        qWarning() << parser.errorText();
        exit(1);
    }
    if (parser.isSet(versionOption))
        parser.showVersion();
    if (parser.isSet(helpOption))
        parser.showHelp();
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
    if (parser.isSet(verboseOption))
        verboseMode = true;
    if (parser.isSet(quietOption)) {
        quietMode = true;
        verboseMode = false;
    }
#if QT_CONFIG(qml_animation)
    if (parser.isSet(slowAnimationsOption))
        QUnifiedTimer::instance()->setSlowModeEnabled(true);
    if (parser.isSet(fixedAnimationsOption))
        QUnifiedTimer::instance()->setConsistentTiming(true);
#endif
    for (const QString &importPath : parser.values(importOption))
        e.addImportPath(importPath);

    QStringList customSelectors;
    for (const QString &selector : parser.values(selectorOption))
        customSelectors.append(selector);
    if (!customSelectors.isEmpty()) {
        QQmlFileSelector *selector =  QQmlFileSelector::get(&e);
        selector->setExtraSelectors(customSelectors);
    }

#if defined(QT_GUI_LIB) && QT_CONFIG(opengl)
    if (qEnvironmentVariableIsSet("QSG_CORE_PROFILE") || qEnvironmentVariableIsSet("QML_CORE_PROFILE")) {
        QSurfaceFormat surfaceFormat;
        surfaceFormat.setStencilBufferSize(8);
        surfaceFormat.setDepthBufferSize(24);
        surfaceFormat.setVersion(4, 1);
        surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
        QSurfaceFormat::setDefaultFormat(surfaceFormat);
    }
#endif

    files << parser.values(qmlFileOption);
    if (parser.isSet(configOption))
        confFile = parser.value(configOption);
    if (parser.isSet(translationOption))
        translationFile = parser.value(translationOption);
    if (parser.isSet(dummyDataOption))
        dummyDir = parser.value(dummyDataOption);
    if (parser.isSet(rhiOption)) {
        qputenv("QSG_RHI", "1");
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
    if (!translationFile.isEmpty()) {
        QTranslator translator;

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

    if (files.count() <= 0) {
#if defined(Q_OS_DARWIN)
        if (applicationType == QmlApplicationTypeGui)
            exitTimerId = static_cast<LoaderApplication *>(app.get())->startTimer(FILE_OPEN_EVENT_WAIT_TIME);
        else
#endif
        noFilesGiven();
    }

    qae = &e;
    loadConf(confFile, !verboseMode);

    // Load files
    QScopedPointer<LoadWatcher> lw(new LoadWatcher(&e, files.count()));

    // Load dummy data before loading QML-files
    if (!dummyDir.isEmpty() && QFileInfo (dummyDir).isDir())
        loadDummyDataFiles(e, dummyDir);

    for (const QString &path : qAsConst(files)) {
        QUrl url = QUrl::fromUserInput(path, QDir::currentPath(), QUrl::AssumeLocalFile);
        if (verboseMode)
            printf("qml: loading %s\n", qPrintable(url.toString()));
        QByteArray strippedFile;
        if (getFileSansBangLine(path, strippedFile))
            // QQmlComponent won't resolve it for us: it doesn't know it's a valid file if we loadData
            e.loadData(strippedFile, e.baseUrl().resolved(url));
        else // Errors or no bang line
            e.load(url);
    }

    if (lw->earlyExit)
        return lw->returnCode;

    return app->exec();
}

#include "main.moc"
