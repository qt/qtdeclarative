/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

    QWindow *window() const Q_DECL_OVERRIDE;

    QRect rect() const Q_DECL_OVERRIDE;
    QRect viewRect() const;

    bool clipsChildren() const;
    QAccessibleInterface *childAt(int x, int y) const Q_DECL_OVERRIDE;

    QAccessibleInterface *parent() const Q_DECL_OVERRIDE;
    QAccessibleInterface *child(int index) const Q_DECL_OVERRIDE;
    int childCount() const Q_DECL_OVERRIDE;
    int indexOfChild(const QAccessibleInterface *iface) const Q_DECL_OVERRIDE;
    QList<QQuickItem *> childItems() const;

    QAccessible::State state() const Q_DECL_OVERRIDE;
    QAccessible::Role role() const Q_DECL_OVERRIDE;
    QString text(QAccessible::Text) const Q_DECL_OVERRIDE;

    bool isAccessible() const;

    // Action Interface
    QStringList actionNames() const Q_DECL_OVERRIDE;
    void doAction(const QString &actionName) Q_DECL_OVERRIDE;
    QStringList keyBindingsForAction(const QString &actionName) const Q_DECL_OVERRIDE;

    // Value Interface
    QVariant currentValue() const Q_DECL_OVERRIDE;
    void setCurrentValue(const QVariant &value) Q_DECL_OVERRIDE;
    QVariant maximumValue() const Q_DECL_OVERRIDE;
    QVariant minimumValue() const Q_DECL_OVERRIDE;
    QVariant minimumStepSize() const Q_DECL_OVERRIDE;


    // Text Interface
    void selection(int selectionIndex, int *startOffset, int *endOffset) const Q_DECL_OVERRIDE;
    int selectionCount() const Q_DECL_OVERRIDE;
    void addSelection(int startOffset, int endOffset) Q_DECL_OVERRIDE;
    void removeSelection(int selectionIndex) Q_DECL_OVERRIDE;
    void setSelection(int selectionIndex, int startOffset, int endOffset) Q_DECL_OVERRIDE;

    // cursor
    int cursorPosition() const Q_DECL_OVERRIDE;
    void setCursorPosition(int position) Q_DECL_OVERRIDE;

    // text
    QString text(int startOffset, int endOffset) const Q_DECL_OVERRIDE;
    QString textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                     int *startOffset, int *endOffset) const Q_DECL_OVERRIDE;
    QString textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                    int *startOffset, int *endOffset) const Q_DECL_OVERRIDE;
    QString textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                 int *startOffset, int *endOffset) const Q_DECL_OVERRIDE;
    int characterCount() const Q_DECL_OVERRIDE;

    // character <-> geometry
    QRect characterRect(int /* offset */) const Q_DECL_OVERRIDE { return QRect(); }
    int offsetAtPoint(const QPoint & /* point */) const Q_DECL_OVERRIDE { return -1; }

    void scrollToSubstring(int /* startIndex */, int /* endIndex */) Q_DECL_OVERRIDE {}
    QString attributes(int /* offset */, int *startOffset, int *endOffset) const Q_DECL_OVERRIDE { *startOffset = 0; *endOffset = 0; return QString(); }

    QTextDocument *textDocument() const;

protected:
    QQuickItem *item() const { return static_cast<QQuickItem*>(object()); }
    void *interface_cast(QAccessible::InterfaceType t) Q_DECL_OVERRIDE;

private:
    QTextDocument *m_doc;
};

QRect itemScreenRect(QQuickItem *item);
QList<QQuickItem *> accessibleUnignoredChildren(QQuickItem *item, bool paintOrder = false);

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

#endif // QACCESSIBLEQUICKITEM_H
