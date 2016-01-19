/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QACCESSIBLEQUICKITEM_H
#define QACCESSIBLEQUICKITEM_H

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

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QtGui/qaccessibleobject.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_ACCESSIBILITY

class QTextDocument;

class QAccessibleQuickItem : public QAccessibleObject, public QAccessibleActionInterface, public QAccessibleValueInterface, public QAccessibleTextInterface
{
public:
    QAccessibleQuickItem(QQuickItem *item);

    QRect rect() const;
    QRect viewRect() const;

    bool clipsChildren() const;
    QAccessibleInterface *childAt(int x, int y) const;

    QAccessibleInterface *parent() const;
    QAccessibleInterface *child(int index) const;
    int childCount() const;
    int indexOfChild(const QAccessibleInterface *iface) const;
    QList<QQuickItem *> childItems() const;

    QAccessible::State state() const;
    QAccessible::Role role() const;
    QString text(QAccessible::Text) const;

    bool isAccessible() const;

    // Action Interface
    QStringList actionNames() const;
    void doAction(const QString &actionName);
    QStringList keyBindingsForAction(const QString &actionName) const;

    // Value Interface
    QVariant currentValue() const;
    void setCurrentValue(const QVariant &value);
    QVariant maximumValue() const;
    QVariant minimumValue() const;
    QVariant minimumStepSize() const;


    // Text Interface
    void selection(int selectionIndex, int *startOffset, int *endOffset) const;
    int selectionCount() const;
    void addSelection(int startOffset, int endOffset);
    void removeSelection(int selectionIndex);
    void setSelection(int selectionIndex, int startOffset, int endOffset);

    // cursor
    int cursorPosition() const;
    void setCursorPosition(int position);

    // text
    QString text(int startOffset, int endOffset) const;
    QString textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                     int *startOffset, int *endOffset) const;
    QString textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                    int *startOffset, int *endOffset) const;
    QString textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                 int *startOffset, int *endOffset) const;
    int characterCount() const;

    // character <-> geometry
    QRect characterRect(int /* offset */) const { return QRect(); }
    int offsetAtPoint(const QPoint & /* point */) const { return -1; }

    void scrollToSubstring(int /* startIndex */, int /* endIndex */) {}
    QString attributes(int /* offset */, int *startOffset, int *endOffset) const { *startOffset = 0; *endOffset = 0; return QString(); }

    QTextDocument *textDocument() const;

protected:
    QQuickItem *item() const { return static_cast<QQuickItem*>(object()); }
    void *interface_cast(QAccessible::InterfaceType t);

private:
    QTextDocument *m_doc;
};

QRect itemScreenRect(QQuickItem *item);
QList<QQuickItem *> accessibleUnignoredChildren(QQuickItem *item, bool paintOrder = false);

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

#endif // QACCESSIBLEQUICKITEM_H
