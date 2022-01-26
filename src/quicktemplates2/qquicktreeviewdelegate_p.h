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

#ifndef QQUICKTREEVIEWDELEGATE_P_H
#define QQUICKTREEVIEWDELEGATE_P_H

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

#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquicktreeview_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickTreeViewDelegatePrivate;

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickTreeViewDelegate : public QQuickAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(qreal indentation READ indentation WRITE setIndentation NOTIFY indentationChanged FINAL)
    Q_PROPERTY(qreal leftMargin READ leftMargin WRITE setLeftMargin NOTIFY leftMarginChanged FINAL)
    Q_PROPERTY(qreal rightMargin READ rightMargin WRITE setRightMargin NOTIFY rightMarginChanged FINAL)

    // Required properties
    Q_PROPERTY(QQuickTreeView *treeView READ treeView WRITE setTreeView NOTIFY treeviewChanged REQUIRED FINAL)
    Q_PROPERTY(bool isTreeNode READ isTreeNode WRITE setIsTreeNode NOTIFY isTreeNodeChanged REQUIRED FINAL)
    Q_PROPERTY(bool hasChildren READ hasChildren WRITE setHasChildren NOTIFY hasChildrenChanged REQUIRED FINAL)
    Q_PROPERTY(bool expanded READ expanded WRITE setExpanded NOTIFY expandedChanged WRITE setExpanded REQUIRED FINAL)
    Q_PROPERTY(int depth READ depth WRITE setDepth NOTIFY depthChanged REQUIRED FINAL)

    QML_NAMED_ELEMENT(TreeViewDelegate)
    QML_ADDED_IN_VERSION(6, 3)

public:
    explicit QQuickTreeViewDelegate(QQuickItem *parent = nullptr);

    qreal indentation() const;
    void setIndentation(qreal indentation);

    bool isTreeNode() const;
    void setIsTreeNode(bool isTreeNode);

    bool hasChildren() const;
    void setHasChildren(bool hasChildren);

    bool expanded() const;
    void setExpanded(bool expanded);

    int depth() const;
    void setDepth(int depth);

    QQuickTreeView *treeView() const;
    void setTreeView(QQuickTreeView *treeView);

    qreal leftMargin() const;
    void setLeftMargin(qreal leftMargin);

    qreal rightMargin() const;
    void setRightMargin(qreal rightMargin);

signals:
    void indicatorChanged();
    void indentationChanged();
    void isTreeNodeChanged();
    void hasChildrenChanged();
    void expandedChanged();
    void depthChanged();
    void treeviewChanged();
    void leftMarginChanged();
    void rightMarginChanged();

protected:
    QFont defaultFont() const override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void componentComplete() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    Q_DISABLE_COPY(QQuickTreeViewDelegate)
    Q_DECLARE_PRIVATE(QQuickTreeViewDelegate)
};

QT_END_NAMESPACE

#endif // QQUICKTREEVIEWDELEGATE_P_H
