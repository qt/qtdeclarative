// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef EDITORWIDGET_H
#define EDITORWIDGET_H

#include <QMutex>
#include <QWidget>

class QTextDocument;
class QPushButton;
class QPlainTextEdit;
class PathEditWidget;

class EditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EditorWidget(QWidget *parent = nullptr);

public slots:
    void updateEditor();
    void openFile();
    void saveFile();
    void closeFile();
    void reloadFile();
    void moveCursorTo(int line, int column);

private:
    void initUI();
    void setupConnections();

private slots:
    void onAppStateChanged(int oldState, int newState);
    void onFileSelected(const QString &filePath);
    void onEditorModified(bool dirty);
    void onEditorTextChanged();

private:
    PathEditWidget *m_pathEdit = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_reloadButton = nullptr;
    QPushButton *m_closeButton = nullptr;
    QMutex m_mutex;
};

#endif // EDITORWIDGET_H
