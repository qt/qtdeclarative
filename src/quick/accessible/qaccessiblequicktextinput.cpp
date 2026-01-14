// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaccessiblequicktextinput_p.h"

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

QAccessibleQuickTextInput::QAccessibleQuickTextInput(QQuickTextInput *textEdit)
    : QAccessibleQuickItem(textEdit)
{
}

void QAccessibleQuickTextInput::removeSelection(int selectionIndex)
{
    if (selectionCount() == 1 && selectionIndex == 0) {
        const int cursorPos = textInput()->cursorPosition();
        textInput()->select(cursorPos, cursorPos);
    }
}

void QAccessibleQuickTextInput::setSelection(int selectionIndex, int startOffset, int endOffset)
{
    if (selectionIndex == 0)
        textInput()->select(startOffset, endOffset);
}

void *QAccessibleQuickTextInput::interface_cast(QAccessible::InterfaceType type)
{
    // Expose EditableTextInterface based on actual text input's editability.
    // Base class QAccessibleQuickItem::interface_cast() handles TextInterface
    // automatically for role() == EditableText|StaticText|Heading.
    if ((type == QAccessible::EditableTextInterface) && !textInput()->isReadOnly())
        return static_cast<QAccessibleEditableTextInterface*>(this);
    return QAccessibleQuickItem::interface_cast(type);
}

void QAccessibleQuickTextInput::deleteText(int startOffset, int endOffset)
{
    textInput()->remove(startOffset, endOffset);
}

void QAccessibleQuickTextInput::insertText(int offset, const QString &text)
{
    textInput()->insert(offset, text);
}

void QAccessibleQuickTextInput::replaceText(int startOffset, int endOffset, const QString &text)
{
    textInput()->remove(startOffset, endOffset);
    textInput()->insert(startOffset, text);
}

#endif // accessibility

QT_END_NAMESPACE
