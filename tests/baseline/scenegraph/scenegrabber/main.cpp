// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QHashFunctions>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>

#include <QtQuick/QQuickView>
#include <QtQuick/QQuickItem>
#include <QtQuickControls2/qquickstyle.h>

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

static const QSize DefaultGrabSize(320, 480);

class GrabbingView : public QQuickView
{
    Q_OBJECT

public:
    GrabbingView(const QString &outputFile)
        : ofile(outputFile), grabNo(0), isGrabbing(false), initDone(false), justShow(outputFile.isEmpty())
    {
        if (justShow)
            return;
        grabTimer = new QTimer(this);
        grabTimer->setSingleShot(true);
        grabTimer->setInterval(SCENE_STABLE_TIME);
        connect(grabTimer, SIGNAL(timeout()), SLOT(grab()));

        connect(this, SIGNAL(afterRendering()), SLOT(startGrabbing()));

        QTimer::singleShot(SCENE_TIMEOUT, this, SLOT(timedOut()));
    }

private slots:
    void startGrabbing()
    {
        if (!initDone) {
            initDone = true;
            grabTimer->start();
        }
    }

    void grab()
    {
        if (isGrabbing)
            return;
        isGrabbing = true;
        grabNo++;
#ifdef GRABBERDEBUG
        printf("grab no. %i\n", grabNo);
#endif
        QImage img = grabWindow();
        if (!img.isNull() && img == lastGrab) {
            sceneStabilized();
        } else {
            lastGrab = img;
            grabTimer->start();
        }

        isGrabbing = false;
    }

    void sceneStabilized()
    {
#ifdef GRABBERDEBUG
        printf("...sceneStabilized IN\n");
#endif
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
#ifdef GRABBERDEBUG
        printf("...sceneStabilized OUT\n");
#endif
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
        else if (ifile.isEmpty()) {
            ifile = arg;
        }
        else if (arg == "-style") {
            if (i < args.size()-1)
                style = args.at(++i);
            else
                argError = true;
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

    GrabbingView v(ofile);
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

    int retVal = a.exec();
#ifdef GRABBERDEBUG
    printf("...retVal=%i\n", retVal);
#endif

    return retVal;
}

#include "main.moc"
