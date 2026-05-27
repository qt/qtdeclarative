// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

QT_USE_NAMESPACE

class SvgPainter;
class VectorImageManager;
class QSettings;
class QLabel;
class QQuickWidget;
class QQuickView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateSource();
    void updateSources();
    void selectDirectory();
    void next();
    void previous();
    void loadDirectory(const QString &newDir);
    void updateIndex(int newIndex);
    void setLooping(bool looping);
    void vectorImageSizeUpdated();
    void updatePlayButton();
    void togglePlaying();

private:
    void updateCurrentDir(const QString &newDir);
    void setDirList(const QStringList &list);
    void setScale(const int scale);

    Ui::MainWindow *ui;
    VectorImageManager *m_manager = nullptr;
    QSettings *m_settings = nullptr;
    QLabel *m_imageLabel = nullptr;
    SvgPainter *m_svgPainter = nullptr;
    QWidget *m_vectorImageWidget = nullptr;
    QQuickWidget *m_lottieAnimationWidget = nullptr;
    QQuickView *m_vectorImageView = nullptr;
};
#endif // MAINWINDOW_H
