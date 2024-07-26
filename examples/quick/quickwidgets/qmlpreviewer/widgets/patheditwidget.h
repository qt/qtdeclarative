// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PATHEDITWIDGET_H
#define PATHEDITWIDGET_H

#include <QWidget>

class QLineEdit;
class QToolButton;

class PathEditWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PathEditWidget(QWidget *parent = nullptr);

    QString filePath() const;

public slots:
    QString saveFilePath();
    void openFilePath();

signals:
    void fileSelected(const QString &filePath);
    void openFileRequested();

private:
    void initUI();
    void setupConnections();
    void setFilePath(const QString &filePath);

private slots:
    void onAppStateChanged(int oldState, int newState);

private:
    QLineEdit *m_lineEdit = nullptr;
    QToolButton *m_toolButton = nullptr;
};

#endif // PATHEDITWIDGET_H
