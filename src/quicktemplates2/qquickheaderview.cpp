/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QtQuickTemplates2/private/qquickheaderview_p_p.h>
#include <algorithm>

/*!
    \qmltype HorizontalHeaderView
    \inqmlmodule QtQuick.Controls
    \ingroup qtquickcontrols2-containers
    \inherits TableView
    \brief Provides a horizontal header view to accompany a \l TableView.

    \include qquickheaderview.qdocinc {detailed-description} {HorizontalHeaderView}

    \sa VerticalHeaderView
*/

/*!
    \qmltype VerticalHeaderView
    \inqmlmodule QtQuick.Controls
    \ingroup qtquickcontrols2-containers
    \inherits TableView
    \brief Provides a vertical header view to accompany a \l TableView.

    \include qquickheaderview.qdocinc {detailed-description} {VerticalHeaderView}

    \sa HorizontalHeaderView
*/

/*!
    \qmlproperty TableView QtQuick::HorizontalHeaderView::syncView

    \include qquickheaderview.qdocinc {syncView} {horizontally}
*/

/*!
    \qmlproperty TableView QtQuick::VerticalHeaderView::syncView

    \include qquickheaderview.qdocinc {syncView} {vertically}
*/

/*!
    \qmlproperty QVariant QtQuick::HorizontalHeaderView::model

    \include qquickheaderview.qdocinc {model} {horizontal}
*/

/*!
    \qmlproperty QVariant QtQuick::VerticalHeaderView::model

    \include qquickheaderview.qdocinc {model} {vertical}
*/

/*!
    \qmlproperty QString QtQuick::HorizontalHeaderView::textRole

    \include qquickheaderview.qdocinc {textRole}
*/

/*!
    \qmlproperty QString QtQuick::VerticalHeaderView::textRole

    \include qquickheaderview.qdocinc {textRole}
*/

QT_BEGIN_NAMESPACE

QQuickHeaderViewBasePrivate::QQuickHeaderViewBasePrivate()
    : QQuickTableViewPrivate()
{
}

QQuickHeaderViewBasePrivate::~QQuickHeaderViewBasePrivate()
{
}

const QPointer<QQuickItem> QQuickHeaderViewBasePrivate::delegateItemAt(int row, int col) const
{
    return loadedTableItem(QPoint(col, row))->item;
}

QVariant QQuickHeaderViewBasePrivate::modelImpl() const
{
    if (auto model = m_headerDataProxyModel.sourceModel())
        return QVariant::fromValue(model.data());
    if (auto model = m_transposeProxyModel.sourceModel())
        return QVariant::fromValue(model);
    return QQuickTableViewPrivate::modelImpl();
}

template <typename P, typename M>
inline bool proxyModelSetter(QQuickHeaderViewBase *const q, P &proxyModel, M *model)
{
    if (model) {
        if (model == proxyModel.sourceModel())
            return true;
        proxyModel.setSourceModel(model);
        const auto &modelVariant = QVariant::fromValue(std::addressof(proxyModel));
        bool isProxyModelChanged = (modelVariant != QQuickTableViewPrivate::get(q)->QQuickTableViewPrivate::modelImpl());
        QQuickTableViewPrivate::get(q)->QQuickTableViewPrivate::setModelImpl(modelVariant);
        //Necessary, since TableView's assigned model not changed, but proxy's source changed
        if (!isProxyModelChanged)
            emit q->modelChanged();
        return true;
    }
    proxyModel.setSourceModel(nullptr);
    return false;
}

void QQuickHeaderViewBasePrivate::setModelImpl(const QVariant &newModel)
{
    Q_Q(QQuickHeaderViewBase);
    m_modelExplicitlySetByUser = true;
    // Case 1: newModel is QAbstractTableModel
    if (proxyModelSetter(q, m_headerDataProxyModel, newModel.value<QAbstractTableModel *>()))
        return;
    // Case 2: newModel is QAbstractItemModel but not QAbstractTableModel
    if (orientation() == Qt::Horizontal
        && proxyModelSetter(q, m_transposeProxyModel, newModel.value<QAbstractItemModel *>()))
        return;

    QQuickTableViewPrivate::setModelImpl(newModel);
}

