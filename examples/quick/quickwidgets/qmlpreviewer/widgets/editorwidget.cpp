// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "editorwidget.h"
#include "codeeditor.h"
#include "patheditwidget.h"
#include "../states/statecontroller.h"
#include "../utility/syntaxhighlighter.h"

#include <QFile>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
static inline QByteArray loadFromFile(const QString &filePath)
{
    QFile file{filePath};
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return {};
    return file.readAll();
}

[[nodiscard]] static inline bool saveToFile(const QString &filePath, const QByteArray &data)
{
    QFile file{filePath};
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return false;
    file.write(data);
    return true;
}
}

EditorWidget::EditorWidget(QWidget *parent)
    : QWidget{parent}
    , m_pathEdit{new PathEditWidget}
    , m_editor{new CodeEditor}
    , m_saveButton{new QPushButton}
    , m_reloadButton{new QPushButton}
    , m_closeButton{new QPushButton}
{
    initUI();

    SyntaxHighlighter *syntaxHighlighter = new SyntaxHighlighter{this};
    syntaxHighlighter->setDocument(m_editor->document());

    setupConnections();
}

void EditorWidget::updateEditor()
{
    const QByteArray fileContent = loadFromFile(m_pathEdit->filePath());
    // TODO: show warning if fileContent.isNull()

    m_mutex.lock();
    QTextDocument *doc = m_editor->document();
    doc->setPlainText(fileContent);
    doc->setModified(false);
    m_mutex.unlock();
}

void EditorWidget::openFile()
{
    switch (StateController::instance()->currentState()) {
    case StateController::NewState:
    case StateController::DirtyState: {
        const auto answer = QMessageBox::question(this, tr("About to Open File"),
                                                  tr("You have unsaved changes. "
                                                     "Are you sure you want to discard them?"),
                                                  QMessageBox::Yes | QMessageBox::No,
                                                  QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    } break;
    default:
        break;
    }

    m_pathEdit->openFilePath();
}

void EditorWidget::saveFile()
{
    if (!m_mutex.tryLock())
        return;

    QTextDocument *doc = m_editor->document();
    QString plainText = doc->toPlainText();

    StateController* stateController = StateController::instance();

    auto save = [&](const QString &filePath) -> bool {
        if (saveToFile(filePath, plainText.toUtf8())) {
            doc->setModified(false);
            stateController->changesSaved(m_pathEdit->filePath());
            return true;
        }
        // TODO: show warning
        return false;
    };

    switch (stateController->currentState()) {
    case StateController::DirtyState:
        if (m_pathEdit->filePath() == DEFAULT_SOURCE_PATH)
            save(m_pathEdit->saveFilePath());
        else
            save(m_pathEdit->filePath());
        break;

    case StateController::NewState:
        save(m_pathEdit->saveFilePath());
        break;

    default:
        break;
    }


    m_mutex.unlock();
}

void EditorWidget::closeFile()
{
    if (!m_mutex.tryLock())
        return;
    StateController::instance()->fileClosed();
    m_mutex.unlock();
}

void EditorWidget::reloadFile()
{
    if (!m_mutex.tryLock())
        return;

    const QString filePath = m_pathEdit->filePath();

    const QByteArray fileContent = loadFromFile(filePath);
    // TODO: show warning if fileContent.isNull()

    QTextDocument *doc = m_editor->document();
    doc->setPlainText(fileContent);
    doc->setModified(false);

    StateController::instance()->setDirty(false);

    m_mutex.unlock();
}

void EditorWidget::moveCursorTo(int line, int column)
{
    if (line < 1 || column < 1)
        return;

    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line - 1);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column - 1);
    m_editor->setTextCursor(cursor);
    m_editor->setFocus();
}

void EditorWidget::initUI()
{
    QHBoxLayout *actionsLayout = new QHBoxLayout;
    actionsLayout->addWidget(m_saveButton);
    actionsLayout->addWidget(m_reloadButton);
    actionsLayout->addWidget(m_closeButton);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_pathEdit);
    layout->addWidget(m_editor);
    layout->addLayout(actionsLayout);
    layout->setContentsMargins(0,0,0,0);
    setLayout(layout);

    m_editor->setCursorWidth(2);
    m_editor->setPlaceholderText(tr("Write code or open a file"));
    m_saveButton->setText(tr("Save"));
    m_saveButton->setEnabled(false);
    m_reloadButton->setText(tr("Reload"));
    m_reloadButton->setEnabled(false);
    m_closeButton->setText(tr("Close"));
    m_closeButton->setEnabled(false);
}

void EditorWidget::setupConnections()
{
    connect(StateController::instance(), &StateController::stateChanged, this,
            &EditorWidget::onAppStateChanged);

    connect(m_pathEdit, &PathEditWidget::fileSelected, this, &EditorWidget::onFileSelected);
    connect(m_pathEdit, &PathEditWidget::openFileRequested, this, &EditorWidget::openFile);
    connect(m_editor, &QPlainTextEdit::modificationChanged, this, &EditorWidget::onEditorModified);
    connect(m_editor, &QPlainTextEdit::textChanged, this, &EditorWidget::onEditorTextChanged);
    connect(m_saveButton, &QPushButton::clicked, this, &EditorWidget::saveFile);
    connect(m_reloadButton, &QPushButton::clicked, this, &EditorWidget::reloadFile);
    connect(m_closeButton, &QPushButton::clicked, this, &EditorWidget::closeFile);
}

void EditorWidget::onAppStateChanged(int oldState, int newState)
{
    Q_UNUSED(oldState);

    m_saveButton->setEnabled((newState == StateController::NewState) ||
                             (newState == StateController::DirtyState));
    m_reloadButton->setEnabled(newState == StateController::DirtyState);
    m_closeButton->setEnabled(newState == StateController::OpenState);

    switch (newState) {
    case StateController::InitState:
        m_editor->clear();
        break;

    default:
        break;
    }
}

void EditorWidget::onFileSelected(const QString &filePath)
{
    StateController *stateController = StateController::instance();

    switch (stateController->currentState()) {
    case StateController::NewState:
        stateController->changesSaved(filePath);
        break;
    case StateController::DirtyState:
        stateController->changesSaved(filePath);
        closeFile();
        break;
    default:
        break;
    }

    const QByteArray fileContent = loadFromFile(filePath);
    // TODO: show warning if fileContent.isNull()

    if (m_mutex.tryLock()) {
        QTextDocument *doc = m_editor->document();
        doc->setPlainText(fileContent);
        doc->setModified(false);
        StateController::instance()->fileLoaded(filePath);
        m_mutex.unlock();
    }
}

void EditorWidget::onEditorModified(bool dirty)
{
    if (!m_mutex.tryLock())
        return;
    StateController::instance()->setDirty(dirty);
    m_mutex.unlock();
}

void EditorWidget::onEditorTextChanged()
{
    if (!m_mutex.tryLock())
        return;

    StateController* stateController = StateController::instance();

    switch (stateController->currentState()) {
    case StateController::InitState:
        if (!m_editor->document()->isEmpty())
            stateController->setDirty(true);
        break;

    case StateController::NewState:
        if (m_editor->document()->isEmpty())
            stateController->setDirty(false);
        break;

    default:
        break;
    }

    m_mutex.unlock();
}
