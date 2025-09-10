// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qabstractanimation.h>
#include <QtCore/qdir.h>
#include <QtCore/qmath.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qpointer.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qloggingcategory.h>

#include <QtGui/QGuiApplication>

#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlfileselector.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>

#include <private/qabstractanimation_p.h>

#ifdef QT_WIDGETS_LIB
#include <QtWidgets/QApplication>
#if QT_CONFIG(filedialog)
#include <QtWidgets/QFileDialog>
#endif // QT_CONFIG(filedialog)
#endif // QT_WIDGETS_LIB

#include <QtCore/QTranslator>
#include <QtCore/QLibraryInfo>

Q_LOGGING_CATEGORY(lcQmlsceneDeprecated, "qt.tools.qmlscene.deprecated")

#ifdef QML_RUNTIME_TESTING
class RenderStatistics
{
public:
    static void updateStats();
    static void printTotalStats();
private:
    static QVector<qreal> timePerFrame;
    static QVector<int> timesPerFrames;
};

QVector<qreal> RenderStatistics::timePerFrame;
QVector<int> RenderStatistics::timesPerFrames;

void RenderStatistics::updateStats()
{
    static QElapsedTimer time;
    static int frames;
    static int lastTime;

    if (frames == 0) {
        time.start();
    } else {
        int elapsed = time.elapsed();
        timesPerFrames.append(elapsed - lastTime);
        lastTime = elapsed;

        if (elapsed > 5000) {
            qreal avgtime = elapsed / (qreal) frames;
            qreal var = 0;
            for (int i = 0; i < timesPerFrames.size(); ++i) {
                qreal diff = timesPerFrames.at(i) - avgtime;
                var += diff * diff;
            }
            var /= timesPerFrames.size();

            printf("Average time per frame: %f ms (%i fps), std.dev: %f ms\n", avgtime, qRound(1000. / avgtime), qSqrt(var));

            timePerFrame.append(avgtime);
            timesPerFrames.clear();
            time.start();
            lastTime = 0;
            frames = 0;
        }
    }
    ++frames;
}

void RenderStatistics::printTotalStats()
{
    int count = timePerFrame.size();
    if (count == 0)
        return;

    qreal minTime = 0;
    qreal maxTime = 0;
    qreal avg = 0;
    for (int i = 0; i < count; ++i) {
        minTime = minTime == 0 ? timePerFrame.at(i) : qMin(minTime, timePerFrame.at(i));
        maxTime = qMax(maxTime, timePerFrame.at(i));
        avg += timePerFrame.at(i);
    }
    avg /= count;

    puts(" ");
    puts("----- Statistics -----");
    printf("Average time per frame: %f ms (%i fps)\n", avg, qRound(1000. / avg));
    printf("Best time per frame: %f ms (%i fps)\n", minTime, int(1000 / minTime));
    printf("Worst time per frame: %f ms (%i fps)\n", maxTime, int(1000 / maxTime));
    puts("----------------------");
    puts(" ");
}
#endif

struct Options
{
    enum QmlApplicationType
    {
        QmlApplicationTypeGui,
        QmlApplicationTypeWidget,
#ifdef QT_WIDGETS_LIB
        DefaultQmlApplicationType = QmlApplicationTypeWidget
#else
        DefaultQmlApplicationType = QmlApplicationTypeGui
#endif
    };

    Options()
        : textRenderType(QQuickWindow::textRenderType())
    {
        // QtWebEngine needs a shared context in order for the GPU thread to
        // upload textures.
        applicationAttributes.append(Qt::AA_ShareOpenGLContexts);
    }

    QUrl url;
    bool originalQml = false;
    bool originalQmlRaster = false;
    bool maximized = false;
    bool fullscreen = false;
    bool transparent = false;
    bool clip = false;
    bool versionDetection = true;
    bool slowAnimations = false;
    bool quitImmediately = false;
    bool resizeViewToRootItem = false;
    bool multisample = false;
    bool coreProfile = false;
    bool verbose = false;
    bool rhi = false;
    bool rhiBackendSet = false;
    QVector<Qt::ApplicationAttribute> applicationAttributes;
    QString translationFile;
    QmlApplicationType applicationType = DefaultQmlApplicationType;
    QQuickWindow::TextRenderType textRenderType;
    QString rhiBackend;
};

