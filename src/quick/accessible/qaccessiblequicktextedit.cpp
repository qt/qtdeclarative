// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaccessiblequicktextedit_p.h"

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

QAccessibleQuickTextEdit::QAccessibleQuickTextEdit(QQuickTextEdit *textEdit)
    : QAccessibleQuickItem(textEdit)
{
}

void QAccessibleQuickTextEdit::removeSelection(int selectionIndex)
{
    if (selectionCount() == 1 && selectionIndex == 0) {
        const int cursorPos = textEdit()->cursorPosition();
        textEdit()->select(cursorPos, cursorPos);
    }
}

void QAccessibleQuickTextEdit::setSelection(int selectionIndex, int startOffset, int endOffset)
{
    if (selectionIndex == 0)
        textEdit()->select(startOffset, endOffset);
}

void *QAccessibleQuickTextEdit::interface_cast(QAccessible::InterfaceType type)
{
    // Expose EditableTextInterface based on actual text edit's editability.
    // Base class QAccessibleQuickItem::interface_cast() handles TextInterface
    // automatically for role() == EditableText|StaticText|Heading.
    if ((type == QAccessible::EditableTextInterface) && !textEdit()->isReadOnly())
        return static_cast<QAccessibleEditableTextInterface *>(this);
    return QAccessibleQuickItem::interface_cast(type);
}

void QAccessibleQuickTextEdit::deleteText(int startOffset, int endOffset)
{
    textEdit()->remove(startOffset, endOffset);
}

void QAccessibleQuickTextEdit::insertText(int offset, const QString &text)
{
    textEdit()->insert(offset, text);
}

void QAccessibleQuickTextEdit::replaceText(int startOffset, int endOffset, const QString &text)
{
    textEdit()->remove(startOffset, endOffset);
    textEdit()->insert(startOffset, text);
}

#endif // accessibility

QT_END_NAMESPACE
