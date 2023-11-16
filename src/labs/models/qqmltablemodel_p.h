// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLTABLEMODEL_P_H
#define QQMLTABLEMODEL_P_H

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

#include "qqmlmodelsglobal_p.h"
#include "qqmltablemodelcolumn_p.h"

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QAbstractTableModel>
#include <QtQml/qqml.h>
#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>
#include <QtQml/QJSValue>
#include <QtQml/QQmlListProperty>

QT_REQUIRE_CONFIG(qml_table_model);

QT_BEGIN_NAMESPACE

class Q_LABSQMLMODELS_PRIVATE_EXPORT QQmlTableModel : public QAbstractTableModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(int columnCount READ columnCount NOTIFY columnCountChanged FINAL)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged FINAL)
    Q_PROPERTY(QVariant rows READ rows WRITE setRows NOTIFY rowsChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QQmlTableModelColumn> columns READ columns CONSTANT FINAL)
    Q_INTERFACES(QQmlParserStatus)
    Q_CLASSINFO("DefaultProperty", "columns")
    QML_NAMED_ELEMENT(TableModel)
    QML_ADDED_IN_VERSION(1, 0)

public:
    QQmlTableModel(QObject *parent = nullptr);
    ~QQmlTableModel() override;

    QVariant rows() const;
    void setRows(const QVariant &rows);

    Q_INVOKABLE void appendRow(const QVariant &row);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QVariant getRow(int rowIndex);
    Q_INVOKABLE void insertRow(int rowIndex, const QVariant &row);
    Q_INVOKABLE void moveRow(int fromRowIndex, int toRowIndex, int rows = 1);
    Q_INVOKABLE void removeRow(int rowIndex, int rows = 1);
    Q_INVOKABLE void setRow(int rowIndex, const QVariant &row);

    QQmlListProperty<QQmlTableModelColumn> columns();

    static void columns_append(QQmlListProperty<QQmlTableModelColumn> *property, QQmlTableModelColumn *value);
    static qsizetype columns_count(QQmlListProperty<QQmlTableModelColumn> *property);
    static QQmlTableModelColumn *columns_at(QQmlListProperty<QQmlTableModelColumn> *property, qsizetype index);
    static void columns_clear(QQmlListProperty<QQmlTableModelColumn> *property);
    static void columns_replace(QQmlListProperty<QQmlTableModelColumn> *property, qsizetype index, QQmlTableModelColumn *value);
    static void columns_removeLast(QQmlListProperty<QQmlTableModelColumn> *property);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_INVOKABLE QVariant data(const QModelIndex &index, const QString &role) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE bool setData(const QModelIndex &index, const QString &role, const QVariant &value);
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::DisplayRole) override;
    QHash<int, QByteArray> roleNames() const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

Q_SIGNALS:
    void columnCountChanged();
    void rowCountChanged();
    void rowsChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

private:
    class ColumnRoleMetadata
    {
    public:
        ColumnRoleMetadata();
        ColumnRoleMetadata(bool isStringRole, const QString &name, int type, const QString &typeName);

        bool isValid() const;

        // If this is false, it's a function role.
        bool isStringRole = false;
        QString name;
        int type = QMetaType::UnknownType;
        QString typeName;
    };

    struct ColumnMetadata
    {
        // Key = role name that will be made visible to the delegate
        // Value = metadata about that role, including actual name in the model data, type, etc.
        QHash<QString, ColumnRoleMetadata> roles;
    };

    enum NewRowOperationFlag {
        OtherOperation, // insert(), set(), etc.
        SetRowsOperation,
        AppendOperation
    };

    void doSetRows(const QVariantList &rowsAsVariantList);
    ColumnRoleMetadata fetchColumnRoleData(const QString &roleNameKey,
        QQmlTableModelColumn *tableModelColumn, int columnIndex) const;
    void fetchColumnMetadata();

    bool validateRowType(const char *functionName, const QVariant &row) const;
    bool validateNewRow(const char *functionName, const QVariant &row,
        int rowIndex, NewRowOperationFlag operation = OtherOperation) const;
    bool validateRowIndex(const char *functionName, const char *argumentName, int rowIndex) const;

    void doInsert(int rowIndex, const QVariant &row);

    bool componentCompleted = false;
    QVariantList mRows;
    QList<QQmlTableModelColumn *> mColumns;
    int mRowCount = 0;
    int mColumnCount = 0;
    // Each entry contains information about the properties of the column at that index.
    QVector<ColumnMetadata> mColumnMetadata;
    // key = property index (0 to number of properties across all columns)
    // value = role name
    QHash<int, QByteArray> mRoleNames;
};

QT_END_NAMESPACE

#endif // QQMLTABLEMODEL_P_H
