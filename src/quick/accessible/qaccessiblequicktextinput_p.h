// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QACCESSIBLEQUICKTEXTINPUT_H
#define QACCESSIBLEQUICKTEXTINPUT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qaccessiblequickitem_p.h"

#include <QtQuick/private/qquicktextinput_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

class Q_QUICK_EXPORT QAccessibleQuickTextInput : public QAccessibleQuickItem,
                                                 public QAccessibleEditableTextInterface
{
public:
    QAccessibleQuickTextInput(QQuickTextInput *textEdit);

    void removeSelection(int selectionIndex) override;
    void setSelection(int selectionIndex, int startOffset, int endOffset) override;

    // QAccessibleInterface interface
    void *interface_cast(QAccessible::InterfaceType) override;

    // QAccessibleEditableTextInterface interface
    void deleteText(int startOffset, int endOffset) override;
    void insertText(int offset, const QString &text) override;
    void replaceText(int startOffset, int endOffset, const QString &text) override;

private:
    QQuickTextInput *textInput() const { return static_cast<QQuickTextInput *>(item()); }
};

#endif // accessibility

QT_END_NAMESPACE

#endif // QACCESSIBLEQUICKTEXTINPUT_H
