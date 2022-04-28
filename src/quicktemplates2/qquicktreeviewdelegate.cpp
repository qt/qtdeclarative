/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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

#include "qquicktreeviewdelegate_p.h"

#include <QtQuickTemplates2/private/qquickabstractbutton_p_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>
#include <QtQuick/private/qquicktreeview_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype TreeViewDelegate
    \inherits AbstractButton
    \inqmlmodule QtQuick.Controls
    \since 6.3
    \ingroup qtquickcontrols2-delegates
    \brief A delegate that can be assigned to a TreeView.

    \image qtquickcontrols2-treeviewdelegate.png

    A TreeViewDelegate is a delegate that can be assigned to the
    \l {TableView::delegate} {delegate property} of a \l TreeView.
    It renders the tree, as well as the other columns, in the view
    using the application style.

    \code
    TreeView {
        anchors.fill: parent
        delegate: TreeViewDelegate {}
        // The model needs to be a QAbstractItemModel
        // model: yourTreeModel
    }
    \endcode

    TreeViewDelegate inherits \l AbstractButton, which means that
    it's composed of three items: a \l[QML]{Control::}{background},
    a \l [QML]{Control::}{contentItem}, and an
    \l [QML]{AbstractButton::}{indicator}. TreeViewDelegate takes care
    of \l {indentation}{indenting} the contentItem and the indicator according
    their location in the tree. The indicator will only be visible if the
    delegate item is inside the \l {isTreeNode}{tree column}, and renders
    a model item \l {hasChildren}{with children}.

    If you change the indicator, it will no longer be indented by default.
    Instead you need to indent it yourself by setting the
    \l [QML] {Item::x}{x position} of the indicator, taking the \l depth and
    \l indentation into account. Below is an example of how to do that:

    \code
    TreeViewDelegate {
        indicator: Item {
            x: leftMargin + (depth * indentation)
        }
    }
    \endcode

    The position of the contentItem is controlled with \l [QML]{Control::}{padding}.
    This means that you can change the contentItem without dealing with indentation.
    But it also means that you should avoid changing padding to something else, as
    that will remove the indentation. The space to the left of the indicator is instead
    controlled with \l leftMargin. The space between the indicator and the contentItem
    is controlled with \l [QML]{Control::}{spacing}. And the space to the right of the
    contentItem is controlled with \l rightMargin.

    \sa TreeView
*/

/*!
    \qmlproperty TreeView QtQuick.Controls::TreeViewDelegate::treeView

    This property points to the \l TreeView that contains the delegate item.
*/

/*!
    \qmlproperty bool QtQuick.Controls::TreeViewDelegate::hasChildren

    This property is \c true if the model item drawn by the delegate
    has children in the model.
*/

/*!
    \qmlproperty bool QtQuick.Controls::TreeViewDelegate::isTreeNode

    This property is \c true if the delegate item draws a node in the tree.
    Only one column in the view will be used to to draw the tree, and
    therefore, only delegate items in that column will have this
    property set to \c true.

    A node in the tree is \l{indentation}{indented} according to its \c depth, and show
    an \l [QML]{AbstractButton::}{indicator} if hasChildren is \c true. Delegate items
    in other columns will have this property set to \c false.
*/

/*!
    \qmlproperty bool QtQuick.Controls::TreeViewDelegate::expanded

    This property is \c true if the model item drawn by the delegate
    is expanded in the view.
*/

/*!
    \qmlproperty int QtQuick.Controls::TreeViewDelegate::depth

    This property holds the depth of the model item drawn by the delegate.
    The depth of a model item is the same as the number of ancestors
    it has in the model.
*/

/*!
    \qmlproperty real QtQuick.Controls::TreeViewDelegate::indentation

    This property holds the space a child is indented horizontally
    relative to its parent.
*/

/*!
    \qmlproperty real QtQuick.Controls::TreeViewDelegate::leftMargin

    This property holds the space between the left edge of the view
    and the left edge of the indicator (in addition to indentation).
    If no indicator is visible, the space will be between the left
    edge of the view and the left edge of the contentItem.

    \sa rightMargin, indentation, {QQuickControl::}{spacing}
*/

/*!
    \qmlproperty real QtQuick.Controls::TreeViewDelegate::rightMargin

    This property holds the space between the right edge of the view
    and the right edge of the contentItem.

    \sa leftMargin, indentation, {QQuickControl::}{spacing}
*/

class QQuickTreeViewDelegatePrivate : public QQuickAbstractButtonPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickTreeViewDelegate)

    void updateIndicatorVisibility();
    QPalette defaultPalette() const override;

public:
    QPointer<QQuickTreeView> m_treeView;
    qreal m_indentation = 18;
    qreal m_leftMargin = 0;
    qreal m_rightMargin = 0;
    bool m_isTreeNode = false;
    bool m_expanded = false;
    bool m_hasChildren = 0;
    int m_depth = 0;
};

void QQuickTreeViewDelegatePrivate::updateIndicatorVisibility()
{
    Q_Q(QQuickTreeViewDelegate);

    if (auto indicator = q_func()->indicator()) {
        const bool insideDelegateBounds = indicator->x() + indicator->width() < q->width();
        indicator->setVisible(m_isTreeNode && m_hasChildren && insideDelegateBounds);
    }
}