#if defined(QMLSCENE_BUNDLE)
QFileInfoList findQmlFiles(const QString &dirName)
{
    QDir dir(dirName);

    QFileInfoList ret;
    if (dir.exists()) {
        const QFileInfoList fileInfos = dir.entryInfoList(QStringList() << "*.qml",
                                                          QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);

        for (const QFileInfo &fileInfo : fileInfos) {
            if (fileInfo.isDir())
                ret += findQmlFiles(fileInfo.filePath());
            else if (fileInfo.fileName().length() > 0 && fileInfo.fileName().at(0).isLower())
                ret.append(fileInfo);
        }
    }

    return ret;
}

static int displayOptionsDialog(Options *options)
{
    QDialog dialog;

    QFormLayout *layout = new QFormLayout(&dialog);

    QComboBox *qmlFileComboBox = new QComboBox(&dialog);
    const QFileInfoList fileInfos = findQmlFiles(":/bundle") + findQmlFiles("./qmlscene-resources");

    for (const QFileInfo &fileInfo : fileInfos)
        qmlFileComboBox->addItem(fileInfo.dir().dirName() + QLatin1Char('/') + fileInfo.fileName(), QVariant::fromValue(fileInfo));

    QCheckBox *originalCheckBox = new QCheckBox(&dialog);
    originalCheckBox->setText("Use original QML viewer");
    originalCheckBox->setChecked(options->originalQml);

    QCheckBox *fullscreenCheckBox = new QCheckBox(&dialog);
    fullscreenCheckBox->setText("Start fullscreen");
    fullscreenCheckBox->setChecked(options->fullscreen);

    QCheckBox *maximizedCheckBox = new QCheckBox(&dialog);
    maximizedCheckBox->setText("Start maximized");
    maximizedCheckBox->setChecked(options->maximized);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal,
                                                       &dialog);
    QObject::connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    layout->addRow("Qml file:", qmlFileComboBox);
    layout->addWidget(originalCheckBox);
    layout->addWidget(maximizedCheckBox);
    layout->addWidget(fullscreenCheckBox);
    layout->addWidget(buttonBox);

    int result = dialog.exec();
    if (result == QDialog::Accepted) {
        QVariant variant = qmlFileComboBox->itemData(qmlFileComboBox->currentIndex());
        QFileInfo fileInfo = variant.value<QFileInfo>();

        if (fileInfo.canonicalFilePath().startsWith(QLatin1Char(':')))
            options->file = QUrl("qrc" + fileInfo.canonicalFilePath());
        else
            options->file = QUrl::fromLocalFile(fileInfo.canonicalFilePath());
        options->originalQml = originalCheckBox->isChecked();
        options->maximized = maximizedCheckBox->isChecked();
        options->fullscreen = fullscreenCheckBox->isChecked();
    }
    return result;
}
#endif

static bool checkVersion(const QUrl &url)
{
    if (!qgetenv("QMLSCENE_IMPORT_NAME").isEmpty())
        fprintf(stderr, "QMLSCENE_IMPORT_NAME is no longer supported.\n");

    if (!url.isLocalFile())
        return true;

    const QString fileName = url.toLocalFile();
    QFile f(fileName);
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        fprintf(stderr, "qmlscene: failed to check version of file '%s', could not open...\n",
                 qPrintable(fileName));
        return false;
    }

    QRegularExpression quick1("^\\s*import +QtQuick +1\\.\\w*");
    QRegularExpression qt47("^\\s*import +Qt +4\\.7");

    QTextStream stream(&f);
    bool codeFound= false;
    while (!codeFound) {
        if (stream.atEnd()) {
            fprintf(stderr, "qmlscene: no code found in file '%s'.\n", qPrintable(fileName));
            return false;
        }
        QString line = stream.readLine();
        if (line.contains(QLatin1Char('{'))) {
            codeFound = true;
        } else {
            QString import;
            QRegularExpressionMatch match = quick1.match(line);
            if (match.hasMatch())
                import = match.captured(0).trimmed();
            else if ((match = qt47.match(line)).hasMatch())
                import = match.captured(0).trimmed();

            if (!import.isNull()) {
                fprintf(stderr, "qmlscene: '%s' is no longer supported.\n"
                         "Use qmlviewer to load file '%s'.\n",
                         qPrintable(import),
                         qPrintable(fileName));
                return false;
            }
        }
    }

    return true;
}

