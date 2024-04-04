// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class SvgPainter;
class SvgManager;
class QSettings;
class QLabel;
class QQuickWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateSources();
    void selectDirectory();
    void next();
    void previous();
    void loadDirectory(const QString &newDir);

private:
    void updateCurrentDir(const QString &newDir);
    void setDirList(const QStringList &list);
    void setScale(const int scale);

    Ui::MainWindow *ui;
    SvgManager *m_manager = nullptr;
    QSettings *m_settings = nullptr;
    QLabel *m_imageLabel = nullptr;
    SvgPainter *m_svgPainter = nullptr;
    QQuickWidget *m_svgImageWidget = nullptr;
    QQuickWidget *m_qmlGeneratorWidget = nullptr;
};
#endif // MAINWINDOW_H
