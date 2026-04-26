// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QStandardItemModel>
#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QQmlExpression>
#include <QtQml/qqmlcomponent.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQmlModels/private/qqmllistmodel_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>
#include <QtQmlModels/private/qqmlvaluefilter_p.h>
#include <QtQmlModels/private/qqmlfunctionfilter_p.h>
#include <QtQmlModels/private/qqmlanyoffilter_p.h>
#include <QtQmlModels/private/qqmlalloffilter_p.h>
#include <QtQmlModels/private/qqmlstringsorter_p.h>
#include <QtQmlModels/private/qqmlfunctionsorter_p.h>

class tst_QQmlSortFilterProxyModel : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQmlSortFilterProxyModel() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void checkProxyModel();

    void validateSfpmFilterListProperty();
    void validateSfpmSorterListProperty();

    void validateFilters();
    void multipleFilters();
    void filterAbility();
    void filterInverted();
    void validateValueFilterProperties();
    void validateFunctionFilterProperties();
    void verifyDynamicSortFilterProperty();

    void validateSorters_data();
    void validateSorters();
    void multipleSorters_data();
    void multipleSorters();
    void sorterAbility_data();
    void sorterAbility();
    void validateRoleSorterProperties_data();
    void validateRoleSorterProperties();
    void validateStringSorterProperties_data();
    void validateStringSorterProperties();
    void validateFunctionSorterProperties_data();
    void validateFunctionSorterProperties();

    void primarySorter_data();
    void primarySorter();

    void anyOfFilter();
    void allOfFilter();
    void anyOfFilterInverted();
    void anyOfFilterEnabled();
    void nestedCompositeFilters();
    void invalidateParentWhenChildFilterEnabledOrDisabled();
    void invalidateChildWhenParentFilterEnabledOrDisabled();
    void compositeFiltersInQml();
};

class CustomTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    static const int s_rowCount = 5;
    static const int s_columnCount = 3;
    CustomTableModel(QObject *parent = nullptr) : QAbstractTableModel(parent) {
        m_data.resize(s_rowCount);
        for (int r = 0; r < s_rowCount; ++r) {
            auto &curRow = m_data[r];
            curRow.resize(s_columnCount);
            for (int c = 0; c < s_columnCount; ++c)
                m_data[r][c] = QString::fromLatin1("Data") + QString::number(r) + QString::number(c);
        }
    }
    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> roles;
        for (int columnIndex = 0; columnIndex < s_columnCount; columnIndex++)
            roles.insertOrAssign(Qt::UserRole + columnIndex,
                                 QString(QString::fromLatin1("column") + QString::number(columnIndex)).toUtf8());
        return roles;
    }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : m_data.count();
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : s_columnCount;
    }
    // 0, 1, 2 =>
    // if row == 0:
    //     first = 0
    //     last = count
    // else if (row == rowCount())
    //     first = rowCount - 1
    //     last = rowCount() - 1 + count
    // else
    //     first = row
    //     last = row + count
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override {
        int first = (row ? (row < rowCount() ? row : rowCount() - 1) : count - 1);
        int last = (row ? (row < rowCount() ? row + count - 1: rowCount() - 1 + count - 1) : count - 1);
        beginInsertRows(parent, first, last);
        int totalRowCount = rowCount() + count;
        m_data.resize(totalRowCount);
        for (int r = 0; r < totalRowCount; ++r) {
            auto &curRow = m_data[r];
            curRow.resize(columnCount());
            for (int c = 0; c < columnCount(); ++c)
                m_data[r][c] = QString::fromLatin1("Data") + QString::number(r) + QString::number(c);
        }
        endInsertRows();
        return true;
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if ((role < Qt::UserRole && role >= Qt::UserRole + s_columnCount) || !index.isValid() || (index.column() != (role - Qt::UserRole)))
            return QVariant();
        return m_data[index.row()][role - Qt::UserRole];
    }
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override {
        if ((role < Qt::UserRole && role >= Qt::UserRole + s_columnCount) || !index.isValid() || (index.column() != (role - Qt::UserRole)))
            return false;
        m_data[index.row()][role - Qt::UserRole] = value;
        dataChanged(index, index, {role});
        return true;
    }
    QList<QList<QVariant>> m_data;
};

class ExpressionObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlScriptString scriptString READ scriptString WRITE setScriptString)

public:
    ExpressionObject(QObject *parent = nullptr) : QObject(parent) {}

    QQmlScriptString scriptString() const { return m_scriptString; }
    void setScriptString(QQmlScriptString scriptString) { m_scriptString = scriptString; }

private:
    QQmlScriptString m_scriptString;
};

void tst_QQmlSortFilterProxyModel::checkProxyModel()
{
    std::unique_ptr<QStandardItemModel> standardModel(new QStandardItemModel(2, 2, this));
    std::unique_ptr<QQmlSortFilterProxyModel> proxyModel(new QQmlSortFilterProxyModel(this));

    // Set the standard model as the source model to the proxy
    proxyModel->setSourceModel(standardModel.get());

    // Test proxy model to ensure that it abides by the rules of item models
    // QAbstractItemModelTester verifies it internally when its created
    {
        std::unique_ptr<QAbstractItemModelTester> abstractModelTest(new QAbstractItemModelTester(proxyModel.get(), this));
    }

    // Set data in the source model and verify the same can be accessed through the proxy model
    for (int row = 0; row < standardModel->rowCount(); ++row) {
        for (int column = 0; column < standardModel->columnCount(); ++column) {
            QString testData;
            testData += QString("Data") + QString::number(row) + QString::number(column);
            standardModel->setData(standardModel->index(row, column), testData, Qt::DisplayRole);
        }
    }

    // Reset the model to the standard model
    proxyModel->setSourceModel(standardModel.get());
    QCOMPARE(proxyModel->rowCount(), standardModel->rowCount());
    QCOMPARE(proxyModel->columnCount(), standardModel->columnCount());
    for (int row = 0; row < proxyModel->rowCount(); ++row) {
        for (int column = 0; column < proxyModel->columnCount(); ++column) {
            QString testData;
            testData += QString("Data") + QString::number(row) + QString::number(column);
            QCOMPARE(testData, proxyModel->data(proxyModel->index(row, column, QModelIndex()), Qt::DisplayRole));
        }
    }
}

void tst_QQmlSortFilterProxyModel::validateSfpmFilterListProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    QSignalSpy filterChangedSignal(sfpmModel, SIGNAL(filtersChanged()));
    QVERIFY(filterChangedSignal.isValid());
    auto filters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    auto *valueFilter = object->property("valueFilter").value<QQmlValueFilter *>();
    QVERIFY(valueFilter);

    // Append filter to the sfpm filters list
    sfpmModel->filters().append(&filters, valueFilter);
    QCOMPARE(filterChangedSignal.count(), 1);
    QCOMPARE(filters.count(&filters), 1);

    auto *functionFilter = object->property("functionFilter0").value<QQmlFunctionFilter *>();
    QVERIFY(functionFilter);
    sfpmModel->filters().append(&filters, functionFilter);
    // Validate the count of the filters in the list
    QCOMPARE(filterChangedSignal.count(), 2);
    QCOMPARE(filters.count(&filters), 2);

    // Access the filters in the filter list
    QCOMPARE(filters.at(&filters, 0), valueFilter);
    QCOMPARE(filters.at(&filters, 1), functionFilter);

    // Clear the filter list
    sfpmModel->filters().clear(&filters);
    QCOMPARE(filterChangedSignal.count(), 3);
    QCOMPARE(filters.count(&filters), 0);
}