static void displayFileDialog(Options *options)
{
#if defined(QT_WIDGETS_LIB) && QT_CONFIG(filedialog)
    if (options->applicationType == Options::QmlApplicationTypeWidget) {
        QString fileName = QFileDialog::getOpenFileName(nullptr, "Open QML file", QString(), "QML Files (*.qml)");
        if (!fileName.isEmpty()) {
            QFileInfo fi(fileName);
            options->url = QUrl::fromLocalFile(fi.canonicalFilePath());
        }
        return;
    }
#endif // QT_WIDGETS_LIB && QT_CONFIG(filedialog)
    Q_UNUSED(options);
    puts("No filename specified...");
}

static void loadDummyDataFiles(QQmlEngine &engine, const QString& directory)
{
    QDir dir(directory+"/dummydata", "*.qml");
    QStringList list = dir.entryList();
    for (int i = 0; i < list.size(); ++i) {
        QString qml = list.at(i);
        QQmlComponent comp(&engine, dir.filePath(qml));
        QObject *dummyData = comp.create();

        if(comp.isError()) {
            const QList<QQmlError> errors = comp.errors();
            for (const QQmlError &error : errors)
                fprintf(stderr, "%s\n", qPrintable(error.toString()));
        }

        if (dummyData) {
            fprintf(stderr, "Loaded dummy data: %s\n", qPrintable(dir.filePath(qml)));
            qml.truncate(qml.size()-4);
            engine.rootContext()->setContextProperty(qml, dummyData);
            dummyData->setParent(&engine);
        }
    }
}

static void usage()
{
    puts("Usage: qmlscene [options] <filename>");
    puts(" ");
    puts(" Options:");
    puts("  --maximized ...................... Run maximized");
    puts("  --fullscreen ..................... Run fullscreen");
    puts("  --transparent .................... Make the window transparent");
    puts("  --multisample .................... Enable multisampling (OpenGL anti-aliasing)");
    puts("  --core-profile ................... Request a core profile OpenGL context");
    puts("  --rhi [vulkan|metal|d3d11|gl] .... Specify backend for the Qt graphics abstraction (RHI).\n");
    puts("  --no-version-detection ........... Do not try to detect the version of the .qml file");
    puts("  --slow-animations ................ Run all animations in slow motion");
    puts("  --resize-to-root ................. Resize the window to the size of the root item");
    puts("  --quit ........................... Quit immediately after starting");
    puts("  --disable-context-sharing ........ Disable the use of a shared GL context for QtQuick Windows\n"
         "                            ........ (remove AA_ShareOpenGLContexts)");
    puts("  --desktop......................... Force use of desktop GL (AA_UseDesktopOpenGL)");
    puts("  --gles............................ Force use of GLES (AA_UseOpenGLES)");
    puts("  --software........................ Force use of software rendering (AA_UseSoftwareOpenGL)");
    puts("  --verbose......................... Print version and graphical diagnostics for the run-time");
#ifdef QT_WIDGETS_LIB
    puts("  --apptype [gui|widgets] .......... Select which application class to use. Default is widgets.");
#endif
    puts("  --textrendertype [qt|native]...... Select the default render type for text-like elements.");
    puts("  -I <path> ........................ Add <path> to the list of import paths");
    puts("  -S <selector> .................... Add <selector> to the list of QQmlFileSelector selectors");
    puts("  -P <path> ........................ Add <path> to the list of plugin paths");
    puts("  -translation <translationfile> ... Set the language to run in");

    puts(" ");
    exit(1);
}

