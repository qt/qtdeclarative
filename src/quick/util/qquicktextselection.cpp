// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

    \note This API is considered tech preview and may change or be removed in
    future versions of Qt.

    \sa TextEdit, QTextCursor
*/

/*! \internal
    QQuickTextSelection provides QML API using QTextCursor.
    QQuickTextControl owns a text cursor, and one instance of
    QQuickTextSelection represents it and delegates all operations to it.
*/
QQuickTextSelection::QQuickTextSelection(QObject *parent)
    : QObject(parent)
{
    // When QQuickTextEdit creates its cursorSelection, it passes itself as the parent
    if (auto *textEdit = qmlobject_cast<QQuickTextEdit *>(parent)) {
        m_control = QQuickTextEditPrivate::get(textEdit)->control;
        connect(m_control, &QQuickTextControl::currentCharFormatChanged,
                this, &QQuickTextSelection::updateFromCharFormat);
        connect(m_control, &QQuickTextControl::cursorPositionChanged,
                this, &QQuickTextSelection::updateFromBlockFormat);
    }
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
    \qmlproperty color QtQuick::TextSelection::font

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
