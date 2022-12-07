// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtQuick/QQuickView>
#include "ui_properties.h"
#include "ui_import.h"

class SplineEditor;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    void showQuickView();

public Q_SLOTS:
    void textEditTextChanged();
    void importData(int result);

protected:
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void initQml();

private:
    QQuickView quickView;
    QWidget *m_placeholder;
    Ui_Properties ui_properties;
    Ui_ImportDialog ui_import;
    SplineEditor *m_splineEditor;

};

#endif // MAINWINDOW_H