QQuickTreeViewDelegate::QQuickTreeViewDelegate(QQuickItem *parent)
    : QQuickAbstractButton(*(new QQuickTreeViewDelegatePrivate), parent)
{
}

void QQuickTreeViewDelegate::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTreeViewDelegate);

    QQuickAbstractButton::geometryChange(newGeometry, oldGeometry);
    d->updateIndicatorVisibility();
}

void QQuickTreeViewDelegate::mousePressEvent(QMouseEvent *event)
{
    QQuickAbstractButton::mousePressEvent(event);

    if (event->buttons() != Qt::LeftButton || event->modifiers() != Qt::NoModifier) {
        // Allow application to add its own pointer handlers that does something
        // other than plain expand/collapse if e.g holding down modifier keys.
        event->ignore();
        return;
    }

    const auto indicator = QQuickAbstractButton::indicator();
    if (indicator && indicator->isVisible()) {
        const auto posInIndicator = mapToItem(indicator, event->position());
        if (indicator->contains(posInIndicator)) {
            const int row = qmlContext(this)->contextProperty(QStringLiteral("row")).toInt();
            treeView()->toggleExpanded(row);
        }
    }
}

void QQuickTreeViewDelegate::mouseDoubleClickEvent(QMouseEvent *event)
{
    QQuickAbstractButton::mouseDoubleClickEvent(event);

    const int row = qmlContext(this)->contextProperty(QStringLiteral("row")).toInt();
    treeView()->toggleExpanded(row);
}

QPalette QQuickTreeViewDelegatePrivate::defaultPalette() const
{
    return QQuickTheme::palette(QQuickTheme::ItemView);
}

QFont QQuickTreeViewDelegate::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::ItemView);
}

qreal QQuickTreeViewDelegate::indentation() const
{
    return d_func()->m_indentation;
}

void QQuickTreeViewDelegate::setIndentation(qreal indentation)
{
    Q_D(QQuickTreeViewDelegate);
    if (qFuzzyCompare(d->m_indentation, indentation))
        return;

    d->m_indentation = indentation;
    emit indentationChanged();
}

bool QQuickTreeViewDelegate::isTreeNode() const
{
    return d_func()->m_isTreeNode;
}

void QQuickTreeViewDelegate::setIsTreeNode(bool isTreeNode)
{
    Q_D(QQuickTreeViewDelegate);
    if (d->m_isTreeNode == isTreeNode)
        return;

    d->m_isTreeNode = isTreeNode;
    d->updateIndicatorVisibility();
    emit isTreeNodeChanged();
}

bool QQuickTreeViewDelegate::hasChildren() const
{
    return d_func()->m_hasChildren;
}

void QQuickTreeViewDelegate::setHasChildren(bool hasChildren)
{
    Q_D(QQuickTreeViewDelegate);
    if (d->m_hasChildren == hasChildren)
        return;

    d->m_hasChildren = hasChildren;
    d->updateIndicatorVisibility();
    emit hasChildrenChanged();
}

bool QQuickTreeViewDelegate::expanded() const
{
    return d_func()->m_expanded;
}

void QQuickTreeViewDelegate::setExpanded(bool expanded)
{
    Q_D(QQuickTreeViewDelegate);
    if (d->m_expanded == expanded)
        return;

    d->m_expanded = expanded;
    emit expandedChanged();
}

int QQuickTreeViewDelegate::depth() const
{
    return d_func()->m_depth;
}

void QQuickTreeViewDelegate::setDepth(int depth)
{
    Q_D(QQuickTreeViewDelegate);
    if (d->m_depth == depth)
        return;

    d->m_depth = depth;
    emit depthChanged();
}

QQuickTreeView *QQuickTreeViewDelegate::treeView() const
{
    return d_func()->m_treeView;
}

void QQuickTreeViewDelegate::setTreeView(QQuickTreeView *treeView)
{
    Q_D(QQuickTreeViewDelegate);
    if (d->m_treeView == treeView)
        return;

    d->m_treeView = treeView;
    emit treeviewChanged();
}

void QQuickTreeViewDelegate::componentComplete()
{
    QQuickAbstractButton::componentComplete();
    d_func()->updateIndicatorVisibility();
}

qreal QQuickTreeViewDelegate::leftMargin() const
{
    return d_func()->m_leftMargin;
}

void QQuickTreeViewDelegate::setLeftMargin(qreal leftMargin)
{
    Q_D(QQuickTreeViewDelegate);
    if (qFuzzyCompare(d->m_leftMargin, leftMargin))
        return;

    d->m_leftMargin = leftMargin;
    emit leftMarginChanged();
}

qreal QQuickTreeViewDelegate::rightMargin() const
{
    return d_func()->m_rightMargin;
}

void QQuickTreeViewDelegate::setRightMargin(qreal rightMargin)
{
    Q_D(QQuickTreeViewDelegate);
    if (qFuzzyCompare(d->m_rightMargin, rightMargin))
        return;

    d->m_rightMargin = rightMargin;
    emit rightMarginChanged();
}

QT_END_NAMESPACE

#include "moc_qquicktreeviewdelegate_p.cpp"