void tst_QQmlSortFilterProxyModel::validateFilters()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    CustomTableModel tableModel;
    QCOMPARE(tableModel.rowCount(), CustomTableModel::s_rowCount);
    QCOMPARE(tableModel.columnCount(), CustomTableModel::s_columnCount);

    // Set the value filter in Qml SFPM
    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    sfpmModel->setSourceModel(&tableModel);
    auto filters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();

    auto resetFilters = [sfpmModel, &filters] {
        sfpmModel->filters().clear(&filters);
        QCOMPARE(sfpmModel->rowCount(), CustomTableModel::s_rowCount);
        QCOMPARE(sfpmModel->columnCount(), CustomTableModel::s_columnCount);
    };

    // Value filter
    {
        auto *valueFilter = object->property("valueFilter").value<QQmlValueFilter *>();
        QVERIFY(valueFilter);
        valueFilter->setValue(tableModel.data(tableModel.index(0, 0, QModelIndex()), Qt::UserRole));
        valueFilter->setColumn(0);
        valueFilter->setRoleName(QString::fromLatin1("column0"));

        sfpmModel->filters().append(&filters, valueFilter);
        QCOMPARE(sfpmModel->rowCount(), 1);
        QCOMPARE(sfpmModel->data(sfpmModel->index(0, 0, QModelIndex()), Qt::UserRole),
                 tableModel.data(tableModel.index(0, 0, QModelIndex()), Qt::UserRole));
    }

    // Function filter
    {
        resetFilters();
        auto *functionFilter = object->property("functionFilter1").value<QQmlFunctionFilter *>();
        QVERIFY(functionFilter);
        functionFilter->setProperty("expression", "Data01");

        sfpmModel->filters().append(&filters, functionFilter);
        QCOMPARE(sfpmModel->rowCount(), 1);
        QCOMPARE(sfpmModel->columnCount(), CustomTableModel::s_columnCount);
    }

    resetFilters();
}

void tst_QQmlSortFilterProxyModel::multipleFilters()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    CustomTableModel tableModel;
    QCOMPARE(tableModel.rowCount(), CustomTableModel::s_rowCount);
    QCOMPARE(tableModel.columnCount(), CustomTableModel::s_columnCount);

    // Set the value filter in Qml SFPM
    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    sfpmModel->setSourceModel(&tableModel);
    auto *functionFilter = object->property("functionFilter0").value<QQmlFunctionFilter *>();
    QVERIFY(functionFilter);
    functionFilter->setProperty("expression", "Data00");

    auto *valueFilter = object->property("valueFilter").value<QQmlValueFilter *>();
    QVERIFY(valueFilter);
    valueFilter->setValue(tableModel.data(tableModel.index(0, 0, QModelIndex()), Qt::UserRole));
    valueFilter->setColumn(0);
    valueFilter->setRoleName(QString::fromLatin1("column0"));

    auto filters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    QSignalSpy filterChangedSignal(sfpmModel, SIGNAL(filtersChanged()));
    sfpmModel->filters().append(&filters, functionFilter);
    QCOMPARE(filterChangedSignal.count(), 1);
    sfpmModel->filters().append(&filters, valueFilter);
    QCOMPARE(filterChangedSignal.count(), 2);
    QCOMPARE(sfpmModel->rowCount(), 1);
    QCOMPARE(sfpmModel->columnCount(), 3);
}

void tst_QQmlSortFilterProxyModel::filterAbility()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    CustomTableModel tableModel;
    QCOMPARE(tableModel.rowCount(), CustomTableModel::s_rowCount);
    QCOMPARE(tableModel.columnCount(), CustomTableModel::s_columnCount);

    // Set the value filter in Qml SFPM
    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    sfpmModel->setSourceModel(&tableModel);

    auto *functionFilter = object->property("functionFilter0").value<QQmlFunctionFilter *>();
    QVERIFY(functionFilter);
    // Filter first two rows of data through regex
    functionFilter->setProperty("useRegularExpression", true);
    functionFilter->setProperty("expression", "Data[0-1]{2}");

    auto *valueFilter = object->property("valueFilter").value<QQmlValueFilter *>();
    QVERIFY(valueFilter);
    valueFilter->setValue(tableModel.data(tableModel.index(0, 0, QModelIndex()), Qt::UserRole));
    valueFilter->setColumn(0);
    valueFilter->setRoleName(QString::fromLatin1("column0"));

    auto filters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    QSignalSpy filterChangedSignal(sfpmModel, SIGNAL(filtersChanged()));
    sfpmModel->filters().append(&filters, functionFilter);
    QCOMPARE(filterChangedSignal.count(), 1);
    sfpmModel->filters().append(&filters, valueFilter);
    QCOMPARE(filterChangedSignal.count(), 2);
    // Function and role both filters the data
    QCOMPARE(sfpmModel->rowCount(), 1);
    QCOMPARE(sfpmModel->columnCount(), 3);
    // Disable the value filter and check the filtered data
    valueFilter->setEnabled(false);
    QCOMPARE(sfpmModel->filters().count(&filters), 2);
    // Function filter still enabled in the filters list
    QCOMPARE(sfpmModel->rowCount(), 2);
    QCOMPARE(sfpmModel->columnCount(), 3);
}

void tst_QQmlSortFilterProxyModel::filterInverted()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    CustomTableModel tableModel;
    QCOMPARE(tableModel.rowCount(), CustomTableModel::s_rowCount);
    QCOMPARE(tableModel.columnCount(), CustomTableModel::s_columnCount);

    // Set the value filter in Qml SFPM
    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    sfpmModel->setSourceModel(&tableModel);
    auto *valueFilter = object->property("valueFilter").value<QQmlValueFilter *>();
    QVERIFY(valueFilter);
    valueFilter->setValue(tableModel.data(tableModel.index(0, 0, QModelIndex()), Qt::UserRole));
    valueFilter->setColumn(0);
    valueFilter->setRoleName(QString::fromLatin1("column0"));
    valueFilter->setInverted(true);
    auto filters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    QSignalSpy filterChangedSignal(sfpmModel, SIGNAL(filtersChanged()));
    sfpmModel->filters().append(&filters, valueFilter);
    QCOMPARE(filterChangedSignal.count(), 1);
    QCOMPARE(sfpmModel->rowCount(), 4);
    QCOMPARE(sfpmModel->columnCount(), CustomTableModel::s_columnCount);
}