static void setWindowTitle(bool verbose, const QObject *topLevel, QWindow *window)
{
    const QString oldTitle = window->title();
    QString newTitle = oldTitle;
    if (newTitle.isEmpty()) {
        newTitle = QLatin1String("qmlscene");
        if (!qobject_cast<const QWindow *>(topLevel) && !topLevel->objectName().isEmpty())
            newTitle += QLatin1String(": ") + topLevel->objectName();
    }
    if (verbose) {
        newTitle += QLatin1String(" [Qt ") + QLatin1String(QT_VERSION_STR) + QLatin1Char(' ')
            + QGuiApplication::platformName() + QLatin1Char(' ');
        newTitle += QLatin1Char(']');
    }
    if (oldTitle != newTitle)
        window->setTitle(newTitle);
}

static QUrl parseUrlArgument(const QString &arg)
{
    const QUrl url = QUrl::fromUserInput(arg, QDir::currentPath(), QUrl::AssumeLocalFile);
    if (!url.isValid()) {
        fprintf(stderr, "Invalid URL: \"%s\"\n", qPrintable(arg));
        return QUrl();
    }
    if (url.isLocalFile()) {
        const QFileInfo fi(url.toLocalFile());
        if (!fi.exists()) {
            fprintf(stderr, "\"%s\" does not exist.\n",
                    qPrintable(QDir::toNativeSeparators(fi.absoluteFilePath())));
            return QUrl();
        }
    }
    return url;
}

static QQuickWindow::TextRenderType parseTextRenderType(const QString &renderType)
{
    if (renderType == QLatin1String("qt"))
        return QQuickWindow::QtTextRendering;
    else if (renderType == QLatin1String("native"))
        return QQuickWindow::NativeTextRendering;

    usage();

    Q_UNREACHABLE_RETURN(QQuickWindow::QtTextRendering);
}

int main(int argc, char ** argv)
{
    Options options;

    QStringList imports;
    QStringList customSelectors;
    QStringList pluginPaths;

    qCWarning(lcQmlsceneDeprecated()) << "Warning: qmlscene is deprecated and will be removed in a future version of Qt. Please use qml instead.";

    // Parse arguments for application attributes to be applied before Q[Gui]Application creation.
    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (!qstrcmp(arg, "--disable-context-sharing")) {
            options.applicationAttributes.removeAll(Qt::AA_ShareOpenGLContexts);
        } else if (!qstrcmp(arg, "--gles")) {
            options.applicationAttributes.append(Qt::AA_UseOpenGLES);
        } else if (!qstrcmp(arg, "--software")) {
            options.applicationAttributes.append(Qt::AA_UseSoftwareOpenGL);
        } else if (!qstrcmp(arg, "--desktop")) {
            options.applicationAttributes.append(Qt::AA_UseDesktopOpenGL);
        } else if (!qstrcmp(arg, "--transparent")) {
            options.transparent = true;
        } else if (!qstrcmp(arg, "--multisample")) {
            options.multisample = true;
        } else if (!qstrcmp(arg, "--core-profile")) {
            options.coreProfile = true;
        } else if (!qstrcmp(arg, "--apptype")) {
            if (++i >= argc)
                usage();
            if (!qstrcmp(argv[i], "gui"))
                options.applicationType = Options::QmlApplicationTypeGui;
        }
    }

    if (qEnvironmentVariableIsSet("QMLSCENE_CORE_PROFILE")
        || qEnvironmentVariableIsSet("QSG_CORE_PROFILE"))
        options.coreProfile = true;

    // Set default surface format before creating the window
    QSurfaceFormat surfaceFormat;
    surfaceFormat.setStencilBufferSize(8);
    surfaceFormat.setDepthBufferSize(24);
    if (options.multisample)
        surfaceFormat.setSamples(16);
    if (options.transparent)
        surfaceFormat.setAlphaBufferSize(8);
    if (options.coreProfile) {
        surfaceFormat.setVersion(4, 1);
        surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    }
    QSurfaceFormat::setDefaultFormat(surfaceFormat);

    for (Qt::ApplicationAttribute a : std::as_const(options.applicationAttributes))
        QCoreApplication::setAttribute(a);
    QScopedPointer<QGuiApplication> app;
