// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qqmlpropertycachecreator.h"

#include <QtQml/private/qqmlpropertycachecreator_p.h>
#include <QtQml/private/qqmltypecompiler_p.h>

// #include <QtQuickTestUtils/private/qmlutils_p.h>

// TODO consider moving to test utils
class FakeObjectContainer
{
public:
    FakeObjectContainer() = default;
    FakeObjectContainer(std::unique_ptr<QmlIR::Document> docPtr,
                        QV4::CompiledData::ResolvedTypeReferenceMap resolvedTypes)
        : m_document(std::move(docPtr)), m_resolvedTypes(std::move(resolvedTypes))
    {
    }

    // --- interface used by QQmlPropertyCacheCreator --------
    using CompiledObject = QmlIR::Object;

    int objectCount() const { return m_document->objects.size(); }
    QmlIR::Object *objectAt(int index) const { return m_document->objects.at(index); }

    QmlIR::PoolList<QmlIR::Function>::Iterator
    objectFunctionsBegin(const QmlIR::Object *object) const
    {
        return object->functionsBegin();
    }
    QmlIR::PoolList<QmlIR::Function>::Iterator objectFunctionsEnd(const QmlIR::Object *object) const
    {
        return object->functionsEnd();
    }

    // used to get IC type name
    QString stringAt(int) const { return {}; }

    using ListPropertyAssignBehavior = QmlIR::Pragma::ListPropertyAssignBehaviorValue;
    ListPropertyAssignBehavior listPropertyAssignBehavior() const { return {}; }

    QUrl url() const { return {}; }

    QQmlType qmlTypeForComponent(const QString & = QString()) const { return {}; };

    QV4::ResolvedTypeReference *resolvedType(int id) const { return m_resolvedTypes.value(id); }
    // -----------------------------------------------------
private:
    std::unique_ptr<QmlIR::Document> m_document =
            std::make_unique<QmlIR::Document>(QString(), QString(), QV4::Compiler::CompilationMode::Debug);
    QV4::CompiledData::ResolvedTypeReferenceMap m_resolvedTypes;
};

/* for the non common type proper mocking of typeLoader and imports is needed*/
static inline auto createQQmlPropertyDataCommonType(const QmlIR::Property &irProperty)
{
    QQmlPropertyCacheVector cacheVector;
    FakeObjectContainer objContainer;

    return QQmlPropertyCacheCreator(&cacheVector, nullptr, nullptr, &objContainer, nullptr, {})
            .tryCreateQQmlPropertyData(irProperty, -1, -1)
            .value_or(QQmlPropertyData());
}

namespace propertyDataComparator {
using Type = qxp::function_ref<bool(const QQmlPropertyData &, const QQmlPropertyData &)>;

const Type onlyFlags = [](const QQmlPropertyData &lhs, const QQmlPropertyData &rhs) -> bool {
    return lhs.flags() == rhs.flags();
};
} // namespace propertyDataComparator

void tst_qqmlpropertycachecreator::tryCreateQQmlPropertyData_commonType_data()
{
    QTest::addColumn<const QmlIR::Property>("irProperty");
    QTest::addColumn<const QQmlPropertyData>("expectedPropertyData");
    QTest::addColumn<propertyDataComparator::Type>("comparator");

    const auto defaultIrPropertyAndFlags = []() {
        QmlIR::Property irProperty = {};
        irProperty.setCommonType(QV4::CompiledData::CommonType::Var);

        QQmlPropertyData::Flags flags = {};
        flags.setType(QQmlPropertyData::Flags::VarPropertyType);
        flags.setIsWritable(true);
        return std::make_pair(std::move(irProperty), std::move(flags));
    };

    {
        auto [irProperty, flags] = defaultIrPropertyAndFlags();

        QQmlPropertyData expectedPropertyData{};
        expectedPropertyData.setFlags(flags);

        QTest::newRow("property var")
                << irProperty << expectedPropertyData << propertyDataComparator::onlyFlags;
    }
    {
        auto [irProperty, flags] = defaultIrPropertyAndFlags();
        irProperty.setIsReadOnly(true);
        flags.setIsWritable(false);

        QQmlPropertyData expectedPropertyData{};
        expectedPropertyData.setFlags(flags);

        QTest::newRow("readonly property var")
                << irProperty << expectedPropertyData << propertyDataComparator::onlyFlags;
    }
    {
        auto [irProperty, flags] = defaultIrPropertyAndFlags();
        irProperty.setIsFinal(true);
        flags.setIsFinal(true);

        QQmlPropertyData expectedPropertyData{};
        expectedPropertyData.setFlags(flags);

        QTest::newRow("final property var")
                << irProperty << expectedPropertyData << propertyDataComparator::onlyFlags;
    }
    {
        auto [irProperty, flags] = defaultIrPropertyAndFlags();
        irProperty.setIsVirtual(true);
        flags.setIsVirtual(true);

        QQmlPropertyData expectedPropertyData{};
        expectedPropertyData.setFlags(flags);

        QTest::newRow("virtual property var")
                << irProperty << expectedPropertyData << propertyDataComparator::onlyFlags;
    }
    {
        auto [irProperty, flags] = defaultIrPropertyAndFlags();
        irProperty.setIsOverride(true);
        flags.setDoesOverride(true);

        QQmlPropertyData expectedPropertyData{};
        expectedPropertyData.setFlags(flags);

        QTest::newRow("override property var")
                << irProperty << expectedPropertyData << propertyDataComparator::onlyFlags;
    }
}

void tst_qqmlpropertycachecreator::tryCreateQQmlPropertyData_commonType()
{
    QFETCH(const QmlIR::Property, irProperty);
    QFETCH(const QQmlPropertyData, expectedPropertyData);
    QFETCH(propertyDataComparator::Type, comparator);

    const auto actualPropertyData = createQQmlPropertyDataCommonType(irProperty);

    QVERIFY(comparator(actualPropertyData, expectedPropertyData));
}

QTEST_MAIN(tst_qqmlpropertycachecreator)