void tst_QQmlSortFilterProxyModel::validateValueFilterProperties()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    CustomTableModel tableModel;
    QCOMPARE(tableModel.rowCount(), CustomTableModel::s_rowCount);
    QCOMPARE(tableModel.columnCount(), CustomTableModel::s_columnCount);

    // Set the value filter in Qml SFPM
    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    sfpmModel->setSourceModel(&tableModel);
    auto filters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    auto *valueFilter = object->property("valueFilter").value<QQmlValueFilter *>();
    QVERIFY(valueFilter);
    sfpmModel->filters().append(&filters, valueFilter);
    QCOMPARE(sfpmModel->filters().count(&filters), 1);

    QSignalSpy valueChngdSpy(valueFilter, &QQmlValueFilter::valueChanged);
    QVERIFY(valueChngdSpy.isValid());
    QSignalSpy roleNameChangedSpy(valueFilter, &QQmlValueFilter::roleNameChanged);
    QVERIFY(roleNameChangedSpy.isValid());
    QSignalSpy columnChangedSpy(valueFilter, &QQmlValueFilter::columnChanged);
    QVERIFY(columnChangedSpy.isValid());

    valueFilter->setRoleName(QString::fromLatin1("column1"));
    QCOMPARE(roleNameChangedSpy.count(), 1);
    valueFilter->setColumn(1);
    QCOMPARE(columnChangedSpy.count(), 1);
    valueFilter->setValue(tableModel.data(tableModel.index(1, 1, QModelIndex()), Qt::UserRole + 1));
    QCOMPARE(valueChngdSpy.count(), 1);
    QCOMPARE(sfpmModel->rowCount(), 1);
    QCOMPARE(sfpmModel->columnCount(), CustomTableModel::s_columnCount);
    QCOMPARE(sfpmModel->data(sfpmModel->index(0, 1, QModelIndex()), Qt::UserRole + 1),
             tableModel.data(tableModel.index(1, 1, QModelIndex()), Qt::UserRole + 1));

    valueFilter->setRoleName(QString::fromLatin1("column2"));
    QCOMPARE(roleNameChangedSpy.count(), 2);
    QCOMPARE(sfpmModel->rowCount(), 0);
    QCOMPARE(sfpmModel->columnCount(), CustomTableModel::s_columnCount);
    valueFilter->setValue(tableModel.data(tableModel.index(2, 2, QModelIndex()), Qt::UserRole + 2));
    QCOMPARE(valueChngdSpy.count(), 2);
    valueFilter->setColumn(2);
    QCOMPARE(columnChangedSpy.count(), 2);
    QCOMPARE(sfpmModel->rowCount(), 1);
    QCOMPARE(sfpmModel->columnCount(), CustomTableModel::s_columnCount);
    QCOMPARE(sfpmModel->data(sfpmModel->index(0, 2, QModelIndex()), Qt::UserRole + 2),
             tableModel.data(tableModel.index(2, 2, QModelIndex()), Qt::UserRole + 2));

    valueFilter->setColumn(-1);
    QCOMPARE(sfpmModel->rowCount(), 1);
    QCOMPARE(sfpmModel->columnCount(), CustomTableModel::s_columnCount);
    QCOMPARE(sfpmModel->data(sfpmModel->index(0, 2, QModelIndex()), Qt::UserRole + 2),
             tableModel.data(tableModel.index(2, 2, QModelIndex()), Qt::UserRole + 2));
}

void tst_QQmlSortFilterProxyModel::validateFunctionFilterProperties()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    CustomTableModel tableModel;
    QCOMPARE(tableModel.rowCount(), CustomTableModel::s_rowCount);
    QCOMPARE(tableModel.columnCount(), CustomTableModel::s_columnCount);

    // Set the value filter in Qml SFPM
    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    sfpmModel->setSourceModel(&tableModel);
    auto filters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();

    auto *functionFilter = object->property("functionFilter1").value<QQmlFunctionFilter *>();
    QVERIFY(functionFilter);
    functionFilter->setProperty("useRegularExpression", true);
    functionFilter->setProperty("expression", ".*");

    filters.append(&filters, functionFilter);
    QCOMPARE(sfpmModel->rowCount(), CustomTableModel::s_rowCount);
    QCOMPARE(sfpmModel->columnCount(), CustomTableModel::s_columnCount);

    functionFilter->setProperty("expression", "Data01");
    // Need to reset here because the model does not see the change in the filter's expression.
    filters.clear(&filters);
    filters.append(&filters, functionFilter);

    QCOMPARE(sfpmModel->rowCount(), 1);
    QCOMPARE(sfpmModel->columnCount(), CustomTableModel::s_columnCount);
}

void tst_QQmlSortFilterProxyModel::verifyDynamicSortFilterProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    CustomTableModel tableModel;
    QCOMPARE(tableModel.rowCount(), CustomTableModel::s_rowCount);
    QCOMPARE(tableModel.columnCount(), CustomTableModel::s_columnCount);

    // Set the value filter in Qml SFPM
    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    sfpmModel->setSourceModel(&tableModel);
    QCOMPARE(sfpmModel->rowCount(), CustomTableModel::s_rowCount);
    QCOMPARE(sfpmModel->columnCount(), CustomTableModel::s_columnCount);

    auto filters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    QCOMPARE(filters.count(&filters), 0);

    auto *functionFilter = object->property("functionFilter1").value<QQmlFunctionFilter *>();
    functionFilter->setProperty("expression", "Data01");
    QVERIFY(functionFilter);
    filters.append(&filters, functionFilter);
    QCOMPARE(filters.count(&filters), 1);
    QCOMPARE(sfpmModel->rowCount(), 1);
    QCOMPARE(sfpmModel->columnCount(), CustomTableModel::s_columnCount);

    QCOMPARE(sfpmModel->dynamicSortFilter(), true);
    sfpmModel->setDynamicSortFilter(false);
    QCOMPARE(sfpmModel->dynamicSortFilter(), false);
    tableModel.setData(tableModel.index(0, 1), QString::fromLatin1("Data99"), Qt::UserRole + 1);
    QCOMPARE(sfpmModel->rowCount(), 1);
    QCOMPARE(sfpmModel->columnCount(), CustomTableModel::s_columnCount);

    sfpmModel->invalidate();
    QCOMPARE(sfpmModel->rowCount(), 0);
}

void tst_QQmlSortFilterProxyModel::validateSfpmSorterListProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    QSignalSpy sorterChangedSignal(sfpmModel, SIGNAL(sortersChanged()));
    QVERIFY(sorterChangedSignal.isValid());
    auto sorters = sfpmModel->property("sorters").value<QQmlListProperty<QQmlSorterBase>>();
    auto *roleSorter = object->property("roleSorter").value<QQmlRoleSorter *>();
    QVERIFY(roleSorter);

    // Append filter to the sfpm filters list
    sfpmModel->sorters().append(&sorters, roleSorter);
    QCOMPARE(sorterChangedSignal.count(), 1);
    QCOMPARE(sorters.count(&sorters), 1);

    auto *stringSorter = object->property("stringSorter").value<QQmlStringSorter *>();
    QVERIFY(stringSorter);
    sfpmModel->sorters().append(&sorters, stringSorter);
    // Validate the count of the filters in the list
    QCOMPARE(sorterChangedSignal.count(), 2);
    QCOMPARE(sorters.count(&sorters), 2);

    // Access the filters in the filter list
    QCOMPARE(sorters.at(&sorters, 0), roleSorter);
    QCOMPARE(sorters.at(&sorters, 1), stringSorter);

    // Clear the filter list
    sfpmModel->sorters().clear(&sorters);
    QCOMPARE(sorterChangedSignal.count(), 3);
    QCOMPARE(sorters.count(&sorters), 0);
}

