/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qdebug.h>
#include <QtCore/qabstractanimation.h>
#include <QtWidgets/qapplication.h>
#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtQuick1/qdeclarativeview.h>
#include <QtCore/qdir.h>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGraphicsView>

#include <QtDeclarative/qdeclarativecontext.h>

// ### This should be private API
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>

#define QT_NO_SCENEGRAPHITEM

#ifndef QT_NO_SCENEGRAPHITEM
#include "scenegraphitem.h"
#endif

#include <QtCore/qmath.h>

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
    static QTime time;
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

            qDebug("Average time per frame: %f ms (%i fps), std.dev: %f ms", avgtime, qRound(1000. / avgtime), qSqrt(var));

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
    int count = timePerFrame.count();
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

    qDebug(" ");
    qDebug("----- Statistics -----");
    qDebug("Average time per frame: %f ms (%i fps)", avg, qRound(1000. / avg));
    qDebug("Best time per frame: %f ms (%i fps)", minTime, int(1000 / minTime));
    qDebug("Worst time per frame: %f ms (%i fps)", maxTime, int(1000 / maxTime));
    qDebug("----------------------");
    qDebug(" ");
}
#endif

class MyQQuickView : public QQuickView
{
public:
    MyQQuickView() : QQuickView()
    {
        setResizeMode(QQuickView::SizeRootObjectToView);
    }
};

class MyDeclarativeView: public QDeclarativeView
{
public:
    MyDeclarativeView(QWidget *parent = 0) : QDeclarativeView(parent)
    {
        setResizeMode(QDeclarativeView::SizeRootObjectToView);
    }
};

#ifndef QT_NO_SCENEGRAPHITEM
class MyGraphicsView: public QGraphicsView
{
public:
    MyGraphicsView(bool clip, QWidget *parent = 0) : QGraphicsView(parent)
    {
        setViewport(new QGLWidget(getFormat()));
        setScene(&scene);
        scene.addItem(&item);
        item.setFlag(QGraphicsItem::ItemClipsToShape, clip);
        QGraphicsTextItem *text;
        text = scene.addText(QLatin1String("Scene graph on graphics view."), QFont(QLatin1String("Times"), 10));
        text->setX(5);
        text->setY(5);
        text->setDefaultTextColor(Qt::black);
        text = scene.addText(QLatin1String("Scene graph on graphics view."), QFont(QLatin1String("Times"), 10));
        text->setX(4);
        text->setY(4);
        text->setDefaultTextColor(Qt::yellow);
    }

    SceneGraphItem *sceneGraphItem() { return &item; }

protected:
    void paintEvent(QPaintEvent *event)
    {
        QGraphicsView::paintEvent(event);

#ifdef QML_RUNTIME_TESTING
        RenderStatistics::updateStats();
#endif

        static bool continuousUpdate = qApp->arguments().contains("--continuous-update");
        if (continuousUpdate)
            QGraphicsView::scene()->update();
    }

    QGraphicsScene scene;
    SceneGraphItem item;
};
#endif

struct Options
{
    Options()
        : originalQml(false)
        , originalQmlRaster(false)
        , maximized(false)
        , fullscreen(false)
        , scenegraphOnGraphicsview(false)
        , clip(false)
        , versionDetection(true)
        , vsync(true)
    {
    }

    QUrl file;
    bool originalQml;
    bool originalQmlRaster;
    bool maximized;
    bool fullscreen;
    bool scenegraphOnGraphicsview;
    bool clip;
    bool versionDetection;
    bool vsync;
};

