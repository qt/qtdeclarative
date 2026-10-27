// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquicktextselection_p.h"

#include <QFont>
#include <QTextOption>
#include <QtQuick/private/qquicktextcontrol_p.h>
#include <QtQuick/private/qquicktextcontrol_p_p.h>
#include <QtQuick/private/qquicktextedit_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype TextSelection
    \nativetype QQuickTextSelection
    \inqmlmodule QtQuick
    \ingroup qtquick-visual
    \ingroup qtquick-input
    \brief Represents a contiguous selection of text and its properties.
    \since 6.7

    \l {QtQuick::TextEdit::cursorSelection}{TextEdit.cursorSelection}
    represents the range of text that is currently selected (for example by
    dragging the mouse). It can be used to query and modify the selected text,
    as well as properties in the \l {QTextCharFormat}{character} and
    \l {QTextBlockFormat}{block} formats.

    Additionally, since 6.11 it's possible to create explicit non-visual instances:

    \qml
    TextEdit {
        id: textEdit

        TextSelection {
            id: sel1
        }
    }
    \endqml

    \c TextEdit.cursorSelection and any explicit TextSelection instances can be
    used to query and modify the specified ranges of text, as well as properties in
    the \l {QTextCharFormat}{character} and \l {QTextBlockFormat}{block} formats.

    \note This API is considered tech preview and may change or be removed in
    future versions of Qt.

    \sa TextEdit, QTextCursor
*/

/*! \internal
    QQuickTextSelection provides QML API using QTextCursor.
    QQuickTextControl owns a text cursor, and one instance of
    QQuickTextSelection represents it and delegates all operations to it.
    But since 6.11, the user can also create other instances of TextSelection
    in QML for the purpose of programmatic editing; in that case
    m_control remains null, m_doc is set, QQuickTextSelection owns the
    QTextCursor, and delegates all operations to it.
*/
QQuickTextSelection::QQuickTextSelection(QObject *parent)
    : QObject(parent)
{
    // When QQuickTextEdit creates its cursorSelection, it passes itself as the parent
    if (auto *textEdit = qmlobject_cast<QQuickTextEdit *>(parent)) {
        m_doc = textEdit->textDocument();
        m_control = QQuickTextEditPrivate::get(textEdit)->control;
        connect(m_control, &QQuickTextControl::currentCharFormatChanged,
                this, &QQuickTextSelection::updateFromCharFormat);
        connect(m_control, &QQuickTextControl::cursorPositionChanged,
                this, &QQuickTextSelection::updateFromBlockFormat);
    }
}

void QQuickTextSelection::componentComplete()
{
    // If TextSelection is declared inside a TextEdit,
    // the user doesn't need to set its document property;
    // but we don't set m_control because this is an independent non-visual selection
    if (!m_doc) {
        if (auto *textEdit = qmlobject_cast<QQuickTextEdit *>(parent())) {
            m_doc = textEdit->textDocument();
            m_cursor = QTextCursor(m_doc->textDocument());
        }
    }
}

/*!
    \since 6.11
    \qmlproperty TextDocument QtQuick::TextSelection::document

    The QQuickTextDocument that contains the selected text.

    \sa QtQuick::TextEdit::textDocument
*/
QQuickTextDocument *QQuickTextSelection::document() const
{
    return m_doc;
}

void QQuickTextSelection::setDocument(QQuickTextDocument *doc)
{
    if (m_doc == doc)
        return;

    m_doc = doc;
    m_cursor = QTextCursor(m_doc->textDocument());
    emit documentChanged();
}

/*!
    \since 6.11
    \qmlproperty int QtQuick::TextSelection::selectionStart

    The position before the first character in the selection.

    \sa QtQuick::TextEdit::selectionStart
*/
int QQuickTextSelection::selectionStart() const
{
    return cursor().selectionStart();
}

void QQuickTextSelection::setSelectionStart(int start)
{
    auto cur = cursor(); // copy
    if (start == cur.selectionStart())
        return;

    if (m_control) {
        cur.setPosition(start, QTextCursor::MoveAnchor);
        m_control->setTextCursor(cur);
    } else {
        m_cursor.setPosition(start, QTextCursor::MoveAnchor);
    }
    emit selectionStartChanged();
}

/*!
    \since 6.11
    \qmlproperty int QtQuick::TextSelection::selectionEnd

    The position after the last character in the selection.

    \sa QtQuick::TextEdit::selectionEnd
*/
int QQuickTextSelection::selectionEnd() const
{
    return cursor().selectionEnd();
}