void tst_QQmlSortFilterProxyModel::validateSorters_data()
{
    QTest::addColumn<QString>("sorterType");
    QTest::addColumn<Qt::SortOrder>("sortOrder");
    QTest::addColumn<QStringList>("initial");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("role sorter ascending") << "roleSorter"
                                           << Qt::AscendingOrder
                                           << QStringList{"canvas", "test", "shine", "beneficiary", "withdrawal"}
                                           << QStringList{"beneficiary", "canvas", "shine", "test", "withdrawal"};
    QTest::newRow("role sorter descending") << "roleSorter"
                                            << Qt::DescendingOrder
                                            << QStringList{"canvas", "test", "shine", "beneficiary", "withdrawal"}
                                            << QStringList{"withdrawal", "test", "shine", "canvas", "beneficiary"};

    QTest::newRow("string sorter ascending") << "stringSorter"
                                             << Qt::AscendingOrder
                                             << QStringList{"æfgt", "abcd", "zyd", "Æagt", "Øtyu"}
                                             << QStringList{"abcd", "zyd", "Æagt", "æfgt", "Øtyu"};
    QTest::newRow("string sorter descending") << "stringSorter"
                                              << Qt::DescendingOrder
                                             << QStringList{"æfgt", "abcd", "zyd", "Æagt", "Øtyu"}
                                             << QStringList{"Øtyu", "æfgt", "Æagt", "zyd", "abcd"};


    QTest::newRow("function sorter ascending") << "functionSorter"
                                                 << Qt::AscendingOrder
                                                 << QStringList{"I", "X", "IV", "IX", "V"}
                                                 << QStringList{"I", "IV", "V", "IX", "X"};
    QTest::newRow("function sorter descending") << "functionSorter"
                                                  << Qt::DescendingOrder
                                                  << QStringList{"I", "X", "IV", "IX", "V"}
                                                  << QStringList{"X", "IX", "V", "IV", "I"};
}

void tst_QQmlSortFilterProxyModel::validateSorters()
{
    QFETCH(QString, sorterType);
    QFETCH(Qt::SortOrder, sortOrder);
    QFETCH(QStringList, initial);
    QFETCH(QStringList, expected);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QStandardItemModel standardModel;
    for (const auto &data : initial)
        standardModel.appendRow(new QStandardItem(data));

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    sfpmModel->setSourceModel(&standardModel);
    auto sorters = sfpmModel->property("sorters").value<QQmlListProperty<QQmlSorterBase>>();
    auto *sorter = object->property(sorterType.toLatin1()).value<QQmlSorterBase *>();
    QVERIFY(sorter);
    sorter->setSortOrder(sortOrder);

    // Set the language and terrritory for the collation
    if (auto stringSorter = qobject_cast<QQmlStringSorter *>(sorter))
        stringSorter->setLocale(QLocale(QLocale::NorwegianBokmal, QLocale::Country::Norway));

    // Create expression on the fly and use it for the function sorter
    if (auto functionSorter = qobject_cast<QQmlFunctionSorter *>(sorter)) {
        functionSorter->setProperty("expression",
                                    "(sfpmTestObject.getValue(lhsData.display) < sfpmTestObject.getValue(rhsData.display)) ? -1 : \
                                     (sfpmTestObject.getValue(lhsData.display) === sfpmTestObject.getValue(rhsData.display)) ? 0 : 1");
    }

    sorters.append(&sorters, sorter);
    QCOMPARE(sorters.count(&sorters), 1);
    int row = 0;
    for (const auto &data: expected)
        QCOMPARE(sfpmModel->data(sfpmModel->index(row++, 0, QModelIndex())), data);
}

void tst_QQmlSortFilterProxyModel::multipleSorters_data()
{
    QTest::addColumn<QStringList>("sortersList");
    QTest::addColumn<Qt::SortOrder>("sortOrder");
    QTest::addColumn<QList<QVariantList>>("initialColumnData");
    QTest::addColumn<QList<QVariantList>>("expectedColumnData");

    QTest::newRow("multiple sorters") << QStringList({"roleSorter", "stringSorter"})
                                      << Qt::AscendingOrder
                                      << QList<QVariantList>({{"abc", "ghi", "def", "xyz", "abc"}, {25, 36, 20, 30, 17}})
                                      << QList<QVariantList>({{"abc", "abc", "def", "ghi", "xyz"}, {17, 25, 20, 36, 30}});
}

void tst_QQmlSortFilterProxyModel::multipleSorters()
{
    QFETCH(QStringList, sortersList);
    QFETCH(Qt::SortOrder, sortOrder);
    QFETCH(QList<QVariantList>, initialColumnData);
    QFETCH(QList<QVariantList>, expectedColumnData);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QStandardItemModel standardModel(initialColumnData.at(0).count(), initialColumnData.count());
    int column = -1;
    for (const auto &columndata : initialColumnData) {
        int row = 0; ++column;
        for (const auto &data: columndata)
            standardModel.setData(standardModel.index(row++, column, QModelIndex()), data, Qt::DisplayRole);
    }

    auto *sorter1 = object->property(sortersList.at(0).toLatin1()).value<QQmlSorterBase *>(); // RoleSorter
    QVERIFY(sorter1);
    sorter1->setSortOrder(sortOrder);
    sorter1->setColumn(1);

    auto *sorter2 = object->property(sortersList.at(1).toLatin1()).value<QQmlSorterBase *>(); // StringSorter
    QVERIFY(sorter2);
    sorter2->setSortOrder(sortOrder);
    sorter2->setColumn(0);

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    sfpmModel->setSourceModel(&standardModel);
    auto sorters = sfpmModel->property("sorters").value<QQmlListProperty<QQmlSorterBase>>();
    sorters.append(&sorters, sorter2); // String sorter (for column 0)
    sorters.append(&sorters, sorter1); // Role sorter (for column 1)

    int expcolumn = -1;
    for (const auto &columndata : expectedColumnData) {
        int row = 0; ++expcolumn;
        for (const auto &data: columndata)
            QCOMPARE(sfpmModel->data(sfpmModel->index(row++, expcolumn, QModelIndex()), Qt::DisplayRole), data);
    }
}

void tst_QQmlSortFilterProxyModel::sorterAbility_data()
{
    QTest::addColumn<QStringList>("sortersList");
    QTest::addColumn<QVariantList>("sortersAbility");
    QTest::addColumn<Qt::SortOrder>("sortOrder");
    QTest::addColumn<QList<QVariantList>>("initialColumnData");
    QTest::addColumn<QList<QVariantList>>("expectedColumnData");

    QTest::newRow("enable or disable sorter") << QStringList({"roleSorter", "stringSorter"})
                                      << QVariantList({false, true})
                                      << Qt::AscendingOrder
                                      << QList<QVariantList>({{"abc", "ghi", "def", "xyz", "abc"}, {25, 36, 20, 30, 17}})
                                      << QList<QVariantList>({{"abc", "abc", "def", "ghi", "xyz"}, {25, 17, 20, 36, 30}});
}

void tst_QQmlSortFilterProxyModel::sorterAbility()
{
    QFETCH(QStringList, sortersList);
    QFETCH(Qt::SortOrder, sortOrder);
    QFETCH(QVariantList, sortersAbility);
    QFETCH(QList<QVariantList>, initialColumnData);
    QFETCH(QList<QVariantList>, expectedColumnData);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QStandardItemModel standardModel(initialColumnData.at(0).count(), initialColumnData.count());
    int column = -1;
    for (const auto &columnData : initialColumnData) {
        int row = 0; ++column;
        for (const auto &data: columnData)
            standardModel.setData(standardModel.index(row++, column, QModelIndex()), data, Qt::DisplayRole);
    }

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    sfpmModel->setSourceModel(&standardModel);

    auto sorters = sfpmModel->property("sorters").value<QQmlListProperty<QQmlSorterBase>>();
    auto *sorter1 = object->property(sortersList.at(0).toLatin1()).value<QQmlSorterBase *>();
    QVERIFY(sorter1);
    sorter1->setSortOrder(sortOrder);
    auto *sorter2 = object->property(sortersList.at(1).toLatin1()).value<QQmlSorterBase *>();
    QVERIFY(sorter2);
    sorter2->setSortOrder(sortOrder);

    sorters.append(&sorters, sorter2);
    sorters.append(&sorters, sorter1);

    // Disable the role sorter and thus sorting shall be restored to the previous state
    if (!sortersAbility.at(0).value<bool>())
        sorter1->setEnabled(false);

    column = -1;
    for (const auto &columnData : expectedColumnData) {
        int row = 0; ++column;
        for (const auto &data: columnData)
            QCOMPARE(sfpmModel->data(sfpmModel->index(row++, column, QModelIndex()), Qt::DisplayRole), data);
    }
}

