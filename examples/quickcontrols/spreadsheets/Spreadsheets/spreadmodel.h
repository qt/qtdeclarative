// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SPREADMODEL_H
#define SPREADMODEL_H

#include "datamodel.h"

#include <QQmlEngine>
#include <QAbstractTableModel>
#include <QItemSelectionModel>
#include <QMutex>


class DataModel;
class SpreadModel;
class Formula;

class SpreadModel final : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_DISABLE_COPY_MOVE(SpreadModel)

    friend class SpreadSelectionModel;
    friend class SpreadCell;
    friend struct Formula;

protected:
    explicit SpreadModel(QObject *parent = nullptr) : QAbstractTableModel(parent) { }

public:
    int rowCount() const { return rowCount(QModelIndex{}); }
    int columnCount() const { return columnCount(QModelIndex{}); }

    const DataModel *dataModel() { return &m_dataModel; }

    int columnNumberFromName(const QString &text);
    int rowNumberFromName(const QString &text);
    // returns nullptr if it's not been created yet.
    static SpreadModel *instance();
    static SpreadModel *create(QQmlEngine *, QJSEngine *);

protected:
    Q_INVOKABLE void update(int row, int column);
    Q_INVOKABLE void clearHighlight();
    Q_INVOKABLE void setHighlight(const QModelIndex &index, bool highlight);
    Q_INVOKABLE bool clearItemData(const QModelIndexList &indexes);
    Q_INVOKABLE bool removeColumns(QModelIndexList indexes);
    Q_INVOKABLE bool removeRows(QModelIndexList indexes);
    Q_INVOKABLE void mapColumn(int model, int view);
    Q_INVOKABLE void mapRow(int model, int view);
    Q_INVOKABLE void resetColumnMapping() { m_viewColumns.clear(); }
    Q_INVOKABLE void resetRowMapping() { m_viewRows.clear(); }

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Q_INVOKABLE bool clearItemData(const QModelIndex &index) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex{}) override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex{}) override;
    bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex{}) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex{}) override;

private:
    Formula parseFormulaString(const QString &text);
    QString formulaValueText(const Formula &formula);
    int getViewColumn(int modelColumn) const;
    int getModelColumn(int viewColumn) const;
    int getViewRow(int modelRow) const;
    int getModelRow(int viewRow) const;

private:
    std::pair<int, int> m_size {1000, 26};  // rows:1-1000, columns:A-Z
    DataModel m_dataModel;
    QMap<int, int> m_viewColumns;
    QMap<int, int> m_viewRows;
    QMutex m_updateMutex;
    static inline SpreadModel *s_instance {nullptr};
};

class SpreadSelectionModel : public QItemSelectionModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(Behavior behavior READ getBehavior WRITE setBehavior NOTIFY behaviorChanged FINAL)

public:
    enum Behavior {
        DisabledBehavior,
        SelectCells,
        SelectColumns,
        SelectRows,
    };
    Q_ENUM(Behavior)

public:
    Q_INVOKABLE void toggleColumn(int column);
    Q_INVOKABLE void deselectColumn(int column);
    Q_INVOKABLE void selectColumn(int column, bool clear = true);
    Q_INVOKABLE void toggleRow(int row);
    Q_INVOKABLE void deselectRow(int row);
    Q_INVOKABLE void selectRow(int row, bool clear = true);

protected:
    Behavior getBehavior() const { return m_behavior; }

protected slots:
    void setBehavior(Behavior);

signals:
    void behaviorChanged();

private:
    Behavior m_behavior = Behavior::DisabledBehavior;
};

class HeaderSelectionModel : public QItemSelectionModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(SpreadSelectionModel* selectionModel READ getSelectionModel WRITE setSelectionModel NOTIFY selectionModelChanged FINAL)
    Q_PROPERTY(Qt::Orientation orientation READ getOrientation WRITE setOrientation NOTIFY orientationChanged FINAL)

protected:
    Q_INVOKABLE void setCurrent(int current = -1);
    SpreadSelectionModel *getSelectionModel() { return m_selectionModel; }
    Qt::Orientation getOrientation() const { return m_orientation; }

protected slots:
    void setSelectionModel(SpreadSelectionModel *selectionModel);
    void setOrientation(Qt::Orientation orientation);

private slots:
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

signals:
    void selectionModelChanged();
    void orientationChanged();

private:
    SpreadSelectionModel *m_selectionModel = nullptr;
    Qt::Orientation m_orientation = Qt::Horizontal;
};

#endif // SPREADMODEL_H