void QQuickTextSelection::setSelectionEnd(int end)
{
    auto cur = cursor(); // copy
    if (end == cur.selectionEnd())
        return;

    if (m_control) {
        cur.setPosition(end, QTextCursor::KeepAnchor);
        m_control->setTextCursor(cur);
    } else {
        m_cursor.setPosition(end, QTextCursor::KeepAnchor);
    }
    emit selectionEndChanged();
}

/*!
    \qmlproperty string QtQuick::TextSelection::text

    The selected text, without any rich text markup.

    Setting this property replaces the selected text with the given string.
*/
QString QQuickTextSelection::text() const
{
    return cursor().selectedText();
}

void QQuickTextSelection::setText(const QString &text)
{
    auto cur = cursor();
    if (cur.selectedText() == text)
        return;

    cur.insertText(text);
    emit textChanged();
}

/*!
    \qmlproperty font QtQuick::TextSelection::font

    The font of the selected text.

    \sa QTextCharFormat::font()
*/
QFont QQuickTextSelection::font() const
{
    return cursor().charFormat().font();
}

void QQuickTextSelection::setFont(const QFont &font)
{
    auto cur = cursor();
    if (cur.selection().isEmpty())
        cur.select(QTextCursor::WordUnderCursor);

    if (font == cur.charFormat().font())
        return;

    QTextCharFormat fmt;
    fmt.setFont(font);
    cur.mergeCharFormat(fmt);
    emit fontChanged();
}

/*!
    \qmlproperty color QtQuick::TextSelection::color

    The foreground color of the selected text.

    \sa QTextCharFormat::foreground()
*/
QColor QQuickTextSelection::color() const
{
    return cursor().charFormat().foreground().color();
}

void QQuickTextSelection::setColor(QColor color)
{
    auto cur = cursor();
    if (cur.selection().isEmpty())
        cur.select(QTextCursor::WordUnderCursor);

    if (color == cur.charFormat().foreground().color())
        return;

    QTextCharFormat fmt;
    fmt.setForeground(color);
    cur.mergeCharFormat(fmt);
    emit colorChanged();
}

/*!
    \qmlproperty enumeration QtQuick::TextSelection::alignment

    The alignment of the block containing the selected text.

    \sa QTextBlockFormat::alignment()
*/
Qt::Alignment QQuickTextSelection::alignment() const
{
    return cursor().blockFormat().alignment();
}

void QQuickTextSelection::setAlignment(Qt::Alignment align)
{
    if (align == alignment())
        return;

    QTextBlockFormat format;
    format.setAlignment(align);
    cursor().mergeBlockFormat(format);
    emit alignmentChanged();
}

/*!
    \since 6.11
    \qmlmethod bool QtQuick::TextSelection::moveSelectionStart(MoveOperation op, int n)

    Deselect text and move \l selectionStart \a n times according to \a op,
    which is one of the following enum values:

    \value TextSelection.NoMove Keep the cursor where it is

    \value TextSelection.Start Move to the start of the document.
    \value TextSelection.StartOfLine Move to the start of the current line.
    \value TextSelection.StartOfBlock Move to the start of the current block.
    \value TextSelection.StartOfWord Move to the start of the current word.
    \value TextSelection.PreviousBlock Move to the start of the previous block.
    \value TextSelection.PreviousCharacter Move to the previous character.
    \value TextSelection.PreviousWord Move to the beginning of the previous word.
    \value TextSelection.Up Move up one line.
    \value TextSelection.Left Move left one character.
    \value TextSelection.WordLeft Move left one word.

    \value TextSelection.End Move to the end of the document.
    \value TextSelection.EndOfLine Move to the end of the current line.
    \value TextSelection.EndOfWord Move to the end of the current word.
    \value TextSelection.EndOfBlock Move to the end of the current block.
    \value TextSelection.NextBlock Move to the beginning of the next block.
    \value TextSelection.NextCharacter Move to the next character.
    \value TextSelection.NextWord Move to the next word.
    \value TextSelection.Down Move down one line.
    \value TextSelection.Right Move right one character.
    \value TextSelection.WordRight Move right one word.

    \value TextSelection.NextCell  Move to the beginning of the next table cell inside the
            current table. If the current cell is the last cell in the row, the
            cursor will move to the first cell in the next row.
    \value TextSelection.PreviousCell  Move to the beginning of the previous table cell
            inside the current table. If the current cell is the first cell in
            the row, the cursor will move to the last cell in the previous row.
    \value TextSelection.NextRow  Move to the first new cell of the next row in the current table.
    \value TextSelection.PreviousRow  Move to the last cell of the previous row in the current table.

    Returns \c true if all operations were completed successfully; otherwise
    returns \c false.

    \sa QTextCursor::movePosition, QTextCursor::MoveAnchor, QTextCursor::MoveOperation
*/
bool QQuickTextSelection::moveSelectionStart(MoveOperation op, int n)
{
    const QTextCursor::MoveOperation qop = static_cast<QTextCursor::MoveOperation>(op);
    if (m_control) {
        auto cur = cursor();
        if (cur.movePosition(qop, QTextCursor::MoveAnchor, n)) {
            m_control->setTextCursor(cur);
            return true;
        }
    } else {
        return m_cursor.movePosition(qop, QTextCursor::MoveAnchor, n);
    }

    return false;
}