void tst_QQmlSortFilterProxyModel::validateRoleSorterProperties_data()
{
    QTest::addColumn<QString>("sorterType");
    QTest::addColumn<Qt::SortOrder>("sortOrder");
    QTest::addColumn<int>("sortColumn");
    QTest::addColumn<bool>("sorterAbility");
    QTest::addColumn<QList<QVariantList>>("initialColumnData");
    QTest::addColumn<QList<QVariantList>>("expectedColumnData");

    QTest::newRow("role sorter enabled_ascend_column0") << "roleSorter"
                                            << Qt::AscendingOrder << 0 << true
                                            << QList<QVariantList>({{"abc", "ghi", "def", "xyz", "abc"}, {25, 36, 20, 30, 17}})
                                            << QList<QVariantList>({{"abc", "abc", "def", "ghi", "xyz"}, {25, 17, 20, 36, 30}});

    QTest::newRow("role sorter enabled_descend_column1") << "roleSorter"
                                            << Qt::DescendingOrder << 1 << true
                                            << QList<QVariantList>({{"abc", "ghi", "def", "xyz", "abc"}, {25, 36, 20, 30, 17}})
                                            << QList<QVariantList>({{"ghi", "xyz", "abc", "def", "abc"}, {36, 30, 25, 20, 17}});

    QTest::newRow("role sorter disabled_descend_column0") << "roleSorter"
                                            << Qt::AscendingOrder << 0 << false
                                            << QList<QVariantList>({{"abc", "ghi", "def", "xyz", "abc"}, {25, 36, 20, 30, 17}})
                                            << QList<QVariantList>({{"abc", "ghi", "def", "xyz", "abc"}, {25, 36, 20, 30, 17}});
}

void tst_QQmlSortFilterProxyModel::validateRoleSorterProperties()
{
    QFETCH(QString, sorterType);
    QFETCH(Qt::SortOrder, sortOrder);
    QFETCH(int, sortColumn);
    QFETCH(bool, sorterAbility);
    QFETCH(QList<QVariantList>, initialColumnData);
    QFETCH(QList<QVariantList>, expectedColumnData);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QStandardItemModel standardModel(initialColumnData.at(0).count(), initialColumnData.count());
    int column = -1;
    for (const auto &columnData: initialColumnData) {
        int row = 0; ++column;
        for (const auto &data: columnData)
            standardModel.setData(standardModel.index(row++, column, QModelIndex()), data, Qt::DisplayRole);
    }

    auto *sorter = object->property(sorterType.toLatin1()).value<QQmlSorterBase *>();
    QVERIFY(sorter);
    sorter->setColumn(sortColumn);
    sorter->setSortOrder(sortOrder);
    sorter->setEnabled(sorterAbility);

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    auto sorters = sfpmModel->property("sorters").value<QQmlListProperty<QQmlSorterBase>>();
    sfpmModel->setSourceModel(&standardModel);
    sorters.append(&sorters, sorter);

    column = -1;
    for (const auto &columnData : expectedColumnData) {
        int row = 0; ++column;
        for (const auto &data: columnData)
            QCOMPARE(sfpmModel->data(sfpmModel->index(row++, column, QModelIndex()), Qt::DisplayRole), data);
    }
}

void tst_QQmlSortFilterProxyModel::validateStringSorterProperties_data()
{
    QTest::addColumn<QString>("sorterType");
    QTest::addColumn<bool>("ignorePunctuation");
    QTest::addColumn<QString>("locale");
    QTest::addColumn<bool>("numericMode");

    QTest::addColumn<QVariantList>("initialColumnData");
    QTest::addColumn<QVariantList>("expectedColumnData");

    QTest::newRow("string sorter incorrect locale") << "stringSorter"
                                                    << false << "en_GB" << false
                                                    << QVariantList({"æfgt", "abcd", "zyd", "Æagt", "Øtyu"})
                                                    << QVariantList({"abcd", "Æagt", "æfgt", "Øtyu", "zyd"});

    QTest::newRow("string sorter correct locale") << "stringSorter"
                                                << false << "nb_NO" << false
                                                << QVariantList({"æfgt", "abcd", "zyd", "Æagt", "Øtyu"})
                                                << QVariantList({"abcd", "zyd", "Æagt", "æfgt", "Øtyu"});

    QTest::newRow("string sorter ignore punctuation") << "stringSorter"
                                                << true << "nb_NO" << false
                                                << QVariantList({"æfgt", "&abcd", "!zyd", "Æagt", "Øtyu"})
                                                << QVariantList({"&abcd", "!zyd", "Æagt", "æfgt", "Øtyu"});

    QTest::newRow("string sorter enable punctuation") << "stringSorter"
                                                << false << "nb_NO" << false
                                                << QVariantList({"æfgt", "&abcd", "!zyd", "Æagt", "Øtyu"})
                                                << QVariantList({"!zyd", "&abcd", "Æagt", "æfgt", "Øtyu"});

    QTest::newRow("string sorter enable numeral mode") << "stringSorter"
                                                   << false << "nb_NO" << true
                                                   << QVariantList({"æ97fgt", "1000abcd", "30zyd", "100Æagt", "99Øtyu"})
                                                   << QVariantList({"30zyd", "99Øtyu", "100Æagt", "1000abcd", "æ97fgt"});

    QTest::newRow("string sorter unset numeral mode") << "stringSorter"
                                               << false << "nb_NO" << false
                                               << QVariantList({"æ97fgt", "1000abcd", "30zyd", "100Æagt", "99Øtyu"})
                                               << QVariantList({"1000abcd", "100Æagt", "30zyd", "99Øtyu", "æ97fgt"});
}

void tst_QQmlSortFilterProxyModel::validateStringSorterProperties()
{
    QFETCH(QString, sorterType);
    QFETCH(bool, ignorePunctuation);
    QFETCH(QString, locale);
    QFETCH(bool, numericMode);

    QFETCH(QVariantList, initialColumnData);
    QFETCH(QVariantList, expectedColumnData);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QStandardItemModel standardModel(initialColumnData.count(), 1);
    int row = -1;
    for (const auto &data: initialColumnData)
        standardModel.setData(standardModel.index(++row, 0, QModelIndex()), data, Qt::DisplayRole);

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    sfpmModel->setSourceModel(&standardModel);

    auto *sorter = object->property(sorterType.toLatin1()).value<QQmlStringSorter *>();
    QVERIFY(sorter);
    sorter->setIgnorePunctuation(ignorePunctuation);
    sorter->setNumericMode(numericMode);
    sorter->setLocale(QLocale(locale));

    auto sorters = sfpmModel->property("sorters").value<QQmlListProperty<QQmlSorterBase>>();
    sorters.append(&sorters, sorter);

    row = -1;
    for (const auto &data: expectedColumnData)
        QCOMPARE(sfpmModel->data(sfpmModel->index(++row, 0, QModelIndex()), Qt::DisplayRole), data);
}