#ifdef QT_WIDGETS_LIB
    if (options.applicationType == Options::QmlApplicationTypeWidget)
        app.reset(new QApplication(argc, argv));
#endif
    if (app.isNull())
        app.reset(new QGuiApplication(argc, argv));
    QCoreApplication::setApplicationName(QStringLiteral("QtQmlViewer"));
    QCoreApplication::setOrganizationName(QStringLiteral("QtProject"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("qt-project.org"));
    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));

    const QStringList arguments = QCoreApplication::arguments();
    for (int i = 1, size = arguments.size(); i < size; ++i) {
        if (!arguments.at(i).startsWith(QLatin1Char('-'))) {
            options.url = parseUrlArgument(arguments.at(i));
        } else {
            const QString lowerArgument = arguments.at(i).toLower();
            if (lowerArgument == QLatin1String("--maximized"))
                options.maximized = true;
            else if (lowerArgument == QLatin1String("--fullscreen"))
                options.fullscreen = true;
            else if (lowerArgument == QLatin1String("--clip"))
                options.clip = true;
            else if (lowerArgument == QLatin1String("--no-version-detection"))
                options.versionDetection = false;
            else if (lowerArgument == QLatin1String("--slow-animations"))
                options.slowAnimations = true;
            else if (lowerArgument == QLatin1String("--quit"))
                options.quitImmediately = true;
           else if (lowerArgument == QLatin1String("-translation"))
                options.translationFile = QLatin1String(argv[++i]);
            else if (lowerArgument == QLatin1String("--resize-to-root"))
                options.resizeViewToRootItem = true;
            else if (lowerArgument == QLatin1String("--verbose"))
                options.verbose = true;
            else if (lowerArgument == QLatin1String("--rhi")) {
                options.rhi = true;
                if (i + 1 < size && !arguments.at(i + 1).startsWith(QLatin1Char('-'))) {
                    options.rhiBackendSet = true;
                    options.rhiBackend = arguments.at(++i);
                }
            } else if (lowerArgument == QLatin1String("-i") && i + 1 < size)
                imports.append(arguments.at(++i));
            else if (lowerArgument == QLatin1String("-s") && i + 1 < size)
                customSelectors.append(arguments.at(++i));
            else if (lowerArgument == QLatin1String("-p") && i + 1 < size)
                pluginPaths.append(arguments.at(++i));
            else if (lowerArgument == QLatin1String("--apptype"))
                ++i; // Consume previously parsed argument
            else if (lowerArgument == QLatin1String("--textrendertype") && i + 1 < size)
                options.textRenderType = parseTextRenderType(arguments.at(++i));
            else if (lowerArgument == QLatin1String("--help")
                     || lowerArgument == QLatin1String("-help")
                     || lowerArgument == QLatin1String("--h")
                     || lowerArgument == QLatin1String("-h"))
                usage();
        }
    }

