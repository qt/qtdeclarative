// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QHashFunctions>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtCore/QLoggingCategory>

#include <QtQuick/QQuickView>
#include <QtQuick/QQuickItem>
#include <QtQuickControls2/qquickstyle.h>
#include <QQmlApplicationEngine>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>

#ifdef Q_OS_WIN
#  include <fcntl.h>
#  include <io.h>
#endif // Q_OS_WIN

// Timeout values:

// A valid screen grab requires the scene to not change
// for SCENE_STABLE_TIME ms
#define SCENE_STABLE_TIME 200

// Give up after SCENE_TIMEOUT ms
#define SCENE_TIMEOUT     6000

//#define GRABBERDEBUG
Q_LOGGING_CATEGORY(lcGrabber, "qt.baseline.scenegrabber")

static const QSize DefaultGrabSize(320, 480);

class GrabbingView : public QQuickView
{
    Q_OBJECT

public:
    GrabbingView(const QString &outputFile, const bool useAppWindow)
        : ofile(outputFile), grabNo(0), isGrabbing(false), initDone(false), justShow(outputFile.isEmpty()), preferAppWindow(useAppWindow)
    {
        if (justShow)
            return;
        grabTimer = new QTimer(this);
        grabTimer->setSingleShot(true);
        grabTimer->setInterval(SCENE_STABLE_TIME);
        connect(grabTimer, &QTimer::timeout, this, &GrabbingView::grab);

        if (!preferAppWindow)
            QObject::connect(this, &QQuickWindow::afterRendering, this, &GrabbingView::startGrabbing);

        QTimer::singleShot(SCENE_TIMEOUT, this, &GrabbingView::timedOut);
    }

    void setApplicationWindow(QWindow* window) {
        qCDebug(lcGrabber) << "Using ApplicationWindow as visual parent" << this;

        appwindow = qobject_cast<QQuickApplicationWindow *>(window);
        if (preferAppWindow)
            QObject::connect(appwindow, &QQuickWindow::afterRendering, this, &GrabbingView::startGrabbing);
    }
    QQuickApplicationWindow* appWindow() { return appwindow; }

private slots:
    void startGrabbing()
    {
        qCDebug(lcGrabber) << "Starting to grab";
        if (!initDone) {
            initDone = true;
            grabTimer->start();
        }
    }

    void grab()
    {
        if (isGrabbing) {
            qCDebug(lcGrabber) << "Already grabbing, skipping";
            return;
        }

        QScopedValueRollback grabGuard(isGrabbing, true);

        grabNo++;
        qCDebug(lcGrabber) << "grab no." << grabNo;
        QImage img;
        img = appwindow ? appwindow->grabWindow() : grabWindow();
        if (!img.isNull() && img == lastGrab) {
            sceneStabilized();
        } else {
            lastGrab = img;
            grabTimer->start();
        }
    }

    void sceneStabilized()
    {
        qCDebug(lcGrabber) << "...sceneStabilized IN";
        if (QGuiApplication::platformName() == QLatin1String("eglfs")) {
            QSize grabSize = initialSize().isEmpty() ? DefaultGrabSize : initialSize();
            lastGrab = lastGrab.copy(QRect(QPoint(0, 0), grabSize));
        }

        if (ofile == "-") {   // Write to stdout
            QFile of;
#ifdef Q_OS_WIN
            // Make sure write to stdout doesn't do LF->CRLF
            _setmode(_fileno(stdout), _O_BINARY);
#endif // Q_OS_WIN
            if (!of.open(1, QIODevice::WriteOnly) || !lastGrab.save(&of, "ppm")) {
                qWarning() << "Error: failed to write grabbed image to stdout.";
                QGuiApplication::exit(2);
                return;
            }
        } else {
            if (!lastGrab.save(ofile)) {
                qWarning() << "Error: failed to store grabbed image to" << ofile;
                QGuiApplication::exit(2);
                return;
            }
        }
        QGuiApplication::exit(0);
        qCDebug(lcGrabber) << "...sceneStabilized OUT";
    }

