// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QMainWindow>
#include <QApplication>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

#include <QQmlError>
#include <QQuickView>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();

private slots:
    void quickViewStatusChanged(QQuickView::Status);
    void sceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);

private:
    QQuickView *m_quickView;
};

//! [ctor]
MainWindow::MainWindow()
    : m_quickView(new QQuickView)
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    m_quickView->setResizeMode(QQuickView::SizeRootObjectToView);
    connect(m_quickView, &QQuickView::statusChanged,
            this, &MainWindow::quickViewStatusChanged);
    connect(m_quickView, &QQuickWindow::sceneGraphError,
            this, &MainWindow::sceneGraphError);
    m_quickView->setSource(QUrl(QStringLiteral("qrc:///embeddedinwidgets/main.qml")));

    QWidget *container = QWidget::createWindowContainer(m_quickView);
    container->setMinimumSize(m_quickView->size());
    container->setFocusPolicy(Qt::TabFocus);

    layout->addWidget(new QLineEdit(QStringLiteral("A QLineEdit")));
    layout->addWidget(container);
    layout->addWidget(new QLineEdit(QStringLiteral("A QLineEdit")));
    setCentralWidget(centralWidget);

    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(tr("Quit"), qApp, &QCoreApplication::quit);
}
//! [ctor]

void MainWindow::quickViewStatusChanged(QQuickView::Status status)
{
    if (status == QQuickView::Error) {
        QStringList errors;
        const auto viewErrors = m_quickView->errors();
        for (const QQmlError &error : viewErrors)
            errors.append(error.toString());
        statusBar()->showMessage(errors.join(QStringLiteral(", ")));
    }
}

void MainWindow::sceneGraphError(QQuickWindow::SceneGraphError, const QString &message)
{
    statusBar()->showMessage(message);
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}

#include "main.moc"
