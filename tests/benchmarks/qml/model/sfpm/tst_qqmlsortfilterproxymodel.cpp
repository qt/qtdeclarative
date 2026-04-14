// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QAbstractListModel>
#include <QQmlComponent>
#include <QSignalSpy>
#include <QRandomGenerator>

#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>
#include <QtQmlModels/private/qqmlvaluefilter_p.h>
#include <QtQmlModels/private/qqmlfunctionfilter_p.h>
#include <QtQmlModels/private/qqmlstringsorter_p.h>
#include <QtQmlModels/private/qqmlfunctionsorter_p.h>

#include <qtest.h>
#include <QtTest/QTest>

#define DEFAULT_MODEL_DATACOUNT (quint64)1000

inline QUrl TEST_FILE(const QString &filename)
{
    return QUrl::fromLocalFile(QLatin1String(SRCDIR) + QLatin1String("/data/") + filename);
}

class tst_qqmlsortfilterproxymodel : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void initTestCase();
    void cleanup();
    void cleanupTestCase();

    void tst_valueFilter();
    void tst_functionFilter_data();
    void tst_functionFilter();
    void tst_roleSorter_data();
    void tst_roleSorter();
    void tst_stringSorter();
    void tst_functionSorter();

private:
    void setFilter(QQmlFilterBase *filter);
    void setSorter(QQmlSorterBase *sorter);

    QAbstractItemModel *m_aimModel;
    QQmlEngine m_engine;
    QObject *m_object;
    QQmlSortFilterProxyModel *m_sfpmModel;
};

typedef struct CustomDataInfo {
    QString name;
    quint8 age;
    QString department;
    quint64 pid;
    QString country;
} CustomData;

class CustomListModel : public QAbstractListModel
{
public:

    CustomListModel(QObject *parent = nullptr, int dataCount = 0) : QAbstractListModel(parent) {
        for (int index = 0; index < dataCount; index++) {
            CustomData data;
            auto [country, name] = [&]() -> std::tuple<QString, QString> {
                const QList<QString> country = {"USA", "Canada", "Germany", "India", "Japan"};
                const int idx = index % 5;
                return  {
                    QString(country[idx]),
                    [&]() -> QString {
                        QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
                        if (country[idx] == "Germany")
                            possibleCharacters += QString("ÄÖÜẞäöüß");
                        QRandomGenerator generator;
                        const int randomStringLength = generator.bounded(5, 10);
                        QString randomString;
                        for (int i = 0; i < randomStringLength; ++i) {
                            int index = QRandomGenerator::global()->generate() % possibleCharacters.length();
                            QChar nextChar = possibleCharacters.at(index);
                            randomString.append(nextChar);
                        }
                        return randomString;
                    }()
                };
            }();
            data.country = country;
            data.name = name;
            data.age = 20 + (index % 30);
            data.department = QString("Department_") + QString::number(index % 10);
            data.pid = 1000000 + index;
            m_values.append(data);
        }
    }

    enum CustomDataRole {
        NameRole = Qt::UserRole,
        AgeRole,
        DepartmentRole,
        PidRole,
        CountryRole
    };

    int rowCount(const QModelIndex &) const final { return m_values.count(); }

    QHash<int, QByteArray> roleNames() const final {
        QHash<int, QByteArray> roles;
        roles.insert(NameRole, "name");
        roles.insert(AgeRole, "age");
        roles.insert(DepartmentRole, "department");
        roles.insert(PidRole, "pid");
        roles.insert(CountryRole, "country");
        return roles;
    }

    QVariant data(const QModelIndex &index, int role) const final {
        if (index.row() < 0 || m_values.count() <= index.row())
            return QVariant();
        const CustomData &data = m_values[index.row()];
        switch (role) {
        case NameRole: return data.name;
        case AgeRole: return data.age;
        case DepartmentRole: return data.department;
        case PidRole: return data.pid;
        case CountryRole: return data.country;
        default: break;
        }
        return QVariant();
    }

private:
    QList<CustomData> m_values;
};

void tst_qqmlsortfilterproxymodel::initTestCase()
{
    // TODO: Need to construct different model data
    m_aimModel = new CustomListModel(this, [](){
        QByteArray rawValue = qgetenv("BENCHMARK_SFPM_DATA_COUNT");
        bool ok;
        int value = rawValue.toInt(&ok);
        return (ok ? value : DEFAULT_MODEL_DATACOUNT);
    }());
    QVERIFY(m_aimModel);

    QQmlComponent component(&m_engine, QUrl(TEST_FILE("sfpm.qml")));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    m_object = component.create();
    QVERIFY(m_object);

    m_sfpmModel = m_object->property("sfpmModel").value<QQmlSortFilterProxyModel *>();
    QVERIFY(m_sfpmModel);
    m_sfpmModel->setDynamicSortFilter(false);
}

void tst_qqmlsortfilterproxymodel::cleanupTestCase()
{
    delete m_object;
    delete m_aimModel;
}

void tst_qqmlsortfilterproxymodel::init()
{
    QSignalSpy modelResetSpy(m_sfpmModel, SIGNAL(modelReset()));
    m_sfpmModel->setSourceModel(m_aimModel);
    QCOMPARE(modelResetSpy.count(), 1);
}