void QQuickHeaderViewBasePrivate::syncModel()
{
    Q_Q(QQuickHeaderViewBase);

    if (assignedSyncView && !m_modelExplicitlySetByUser) {
        auto newModel = assignedSyncView->model();
        if (auto m = newModel.value<QAbstractItemModel *>())
            proxyModelSetter(q, m_headerDataProxyModel, m);
    }

    QQuickTableViewPrivate::syncModel();

    isTransposed = false;
    const auto aim = model->abstractItemModel();
    if (orientation() == Qt::Horizontal) {
        // For models that are just a list or a number, and especially not a
        // table, we transpose the view when the orientation is horizontal.
        // The model (list) will then be laid out horizontally rather than
        // vertically, which is the otherwise the default.
        isTransposed = !aim || aim->columnCount() == 1;
    }
    if (m_textRole.isEmpty() && aim)
        m_textRole = QLatin1String("display");
}

void QQuickHeaderViewBasePrivate::syncSyncView()
{
    Q_Q(QQuickHeaderViewBase);
    if (assignedSyncDirection != orientation()) {
        qmlWarning(q_func()) << "Setting syncDirection other than Qt::"
                             << QVariant::fromValue(orientation()).toString()
                             << " is invalid.";
        assignedSyncDirection = orientation();
    }
    if (assignedSyncView) {
        QBoolBlocker fixupGuard(inUpdateContentSize, true);
        if (orientation() == Qt::Horizontal) {
            q->setLeftMargin(assignedSyncView->leftMargin());
            q->setRightMargin(assignedSyncView->rightMargin());
        } else {
            q->setTopMargin(assignedSyncView->topMargin());
            q->setBottomMargin(assignedSyncView->bottomMargin());
        }
    }
    QQuickTableViewPrivate::syncSyncView();
}

QQuickHeaderViewBase::QQuickHeaderViewBase(Qt::Orientation orient, QQuickItem *parent)
    : QQuickTableView(*(new QQuickHeaderViewBasePrivate), parent)
{
    d_func()->setOrientation(orient);
    setSyncDirection(orient);
}

QQuickHeaderViewBase::QQuickHeaderViewBase(QQuickHeaderViewBasePrivate &dd, QQuickItem *parent)
    : QQuickTableView(dd, parent)
{
}

QQuickHeaderViewBase::~QQuickHeaderViewBase()
{
}

QString QQuickHeaderViewBase::textRole() const
{
    Q_D(const QQuickHeaderViewBase);
    return d->m_textRole;
}

void QQuickHeaderViewBase::setTextRole(const QString &role)
{
    Q_D(QQuickHeaderViewBase);
    if (d->m_textRole == role)
        return;

    d->m_textRole = role;
    emit textRoleChanged();
}

Qt::Orientation QQuickHeaderViewBasePrivate::orientation() const
{
    return m_headerDataProxyModel.orientation();
}

void QQuickHeaderViewBasePrivate::setOrientation(Qt::Orientation orientation)
{
    if (QQuickHeaderViewBasePrivate::orientation() == orientation)
        return;
    m_headerDataProxyModel.setOrientation(orientation);
}

QQuickVerticalHeaderView::QQuickVerticalHeaderView(QQuickVerticalHeaderViewPrivate &dd, QQuickItem *parent)
    : QQuickHeaderViewBase(dd, parent)
{
}

/*! \internal
    \class QHeaderDataProxyModel
    \brief
    QHeaderDataProxyModel is a proxy AbstractItemModel type that maps
    source model's headerData() to correspondent data()
 */
QHeaderDataProxyModel::QHeaderDataProxyModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

QHeaderDataProxyModel::~QHeaderDataProxyModel() = default;

void QHeaderDataProxyModel::setSourceModel(QAbstractItemModel *newSourceModel)
{
    if (m_model == newSourceModel)
        return;
    beginResetModel();
    disconnectFromModel();
    m_model = newSourceModel;
    connectToModel();
    endResetModel();
}

QModelIndex QHeaderDataProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
}

QModelIndex QHeaderDataProxyModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

QModelIndex QHeaderDataProxyModel::sibling(int row, int column, const QModelIndex &) const
{
    return index(row, column);
}

int QHeaderDataProxyModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_model.isNull() ? -1 : (m_orientation == Qt::Horizontal ? 1 : m_model->rowCount(parent));
}

int QHeaderDataProxyModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_model.isNull() ? -1 : (m_orientation == Qt::Vertical ? 1 : m_model->columnCount(parent));
}

QVariant QHeaderDataProxyModel::data(const QModelIndex &index, int role) const
{
    if (m_model.isNull())
        return QVariant();
    if (!hasIndex(index.row(), index.column()))
        return QModelIndex();
    auto section = m_orientation == Qt::Vertical ? index.row() : index.column();
    return m_model->headerData(section, m_orientation, role);
}

