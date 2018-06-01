/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef QQUICKTABLEVIEW_P_H
#define QQUICKTABLEVIEW_P_H

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

#include <QtCore/qpointer.h>
#include <QtQuick/private/qtquickglobal_p.h>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQml/private/qqmlnullablevalue_p.h>

QT_BEGIN_NAMESPACE

class QQuickTableViewAttached;
class QQuickTableViewPrivate;
class QQmlChangeSet;

class Q_QUICK_PRIVATE_EXPORT QQuickTableView : public QQuickFlickable
{
    Q_OBJECT

    Q_PROPERTY(int rows READ rows NOTIFY rowsChanged)
    Q_PROPERTY(int columns READ columns NOTIFY columnsChanged)
    Q_PROPERTY(qreal rowSpacing READ rowSpacing WRITE setRowSpacing NOTIFY rowSpacingChanged)
    Q_PROPERTY(qreal columnSpacing READ columnSpacing WRITE setColumnSpacing NOTIFY columnSpacingChanged)
    Q_PROPERTY(qreal topMargin READ topMargin WRITE setTopMargin NOTIFY topMarginChanged)
    Q_PROPERTY(qreal bottomMargin READ bottomMargin WRITE setBottomMargin NOTIFY bottomMarginChanged)
    Q_PROPERTY(qreal leftMargin READ leftMargin WRITE setLeftMargin NOTIFY leftMarginChanged)
    Q_PROPERTY(qreal rightMargin READ rightMargin WRITE setRightMargin NOTIFY rightMarginChanged)
    Q_PROPERTY(int cacheBuffer READ cacheBuffer WRITE setCacheBuffer NOTIFY cacheBufferChanged)

    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)

public:
    QQuickTableView(QQuickItem *parent = nullptr);

    int rows() const;
    int columns() const;

    qreal rowSpacing() const;
    void setRowSpacing(qreal spacing);

    qreal columnSpacing() const;
    void setColumnSpacing(qreal spacing);

    qreal topMargin() const;
    void setTopMargin(qreal margin);

    qreal bottomMargin() const;
    void setBottomMargin(qreal margin);

    qreal leftMargin() const;
    void setLeftMargin(qreal margin);

    qreal rightMargin() const;
    void setRightMargin(qreal margin);

    int cacheBuffer() const;
    void setCacheBuffer(int newBuffer);

    QVariant model() const;
    void setModel(const QVariant &newModel);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *);

    static QQuickTableViewAttached *qmlAttachedProperties(QObject *);

Q_SIGNALS:
    void rowsChanged();
    void columnsChanged();
    void rowSpacingChanged();
    void columnSpacingChanged();
    void topMarginChanged();
    void bottomMarginChanged();
    void leftMarginChanged();
    void rightMarginChanged();
    void cacheBufferChanged();
    void modelChanged();
    void delegateChanged();

protected:
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void viewportMoved(Qt::Orientations orientation) override;
    void componentComplete() override;

private:
    Q_DISABLE_COPY(QQuickTableView)
    Q_DECLARE_PRIVATE(QQuickTableView)
};

class Q_QUICK_PRIVATE_EXPORT QQuickTableViewAttached : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickTableView *tableView READ tableView NOTIFY tableViewChanged)
    Q_PROPERTY(qreal cellWidth READ cellWidth WRITE setCellWidth NOTIFY cellWidthChanged)
    Q_PROPERTY(qreal cellHeight READ cellHeight WRITE setCellHeight NOTIFY cellHeightChanged)
    Q_PROPERTY(int row READ row NOTIFY rowChanged)
    Q_PROPERTY(int column READ column NOTIFY columnChanged)

public:
    QQuickTableViewAttached(QObject *parent)
        : QObject(parent) {}

    QQuickTableView *tableView() const { return m_tableview; }
    void setTableView(QQuickTableView *newTableView) {
        if (newTableView == m_tableview)
            return;
        m_tableview = newTableView;
        Q_EMIT tableViewChanged();
    }

    qreal cellWidth() const { return m_cellWidth; }
    void setCellWidth(qreal newWidth) {
        if (newWidth == m_cellWidth)
            return;
        m_cellWidth = newWidth;
        Q_EMIT cellWidthChanged();
    }

    qreal cellHeight() const { return m_cellHeight; }
    void setCellHeight(qreal newHeight) {
        if (newHeight == m_cellHeight)
            return;
        m_cellHeight = newHeight;
        Q_EMIT cellHeightChanged();
    }

    int row() const { return m_row; }
    void setRow(int newRow) {
        if (newRow == m_row)
            return;
        m_row = newRow;
        Q_EMIT rowChanged();
    }

    int column() const { return m_column; }
    void setColumn(int newColumn) {
        if (newColumn == m_column)
            return;
        m_column = newColumn;
        Q_EMIT columnChanged();
    }

Q_SIGNALS:
    void tableViewChanged();
    void cellWidthChanged();
    void cellHeightChanged();
    void rowChanged();
    void columnChanged();

private:
    QPointer<QQuickTableView> m_tableview;
    int m_row = -1;
    int m_column = -1;
    QQmlNullableValue<qreal> m_cellWidth;
    QQmlNullableValue<qreal> m_cellHeight;

    friend class QQuickTableViewPrivate;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickTableView)
QML_DECLARE_TYPEINFO(QQuickTableView, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKTABLEVIEW_P_H