void tst_qqmlsortfilterproxymodel::cleanup()
{
    auto filters = m_sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    m_sfpmModel->filters().clear(&filters);
    auto sorters = m_sfpmModel->property("sorters").value<QQmlListProperty<QQmlSorterBase>>();
    m_sfpmModel->sorters().clear(&sorters);

    QSignalSpy modelResetSpy(m_sfpmModel, SIGNAL(modelReset()));
    CustomListModel model(this, 0);
    m_sfpmModel->setSourceModel(&model);
    QCOMPARE(modelResetSpy.count(), 1);
}

void tst_qqmlsortfilterproxymodel::setFilter(QQmlFilterBase *filter)
{
    auto filters = m_sfpmModel->property("filters").value<QQmlListProperty<QQmlFilterBase>>();
    if (m_sfpmModel->filters().count(&filters) <= 0) {
        m_sfpmModel->filters().append(&filters, filter);
        QCOMPARE(m_sfpmModel->filters().count(&filters), 1);
    }
}

void tst_qqmlsortfilterproxymodel::setSorter(QQmlSorterBase *sorter)
{
    auto sorters = m_sfpmModel->property("sorters").value<QQmlListProperty<QQmlSorterBase>>();
    if (m_sfpmModel->sorters().count(&sorters) <= 0) {
        m_sfpmModel->sorters().append(&sorters, sorter);
        QCOMPARE(m_sfpmModel->sorters().count(&sorters), 1);
    }
}

void tst_qqmlsortfilterproxymodel::tst_valueFilter()
{
    auto *valueFilter = m_object->property("valueFilter").value<QQmlValueFilter *>();
    QVERIFY(valueFilter);
    valueFilter->setRoleName(QString::fromLatin1("age"));
    valueFilter->setValue(50);
    QBENCHMARK_ONCE {
        setFilter(valueFilter);
    }
}

void tst_qqmlsortfilterproxymodel::tst_functionFilter_data()
{
    QTest::addColumn<QString>("functionFilterConfig");

    QTest::newRow("integer") << R"(
            import QtQuick;
            import QtQml.Models;
            FunctionFilter {
                function filter(age: int): bool {
                    return (age === 50)
                }
            }
        )";

    QTest::newRow("regular expression") << R"(
            import QtQuick;
            import QtQml.Models;
            FunctionFilter {
                property int count: 0
                function filter(department: string): bool {
                    return (/^Department_[3-7]$/.exec(department) !== null)
                }
            }
        )";
}

void tst_qqmlsortfilterproxymodel::tst_functionFilter()
{
    QFETCH(QString, functionFilterConfig);

    QQmlComponent component(&m_engine);
    component.setData(functionFilterConfig.toUtf8(), QUrl());
    QVERIFY(component.status() == QQmlComponent::Ready);
    auto *functionFilter = static_cast<QQmlFunctionFilter *>(component.create(qmlContext(m_sfpmModel)));
    QBENCHMARK_ONCE {
        setFilter(functionFilter);
    }
}

void tst_qqmlsortfilterproxymodel::tst_roleSorter_data()
{
    QTest::addColumn<QString>("sortRole");

    QTest::newRow("integer") << QString("age");
    QTest::newRow("string") << QString("country");
}

void tst_qqmlsortfilterproxymodel::tst_roleSorter()
{
    QFETCH(QString, sortRole);
    auto *roleSorter = m_object->property("roleSorter").value<QQmlRoleSorter *>();
    QVERIFY(roleSorter);
    roleSorter->setRoleName(sortRole);
    QBENCHMARK_ONCE {
        setSorter(roleSorter);
    }
}

void tst_qqmlsortfilterproxymodel::tst_stringSorter()
{
    auto *stringSorter = m_object->property("stringSorter").value<QQmlStringSorter *>();
    QVERIFY(stringSorter);
    stringSorter->setLocale(QLocale("de_DE"));
    stringSorter->setRoleName("name");
    QBENCHMARK_ONCE {
        setSorter(stringSorter);
    }
}

void tst_qqmlsortfilterproxymodel::tst_functionSorter()
{
    QString qmlfunctionSorterStr = R"(
            import QtQuick;
            import QtQml.Models;
            FunctionSorter {
                component SorterRoleData: QtObject { property int age }
                function sort(lhsData: SorterRoleData, rhsData: SorterRoleData): int {
                    return (lhsData.age < rhsData.age ? 1 : -1);
                }
            }
        )";
    QQmlComponent component(&m_engine);
    component.setData(qmlfunctionSorterStr.toUtf8(), QUrl());
    QVERIFY(component.status() == QQmlComponent::Ready);
    auto *functionSorter = static_cast<QQmlFunctionSorter *>(component.create(qmlContext(m_sfpmModel)));
    QBENCHMARK_ONCE {
        setSorter(functionSorter);
    }
}

QTEST_MAIN(tst_qqmlsortfilterproxymodel);

#include "tst_qqmlsortfilterproxymodel.moc"