#if defined(QMLSCENE_BUNDLE)
Q_DECLARE_METATYPE(QFileInfo);
QFileInfoList findQmlFiles(const QString &dirName)
{
    QDir dir(dirName);

    QFileInfoList ret;
    if (dir.exists()) {
        QFileInfoList fileInfos = dir.entryInfoList(QStringList() << "*.qml",
                                                    QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);

        foreach (QFileInfo fileInfo, fileInfos) {
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
    QFileInfoList fileInfos = findQmlFiles(":/bundle") + findQmlFiles("./qmlscene-resources");

    foreach (QFileInfo fileInfo, fileInfos)
        qmlFileComboBox->addItem(fileInfo.dir().dirName() + "/" + fileInfo.fileName(), QVariant::fromValue(fileInfo));

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

        if (fileInfo.canonicalFilePath().startsWith(":"))
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

static void checkAndAdaptVersion(const QUrl &url)
{
    if (!qgetenv("QMLSCENE_IMPORT_NAME").isEmpty()) {
        return;
    }

    QString fileName = url.toLocalFile();
    if (fileName.isEmpty())
        return;

    QFile f(fileName);
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        qWarning("qmlscene: failed to check version of file '%s', could not open...",
                 qPrintable(fileName));
        return;
    }

    QRegExp quick1("^\\s*import +QtQuick +1\\.");
    QRegExp quick2("^\\s*import +QtQuick +2\\.");
    QRegExp qt47("^\\s*import +Qt +4\\.7");

    QString envToWrite;
    QString compat;

    QTextStream stream(&f);
    bool codeFound= false;
    while (!codeFound) {
        QString line = stream.readLine();
        if (line.contains("{"))
            codeFound = true;
        if (envToWrite.isEmpty() && quick1.indexIn(line) >= 0) {
            envToWrite = QLatin1String("quick1");
            compat = QLatin1String("QtQuick 1.0");
        } else if (envToWrite.isEmpty() && qt47.indexIn(line) >= 0) {
            envToWrite = QLatin1String("qt");
            compat = QLatin1String("Qt 4.7");
        } else if (quick2.indexIn(line) >= 0) {
            envToWrite.clear();
            compat.clear();
            break;
        }
    }

    if (!envToWrite.isEmpty()) {
        qWarning("qmlscene: Autodetecting compatibility import \"%s\"...", qPrintable(compat));
        if (qgetenv("QMLSCENE_IMPORT_NAME").isEmpty())
            qputenv("QMLSCENE_IMPORT_NAME", envToWrite.toLatin1().constData());
    }
}

static void displayFileDialog(Options *options)
{
    QString fileName = QFileDialog::getOpenFileName(0, "Open QML file", QString(), "QML Files (*.qml)");
    if (!fileName.isEmpty()) {
        QFileInfo fi(fileName);
        options->file = QUrl::fromLocalFile(fi.canonicalFilePath());
    }
}

static void loadDummyDataFiles(QDeclarativeEngine &engine, const QString& directory)
{
    QDir dir(directory+"/dummydata", "*.qml");
    QStringList list = dir.entryList();
    for (int i = 0; i < list.size(); ++i) {
        QString qml = list.at(i);
        QFile f(dir.filePath(qml));
        f.open(QIODevice::ReadOnly);
        QByteArray data = f.readAll();
        QDeclarativeComponent comp(&engine);
        comp.setData(data, QUrl());
        QObject *dummyData = comp.create();

        if(comp.isError()) {
            QList<QDeclarativeError> errors = comp.errors();
            foreach (const QDeclarativeError &error, errors) {
                qWarning() << error;
            }
        }

        if (dummyData) {
            qWarning() << "Loaded dummy data:" << dir.filePath(qml);
            qml.truncate(qml.length()-4);
            engine.rootContext()->setContextProperty(qml, dummyData);
            dummyData->setParent(&engine);
        }
    }
}

static void usage()
{
    qWarning("Usage: qmlscene [options] <filename>");
    qWarning(" ");
    qWarning(" options:");
    qWarning("  --maximized ............................... run maximized");
    qWarning("  --fullscreen .............................. run fullscreen");
    qWarning("  --original-qml ............................ run using QGraphicsView instead of scenegraph (OpenGL engine)");
    qWarning("  --original-qml-raster ..................... run using QGraphicsView instead of scenegraph (Raster engine)");
    qWarning("  --no-multisample .......................... Disable multisampling (anti-aliasing)");
    qWarning("  --continuous-update ....................... Continuously render the scene");
    qWarning("  --nonblocking-swap ........................ Do not wait for v-sync to swap buffers");
    qWarning("  --stereo .................................. Enable stereo on the GL context");
#ifndef QT_NO_SCENEGRAPHITEM
    qWarning("  --sg-on-gv [--clip] ....................... Scenegraph on graphicsview (and clip to item)");
#endif
    qWarning("  --no-version-detection .................... Do not try to detect the version of the .qml file");
    qWarning("  --no-vsync-animations ..................... Do not use vsync based animations");

    qWarning(" ");
    exit(1);
}

int main(int argc, char ** argv)
{
#ifdef Q_WS_X11
    QApplication::setAttribute(Qt::AA_X11InitThreads);
#endif

    Options options;

    QStringList imports;
    for (int i = 1; i < argc; ++i) {
        if (*argv[i] != '-' && QFileInfo(QFile::decodeName(argv[i])).exists()) {
            options.file = QUrl::fromLocalFile(argv[i]);
        } else {
            const QString lowerArgument = QString::fromLatin1(argv[i]).toLower();
            if (lowerArgument == QLatin1String("--original-qml"))
                options.originalQml = true;
            else if (lowerArgument == QLatin1String("--original-qml-raster"))
                options.originalQmlRaster = true;
            else if (lowerArgument == QLatin1String("--maximized"))
                options.maximized = true;
            else if (lowerArgument == QLatin1String("--fullscreen"))
                options.fullscreen = true;
            else if (lowerArgument == QLatin1String("--sg-on-gv"))
                options.scenegraphOnGraphicsview = true;
            else if (lowerArgument == QLatin1String("--clip"))
                options.clip = true;
            else if (lowerArgument == QLatin1String("--no-version-detection"))
                options.versionDetection = false;
            else if (lowerArgument == QLatin1String("-i") && i + 1 < argc)
                imports.append(QString::fromLatin1(argv[++i]));
            else if (lowerArgument == QLatin1String("--no-vsync-animations"))
                options.vsync = false;
            else if (lowerArgument == QLatin1String("--help")
                     || lowerArgument == QLatin1String("-help")
                     || lowerArgument == QLatin1String("--h")
                     || lowerArgument == QLatin1String("-h"))
                usage();
        }
    }

    QApplication::setGraphicsSystem("raster");

    QApplication app(argc, argv);
    app.setApplicationName("QtQmlViewer");
    app.setOrganizationName("Nokia");
    app.setOrganizationDomain("nokia.com");

    if (options.file.isEmpty())
#if defined(QMLSCENE_BUNDLE)
        displayOptionsDialog(&options);
#else
        displayFileDialog(&options);
#endif

    QWindow *window = 0;
    QDeclarativeEngine *engine = 0;

    int exitCode = 0;

    if (!options.file.isEmpty()) {
#ifndef QT_NO_SCENEGRAPHITEM
        if (options.scenegraphOnGraphicsview) {
            MyGraphicsView *gvView = new MyGraphicsView(options.clip);
            SceneGraphItem *item = gvView->sceneGraphItem();
            engine = item->engine();
            for (int i = 0; i < imports.size(); ++i)
                engine->addImportPath(imports.at(i));
            view = gvView;
            if (options.file.isLocalFile()) {
                QFileInfo fi(options.file.toLocalFile());
                loadDummyDataFiles(*engine, fi.path());
            }
            item->setSource(options.file);
        } else
#endif
        if (options.versionDetection)
            checkAndAdaptVersion(options.file);
        QQuickView *qxView = new MyQQuickView();
        qxView->setVSyncAnimations(options.vsync);
        engine = qxView->engine();
        for (int i = 0; i < imports.size(); ++i)
            engine->addImportPath(imports.at(i));
        window = qxView;
        if (options.file.isLocalFile()) {
            QFileInfo fi(options.file.toLocalFile());
            loadDummyDataFiles(*engine, fi.path());
        }
        qxView->setSource(options.file);

        QObject::connect(engine, SIGNAL(quit()), QCoreApplication::instance(), SLOT(quit()));

        window->setWindowFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
        if (options.fullscreen)
            window->showFullScreen();
        else if (options.maximized)
            window->showMaximized();
        else
            window->show();


#ifdef Q_OS_MAC
        window->raise();
#endif

        exitCode = app.exec();

        delete window;

#ifdef QML_RUNTIME_TESTING
        RenderStatistics::printTotalStats();
#endif
    }

    return exitCode;
}

