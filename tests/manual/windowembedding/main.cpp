// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui>
#include <QtWidgets>
#include <QQmlEngine>
#include <QQuickView>
#include <QQmlContext>

class TestWindow : public QRasterWindow
{
public:
    using QRasterWindow::QRasterWindow;
    TestWindow(const QBrush &brush) : m_brush(brush) {}

protected:
    void mousePressEvent(QMouseEvent *) override
    {
        m_pressed = true;
        update();
    }

    void mouseReleaseEvent(QMouseEvent *) override
    {
        m_pressed = false;
        update();
    }

    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(event->rect(), m_pressed ? QGradient(QGradient::JuicyPeach) : m_brush);
    }

private:
    QBrush m_brush = QGradient(QGradient::DustyGrass);
    bool m_pressed = false;
};

#ifdef Q_OS_APPLE
#include <WebKit/WebKit.h>
#include <MapKit/MapKit.h>
#include <AVKit/AVKit.h>
#endif

int main(int argc, char* argv[])
{
    QApplication app(argc,argv);
    app.setOrganizationName("QtProject");
    app.setOrganizationDomain("qt-project.org");
    app.setApplicationName(QFileInfo(app.applicationFilePath()).baseName());

    QQuickView view;
    view.connect(view.engine(), &QQmlEngine::quit, &app, &QCoreApplication::quit);

#ifdef Q_OS_MACOS
    view.engine()->addImportPath(app.applicationDirPath() + QStringLiteral("/../PlugIns"));
#endif

    TestWindow testWindow;

    QWindow *mapWindow = nullptr;
    QWindow *webViewWindow = nullptr;
    QWindow *videoWindow = nullptr;

#ifdef Q_OS_APPLE
    auto *webView = [WKWebView new];
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"https://www.qt.io"]]];
    webViewWindow = QWindow::fromWinId(WId(webView));

    auto *mapView = [MKMapView new];
    mapWindow = QWindow::fromWinId(WId(mapView));

# ifndef Q_OS_IOS
    auto *videoView = [AVPlayerView new];
    videoView.player = [AVPlayer playerWithURL:[NSURL URLWithString:
        @"https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4"]];
    videoWindow = QWindow::fromWinId(WId(videoView));
# endif

#else
    mapWindow = &testWindow;
    webViewWindow = &testWindow;
    videoWindow = &testWindow;
#endif

    auto *calendarWidget = new QCalendarWidget;
    calendarWidget->setAttribute(Qt::WA_NativeWindow);
    QWindow *widgetWindow = calendarWidget->windowHandle();

    auto *context = view.engine()->rootContext();
    context->setContextProperty("testWindow", &testWindow);
    context->setContextProperty("widgetWindow", widgetWindow);
    context->setContextProperty("mapWindow", mapWindow);
    context->setContextProperty("webViewWindow", webViewWindow);
    context->setContextProperty("videoWindow", videoWindow);

#if defined(QT_MULTIMEDIA_LIB)
    context->setContextProperty("haveQtMultimedia", true);
#endif

    view.setSource(QUrl("qrc:/qt/qml/windowembeddingexample/windowembedding.qml"));
    if (view.status() == QQuickView::Error)
        return -1;

    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.show();

    return app.exec();
}
