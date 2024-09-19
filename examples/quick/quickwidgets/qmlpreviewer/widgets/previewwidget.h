// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PREVIEWWIDGET_H
#define PREVIEWWIDGET_H

#include "../models/errorlistmodel.h"
#include <QWidget>

class QListView;
class QQuickView;

class PreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget *parent = nullptr);

    QString sourcePath() const;
    void setSourcePath(const QString &path);

private:
    void initUI();
    void setupConnections();

signals:
    void errorPositionSelected(int line, int column);

private slots:
    void onAppStateChanged(int oldState, int newState);
    void onQuickWidetStatusChanged(int status);

private:
    ErrorListModel m_errorListModel;
    QListView *m_errorListView = nullptr;
    QQuickView *m_quickView = nullptr;
};

#endif // PREVIEWWIDGET_H