#if QT_CONFIG(translation)
    QLocale locale;
    QTranslator qtTranslator;
    if (qtTranslator.load(locale, QLatin1String("qt"), QLatin1String("_"), QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QCoreApplication::installTranslator(&qtTranslator);
    QTranslator translator;
    if (translator.load(locale, QLatin1String("qmlscene"), QLatin1String("_"), QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QCoreApplication::installTranslator(&translator);

    QTranslator qmlTranslator;
    if (!options.translationFile.isEmpty()) {
        if (qmlTranslator.load(options.translationFile)) {
            QCoreApplication::installTranslator(&qmlTranslator);
        } else {
            fprintf(stderr, "Could not load the translation file \"%s\"\n",
                    qPrintable(options.translationFile));
        }
    }
#endif

    QQuickWindow::setTextRenderType(options.textRenderType);

    QUnifiedTimer::instance()->setSlowModeEnabled(options.slowAnimations);

    if (options.rhi) {
        if (options.rhiBackendSet)
            qputenv("QSG_RHI_BACKEND", options.rhiBackend.toLatin1());
        else
            qunsetenv("QSG_RHI_BACKEND");
    }

    if (options.url.isEmpty())
#if defined(QMLSCENE_BUNDLE)
        displayOptionsDialog(&options);
#else
        displayFileDialog(&options);
#endif

    int exitCode = 0;

    if (options.verbose)
        puts(QLibraryInfo::build());

    if (!options.url.isEmpty()) {
        if (!options.versionDetection || checkVersion(options.url)) {
            // TODO: as soon as the engine construction completes, the debug service is
            // listening for connections.  But actually we aren't ready to debug anything.
            QQmlEngine engine;
            QQmlFileSelector* selector = new QQmlFileSelector(&engine, &engine);
            selector->setExtraSelectors(customSelectors);
            QPointer<QQmlComponent> component = new QQmlComponent(&engine);
            for (int i = 0; i < imports.size(); ++i)
                engine.addImportPath(imports.at(i));
            for (int i = 0; i < pluginPaths.size(); ++i)
                engine.addPluginPath(pluginPaths.at(i));
            if (options.url.isLocalFile()) {
                QFileInfo fi(options.url.toLocalFile());
#if QT_CONFIG(translation)
                QTranslator *translator = new QTranslator(app.get());
                if (translator->load(QLocale(), QLatin1String("qml"), QLatin1String("_"), fi.path() + QLatin1String("/i18n")))
                    QCoreApplication::installTranslator(translator);
#endif
                loadDummyDataFiles(engine, fi.path());
            }
            QObject::connect(&engine, SIGNAL(quit()), QCoreApplication::instance(), SLOT(quit()));
            QObject::connect(&engine, &QQmlEngine::exit, QCoreApplication::instance(), &QCoreApplication::exit);
            component->loadUrl(options.url);
            while (component->isLoading())
                QCoreApplication::processEvents();
            if ( !component->isReady() ) {
                fprintf(stderr, "%s\n", qPrintable(component->errorString()));
                return -1;
            }

            QObject *topLevel = component->create();
            if (!topLevel && component->isError()) {
                fprintf(stderr, "%s\n", qPrintable(component->errorString()));
                return -1;
            }

            QScopedPointer<QQuickWindow> window(qobject_cast<QQuickWindow *>(topLevel));
            if (window) {
                engine.setIncubationController(window->incubationController());
            } else {
                QQuickItem *contentItem = qobject_cast<QQuickItem *>(topLevel);
                if (contentItem) {
                    QQuickView* qxView = new QQuickView(&engine, nullptr);
                    window.reset(qxView);
                    // Set window default properties; the qml can still override them
                    if (options.resizeViewToRootItem)
                        qxView->setResizeMode(QQuickView::SizeViewToRootObject);
                    else
                        qxView->setResizeMode(QQuickView::SizeRootObjectToView);
                    qxView->setContent(options.url, component, contentItem);
                }
            }

            if (window) {
                setWindowTitle(options.verbose, topLevel, window.data());
                if (options.transparent) {
                    window->setColor(QColor(Qt::transparent));
                    window->setFlags(Qt::FramelessWindowHint);
                }
                window->setFormat(surfaceFormat);

                if (window->flags() == Qt::Window) // Fix window flags unless set by QML.
                    window->setFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint | Qt::WindowFullscreenButtonHint);

                if (options.fullscreen)
                    window->showFullScreen();
                else if (options.maximized)
                    window->showMaximized();
                else if (!window->isVisible())
                    window->show();
            }

            if (options.quitImmediately)
                QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);

            // Now would be a good time to inform the debug service to start listening.

            exitCode = app->exec();

#ifdef QML_RUNTIME_TESTING
            RenderStatistics::printTotalStats();
#endif
            // Ready to exit. Notice that the component might be owned by
            // QQuickView if one was created. That case is tracked by
            // QPointer, so it is safe to delete the component here.
            delete component;
        } else {
            exitCode = 1;
        }
    }

    return exitCode;
}