void tst_QQmlSortFilterProxyModel::validateFunctionSorterProperties_data()
{
    QTest::addColumn<QString>("sorterType");
    QTest::addColumn<QString>("expression");
    QTest::addColumn<QVariantList>("initialColumnData");
    QTest::addColumn<QVariantList>("expectedColumnData");

    QTest::newRow("function sorter no role data")
            << "functionSorter"
            << "(lhsData.display < rhsData.display) ? -1 : 1"
            << QVariantList({"V", "IV", "X", "IX", "III"})
            << QVariantList({"III", "IV", "IX", "V", "X"});

    QTest::newRow("function sorter access incorrect role names")
            << "functionSorter"
            << "(lhsData.display1 < rhsData.display2) ? -1 : 1"
            << QVariantList({"V", "IV", "X", "IX", "III"})
            << QVariantList({"V", "IV", "X", "IX", "III"});

    QTest::newRow("function sorter access valid role name")
            << "functionSorter"
            << "(sfpmTestObject.getValue(lhsData.display) < sfpmTestObject.getValue(rhsData.display) ? 1 : -1)"
            << QVariantList({"V", "IV", "X", "IX", "III"})
            << QVariantList({"X", "IX", "V", "IV", "III"});
}

void tst_QQmlSortFilterProxyModel::validateFunctionSorterProperties()
{
    QFETCH(QString, sorterType);
    QFETCH(QString, expression);
    QFETCH(QVariantList, initialColumnData);
    QFETCH(QVariantList, expectedColumnData);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QStandardItemModel standardModel(initialColumnData.count(), 1);
    int row = -1;
    for (const auto &data: initialColumnData)
        standardModel.setData(standardModel.index(++row, 0, QModelIndex()), data, Qt::DisplayRole);

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    sfpmModel->setSourceModel(&standardModel);

    auto *sorter = object->property(sorterType.toLatin1()).value<QQmlFunctionSorter *>();
    QVERIFY(sorter);

    // Custom qml property specifically for testing
    if (!expression.isEmpty())
        sorter->setProperty("expression", expression);

    auto sorters = sfpmModel->property("sorters").value<QQmlListProperty<QQmlSorterBase>>();
    sorters.append(&sorters, sorter);

    row = -1;
    for (const auto &data: expectedColumnData)
        QCOMPARE(sfpmModel->data(sfpmModel->index(++row, 0, QModelIndex()), Qt::DisplayRole), data);
}

void tst_QQmlSortFilterProxyModel::primarySorter_data()
{
    QTest::addColumn<QString>("sorter");
    QTest::addColumn<QString>("primarySorter");
    QTest::addColumn<int>("primarySortColumn");
    QTest::addColumn<Qt::SortOrder>("sortOrder");
    QTest::addColumn<QList<QVariantList>>("initialColumnData");
    QTest::addColumn<QList<QVariantList>>("expectedColumnData");

    QTest::newRow("multiple sorters") << "stringSorter"
                                      << "roleSorter"
                                      << 1
                                      << Qt::AscendingOrder
                                      << QList<QVariantList>({{"abc", "ghi", "def", "xyz", "abc"}, {25, 36, 20, 30, 17}})
                                      << QList<QVariantList>({{"abc", "def", "abc", "xyz", "ghi"}, {17, 20, 25, 30, 36}});
}

void tst_QQmlSortFilterProxyModel::primarySorter()
{
    QFETCH(QString, sorter);
    QFETCH(QString, primarySorter);
    QFETCH(int, primarySortColumn);
    QFETCH(Qt::SortOrder, sortOrder);
    QFETCH(QList<QVariantList>, initialColumnData);
    QFETCH(QList<QVariantList>, expectedColumnData);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCommon.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QStandardItemModel standardModel(initialColumnData.at(0).count(), initialColumnData.count());
    int column = -1;
    for (const auto &columnData : initialColumnData) {
        int row = 0; ++column;
        for (const auto &data: columnData)
            standardModel.setData(standardModel.index(row++, column, QModelIndex()), data, Qt::DisplayRole);
    }

    auto *sorter1 = object->property(sorter.toLatin1()).value<QQmlSorterBase *>(); // String Sorter
    QVERIFY(sorter1);
    sorter1->setSortOrder(sortOrder);
    sorter1->setColumn(0);
    sorter1->setPriority(0);

    auto *primarySorter1 = object->property(primarySorter.toLatin1()).value<QQmlSorterBase *>(); // Role Sorter as the primary sorter
    QVERIFY(primarySorter1);
    primarySorter1->setSortOrder(sortOrder);
    primarySorter1->setColumn(primarySortColumn);
    primarySorter1->setPriority(1);

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    sfpmModel->setSourceModel(&standardModel);
    auto sorters = sfpmModel->property("sorters").value<QQmlListProperty<QQmlSorterBase>>();
    sorters.append(&sorters, sorter1);
    sorters.append(&sorters, primarySorter1);

    sfpmModel->setPrimarySorter(primarySorter1);

    column = -1;
    for (const auto &columnData : expectedColumnData) {
        int row = 0; ++column;
        for (const auto &data: columnData)
            QCOMPARE(sfpmModel->data(sfpmModel->index(row++, column, QModelIndex()), Qt::DisplayRole), data);
    }
}

void tst_QQmlSortFilterProxyModel::anyOfFilter()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCompositeFilters.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);

    CustomTableModel tableModel;
    sfpmModel->setSourceModel(&tableModel);

    auto *childFilter1 = object->property("anyOfChild1").value<QQmlValueFilter *>();
    QVERIFY(childFilter1);
    auto *childFilter2 = object->property("anyOfChild2").value<QQmlValueFilter *>();
    QVERIFY(childFilter2);
    auto *anyOfFilter  = object->property("anyOfFilter").value<QQmlAnyOfFilter *>();
    QVERIFY(anyOfFilter);

    // child1 matches row 0, child2 matches row 1
    childFilter1->setRoleName("column0");
    childFilter1->setValue(tableModel.data(tableModel.index(0, 0), Qt::UserRole)); // "Data00"
    childFilter2->setRoleName("column0");
    childFilter2->setValue(tableModel.data(tableModel.index(1, 0), Qt::UserRole)); // "Data10"

    auto childFilters = anyOfFilter->filtersListProperty();
    anyOfFilter->filtersListProperty().append(&childFilters, childFilter1);
    anyOfFilter->filtersListProperty().append(&childFilters, childFilter2);

    auto sfpmFilters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    sfpmModel->filters().append(&sfpmFilters, anyOfFilter);

    // OR logic, 2 rows should be visible
    QCOMPARE(sfpmModel->rowCount(), 2);
    QCOMPARE(sfpmModel->columnCount(), CustomTableModel::s_columnCount);
    QCOMPARE(sfpmModel->data(sfpmModel->index(0, 0, QModelIndex()), Qt::UserRole),
             tableModel.data(tableModel.index(0, 0, QModelIndex()), Qt::UserRole));
    QCOMPARE(sfpmModel->data(sfpmModel->index(1, 0, QModelIndex()), Qt::UserRole),
             tableModel.data(tableModel.index(1, 0, QModelIndex()), Qt::UserRole));
}