/*!
    \since 6.11
    \qmlmethod void QtQuick::TextSelection::moveSelectionEnd(MoveOperation op, int n)

    Move \l selectionEnd \a n times according to \a op, which is
    one of the enum values as used in moveSelectionStart().

    If moveSelectionStart() was called immediately before,
    \c {selectionEnd == selectionStart}, and this function moves it from there
    so that a range of text becomes selected.

    \sa QTextCursor::movePosition, QTextCursor::KeepAnchor, moveSelectionStart(), QTextCursor::MoveOperation
*/
bool QQuickTextSelection::moveSelectionEnd(MoveOperation op, int n)
{
    const QTextCursor::MoveOperation qop = static_cast<QTextCursor::MoveOperation>(op);
    if (m_control) {
        auto cur = cursor();
        if (cur.movePosition(qop, QTextCursor::KeepAnchor, n)) {
            m_control->setTextCursor(cur);
            return true;
        }
    } else {
        return m_cursor.movePosition(qop, QTextCursor::KeepAnchor, n);
    }

    return false;
}

/*!
    \since 6.11
    \qmlmethod void QtQuick::TextSelection::duplicate()

    Copy the selected text forward, keeping all formatting intact
    in the copy, and end with the copy as the selection.

    For example if an entire line is selected, this function inserts a copy
    of that line immediately below, and selects the copy. If a word is
    is selected, this function copies it to the right and selects it.
*/
void QQuickTextSelection::duplicate()
{
    auto sel = cursor().selection();
    const auto start = selectionEnd();
    setSelectionStart(start);
    cursor().insertFragment(sel);

    // the fragment is inserted at the right place; now select it
    const auto end = selectionEnd();
    setSelectionStart(start);
    setSelectionEnd(end);
}

/*!
    \since 6.11
    \qmlmethod void QtQuick::TextSelection::linkTo(url destination)

    Create a hyperlink from the selected text to \a destination.

    \sa QTextCharFormat::setAnchorHref()
*/
void QQuickTextSelection::linkTo(const QUrl &destination)
{
    auto cur = cursor();
    if (cur.selection().isEmpty())
        cur.select(QTextCursor::WordUnderCursor);
    cur.beginEditBlock();
    QTextCharFormat fmt = cur.charFormat();
    fmt.setForeground(QPalette().link());
    fmt.setAnchor(true);
    fmt.setAnchorHref(destination.toString());
    cur.setCharFormat(fmt);
    cur.endEditBlock();
}

/*! \internal
    Return the cursor, which is either the graphically-manipulable cursor from
    QQuickTextControl if that is set, or else the internally-stored cursor
    with which the user is trying to mutate and/or monitor the underlying document,
    in the case that TextSelection is declared in QML.
*/
QTextCursor QQuickTextSelection::cursor() const
{
    if (m_control)
        return m_control->textCursor();
    return m_cursor;
}

inline void QQuickTextSelection::updateFromCharFormat(const QTextCharFormat &fmt)
{
    if (fmt.font() != m_charFormat.font())
        emit fontChanged();
    if (fmt.foreground().color() != m_charFormat.foreground().color())
        emit colorChanged();

    m_charFormat = fmt;
}

inline void QQuickTextSelection::updateFromBlockFormat()
{
    QTextBlockFormat fmt = cursor().blockFormat();

    if (fmt.alignment() != m_blockFormat.alignment())
        emit alignmentChanged();

    m_blockFormat = fmt;
}

QT_END_NAMESPACE

#include "moc_qquicktextselection_p.cpp"
