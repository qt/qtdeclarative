// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKHEADERVIEW_P_H
#define QQUICKHEADERVIEW_P_H

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

#include <private/qquicktableview_p.h>
#include <private/qtquicktemplates2global_p.h>

QT_BEGIN_NAMESPACE

class QQuickHeaderViewBase;
class QQuickHeaderViewBasePrivate;
class Q_QUICKTEMPLATES2_EXPORT QQuickHeaderViewBase : public QQuickTableView
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickHeaderViewBase)
    Q_PROPERTY(QString textRole READ textRole WRITE setTextRole NOTIFY textRoleChanged FINAL)

public:
    explicit QQuickHeaderViewBase(Qt::Orientation orient, QQuickItem *parent = nullptr);
    ~QQuickHeaderViewBase();

    QString textRole() const;
    void setTextRole(const QString &role);

protected:
    QQuickHeaderViewBase(QQuickHeaderViewBasePrivate &dd, QQuickItem *parent);

Q_SIGNALS:
    void textRoleChanged();

private:
    Q_DISABLE_COPY(QQuickHeaderViewBase)
    friend class QQuickHorizontalHeaderView;
    friend class QQuickVerticalHeaderView;
};

class QQuickHorizontalHeaderViewPrivate;
class Q_QUICKTEMPLATES2_EXPORT QQuickHorizontalHeaderView : public QQuickHeaderViewBase
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickHorizontalHeaderView)
    Q_PROPERTY(bool movableColumns READ movableColumns WRITE setMovableColumns NOTIFY movableColumnsChanged REVISION(6, 8) FINAL)
    Q_PROPERTY(bool showSortIndicator READ showSortIndicator WRITE setShowSortIndicator
                       NOTIFY showSortIndicatorChanged REVISION(6, 13))
    Q_PROPERTY(bool sortIndicatorClearable READ sortIndicatorClearable WRITE setSortIndicatorClearable
                       NOTIFY sortIndicatorClearableChanged REVISION(6, 13))
    QML_NAMED_ELEMENT(HorizontalHeaderView)
    QML_ADDED_IN_VERSION(2, 15)

public:
    QQuickHorizontalHeaderView(QQuickItem *parent = nullptr);
    ~QQuickHorizontalHeaderView() override;

    bool movableColumns() const;
    void setMovableColumns(bool movableColumns);

    bool showSortIndicator() const;
    void setShowSortIndicator(bool show);

    bool sortIndicatorClearable() const;
    void setSortIndicatorClearable(bool clearable);

Q_SIGNALS:
    Q_REVISION(6, 8) void movableColumnsChanged();
    Q_REVISION(6, 13) void showSortIndicatorChanged();
    Q_REVISION(6, 13) void sortIndicatorClearableChanged();

    Q_REVISION(6,13) void headerClicked(int logicalColumn);

protected:
    QQuickHorizontalHeaderView(QQuickHorizontalHeaderViewPrivate &dd, QQuickItem *parent);

private:
    Q_DISABLE_COPY(QQuickHorizontalHeaderView)
};

class QQuickVerticalHeaderViewPrivate;
class Q_QUICKTEMPLATES2_EXPORT QQuickVerticalHeaderView : public QQuickHeaderViewBase
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickVerticalHeaderView)
    Q_PROPERTY(bool movableRows READ movableRows WRITE setMovableRows NOTIFY movableRowsChanged REVISION(6, 8) FINAL)
    QML_NAMED_ELEMENT(VerticalHeaderView)
    QML_ADDED_IN_VERSION(2, 15)

public:
    QQuickVerticalHeaderView(QQuickItem *parent = nullptr);
    ~QQuickVerticalHeaderView() override;

    bool movableRows() const;
    void setMovableRows(bool movableRows);

Q_SIGNALS:
    Q_REVISION(6, 8) void movableRowsChanged();

protected:
    QQuickVerticalHeaderView(QQuickVerticalHeaderViewPrivate &dd, QQuickItem *parent);

private:
    Q_DISABLE_COPY(QQuickVerticalHeaderView)
};

QT_END_NAMESPACE

#endif // QQUICKHEADERVIEW_P_H