bool QHeaderDataProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!hasIndex(index.row(), index.column()))
        return false;
    auto section = m_orientation == Qt::Vertical ? index.row() : index.column();
    auto ret = m_model->setHeaderData(section, m_orientation, value, role);
    emit dataChanged(index, index, { role });
    return ret;
}

bool QHeaderDataProxyModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return rowCount(parent) > 0 && columnCount(parent) > 0;
    return false;
}

QVariant QHeaderDataProxyModel::variantValue() const
{
    return QVariant::fromValue(static_cast<QObject *>(const_cast<QHeaderDataProxyModel *>(this)));
}

void QHeaderDataProxyModel::setOrientation(Qt::Orientation o)
{
    if (o == m_orientation)
        return;
    beginResetModel();
    m_orientation = o;
    endResetModel();
}

Qt::Orientation QHeaderDataProxyModel::orientation() const
{
    return m_orientation;
}

QPointer<QAbstractItemModel> QHeaderDataProxyModel::sourceModel() const
{
    return m_model;
}

void QHeaderDataProxyModel::connectToModel()
{
    if (m_model.isNull())
        return;
    connect(m_model, &QAbstractItemModel::headerDataChanged,
        this, [this](Qt::Orientation orient, int first, int last) {
            if (orient != orientation())
                return;
            if (orient == Qt::Horizontal) {
                emit dataChanged(createIndex(0, first), createIndex(0, last));
            } else {
                emit dataChanged(createIndex(first, 0), createIndex(last, 0));
            }
        });
    connect(m_model, &QAbstractItemModel::modelAboutToBeReset,
        this, &QHeaderDataProxyModel::modelAboutToBeReset, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::modelReset,
        this, &QHeaderDataProxyModel::modelReset, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::rowsAboutToBeMoved,
        this, &QHeaderDataProxyModel::rowsAboutToBeMoved, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::rowsMoved,
        this, &QHeaderDataProxyModel::rowsMoved, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::rowsAboutToBeInserted,
        this, &QHeaderDataProxyModel::rowsAboutToBeInserted, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::rowsInserted,
        this, &QHeaderDataProxyModel::rowsInserted, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved,
        this, &QHeaderDataProxyModel::rowsAboutToBeRemoved, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::rowsRemoved,
        this, &QHeaderDataProxyModel::rowsRemoved, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::columnsAboutToBeMoved,
        this, &QHeaderDataProxyModel::columnsAboutToBeMoved, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::columnsMoved,
        this, &QHeaderDataProxyModel::columnsMoved, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::columnsAboutToBeInserted,
        this, &QHeaderDataProxyModel::columnsAboutToBeInserted, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::columnsInserted,
        this, &QHeaderDataProxyModel::columnsInserted, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::columnsAboutToBeRemoved,
        this, &QHeaderDataProxyModel::columnsAboutToBeRemoved, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::columnsRemoved,
        this, &QHeaderDataProxyModel::columnsRemoved, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::layoutAboutToBeChanged,
        this, &QHeaderDataProxyModel::layoutAboutToBeChanged, Qt::UniqueConnection);
    connect(m_model, &QAbstractItemModel::layoutChanged,
        this, &QHeaderDataProxyModel::layoutChanged, Qt::UniqueConnection);
}

void QHeaderDataProxyModel::disconnectFromModel()
{
    if (m_model.isNull())
        return;
    m_model->disconnect(this);
}

QQuickHorizontalHeaderView::QQuickHorizontalHeaderView(QQuickItem *parent)
    : QQuickHeaderViewBase(Qt::Horizontal, parent)
{
    setFlickableDirection(FlickableDirection::HorizontalFlick);
}

QQuickHorizontalHeaderView::~QQuickHorizontalHeaderView()
{
}

QQuickVerticalHeaderView::QQuickVerticalHeaderView(QQuickItem *parent)
    : QQuickHeaderViewBase(Qt::Vertical, parent)
{
    setFlickableDirection(FlickableDirection::VerticalFlick);
}

QQuickVerticalHeaderView::~QQuickVerticalHeaderView()
{
}

QQuickHorizontalHeaderViewPrivate::QQuickHorizontalHeaderViewPrivate() = default;

QQuickHorizontalHeaderViewPrivate::~QQuickHorizontalHeaderViewPrivate() = default;

QQuickVerticalHeaderViewPrivate::QQuickVerticalHeaderViewPrivate() = default;

QQuickVerticalHeaderViewPrivate::~QQuickVerticalHeaderViewPrivate() = default;

QT_END_NAMESPACE

#include "moc_qquickheaderview_p_p.cpp"

#include "moc_qquickheaderview_p.cpp"
