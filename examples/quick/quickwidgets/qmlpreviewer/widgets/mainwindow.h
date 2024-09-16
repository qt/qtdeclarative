// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class PreviewWidget;
class EditorWidget;
class QFileSystemWatcher;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void initUI();
    void initMenuBar();
    void setupConnections();
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onAppStateChanged(int oldState, int newState);
    void onFileChanged(const QString &path);
    void onMenuBarTriggered(QAction *action);

private:
    EditorWidget *m_editorWidget = nullptr;
    PreviewWidget *m_previewWidget = nullptr;
    QFileSystemWatcher *m_fileWatcher = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_closeAction = nullptr;
    QAction *m_reloadAction = nullptr;
    QAction *m_quitAction = nullptr;
};
#endif // MAINWINDOW_H
