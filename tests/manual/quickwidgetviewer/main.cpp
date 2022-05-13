// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QQmlEngine>
#include <QQuickWidget>
#include <QUrl>

#include <iostream>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("QQuickWidget Viewer");
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("url", "The URL to open.");
    parser.process(app);

    QUrl url;
    if (parser.positionalArguments().isEmpty())  {
        QFileDialog fileDialog(nullptr, "Select QML File");
        fileDialog.setMimeTypeFilters(QStringList("text/x-qml"));
        if (fileDialog.exec() != QDialog::Accepted)
            return 0;
        url = fileDialog.selectedUrls().constFirst();
    } else {
        url = QUrl::fromUserInput(parser.positionalArguments().constFirst(),
                                  QDir::currentPath(), QUrl::AssumeLocalFile);
        if (!url.isValid()) {
            std::cerr << qPrintable(url.errorString()) << '\n';
            return -1;
        }
    }

    QQuickWidget w(url);
    w.setAttribute(Qt::WA_AcceptTouchEvents);
    if (w.status() == QQuickWidget::Error)
        return -1;
    QObject::connect(w.engine(), &QQmlEngine::quit, &app, &QCoreApplication::quit);
    w.show();

    std::cout << "Qt " << QT_VERSION_STR << ' ' << qPrintable(app.platformName());
    if (QOpenGLContext *openglContext = w.quickWindow()->openglContext()) {
        QOpenGLFunctions *glFunctions = openglContext->functions();
        std::cout << " OpenGL \"" << glFunctions->glGetString(GL_RENDERER)
            << "\" \"" << glFunctions->glGetString(GL_VERSION) << '"';
    }
    const qreal devicePixelRatio = w.devicePixelRatio();
    if (!qFuzzyCompare(devicePixelRatio, qreal(1)))
        std::cout << ", DPR=" << devicePixelRatio;
    std::cout << '\n';

    return app.exec();

}