    void timedOut()
    {
        qWarning() << "Error: timed out waiting for scene to stabilize." << grabNo << "grab(s) done. Last grab was" << (lastGrab.isNull() ? "invalid." : "valid.");
        QGuiApplication::exit(3);
    }

private:
    QImage lastGrab;
    QTimer *grabTimer = nullptr;
    QString ofile;
    int grabNo;
    bool isGrabbing;
    bool initDone;
    bool justShow;
    QQuickApplicationWindow *appwindow = nullptr;
    bool preferAppWindow = false;
};


int main(int argc, char *argv[])
{
    QHashSeed::setDeterministicGlobalSeed();

    QGuiApplication a(argc, argv);

    // Parse command line
    QString ifile, ofile, style;
    bool noText = false;
    bool justShow = false;

    QStringList args = a.arguments();
    int i = 0;
    bool argError = false;
    bool useAppWindow = false;
    while (++i < args.size()) {
        QString arg = args.at(i);
        if ((arg == "-o") && (i < args.size()-1)) {
            ofile = args.at(++i);
        }
        else if (arg == "-notext") {
            noText = true;
        }
        else if (arg == "--cache-distance-fields") {
            ;
        }
        else if (arg == "-viewonly") {
            justShow = true;
        }
        else if (arg == "-style") {
            if (i < args.size()-1)
                style = args.at(++i);
            else {
                argError = true;
                break;
            }
        }
        else if (arg == "-useAppWindow") {
            useAppWindow = true;
        }
        else if (ifile.isEmpty()) {
            ifile = arg;
        }
        else {
            argError = true;
            break;
        }
    }
    if (argError || ifile.isEmpty() || (ofile.isEmpty() && !justShow)) {
        qWarning() << "Usage:" << args.at(0).toLatin1().constData()
                   << "[-notext] [-style stylename] <qml-infile> {-o <outfile or - for ppm on stdout>|-viewonly}";
        return 1;
    }

    QFileInfo ifi(ifile);
    if (!ifi.exists() || !ifi.isReadable() || !ifi.isFile()) {
        qWarning() << args.at(0).toLatin1().constData() << " error: unreadable input file" << ifile;
        return 1;
    }
    // End parsing

    if (!style.isEmpty())
        QQuickStyle::setStyle(style);

    GrabbingView v(ofile, useAppWindow);

    if (useAppWindow) {
        QQmlEngine *engine = new QQmlEngine;
        {
            // test qml component creation
            QQmlComponent component(engine, QUrl::fromLocalFile(ifile));
            auto *itemObject = qobject_cast<QQuickItem*>(component.create());

            // TODO: Hack to import native style forcefully for windows
            QString appCompStr = "import QtQuick.Controls\n";
            if (QQuickStyle::name() == "Windows")
                appCompStr += "import QtQuick.NativeStyle\n";
            appCompStr += "ApplicationWindow{}";

            // Create application window
            QQmlComponent appWndComp(engine);
            appWndComp.setData(appCompStr.toLatin1(), QUrl::fromLocalFile(""));
            auto *appWindow = qobject_cast<QQuickApplicationWindow *>(appWndComp.create());

            if (!appWindow) {
                qWarning() << "Error: failed to create application window.";
                QGuiApplication::exit(2);
            }

            appWindow->resize(itemObject->size().toSize());

            // Use application window as visual parent to the loaded component
            v.setApplicationWindow(appWindow);
            itemObject->setParentItem(v.appWindow()->contentItem());
            itemObject->setParent(v.appWindow());
        }
        v.appWindow()->show();
    } else {
        v.setSource(QUrl::fromLocalFile(ifile));

        if (noText) {
            const QList<QQuickItem*> items = v.rootObject()->findChildren<QQuickItem*>();
            for (QQuickItem *item : items) {
                if (QByteArray(item->metaObject()->className()).contains("Text"))
                    item->setVisible(false);
            }
        }

        if (v.initialSize().isEmpty())
            v.resize(DefaultGrabSize);

        v.show();
    }

    int retVal = a.exec();
    qCDebug(lcGrabber) << "...retVal=" << retVal;

    return retVal;
}

#include "main.moc"
