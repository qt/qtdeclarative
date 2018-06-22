/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include <QtCore/QObject>
#include <QtCore/QAbstractTableModel>
#include <QtQml/qqml.h>
#include <QtQml/private/qtqmlglobal_p.h>
#include <QtQml/QJSValue>

QT_BEGIN_NAMESPACE

class Q_QML_PRIVATE_EXPORT QQmlTableModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_PROPERTY(int columnCount READ columnCount NOTIFY columnCountChanged FINAL)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged FINAL)
    Q_PROPERTY(QVariant rows READ rows WRITE setRows NOTIFY rowsChanged FINAL)

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

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void columnCountChanged();
    void rowCountChanged();
    void rowsChanged();

private:
    class ColumnPropertyInfo
    {
    public:
        ColumnPropertyInfo();
        ColumnPropertyInfo(const QString &name, QVariant::Type type, const QString &typeName);

        bool isValid() const;

        QString name;
        QVariant::Type type = QVariant::Invalid;
        QString typeName;
    };

    enum NewRowOperationFlag {
        OtherOperation, // insert(), set(), etc.
        AppendOperation
    };

    bool validateRowType(const char *functionName, const QVariant &row) const;
    bool validateNewRow(const char *functionName, const QVariant &row,
        int rowIndex, NewRowOperationFlag appendFlag = OtherOperation) const;
    bool validateRowIndex(const char *functionName, const char *argumentName, int rowIndex) const;
    bool validateColumnPropertyTypes(const char *functionName, const QVariantMap &column, int columnIndex) const;
    bool validateColumnPropertyType(const char *functionName, const QString &propertyName,
        const QVariant &propertyValue, const ColumnPropertyInfo &expectedPropertyFormat, int columnIndex) const;

    ColumnPropertyInfo findColumnPropertyInfo(int columnIndex, const QString &columnPropertyName) const;

    void doInsert(int rowIndex, const QVariant &row);

    QVariantList mRows;
    int mRowCount = 0;
    int mColumnCount = 0;
    QVector<QVector<ColumnPropertyInfo>> mColumnProperties;
    // key = property index (0 to number of properties across all columns)
    // value = role name
    QHash<int, QByteArray> mRoleNames;
    // Contains the role key to be used as the display role for each column
    // when "display" isn't explicitly specified.
    // key = column index
    // value = index (key) into mRoleNames
    QHash<int, int> mDefaultDisplayRoles;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQmlTableModel)

#endif // QQMLTABLEMODEL_P_H
