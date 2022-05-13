// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QQuickWidget>
#include <QQuickView>

QT_FORWARD_DECLARE_CLASS(QRadioButton)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLayout)

class MainWindow : public QWidget
{
public:
    explicit MainWindow(bool transparency, bool noRenderAlpha);

protected:
    void resizeEvent(QResizeEvent*);

private slots:
    void updateView();
    void onStatusChangedView(QQuickView::Status status);
    void onStatusChangedWidget(QQuickWidget::Status status);
    void onSceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);

private:
    void switchTo(QWidget *view);

    QRadioButton *m_radioView;
    QRadioButton *m_radioWidget;
    QCheckBox *m_checkboxMultiSample;
    QLabel *m_labelStatus;
    QLayout *m_containerLayout;
    QWidget *m_currentView;
    QObject *m_currentRootObject;
    QLabel *m_overlayLabel;
    QCheckBox *m_checkboxOverlayVisible;

    enum State {
        Unknown,
        UseWidget,
        UseWindow
    } m_state;

    QSurfaceFormat m_format;

    bool m_transparent;
    bool m_noRenderAlpha;
};

#endif