void tst_QQmlSortFilterProxyModel::allOfFilter()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCompositeFilters.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);

    CustomTableModel tableModel;
    sfpmModel->setSourceModel(&tableModel);

    auto *childFilter1 = object->property("allOfChild1").value<QQmlValueFilter *>();
    QVERIFY(childFilter1);
    auto *childFilter2 = object->property("allOfChild2").value<QQmlValueFilter *>();
    QVERIFY(childFilter2);
    auto *allOfFilter  = object->property("allOfFilter").value<QQmlAllOfFilter *>();
    QVERIFY(allOfFilter);

    // child1 matches row 0 via column0, child2 matches row 0 via column1
    // Only row 0 satisfies both conditions
    childFilter1->setRoleName("column0");
    childFilter1->setValue(tableModel.data(tableModel.index(0, 0), Qt::UserRole)); // "Data00"
    childFilter2->setRoleName("column1");
    childFilter2->setValue(tableModel.data(tableModel.index(0, 1), Qt::UserRole + 1)); // "Data01"

    auto childFilters = allOfFilter->filtersListProperty();
    allOfFilter->filtersListProperty().append(&childFilters, childFilter1);
    allOfFilter->filtersListProperty().append(&childFilters, childFilter2);

    auto sfpmFilters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    sfpmModel->filters().append(&sfpmFilters, allOfFilter);

    QCOMPARE(sfpmModel->rowCount(), 1);
    QCOMPARE(sfpmModel->columnCount(), CustomTableModel::s_columnCount);
    QCOMPARE(sfpmModel->data(sfpmModel->index(0, 0, QModelIndex()), Qt::UserRole),
             tableModel.data(tableModel.index(0, 0), Qt::UserRole));
}

void tst_QQmlSortFilterProxyModel::anyOfFilterInverted()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCompositeFilters.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);

    CustomTableModel tableModel;
    sfpmModel->setSourceModel(&tableModel);

    auto *childFilter1 = object->property("anyOfChild1").value<QQmlValueFilter *>();
    QVERIFY(childFilter1);
    auto *childFilter2 = object->property("anyOfChild2").value<QQmlValueFilter *>();
    QVERIFY(childFilter2);
    auto *anyOfFilter = object->property("anyOfFilter").value<QQmlAnyOfFilter *>();
    QVERIFY(anyOfFilter);

    childFilter1->setRoleName("column0");
    childFilter1->setValue(tableModel.data(tableModel.index(0, 0), Qt::UserRole)); // "Data00"
    childFilter2->setRoleName("column0");
    childFilter2->setValue(tableModel.data(tableModel.index(1, 0), Qt::UserRole)); // "Data10"

    auto childFilters = anyOfFilter->filtersListProperty();
    anyOfFilter->filtersListProperty().append(&childFilters, childFilter1);
    anyOfFilter->filtersListProperty().append(&childFilters, childFilter2);
    anyOfFilter->setInverted(true);

    auto sfpmFilters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    sfpmModel->filters().append(&sfpmFilters, anyOfFilter);

    QCOMPARE(sfpmModel->rowCount(), 3);
}

void tst_QQmlSortFilterProxyModel::anyOfFilterEnabled()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCompositeFilters.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);

    CustomTableModel tableModel;
    sfpmModel->setSourceModel(&tableModel);

    auto *childFilter1 = object->property("anyOfChild1").value<QQmlValueFilter *>();
    QVERIFY(childFilter1);
    auto *childFilter2 = object->property("anyOfChild2").value<QQmlValueFilter *>();
    QVERIFY(childFilter2);
    auto *anyOfFilter  = object->property("anyOfFilter").value<QQmlAnyOfFilter *>();
    QVERIFY(anyOfFilter);

    childFilter1->setRoleName("column0");
    childFilter1->setValue(tableModel.data(tableModel.index(0, 0), Qt::UserRole));
    childFilter2->setRoleName("column0");
    childFilter2->setValue(tableModel.data(tableModel.index(1, 0), Qt::UserRole));

    auto childFilters = anyOfFilter->filtersListProperty();
    anyOfFilter->filtersListProperty().append(&childFilters, childFilter1);
    anyOfFilter->filtersListProperty().append(&childFilters, childFilter2);

    auto sfpmFilters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    sfpmModel->filters().append(&sfpmFilters, anyOfFilter);

    QCOMPARE(sfpmModel->rowCount(), 2);

    // Disabling the composite skips the entire group — all rows visible
    anyOfFilter->setEnabled(false);
    QCOMPARE(sfpmModel->rowCount(), CustomTableModel::s_rowCount);

    // Re-enabling restores the filter
    anyOfFilter->setEnabled(true);
    QCOMPARE(sfpmModel->rowCount(), 2);
}

void tst_QQmlSortFilterProxyModel::nestedCompositeFilters()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCompositeFilters.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);

    CustomTableModel tableModel;
    sfpmModel->setSourceModel(&tableModel);

    auto *anyChildFilter1  = object->property("nestedAnyOfChild1").value<QQmlValueFilter *>();
    QVERIFY(anyChildFilter1);
    auto *anyChildFilter2  = object->property("nestedAnyOfChild2").value<QQmlValueFilter *>();
    QVERIFY(anyChildFilter2);
    auto *allChildFilter   = object->property("nestedAllOfChild").value<QQmlValueFilter *>();
    QVERIFY(allChildFilter);
    auto *nestedAnyOfFilter = object->property("nestedAnyOf").value<QQmlAnyOfFilter *>();
    QVERIFY(nestedAnyOfFilter);
    auto *nestedAllOfFilter = object->property("nestedAllOf").value<QQmlAllOfFilter *>();
    QVERIFY(nestedAllOfFilter);

    // AnyOf matches rows 0 and 1
    anyChildFilter1->setRoleName("column0");
    anyChildFilter1->setValue(tableModel.data(tableModel.index(0, 0), Qt::UserRole)); // "Data00"
    anyChildFilter2->setRoleName("column0");
    anyChildFilter2->setValue(tableModel.data(tableModel.index(1, 0), Qt::UserRole)); // "Data10"

    // AllOf's own ValueFilter matches only row 0 (column1 = "Data01")
    allChildFilter->setRoleName("column1");
    allChildFilter->setValue(tableModel.data(tableModel.index(0, 1), Qt::UserRole + 1)); // "Data01"

    auto anyChildFilters = nestedAnyOfFilter->filtersListProperty();
    nestedAnyOfFilter->filtersListProperty().append(&anyChildFilters, anyChildFilter1);
    nestedAnyOfFilter->filtersListProperty().append(&anyChildFilters, anyChildFilter2);

    // AllOf contains: AnyOf (rows 0|1) AND ValueFilter (row 0 only) → row 0
    auto allChildFilters = nestedAllOfFilter->filtersListProperty();
    nestedAllOfFilter->filtersListProperty().append(&allChildFilters, nestedAnyOfFilter);
    nestedAllOfFilter->filtersListProperty().append(&allChildFilters, allChildFilter);

    auto sfpmFilters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    sfpmModel->filters().append(&sfpmFilters, nestedAllOfFilter);

    // AllOf { AnyOf { row0, row1 }, ValueFilter { row0 } } → only row 0
    QCOMPARE(sfpmModel->rowCount(), 1);
    QCOMPARE(sfpmModel->data(sfpmModel->index(0, 0, QModelIndex()), Qt::UserRole),
             tableModel.data(tableModel.index(0, 0), Qt::UserRole));
}

