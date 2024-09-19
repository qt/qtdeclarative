// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "editorwidget.h"
#include "previewwidget.h"
#include "../states/statecontroller.h"

#include <QCloseEvent>
#include <QFileSystemWatcher>
#include <QLayout>
#include <QMenuBar>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_editorWidget{new EditorWidget}
    , m_previewWidget{new PreviewWidget}
    , m_fileWatcher{new QFileSystemWatcher}
{
    setWindowIcon(QIcon(":/resources/logo.png"));
    setWindowTitle(tr("QML Previewer"));
    resize(1400, 800);

    initUI();

    initMenuBar();

    setupConnections();

    StateController::instance()->fileLoaded(":/resources/default.qml");
    m_editorWidget->updateEditor();
}

MainWindow::~MainWindow() {}

void MainWindow::initUI()
{
    QWidget *centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    centralWidget->setLayout(horizontalLayout);

    horizontalLayout->addWidget(m_editorWidget, 1);
    horizontalLayout->addWidget(m_previewWidget, 1);
}

void MainWindow::initMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    m_openAction = fileMenu->addAction(tr("&Open"));
    m_saveAction = fileMenu->addAction(tr("&Save"));
    m_saveAction->setEnabled(false);
    m_closeAction = fileMenu->addAction(tr("&Close"));
    m_closeAction->setEnabled(false);
    m_reloadAction = fileMenu->addAction(tr("&Reload"));
    m_reloadAction->setEnabled(false);

    fileMenu->addSeparator();

    m_quitAction = fileMenu->addAction(tr("&Quit"));

#if QT_CONFIG(shortcut)
    m_openAction->setShortcut(QKeySequence::Open);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_closeAction->setShortcut(QKeySequence::Close);
    m_reloadAction->setShortcut(Qt::CTRL | Qt::Key_R);
    m_quitAction->setShortcut(QKeySequence::Quit);
#endif
}

void MainWindow::setupConnections()
{
    connect(StateController::instance(), &StateController::stateChanged, this,
            &MainWindow::onAppStateChanged);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::onFileChanged);
    connect(menuBar(), &QMenuBar::triggered, this, &MainWindow::onMenuBarTriggered);
    connect(m_previewWidget, &PreviewWidget::errorPositionSelected, m_editorWidget,
            &EditorWidget::moveCursorTo);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    switch (StateController::instance()->currentState()) {
    case StateController::NewState:
    case StateController::DirtyState: {
        const QMessageBox::StandardButton answer =
            QMessageBox::question(this, tr("About to Close"),
                                  tr("You have unsaved changes. "
                                     "Are you sure you want to close without saving?"),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer == QMessageBox::Yes)
            event->accept();
        else
            event->ignore();
    } break;

    default:
        event->accept();
        break;
    }
}

void MainWindow::onAppStateChanged(int oldState, int newState)
{
    switch (newState) {
    case StateController::InitState:
        if (!m_previewWidget->sourcePath().isEmpty()) {
            if (const QStringList files = m_fileWatcher->files(); !files.isEmpty())
                m_fileWatcher->removePaths(files);
            m_previewWidget->setSourcePath(QString{});
        }
        m_saveAction->setEnabled(false);
        m_closeAction->setEnabled(false);
        m_reloadAction->setEnabled(false);
        break;

    case StateController::OpenState:
        if (const QStringList files = m_fileWatcher->files(); !files.isEmpty())
            m_fileWatcher->removePaths(files);
        m_fileWatcher->addPath(StateController::instance()->filePath());
        m_previewWidget->setSourcePath(StateController::instance()->filePath());
        m_saveAction->setEnabled(false);
        m_closeAction->setEnabled(true);
        m_reloadAction->setEnabled(false);
        break;

    case StateController::NewState:
        m_saveAction->setEnabled(true);
        m_closeAction->setEnabled(false);
        m_reloadAction->setEnabled(false);
        break;

    case StateController::DirtyState:
        m_saveAction->setEnabled(true);
        m_closeAction->setEnabled(false);
        m_reloadAction->setEnabled(true);
        break;

    default:
        Q_UNREACHABLE();
        break;
    }
}

void MainWindow::onFileChanged(const QString &path)
{
    StateController* stateController = StateController::instance();

    m_fileWatcher->addPath(path);

    if (stateController->currentState() == StateController::DirtyState) {
        const QMessageBox::StandardButton answer =
            QMessageBox::question(this, tr("Reload File"),
                                  tr("The file <i>%1</i> has been changed on disk. "
                                     "Do you want to reload it?").arg(path),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer == QMessageBox::No)
            return;
    }

    m_editorWidget->updateEditor();
    m_previewWidget->setSourcePath(path);
    stateController->setDirty(false);
}

void MainWindow::onMenuBarTriggered(QAction *action)
{
    if (action == m_openAction)
        m_editorWidget->openFile();
    else if (action == m_saveAction)
        m_editorWidget->saveFile();
    else if (action == m_closeAction)
        m_editorWidget->closeFile();
    else if (action == m_reloadAction)
        m_editorWidget->reloadFile();
    else if (action == m_quitAction)
        QCoreApplication::quit();
}
