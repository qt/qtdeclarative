// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"
#include "editorwidget.h"
#include "../states/statecontroller.h"

#include <QFileSystemWatcher>
#include <QLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QQmlEngine>
#include <QQuickWidget>

namespace {
static constexpr auto QUIT_SHORTCUT = QKeySequence::Quit;
static constexpr auto OPEN_SHORTCUT = QKeySequence::Open;
static constexpr auto SAVE_SHORTCUT = QKeySequence::Save;
static constexpr auto CLOSE_SHORTCUT = QKeySequence::Close;
static constexpr auto RELOAD_SHORTCUT = Qt::CTRL | Qt::Key_R;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_editorWidget{new EditorWidget}
    , m_previewWidget{new QQuickWidget}
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

    m_previewWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    horizontalLayout->addWidget(m_previewWidget, 1);
}

void MainWindow::initMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    m_openAction = fileMenu->addAction(tr("&Open"));
    m_openAction->setShortcut(OPEN_SHORTCUT);
    m_saveAction = fileMenu->addAction(tr("&Save"));
    m_saveAction->setShortcut(SAVE_SHORTCUT);
    m_saveAction->setEnabled(false);
    m_closeAction = fileMenu->addAction(tr("&Close"));
    m_closeAction->setShortcut(CLOSE_SHORTCUT);
    m_closeAction->setEnabled(false);
    m_reloadAction = fileMenu->addAction(tr("&Reload"));
    m_reloadAction->setShortcut(::RELOAD_SHORTCUT);
    m_reloadAction->setEnabled(false);

    fileMenu->addSeparator();

    QAction *quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QUIT_SHORTCUT);
}

void MainWindow::setupConnections()
{
    connect(StateController::instance(), &StateController::stateChanged, this,
            &MainWindow::onAppStateChanged);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::onFileChanged);
    connect(menuBar(), &QMenuBar::triggered, this, &MainWindow::onMenuBarTriggered);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    switch (StateController::instance()->currentState()) {
    case StateController::NewState:
    case StateController::DirtyState: {
        const QMessageBox::StandardButton answer =
            QMessageBox::question(this, tr("About to Close"),
                                  tr("There are some unsaved changes. "
                                     "Do you want to close withou saving?"),
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
        if (!m_previewWidget->source().isEmpty()) {
            m_previewWidget->engine()->clearComponentCache();
            if (const QStringList files = m_fileWatcher->files(); !files.isEmpty())
                m_fileWatcher->removePaths(files);
            m_previewWidget->setSource(QUrl{});
        }
        m_saveAction->setEnabled(false);
        m_closeAction->setEnabled(false);
        m_reloadAction->setEnabled(false);
        break;

    case StateController::OpenState:
        m_previewWidget->engine()->clearComponentCache();
        if (const QStringList files = m_fileWatcher->files(); !files.isEmpty())
            m_fileWatcher->removePaths(files);
        m_fileWatcher->addPath(StateController::instance()->filePath());
        m_previewWidget->setSource(QUrl::fromLocalFile(StateController::instance()->filePath()));
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
    m_previewWidget->engine()->clearComponentCache();
    m_previewWidget->setSource(QUrl::fromLocalFile(path));
    stateController->setDirty(false);
}

void MainWindow::onMenuBarTriggered(QAction *action)
{
    const QKeySequence shortcut = action->shortcut();
    if (shortcut == OPEN_SHORTCUT)
        m_editorWidget->openFile();
    else if (shortcut == SAVE_SHORTCUT)
        m_editorWidget->saveFile();
    else if (shortcut == CLOSE_SHORTCUT)
        m_editorWidget->closeFile();
    else if (shortcut == RELOAD_SHORTCUT)
        m_editorWidget->reloadFile();
    else if (shortcut == QUIT_SHORTCUT)
        QCoreApplication::quit();
}