void tst_QQmlSortFilterProxyModel::invalidateParentWhenChildFilterEnabledOrDisabled()
{
    // Filter configuration: AllOf { AnyOf { VF1{}, VF2{} } }
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCompositeFilters.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    CustomTableModel tableModel;
    sfpmModel->setSourceModel(&tableModel);

    auto *valueFilter1 = object->property("nestedAnyOfChild1").value<QQmlValueFilter *>();
    QVERIFY(valueFilter1);
    auto *valueFilter2 = object->property("nestedAnyOfChild2").value<QQmlValueFilter *>();
    QVERIFY(valueFilter2);
    auto *innerAnyOfFilter = object->property("nestedAnyOf").value<QQmlAnyOfFilter *>();
    QVERIFY(innerAnyOfFilter);
    auto *outerAllOfFiltler = object->property("nestedAllOf").value<QQmlAllOfFilter *>();
    QVERIFY(outerAllOfFiltler);

    valueFilter1->setRoleName("column0");
    valueFilter1->setValue(tableModel.data(tableModel.index(0, 0), Qt::UserRole));   // "Data00" → row 0
    valueFilter2->setRoleName("column0");
    valueFilter2->setValue(tableModel.data(tableModel.index(1, 0), Qt::UserRole));   // "Data10" → row 1

    auto anyFilters = innerAnyOfFilter->filtersListProperty();
    innerAnyOfFilter->filtersListProperty().append(&anyFilters, valueFilter1);
    innerAnyOfFilter->filtersListProperty().append(&anyFilters, valueFilter2);

    auto allFilters = outerAllOfFiltler->filtersListProperty();
    outerAllOfFiltler->filtersListProperty().append(&allFilters, innerAnyOfFilter);

    auto sfpmFilters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    sfpmModel->filters().append(&sfpmFilters, outerAllOfFiltler);

    QCOMPARE(sfpmModel->rowCount(), 2);

    // Disable one of the value filter and check for the filtered data
    // "Data00" should be filtered out
    valueFilter1->setEnabled(false);
    QCOMPARE(sfpmModel->rowCount(), 1);
    QCOMPARE(sfpmModel->data(sfpmModel->index(0, 0, QModelIndex()), Qt::UserRole),
             tableModel.data(tableModel.index(1, 0), Qt::UserRole)); // row 1 ("Data10")

    // Reenable the same value filter and check for the filtered data
    valueFilter1->setEnabled(true);
    QCOMPARE(sfpmModel->rowCount(), 2);
    QCOMPARE(sfpmModel->data(sfpmModel->index(0, 0, QModelIndex()), Qt::UserRole),
             tableModel.data(tableModel.index(0, 0), Qt::UserRole)); // row 1 ("Dat00")
    QCOMPARE(sfpmModel->data(sfpmModel->index(1, 0, QModelIndex()), Qt::UserRole),
             tableModel.data(tableModel.index(1, 0), Qt::UserRole)); // row 2 ("Data10")
}


void tst_QQmlSortFilterProxyModel::invalidateChildWhenParentFilterEnabledOrDisabled()
{
    // Filter configuration: AllOf { AnyOf { VF1{}, VF2{} } }
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCompositeFilters.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    auto *sfpmModel = object->property("sfpmProxyModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(sfpmModel);
    CustomTableModel tableModel;
    sfpmModel->setSourceModel(&tableModel);

    auto *valueFilter1 = object->property("nestedAnyOfChild1").value<QQmlValueFilter *>();
    QVERIFY(valueFilter1);
    auto *valueFilter2 = object->property("nestedAnyOfChild2").value<QQmlValueFilter *>();
    QVERIFY(valueFilter2);
    auto *innerAnyOfFilter = object->property("nestedAnyOf").value<QQmlAnyOfFilter *>();
    QVERIFY(innerAnyOfFilter);
    auto *outerAllOfFiltler = object->property("nestedAllOf").value<QQmlAllOfFilter *>();
    QVERIFY(outerAllOfFiltler);

    valueFilter1->setRoleName("column0");
    valueFilter1->setValue(tableModel.data(tableModel.index(0, 0), Qt::UserRole));   // "Data00" → row 0
    valueFilter2->setRoleName("column0");
    valueFilter2->setValue(tableModel.data(tableModel.index(1, 0), Qt::UserRole));   // "Data10" → row 1

    auto anyFilters = innerAnyOfFilter->filtersListProperty();
    innerAnyOfFilter->filtersListProperty().append(&anyFilters, valueFilter1);
    innerAnyOfFilter->filtersListProperty().append(&anyFilters, valueFilter2);

    auto allFilters = outerAllOfFiltler->filtersListProperty();
    outerAllOfFiltler->filtersListProperty().append(&allFilters, innerAnyOfFilter);

    auto sfpmFilters = sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    sfpmModel->filters().append(&sfpmFilters, outerAllOfFiltler);
    QCOMPARE(sfpmModel->rowCount(), 2);

    // Disabling the outer filter make the complete subtree to be disabled
    outerAllOfFiltler->setEnabled(false);
    QCOMPARE(sfpmModel->rowCount(), tableModel.rowCount());

    outerAllOfFiltler->setEnabled(true);
    QCOMPARE(sfpmModel->rowCount(), 2);
}

void tst_QQmlSortFilterProxyModel::compositeFiltersInQml()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sfpmCompositeFilters.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    auto *colorListModel = object->property("colorList").value<QQmlListModel *>();
    QVERIFY(colorListModel);
    const int nameRole = colorListModel->roleNames().key("name", -1);
    QVERIFY(nameRole >= 0);

    auto getNames = [&](QQmlSortFilterProxyModel *model) -> QStringList {
        QStringList names;
        for (int i = 0; i < model->rowCount(); ++i)
            names << model->data(model->index(i, 0, QModelIndex()), nameRole).toString();
        return names;
    };

    // AnyOfFilter: color = "red" OR "blue"
    auto *redOrBlueModel = object->property("redOrBlueModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(redOrBlueModel);
    QCOMPARE(getNames(redOrBlueModel), QStringList({"red1", "red2", "blue1", "blue2"}));

    // AllOfFilter: color = "red" AND active = true (1 row)
    auto *activeRedModel = object->property("activeRedModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(activeRedModel);
    QCOMPARE(getNames(activeRedModel), QStringList({"red1"}));

    // AllOf { AnyOf { red, blue }, active = true } (3 rows)
    auto *activeRedOrBlueModel = object->property("activeRedOrBlueModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(activeRedOrBlueModel);
    QCOMPARE(getNames(activeRedOrBlueModel), QStringList({"red1", "blue1"}));

    // AllOf { AnyOf (disabled) { red, blue }, active = true } (3 rows)
    auto *activeDisabledRedOrBlueModel = object->property("activeDisabledRedOrBlueModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(activeDisabledRedOrBlueModel);
    QCOMPARE(getNames(activeDisabledRedOrBlueModel), QStringList({"red1", "blue1", "green1"}));

    // AllOf { AllOf (inverted) { red, active = true }, active = false } (3 rows)
    auto *invertedAllOfRedAndBlueModel = object->property("invertedAllOfRedAndBlueModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(invertedAllOfRedAndBlueModel);
    QCOMPARE(getNames(invertedAllOfRedAndBlueModel), QStringList({"red2", "blue2", "green2"}));

    // AllOf (inverted) { AnyOf (inverted) { red, blue }, active = false } (5 rows)
    auto *invertedAnyOfAllOfRedOrBlueModel = object->property("invertedAnyOfAllOfRedOrBlueModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(invertedAnyOfAllOfRedOrBlueModel);
    QCOMPARE(getNames(invertedAnyOfAllOfRedOrBlueModel), QStringList({"red1", "red2", "blue1", "blue2", "green1"}));

    // AnyOf { AnyOf { }, active = true } (3 rows)
    auto *nestedEmptyAnyOf = object->property("nestedEmptyAnyOf").value<QQmlSortFilterProxyModel *>();
    QVERIFY(nestedEmptyAnyOf);
    QCOMPARE(getNames(nestedEmptyAnyOf), QStringList({"red1", "blue1", "green1"}));
}

QTEST_MAIN(tst_QQmlSortFilterProxyModel)

#include "tst_qqmlsortfilterproxymodel.moc"
