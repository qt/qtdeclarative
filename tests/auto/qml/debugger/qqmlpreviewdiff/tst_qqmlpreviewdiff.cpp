// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qqmlpreviewpatch.h"

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtCore/qdir.h>

#include <private/qqmlcomponent_p.h>
#include <private/qqmlpreviewdiff_p.h>
#include <private/qv4executablecompilationunit_p.h>

#include <algorithm>
#include <cstring>

using namespace QV4::CompiledData;

static qsizetype countByType(const CompilationUnitDiff &diff, ChangeType type)
{
    return std::count_if(diff.changes.begin(), diff.changes.end(),
                         [type](const Change &c) { return c.type == type; });
}


class tst_QQmlPreviewDiff : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQmlPreviewDiff();

private:
    QQmlRefPointer<QV4::CompiledData::CompilationUnit> loadUnit(const QString &fileName);
    CompilationUnitDiff diffFiles(const QString &oldFile, const QString &newFile);
    void verifyPatchProducesByteIdenticalUnit(const QString &oldFile, const QString &newFile);

private slots:
    void identicalUnits();
    void constantValueChange();
    void multipleConstantChanges();
    void bindingChange();
    void sameSizeBindingChange();
    void bindingToConstant();
    void constantToBinding();
    void objectBindingAddRemove();
    void objectBindingChanged();
    void propertyAddition();
    void propertyRemoval();
    void objectAddition();
    void objectRemoval();
    void baseTypeChange();
    void nestedObjectChange();
    void innerBaseTypeChange();
    void mixedChanges();

    // Location vs content change tests
    void bindingLocationOnlyChange();
    void bindingContentOnlyChange();
    void bindingContentAndLocationChange();
    void propertyLocationOnlyChange();
    void propertyContentOnlyChange();
    void propertyContentAndLocationChange();
    void aliasLocationOnlyChange();
    void aliasContentOnlyChange();
    void aliasContentAndLocationChange();

    // Enum tests
    void enumChange();
    void enumAddition();
    void enumRemoval();
    void enumLocationOnlyChange();
    void enumContentAndLocationChange();

    // Enum value tests (individual values within an enum)
    void enumValueChange();
    void enumValueLocationChange();

    // Signal tests
    void signalChange();
    void signalAddition();
    void signalRemoval();
    void signalLocationOnlyChange();
    void signalContentAndLocationChange();

    // Patch verification tests
    void patch_data();
    void patch();

    // Coverage gap tests
    void unitFlagsChange();
    void translationBindingChange();
    void translationBindingCrossFileChange();
    void inlineComponentChange();
    void requiredPropertyExtraDataChange();

    // Regression tests for comparison gaps that have been fixed
    void signalParameterCommonTypeChange();
    void functionTypedParamFalsePositive();

    // Comparison gap tests (these should fail until the gaps are fixed)
    void regexpFlagChange();
    void classTableChange();
    void templateObjectTableChange();
    void jsClassTableChange();
    void blockTableChange();

    // Folded enum binding tests (Type_Number bindings with IsResolvedEnum)
    void foldedEnumOutOfRange();
    void foldedEnumChange();

    // Cross-product test for all QML file pairs
    void patchAllFilePairs_data();
    void patchAllFilePairs();

private:
    QQmlEngine engine;
};

tst_QQmlPreviewDiff::tst_QQmlPreviewDiff() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

QQmlRefPointer<QV4::CompiledData::CompilationUnit> tst_QQmlPreviewDiff::loadUnit(
        const QString &fileName)
{
    QQmlComponent component(&engine, testFileUrl(fileName));
    if (!component.isReady()) {
        qWarning() << "Component errors:" << component.errors();
        return nullptr;
    }

    const auto *priv = QQmlComponentPrivate::get(&component);
    return (priv && priv->compilationUnit())
            ? priv->compilationUnit()->baseCompilationUnit()
            : nullptr;
}

CompilationUnitDiff tst_QQmlPreviewDiff::diffFiles(const QString &oldFile, const QString &newFile)
{
    const auto oldUnit = loadUnit(oldFile);
    const auto newUnit = loadUnit(newFile);
    if (!oldUnit || !newUnit)
        return CompilationUnitDiff{};
    return diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
}

void tst_QQmlPreviewDiff::verifyPatchProducesByteIdenticalUnit(const QString &oldFile,
                                                               const QString &newFile)
{
    using namespace QV4::CompiledData;

    const auto oldUnit = loadUnit(oldFile);
    const auto newUnit = loadUnit(newFile);
    QVERIFY(oldUnit);
    QVERIFY(newUnit);

    // Get the diff
    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Apply patch (using only old unit and diff)
    QString errorMessage;
    const QByteArray patchedData = patchCompilationUnit(oldUnit->unitData(), diff, &errorMessage);
    QVERIFY2(!patchedData.isEmpty(), qPrintable(errorMessage));

    // Compare with new unit
    const auto *patchedUnit = reinterpret_cast<const Unit *>(patchedData.constData());
    const auto *expectedUnit = newUnit->unitData();

    QCOMPARE(patchedUnit->unitSize, expectedUnit->unitSize);
    if (std::memcmp(patchedUnit, expectedUnit, expectedUnit->unitSize) != 0) {
        const char *p = patchedData.constData();
        const char *e = reinterpret_cast<const char *>(expectedUnit);
        int shown = 0;
        for (quint32 i = 0; i < expectedUnit->unitSize && shown < 30; i++) {
            if (p[i] != e[i]) {
                qDebug() << "Byte diff at offset" << i
                         << ": patched=" << (unsigned char)p[i]
                         << "expected=" << (unsigned char)e[i];
                shown++;
            }
        }
        qDebug() << "Old strings:" << oldUnit->unitData()->stringTableSize;
        for (quint32 i = 0; i < oldUnit->unitData()->stringTableSize; i++)
            qDebug() << "  " << i << ":" << oldUnit->unitData()->stringAtInternal(i);
        qDebug() << "New strings:" << expectedUnit->stringTableSize;
        for (quint32 i = 0; i < expectedUnit->stringTableSize; i++)
            qDebug() << "  " << i << ":" << expectedUnit->stringAtInternal(i);
        qDebug() << "Patched strings:" << patchedUnit->stringTableSize;
        for (quint32 i = 0; i < patchedUnit->stringTableSize; i++)
            qDebug() << "  " << i << ":" << patchedUnit->stringAtInternal(i);
        for (const auto &c : diff.changes)
            qDebug() << "Change type:" << int(c.type) << "obj:" << c.objectIndex << "idx:" << c.index;
    }
    QVERIFY2(std::memcmp(patchedUnit, expectedUnit, expectedUnit->unitSize) == 0,
             "Patched unit does not match new unit byte-for-byte");
}

void tst_QQmlPreviewDiff::identicalUnits()
{
    const auto diff = diffFiles("Identical1.qml", "Identical2.qml");
    QVERIFY(diff.success);

    // Files with the same content but different paths will have a metadata change
    // Check that there are no semantic changes (only metadata)
    bool hasOnlyMetadata =
            std::all_of(diff.changes.begin(), diff.changes.end(), [](const Change &c) {
                return c.type == ChangeType::UnitMetadataChanged
                        || c.type == ChangeType::StringDataChanged;
            });
    QVERIFY2(hasOnlyMetadata, "Expected only metadata changes between identical QML files");
}

void tst_QQmlPreviewDiff::constantValueChange()
{
    const auto diff = diffFiles("ConstantOld.qml", "ConstantNew.qml");
    QVERIFY(diff.success);

    QCOMPARE(countByType(diff, ChangeType::BindingChanged), 1);

    // Find the BindingChanged change
    const Change *change = nullptr;
    for (const auto &c : diff.changes) {
        if (c.type == ChangeType::BindingChanged) {
            change = &c;
            break;
        }
    }
    QVERIFY(change != nullptr);

    // Load the new unit to verify the index points to the correct binding
    const auto newUnit = loadUnit("ConstantNew.qml");
    const auto *newObj = newUnit->unitData()->qmlUnit()->objectAt(change->objectIndex);
    QVERIFY(change->index < newObj->nBindings);

    const auto *changedBinding = newObj->bindingTable() + change->index;
    QString propName = newUnit->unitData()->stringAtInternal(changedBinding->propertyNameIndex);
    QCOMPARE(propName, QString("width")); // The width property changed from 100 to 150
}

void tst_QQmlPreviewDiff::multipleConstantChanges()
{
    const auto diff = diffFiles("MultiConstantOld.qml", "MultiConstantNew.qml");
    QVERIFY(diff.success);

    QCOMPARE(countByType(diff, ChangeType::BindingChanged), 3);

    // Verify that all BindingChanged entries point to valid bindings in the new compilation unit
    const auto newUnit = loadUnit("MultiConstantNew.qml");
    const auto *newObj = newUnit->unitData()->qmlUnit()->objectAt(0);

    for (const auto &change : diff.changes) {
        if (change.type != ChangeType::BindingChanged)
            continue;

        QVERIFY(change.index < newObj->nBindings);

        // The index should point to a valid binding
        const auto *changedBinding = newObj->bindingTable() + change.index;
        QString propName = newUnit->unitData()->stringAtInternal(changedBinding->propertyNameIndex);

        // Should be one of the changed properties
        QVERIFY(propName == "width" || propName == "height" || propName == "color");
    }
}

void tst_QQmlPreviewDiff::bindingChange()
{
    const auto diff = diffFiles("BindingOld.qml", "BindingNew.qml");
    QVERIFY(diff.success);

    QCOMPARE(countByType(diff, ChangeType::BindingChanged), 0);
    QCOMPARE(countByType(diff, ChangeType::FunctionChanged), 1);
}

void tst_QQmlPreviewDiff::sameSizeBindingChange()
{
    // Test that byte-by-byte comparison works even when bytecode size is identical
    const auto oldUnit = loadUnit("SameSizeBindingOld.qml");
    const auto newUnit = loadUnit("SameSizeBindingNew.qml");
    QVERIFY(oldUnit && newUnit);

    // Verify that the compiled functions have the same size
    const auto *oldObj = oldUnit->unitData()->qmlUnit()->objectAt(0);
    const auto *newObj = newUnit->unitData()->qmlUnit()->objectAt(0);

    // Find the width binding in both
    const QV4::CompiledData::Binding *oldWidthBinding = nullptr;
    const QV4::CompiledData::Binding *newWidthBinding = nullptr;

    for (quint32 i = 0; i < oldObj->nBindings; ++i) {
        const auto *b = oldObj->bindingTable() + i;
        QString propName = oldUnit->unitData()->stringAtInternal(b->propertyNameIndex);
        if (propName == "width" && b->type() == QV4::CompiledData::Binding::Type_Script) {
            oldWidthBinding = b;
            break;
        }
    }

    for (quint32 i = 0; i < newObj->nBindings; ++i) {
        const auto *b = newObj->bindingTable() + i;
        QString propName = newUnit->unitData()->stringAtInternal(b->propertyNameIndex);
        if (propName == "width" && b->type() == QV4::CompiledData::Binding::Type_Script) {
            newWidthBinding = b;
            break;
        }
    }

    QVERIFY(oldWidthBinding);
    QVERIFY(newWidthBinding);

    const auto *oldFunc =
            oldUnit->unitData()->functionAt(oldWidthBinding->value.compiledScriptIndex);
    const auto *newFunc =
            newUnit->unitData()->functionAt(newWidthBinding->value.compiledScriptIndex);

    // Verify same code size (this is what we're testing - that size alone isn't enough)
    QCOMPARE(oldFunc->codeSize, newFunc->codeSize);

    // Now verify the diff detects the change despite identical size.
    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);
    QCOMPARE(countByType(diff, ChangeType::BindingChanged), 0);
    QCOMPARE(countByType(diff, ChangeType::FunctionChanged), 1);
}

void tst_QQmlPreviewDiff::bindingToConstant()
{
    const auto diff = diffFiles("BindingToConstantOld.qml", "BindingToConstantNew.qml");
    QVERIFY(diff.success);
    // At least 1 binding change for the width binding type change
    // May have more if other bindings have index changes due to string table shifts
    QVERIFY(countByType(diff, ChangeType::BindingChanged) >= 1);
}

void tst_QQmlPreviewDiff::constantToBinding()
{
    const auto diff = diffFiles("ConstantToBindingOld.qml", "ConstantToBindingNew.qml");
    QVERIFY(diff.success);
    // At least 1 binding change for the width binding type change
    QVERIFY(countByType(diff, ChangeType::BindingChanged) >= 1);
}

void tst_QQmlPreviewDiff::objectBindingAddRemove()
{
    // Test that changes in binding property names are detected.
    // Old file: rect1 has anchors.fill: parent
    // New file: rect1 has anchors.centerIn: parent
    // The anchors group creates object 2, and changing from fill to centerIn means
    // the bindings on that object are different (fill binding removed, centerIn added).
    const auto oldUnit = loadUnit("ObjectBindingAddRemoveOld.qml");
    const auto newUnit = loadUnit("ObjectBindingAddRemoveNew.qml");
    QVERIFY(oldUnit && newUnit);

    // Both should have 4 objects: root Rectangle, child Rectangle, anchors group object, child Text
    QCOMPARE(oldUnit->unitData()->qmlUnit()->nObjects, 4u);
    QCOMPARE(newUnit->unitData()->qmlUnit()->nObjects, 4u);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // The change from anchors.fill to anchors.centerIn involves string and potentially location changes
    // With location-separated comparison, we may get BindingChanged or BindingLocationChanged
    // Just verify that some binding-related change was detected
    QVERIFY(countByType(diff, ChangeType::BindingChanged)
                    + countByType(diff, ChangeType::BindingLocationChanged)
                    + countByType(diff, ChangeType::StringDataChanged)
            > 0);
}

void tst_QQmlPreviewDiff::objectBindingChanged()
{
    const auto oldUnit = loadUnit("ObjectBindingChangeOld.qml");
    const auto newUnit = loadUnit("ObjectBindingChangeNew.qml");
    QVERIFY(oldUnit && newUnit);

    QCOMPARE(oldUnit->unitData()->qmlUnit()->nObjects, 4u);
    QCOMPARE(newUnit->unitData()->qmlUnit()->nObjects, 4u);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());

    QVERIFY(diff.success);

    // With position-based comparison:
    // - Objects at same positions are compared directly
    // - Object structure changed (Rectangle with Text child swapped with empty Rectangle)
    // Just verify patching works (already covered by patchAllFilePairs)
    QVERIFY(diff.changes.size() > 0);
}

void tst_QQmlPreviewDiff::propertyAddition()
{
    // Verify structural difference directly from units
    const auto oldUnit = loadUnit("PropertyAddOld.qml");
    const auto newUnit = loadUnit("PropertyAddNew.qml");
    QVERIFY(oldUnit && newUnit);
    QCOMPARE(oldUnit->unitData()->qmlUnit()->objectAt(0)->nProperties, 0u);
    QCOMPARE(newUnit->unitData()->qmlUnit()->objectAt(0)->nProperties, 1u);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());

    QCOMPARE(countByType(diff, ChangeType::PropertyAdded), 1);

    // Find the PropertyAdded change
    const Change *change = nullptr;
    for (const auto &c : diff.changes) {
        if (c.type == ChangeType::PropertyAdded) {
            change = &c;
            break;
        }
    }
    QVERIFY(change != nullptr);

    const auto *newObj = newUnit->unitData()->qmlUnit()->objectAt(change->objectIndex);
    QVERIFY(change->index < newObj->nProperties);

    const auto *addedProp = newObj->propertyTable() + change->index;
    QString propName = newUnit->unitData()->stringAtInternal(addedProp->nameIndex());
    QCOMPARE(propName, QString("myProp"));
}

void tst_QQmlPreviewDiff::propertyRemoval()
{
    const auto oldUnit = loadUnit("PropertyRemoveOld.qml");
    const auto newUnit = loadUnit("PropertyRemoveNew.qml");
    QVERIFY(oldUnit && newUnit);
    QCOMPARE(oldUnit->unitData()->qmlUnit()->objectAt(0)->nProperties, 1u);
    QCOMPARE(newUnit->unitData()->qmlUnit()->objectAt(0)->nProperties, 0u);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());

    QCOMPARE(countByType(diff, ChangeType::PropertyRemoved), 1);

    // Find the PropertyRemoved change
    const Change *change = nullptr;
    for (const auto &c : diff.changes) {
        if (c.type == ChangeType::PropertyRemoved) {
            change = &c;
            break;
        }
    }
    QVERIFY(change != nullptr);

    const auto *oldObj = oldUnit->unitData()->qmlUnit()->objectAt(change->objectIndex);
    QVERIFY(change->index < oldObj->nProperties);

    const auto *removedProp = oldObj->propertyTable() + change->index;
    QString propName = oldUnit->unitData()->stringAtInternal(removedProp->nameIndex());
    QCOMPARE(propName, QString("myProp"));
}

void tst_QQmlPreviewDiff::objectAddition()
{
    const auto oldUnit = loadUnit("ObjectAddOld.qml");
    const auto newUnit = loadUnit("ObjectAddNew.qml");
    QVERIFY(oldUnit && newUnit);
    QCOMPARE(oldUnit->unitData()->qmlUnit()->nObjects, 1u);
    QCOMPARE(newUnit->unitData()->qmlUnit()->nObjects, 2u);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());

    QCOMPARE(countByType(diff, ChangeType::ObjectAdded), 1);
}

void tst_QQmlPreviewDiff::objectRemoval()
{
    const auto diff = diffFiles("ObjectRemoveOld.qml", "ObjectRemoveNew.qml");

    QVERIFY(diff.success);
    QCOMPARE(countByType(diff, ChangeType::ObjectRemoved), 1);
}

void tst_QQmlPreviewDiff::baseTypeChange()
{
    const auto oldUnit = loadUnit("BaseTypeOld.qml");
    const auto newUnit = loadUnit("BaseTypeNew.qml");
    QVERIFY(oldUnit && newUnit);

    // Verify type names differ semantically
    const auto *oldObj = oldUnit->unitData()->qmlUnit()->objectAt(0);
    const auto *newObj = newUnit->unitData()->qmlUnit()->objectAt(0);
    const QString oldType = oldUnit->unitData()->stringAtInternal(oldObj->inheritedTypeNameIndex);
    const QString newType = newUnit->unitData()->stringAtInternal(newObj->inheritedTypeNameIndex);
    QVERIFY(oldType != newType);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());

    // With position-based comparison, the type name change is detected via StringChanged
    // (the string at position N changed from "Rectangle" to "Item")
    // ObjectChanged is only emitted when indices differ, not when string content differs
    QVERIFY(countByType(diff, ChangeType::StringDataChanged) >= 1);
}

void tst_QQmlPreviewDiff::nestedObjectChange()
{
    const auto diff = diffFiles("NestedOld.qml", "NestedNew.qml");
    QVERIFY(diff.success);

    // The change from text: "Hello" to text: "World" is detected via StringChanged
    // (string at the stringIndex position changed from "Hello" to "World")
    // With position-based comparison, binding structure doesn't change if indices are same
    QVERIFY(countByType(diff, ChangeType::StringDataChanged) >= 1);
}

void tst_QQmlPreviewDiff::innerBaseTypeChange()
{
    const auto diff = diffFiles("InnerBaseTypeOld.qml", "InnerBaseTypeNew.qml");
    QVERIFY(diff.success);

    // The change in base type for the inner object triggers a change in the object
    // binding for the outer object.
    QCOMPARE(countByType(diff, ChangeType::ObjectChanged), 1);
    QCOMPARE(countByType(diff, ChangeType::BindingChanged), 1);
}

void tst_QQmlPreviewDiff::mixedChanges()
{
    const auto oldUnit = loadUnit("MixedOld.qml");
    const auto newUnit = loadUnit("MixedNew.qml");
    QVERIFY(oldUnit && newUnit);

    // Verify structural differences
    QCOMPARE(oldUnit->unitData()->qmlUnit()->nObjects, 2u);
    QCOMPARE(newUnit->unitData()->qmlUnit()->nObjects, 3u);
    QCOMPARE(oldUnit->unitData()->qmlUnit()->objectAt(0)->nProperties, 0u);
    QCOMPARE(newUnit->unitData()->qmlUnit()->objectAt(0)->nProperties, 1u);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());

    // Structural changes should come first in the list
    QCOMPARE(countByType(diff, ChangeType::PropertyAdded), 1);
    QCOMPARE(countByType(diff, ChangeType::ObjectAdded), 1);
    QVERIFY(countByType(diff, ChangeType::BindingChanged) >= 2);
}

void tst_QQmlPreviewDiff::bindingLocationOnlyChange()
{
    const auto oldUnit = loadUnit("BindingLocationOnlyOld.qml");
    const auto newUnit = loadUnit("BindingLocationOnlyNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have location change(s) but no content changes for bindings
    QCOMPARE(countByType(diff, ChangeType::BindingChanged), 0);
    QVERIFY(countByType(diff, ChangeType::BindingLocationChanged) > 0);

    verifyPatchProducesByteIdenticalUnit("BindingLocationOnlyOld.qml",
                                         "BindingLocationOnlyNew.qml");
}

void tst_QQmlPreviewDiff::bindingContentOnlyChange()
{
    const auto oldUnit = loadUnit("BindingContentOnlyOld.qml");
    const auto newUnit = loadUnit("BindingContentOnlyNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have content change(s) but no location-only changes for bindings
    QVERIFY(countByType(diff, ChangeType::BindingChanged) > 0
            || countByType(diff, ChangeType::ConstantChanged) > 0);
    QCOMPARE(countByType(diff, ChangeType::BindingLocationChanged), 0);

    verifyPatchProducesByteIdenticalUnit("BindingContentOnlyOld.qml", "BindingContentOnlyNew.qml");
}

void tst_QQmlPreviewDiff::bindingContentAndLocationChange()
{
    const auto oldUnit = loadUnit("BindingContentAndLocationOld.qml");
    const auto newUnit = loadUnit("BindingContentAndLocationNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have content changes; the Changed entry already carries the new location
    bool hasContentChange = countByType(diff, ChangeType::BindingChanged) > 0
            || countByType(diff, ChangeType::ConstantChanged) > 0;
    QVERIFY(hasContentChange);

    verifyPatchProducesByteIdenticalUnit("BindingContentAndLocationOld.qml",
                                         "BindingContentAndLocationNew.qml");
}

void tst_QQmlPreviewDiff::propertyLocationOnlyChange()
{
    const auto oldUnit = loadUnit("PropertyLocationOnlyOld.qml");
    const auto newUnit = loadUnit("PropertyLocationOnlyNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have location change but no content change for property
    QCOMPARE(countByType(diff, ChangeType::PropertyChanged), 0);
    QVERIFY(countByType(diff, ChangeType::PropertyLocationChanged) > 0);

    verifyPatchProducesByteIdenticalUnit("PropertyLocationOnlyOld.qml",
                                         "PropertyLocationOnlyNew.qml");
}

void tst_QQmlPreviewDiff::propertyContentOnlyChange()
{
    const auto oldUnit = loadUnit("PropertyContentOnlyOld.qml");
    const auto newUnit = loadUnit("PropertyContentOnlyNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have content change but no location-only change for property
    QVERIFY(countByType(diff, ChangeType::PropertyChanged) > 0);
    QCOMPARE(countByType(diff, ChangeType::PropertyLocationChanged), 0);

    verifyPatchProducesByteIdenticalUnit("PropertyContentOnlyOld.qml",
                                         "PropertyContentOnlyNew.qml");
}

void tst_QQmlPreviewDiff::propertyContentAndLocationChange()
{
    const auto oldUnit = loadUnit("PropertyContentAndLocationOld.qml");
    const auto newUnit = loadUnit("PropertyContentAndLocationNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have content changes; the Changed entry already carries the new location
    QVERIFY(countByType(diff, ChangeType::PropertyChanged) > 0);

    verifyPatchProducesByteIdenticalUnit("PropertyContentAndLocationOld.qml",
                                         "PropertyContentAndLocationNew.qml");
}

void tst_QQmlPreviewDiff::aliasLocationOnlyChange()
{
    const auto oldUnit = loadUnit("AliasLocationOnlyOld.qml");
    const auto newUnit = loadUnit("AliasLocationOnlyNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have location change but no content change for alias
    QCOMPARE(countByType(diff, ChangeType::AliasChanged), 0);
    QVERIFY(countByType(diff, ChangeType::AliasLocationChanged) > 0);

    verifyPatchProducesByteIdenticalUnit("AliasLocationOnlyOld.qml", "AliasLocationOnlyNew.qml");
}

void tst_QQmlPreviewDiff::aliasContentOnlyChange()
{
    const auto oldUnit = loadUnit("AliasContentOnlyOld.qml");
    const auto newUnit = loadUnit("AliasContentOnlyNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have content change but no location-only change for alias
    QVERIFY(countByType(diff, ChangeType::AliasChanged) > 0);
    QCOMPARE(countByType(diff, ChangeType::AliasLocationChanged), 0);

    verifyPatchProducesByteIdenticalUnit("AliasContentOnlyOld.qml", "AliasContentOnlyNew.qml");
}

void tst_QQmlPreviewDiff::aliasContentAndLocationChange()
{
    const auto oldUnit = loadUnit("AliasContentAndLocationOld.qml");
    const auto newUnit = loadUnit("AliasContentAndLocationNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have content changes; the Changed entry already carries the new location
    QVERIFY(countByType(diff, ChangeType::AliasChanged) > 0);

    verifyPatchProducesByteIdenticalUnit("AliasContentAndLocationOld.qml",
                                         "AliasContentAndLocationNew.qml");
}

void tst_QQmlPreviewDiff::enumChange()
{
    const auto oldUnit = loadUnit("EnumOld.qml");
    const auto newUnit = loadUnit("EnumNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have EnumChanged for the modified enum
    QCOMPARE(countByType(diff, ChangeType::EnumChanged), 1);
    // Should not have additions or removals
    QCOMPARE(countByType(diff, ChangeType::EnumAdded), 0);
    QCOMPARE(countByType(diff, ChangeType::EnumRemoved), 0);

    verifyPatchProducesByteIdenticalUnit("EnumOld.qml", "EnumNew.qml");
}

void tst_QQmlPreviewDiff::enumAddition()
{
    const auto oldUnit = loadUnit("AddEnumOld.qml");
    const auto newUnit = loadUnit("AddEnumNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have EnumAdded for the new enum
    QCOMPARE(countByType(diff, ChangeType::EnumAdded), 1);
    // Should not have changes or removals
    QCOMPARE(countByType(diff, ChangeType::EnumChanged), 0);
    QCOMPARE(countByType(diff, ChangeType::EnumRemoved), 0);

    verifyPatchProducesByteIdenticalUnit("AddEnumOld.qml", "AddEnumNew.qml");
}

void tst_QQmlPreviewDiff::enumRemoval()
{
    const auto oldUnit = loadUnit("RemoveEnumOld.qml");
    const auto newUnit = loadUnit("RemoveEnumNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have EnumRemoved for the removed enum
    QCOMPARE(countByType(diff, ChangeType::EnumRemoved), 1);
    // Should not have additions or changes
    QCOMPARE(countByType(diff, ChangeType::EnumAdded), 0);
    QCOMPARE(countByType(diff, ChangeType::EnumChanged), 0);

    verifyPatchProducesByteIdenticalUnit("RemoveEnumOld.qml", "RemoveEnumNew.qml");
}

void tst_QQmlPreviewDiff::signalChange()
{
    const auto oldUnit = loadUnit("SignalOld.qml");
    const auto newUnit = loadUnit("SignalNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have SignalChanged for the modified signal (parameters changed)
    QCOMPARE(countByType(diff, ChangeType::SignalChanged), 1);
    // Should not have additions or removals
    QCOMPARE(countByType(diff, ChangeType::SignalAdded), 0);
    QCOMPARE(countByType(diff, ChangeType::SignalRemoved), 0);

    verifyPatchProducesByteIdenticalUnit("SignalOld.qml", "SignalNew.qml");
}

void tst_QQmlPreviewDiff::signalAddition()
{
    const auto oldUnit = loadUnit("AddSignalOld.qml");
    const auto newUnit = loadUnit("AddSignalNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have SignalAdded for the new signals (pressed and released)
    QCOMPARE(countByType(diff, ChangeType::SignalAdded), 2);
    // Should not have changes or removals
    QCOMPARE(countByType(diff, ChangeType::SignalChanged), 0);
    QCOMPARE(countByType(diff, ChangeType::SignalRemoved), 0);

    verifyPatchProducesByteIdenticalUnit("AddSignalOld.qml", "AddSignalNew.qml");
}

void tst_QQmlPreviewDiff::signalRemoval()
{
    const auto oldUnit = loadUnit("RemoveSignalOld.qml");
    const auto newUnit = loadUnit("RemoveSignalNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have SignalRemoved for the removed signals (activated and deactivated)
    QCOMPARE(countByType(diff, ChangeType::SignalRemoved), 2);
    // Should not have additions or changes
    QCOMPARE(countByType(diff, ChangeType::SignalAdded), 0);
    QCOMPARE(countByType(diff, ChangeType::SignalChanged), 0);

    verifyPatchProducesByteIdenticalUnit("RemoveSignalOld.qml", "RemoveSignalNew.qml");
}

void tst_QQmlPreviewDiff::enumLocationOnlyChange()
{
    const auto oldUnit = loadUnit("EnumLocationOnlyOld.qml");
    const auto newUnit = loadUnit("EnumLocationOnlyNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have EnumLocationChanged but not EnumChanged (only location differs)
    QCOMPARE(countByType(diff, ChangeType::EnumLocationChanged), 1);
    QCOMPARE(countByType(diff, ChangeType::EnumChanged), 0);
    QCOMPARE(countByType(diff, ChangeType::EnumAdded), 0);
    QCOMPARE(countByType(diff, ChangeType::EnumRemoved), 0);

    verifyPatchProducesByteIdenticalUnit("EnumLocationOnlyOld.qml", "EnumLocationOnlyNew.qml");
}

void tst_QQmlPreviewDiff::enumContentAndLocationChange()
{
    const auto oldUnit = loadUnit("EnumContentAndLocationOld.qml");
    const auto newUnit = loadUnit("EnumContentAndLocationNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // EnumChanged subsumes location; no separate EnumLocationChanged needed
    QCOMPARE(countByType(diff, ChangeType::EnumChanged), 1);
    QCOMPARE(countByType(diff, ChangeType::EnumLocationChanged), 0);
    QCOMPARE(countByType(diff, ChangeType::EnumAdded), 0);
    QCOMPARE(countByType(diff, ChangeType::EnumRemoved), 0);

    verifyPatchProducesByteIdenticalUnit("EnumContentAndLocationOld.qml",
                                         "EnumContentAndLocationNew.qml");
}

void tst_QQmlPreviewDiff::enumValueChange()
{
    const auto oldUnit = loadUnit("EnumValueChangeOld.qml");
    const auto newUnit = loadUnit("EnumValueChangeNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Only individual enum values differ (Loading and Error are reordered).
    QCOMPARE(countByType(diff, ChangeType::EnumChanged), 1);
    QCOMPARE(countByType(diff, ChangeType::EnumLocationChanged), 0);

    verifyPatchProducesByteIdenticalUnit("EnumValueChangeOld.qml", "EnumValueChangeNew.qml");
}

void tst_QQmlPreviewDiff::enumValueLocationChange()
{
    const auto oldUnit = loadUnit("EnumValueLocationOnlyOld.qml");
    const auto newUnit = loadUnit("EnumValueLocationOnlyNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Enum name, value count, and value content are the same. The enum's own location
    // is the same (the "enum" keyword stays on the same line). Only the individual
    // enum value locations differ (single-line vs multi-line formatting).
    QCOMPARE(countByType(diff, ChangeType::EnumChanged), 0);
    QCOMPARE(countByType(diff, ChangeType::EnumLocationChanged), 0);

    verifyPatchProducesByteIdenticalUnit("EnumValueLocationOnlyOld.qml",
                                         "EnumValueLocationOnlyNew.qml");
}

void tst_QQmlPreviewDiff::signalLocationOnlyChange()
{
    const auto oldUnit = loadUnit("SignalLocationOnlyOld.qml");
    const auto newUnit = loadUnit("SignalLocationOnlyNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Should have SignalLocationChanged but not SignalChanged (only location differs)
    QCOMPARE(countByType(diff, ChangeType::SignalLocationChanged), 1);
    QCOMPARE(countByType(diff, ChangeType::SignalChanged), 0);
    QCOMPARE(countByType(diff, ChangeType::SignalAdded), 0);
    QCOMPARE(countByType(diff, ChangeType::SignalRemoved), 0);

    verifyPatchProducesByteIdenticalUnit("SignalLocationOnlyOld.qml", "SignalLocationOnlyNew.qml");
}

void tst_QQmlPreviewDiff::signalContentAndLocationChange()
{
    const auto oldUnit = loadUnit("SignalContentAndLocationOld.qml");
    const auto newUnit = loadUnit("SignalContentAndLocationNew.qml");
    QVERIFY(oldUnit && newUnit);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // SignalChanged subsumes location; no separate SignalLocationChanged needed
    QCOMPARE(countByType(diff, ChangeType::SignalChanged), 1);
    QCOMPARE(countByType(diff, ChangeType::SignalLocationChanged), 0);
    QCOMPARE(countByType(diff, ChangeType::SignalAdded), 0);
    QCOMPARE(countByType(diff, ChangeType::SignalRemoved), 0);

    verifyPatchProducesByteIdenticalUnit("SignalContentAndLocationOld.qml",
                                         "SignalContentAndLocationNew.qml");
}

// Patch verification tests
// Verify that applying the diff produces a unit that is byte-for-byte identical to the new unit
void tst_QQmlPreviewDiff::patch_data()
{
    QTest::addColumn<QString>("oldFile");
    QTest::addColumn<QString>("newFile");

    QTest::newRow("Identical") << "Identical1.qml"
                               << "Identical2.qml";
    QTest::newRow("Constant change") << "ConstantOld.qml"
                                     << "ConstantNew.qml";
    QTest::newRow("Multiple constant changes") << "MultiConstantOld.qml"
                                               << "MultiConstantNew.qml";
    QTest::newRow("Binding change") << "BindingOld.qml"
                                    << "BindingNew.qml";
    QTest::newRow("Same size binding change") << "SameSizeBindingOld.qml"
                                              << "SameSizeBindingNew.qml";
    QTest::newRow("Binding to constant") << "BindingToConstantOld.qml"
                                         << "BindingToConstantNew.qml";
    QTest::newRow("Constant to binding") << "ConstantToBindingOld.qml"
                                         << "ConstantToBindingNew.qml";
    QTest::newRow("Object binding add/remove") << "ObjectBindingAddRemoveOld.qml"
                                               << "ObjectBindingAddRemoveNew.qml";
    QTest::newRow("Property addition") << "PropertyAddOld.qml" << "PropertyAddNew.qml";
    QTest::newRow("Property removal") << "PropertyRemoveOld.qml" << "PropertyRemoveNew.qml";
    QTest::newRow("Object addition") << "ObjectAddOld.qml" << "ObjectAddNew.qml";
    QTest::newRow("Object removal") << "ObjectRemoveOld.qml" << "ObjectRemoveNew.qml";
    QTest::newRow("Base type change") << "BaseTypeOld.qml" << "BaseTypeNew.qml";
    QTest::newRow("Nested object change") << "NestedOld.qml" << "NestedNew.qml";
    QTest::newRow("Mixed changes") << "MixedOld.qml" << "MixedNew.qml";

    // More fine grained tests
    QTest::newRow("Property Add") << "AddPropertyOld.qml" << "AddPropertyNew.qml";
    QTest::newRow("Property Remove") << "RemovePropertyOld.qml" << "RemovePropertyNew.qml";
    QTest::newRow("Property Rename") << "RenamePropertyOld.qml" << "RenamePropertyNew.qml";
    QTest::newRow("Property Change Type")
            << "ChangePropertyTypeOld.qml" << "ChangePropertyTypeNew.qml";
    QTest::newRow("Property Change DefaultValue")
            << "ChangeDefaultValueOld.qml" << "ChangeDefaultValueNew.qml";
    QTest::newRow("Property Add Alias") << "AddAliasOld.qml" << "AddAliasNew.qml";
    QTest::newRow("Property Remove Alias") << "RemoveAliasOld.qml" << "RemoveAliasNew.qml";
}

void tst_QQmlPreviewDiff::patch()
{
    QFETCH(QString, oldFile);
    QFETCH(QString, newFile);

    verifyPatchProducesByteIdenticalUnit(oldFile, newFile);
}

void tst_QQmlPreviewDiff::patchAllFilePairs_data()
{
    QTest::addColumn<QString>("oldFile");
    QTest::addColumn<QString>("newFile");

    // Get all QML files in the data directory
    QDir dataDir(dataDirectory());
    QStringList qmlFiles = dataDir.entryList(QStringList() << "*.qml", QDir::Files, QDir::Name);

    // Generate all pairs (cross product, excluding self-pairs)
    for (const QString &oldFile : qmlFiles) {
        for (const QString &newFile : qmlFiles) {
            if (oldFile == newFile)
                continue;

            QString testName = QString("%1_to_%2").arg(oldFile, newFile);
            testName.replace(".qml", "");
            QTest::newRow(testName.toLatin1().constData()) << oldFile << newFile;
        }
    }
}

void tst_QQmlPreviewDiff::patchAllFilePairs()
{
    QFETCH(QString, oldFile);
    QFETCH(QString, newFile);

    const auto oldUnit = loadUnit(oldFile);
    const auto newUnit = loadUnit(newFile);
    QVERIFY2(oldUnit, qPrintable(QString("Failed to load old unit: %1").arg(oldFile)));
    QVERIFY2(newUnit, qPrintable(QString("Failed to load new unit: %1").arg(newFile)));

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY2(diff.success, "Diff creation failed");

    QString errorMessage;
    const QByteArray patchedData = patchCompilationUnit(oldUnit->unitData(), diff, &errorMessage);
    QVERIFY2(!patchedData.isEmpty(), qPrintable(errorMessage));

    const auto *patchedUnit = reinterpret_cast<const Unit *>(patchedData.constData());
    const auto *expectedUnit = newUnit->unitData();

    QCOMPARE(patchedUnit->unitSize, expectedUnit->unitSize);
    QVERIFY2(std::memcmp(patchedUnit, expectedUnit, expectedUnit->unitSize) == 0,
             "Patched unit does not match new unit byte-for-byte");
}

void tst_QQmlPreviewDiff::unitFlagsChange()
{
    const auto oldUnit = loadUnit("UnitFlagsOld.qml");
    const auto newUnit = loadUnit("UnitFlagsNew.qml");
    QVERIFY(oldUnit && newUnit);

    // The pragma changes the Unit::flags field (ComponentsBound bit).
    QVERIFY(oldUnit->unitData()->flags != newUnit->unitData()->flags);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);
    QCOMPARE(countByType(diff, ChangeType::UnitMetadataChanged), 1);
    QCOMPARE(std::get<UnitHunk>(diff.changes.front().data).flags,
             quint32(newUnit->unitData()->flags));

    QString errorMessage;
    const QByteArray patched = patchCompilationUnit(oldUnit->unitData(), diff, &errorMessage);
    QVERIFY(!patched.isEmpty());

    const auto *patchedUnit = reinterpret_cast<const Unit *>(patched.constData());
    const auto *expectedUnit = newUnit->unitData();
    QVERIFY(std::memcmp(patchedUnit, expectedUnit, expectedUnit->unitSize) == 0);
}

void tst_QQmlPreviewDiff::translationBindingChange()
{
    const auto oldUnit = loadUnit("TranslationBindingOld.qml");
    const auto newUnit = loadUnit("TranslationBindingNew.qml");
    QVERIFY(oldUnit && newUnit);

    // Verify the files actually produce Type_Translation bindings.
    const auto *qmlUnit = oldUnit->unitData()->qmlUnit();
    bool hasTranslationBinding = false;
    for (quint32 i = 0; i < qmlUnit->nObjects && !hasTranslationBinding; ++i) {
        const Object *obj = qmlUnit->objectAt(i);
        for (quint32 b = 0; b < obj->nBindings; ++b) {
            if (obj->bindingTable()[b].type() == Binding::Type_Translation) {
                hasTranslationBinding = true;
                break;
            }
        }
    }
    QVERIFY(hasTranslationBinding);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);
    QVERIFY(countByType(diff, ChangeType::TranslationDataChanged) > 0);

    QString errorMessage;
    const QByteArray patched = patchCompilationUnit(oldUnit->unitData(), diff, &errorMessage);
    QVERIFY(!patched.isEmpty());

    const auto *patchedUnit = reinterpret_cast<const Unit *>(patched.constData());
    const auto *expectedUnit = newUnit->unitData();
    QVERIFY(std::memcmp(patchedUnit, expectedUnit, expectedUnit->unitSize) == 0);
}

void tst_QQmlPreviewDiff::translationBindingCrossFileChange()
{
    // Test patching from a file WITHOUT translation bindings to one WITH translation bindings.
    const auto oldUnit = loadUnit("AddAliasNew.qml");
    const auto newUnit = loadUnit("TranslationBindingOld.qml");
    QVERIFY(oldUnit && newUnit);

    QCOMPARE(oldUnit->unitData()->translationTableSize, 0u);
    QVERIFY(newUnit->unitData()->translationTableSize > 0u);

    const Unit *ou = oldUnit->unitData();
    const Unit *nu = newUnit->unitData();

    const auto diff = diffCompilationUnits(ou, nu);
    QVERIFY(diff.success);

    QString errorMessage;
    const QByteArray patched = patchCompilationUnit(ou, diff, &errorMessage);
    QVERIFY2(!patched.isEmpty(), qPrintable(errorMessage));

    const auto *patchedUnit = reinterpret_cast<const Unit *>(patched.constData());
    QVERIFY(std::memcmp(patchedUnit, nu, nu->unitSize) == 0);
}

void tst_QQmlPreviewDiff::inlineComponentChange()
{
    const auto oldUnit = loadUnit("InlineComponentOld.qml");
    const auto newUnit = loadUnit("InlineComponentNew.qml");
    QVERIFY(oldUnit && newUnit);

    // Verify the new unit has inline components; the old one does not.
    auto countInlineComponents = [](const Unit *unit) {
        quint32 total = 0;
        const auto *qml = unit->qmlUnit();
        for (quint32 i = 0; i < qml->nObjects; ++i)
            total += qml->objectAt(i)->nInlineComponents;
        return total;
    };
    QCOMPARE(countInlineComponents(oldUnit->unitData()), 0u);
    QVERIFY(countInlineComponents(newUnit->unitData()) > 0u);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);
    QVERIFY(countByType(diff, ChangeType::InlineComponentAdded) > 0);

    QString errorMessage;
    const QByteArray patched = patchCompilationUnit(oldUnit->unitData(), diff, &errorMessage);
    QVERIFY(!patched.isEmpty());

    const auto *patchedUnit = reinterpret_cast<const Unit *>(patched.constData());
    const auto *expectedUnit = newUnit->unitData();
    QVERIFY(std::memcmp(patchedUnit, expectedUnit, expectedUnit->unitSize) == 0);
}

void tst_QQmlPreviewDiff::requiredPropertyExtraDataChange()
{
    const auto oldUnit = loadUnit("RequiredPropertyExtraDataOld.qml");
    const auto newUnit = loadUnit("RequiredPropertyExtraDataNew.qml");
    QVERIFY(oldUnit && newUnit);

    // Verify the new unit has RequiredPropertyExtraData entries; the old one does not.
    auto countRequiredExtra = [](const Unit *unit) {
        quint32 total = 0;
        const auto *qml = unit->qmlUnit();
        for (quint32 i = 0; i < qml->nObjects; ++i)
            total += qml->objectAt(i)->nRequiredPropertyExtraData;
        return total;
    };
    QCOMPARE(countRequiredExtra(oldUnit->unitData()), 0u);
    QVERIFY(countRequiredExtra(newUnit->unitData()) > 0u);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);
    QVERIFY(countByType(diff, ChangeType::RequiredPropertyExtraDataAdded) > 0);

    QString errorMessage;
    const QByteArray patched = patchCompilationUnit(oldUnit->unitData(), diff, &errorMessage);
    QVERIFY(!patched.isEmpty());

    const auto *patchedUnit = reinterpret_cast<const Unit *>(patched.constData());
    const auto *expectedUnit = newUnit->unitData();
    QVERIFY(std::memcmp(patchedUnit, expectedUnit, expectedUnit->unitSize) == 0);
}

// Regression test: signalContentEqual now compares common-type parameter values.
// When both old and new signal parameters have indexIsCommonType() == true,
// the actual common type (e.g. Int vs Real) must be compared.
void tst_QQmlPreviewDiff::signalParameterCommonTypeChange()
{
    const auto oldUnit = loadUnit("SignalCommonTypeOld.qml");
    const auto newUnit = loadUnit("SignalCommonTypeNew.qml");
    QVERIFY(oldUnit && newUnit);

    // Verify both signals have parameters with common types
    const auto *oldObj = oldUnit->unitData()->qmlUnit()->objectAt(0);
    const auto *newObj = newUnit->unitData()->qmlUnit()->objectAt(0);
    QCOMPARE(oldObj->nSignals, 1u);
    QCOMPARE(newObj->nSignals, 1u);

    const Signal *oldSig = oldObj->signalAt(0);
    const Signal *newSig = newObj->signalAt(0);
    QCOMPARE(oldSig->nParameters, 1u);
    QCOMPARE(newSig->nParameters, 1u);

    // Both parameter types should be common types (int and real)
    QVERIFY(oldSig->parameterAt(0)->type.indexIsCommonType());
    QVERIFY(newSig->parameterAt(0)->type.indexIsCommonType());

    // But they should be DIFFERENT common types
    QVERIFY(oldSig->parameterAt(0)->type.typeNameIndexOrCommonType()
            != newSig->parameterAt(0)->type.typeNameIndexOrCommonType());

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // The signal parameter type changed from int to real — both are common types.
    // signalContentEqual must detect this via parameterTypeContentEqual.
    QVERIFY(countByType(diff, ChangeType::SignalChanged) >= 1);
}

// Regression test: parameterTypeContentEqual no longer has a ! precedence error.
// The old code had: !oldType.indexIsCommonType() != newType.indexIsCommonType()
// where ! bound to the first operand only, producing spurious FunctionChanged
// for any function whose formal parameters or return type are common types.
void tst_QQmlPreviewDiff::functionTypedParamFalsePositive()
{
    const auto oldUnit = loadUnit("FunctionTypedParamOld.qml");
    const auto newUnit = loadUnit("FunctionTypedParamNew.qml");
    QVERIFY(oldUnit && newUnit);

    // The only meaningful change is the width constant (100 → 200).
    // The function test(x: int): int is identical in both files.
    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // Verify the width change is detected
    QVERIFY(countByType(diff, ChangeType::BindingChanged)
                    + countByType(diff, ChangeType::ConstantChanged)
            > 0);

    // The function is identical — FunctionChanged must not be emitted.
    QCOMPARE(countByType(diff, ChangeType::FunctionChanged), 0);
}

// Gap: the diff engine never iterates the regexp table.
// Changing a regex flag (e.g. adding /i for case-insensitive) modifies only the
// RegExp entry's flags field — the bytecode (LoadRegExp <index>) is identical,
// so functionContentEqual and bindingContentEqual both return true. The change
// goes completely undetected and the patcher copies the old regexp data verbatim.
void tst_QQmlPreviewDiff::regexpFlagChange()
{
    const auto oldUnit = loadUnit("RegExpFlagOld.qml");
    const auto newUnit = loadUnit("RegExpFlagNew.qml");
    QVERIFY(oldUnit && newUnit);

    // Both units must contain at least one regexp
    QVERIFY(oldUnit->unitData()->regexpTableSize > 0);
    QVERIFY(newUnit->unitData()->regexpTableSize > 0);
    QCOMPARE(oldUnit->unitData()->regexpTableSize, newUnit->unitData()->regexpTableSize);

    // Verify the regexp entries actually differ (flags changed: 0 → IgnoreCase)
    bool regexpsDiffer = false;
    for (quint32 i = 0; i < oldUnit->unitData()->regexpTableSize; ++i) {
        const auto *oldRe = oldUnit->unitData()->regexpAt(i);
        const auto *newRe = newUnit->unitData()->regexpAt(i);
        if (oldRe->flags() != newRe->flags()
            || oldRe->stringIndex() != newRe->stringIndex()) {
            regexpsDiffer = true;
            break;
        }
    }
    QVERIFY2(regexpsDiffer,
             "Test precondition: regexp table entries should differ between old and new units");

    // The diff+patch cycle must produce a byte-identical unit.
    // Since the diff engine never compares the regexp table, the flag change
    // goes undetected and the patcher copies the old regexp data verbatim.
    verifyPatchProducesByteIdenticalUnit("RegExpFlagOld.qml", "RegExpFlagNew.qml");
}

// Gap: the diff engine never iterates the class table.
// Adding a static method to a JS class changes the Class entry's nStaticMethods
// and adds a Method entry. The class table is never compared or patched.
void tst_QQmlPreviewDiff::classTableChange()
{
    const auto oldUnit = loadUnit("ClassTableOld.qml");
    const auto newUnit = loadUnit("ClassTableNew.qml");
    QVERIFY(oldUnit && newUnit);

    QVERIFY(oldUnit->unitData()->classTableSize > 0);
    QVERIFY(newUnit->unitData()->classTableSize > 0);

    verifyPatchProducesByteIdenticalUnit("ClassTableOld.qml", "ClassTableNew.qml");
}

// Gap: the diff engine never iterates the template object table.
// Adding an interpolation to a tagged template literal changes the
// TemplateObject's size field. The table is never compared or patched.
void tst_QQmlPreviewDiff::templateObjectTableChange()
{
    const auto oldUnit = loadUnit("TemplateObjectOld.qml");
    const auto newUnit = loadUnit("TemplateObjectNew.qml");
    QVERIFY(oldUnit && newUnit);

    QVERIFY(oldUnit->unitData()->templateObjectTableSize > 0);
    QVERIFY(newUnit->unitData()->templateObjectTableSize > 0);

    verifyPatchProducesByteIdenticalUnit("TemplateObjectOld.qml", "TemplateObjectNew.qml");
}

// Gap: the diff engine never iterates the JS class table.
// Adding a property to an object literal changes the JSClass member count.
// The table is never compared or patched.
void tst_QQmlPreviewDiff::jsClassTableChange()
{
    const auto oldUnit = loadUnit("JsClassTableOld.qml");
    const auto newUnit = loadUnit("JsClassTableNew.qml");
    QVERIFY(oldUnit && newUnit);

    QVERIFY(oldUnit->unitData()->jsClassTableSize > 0);
    QVERIFY(newUnit->unitData()->jsClassTableSize > 0);

    verifyPatchProducesByteIdenticalUnit("JsClassTableOld.qml", "JsClassTableNew.qml");
}

// Gap: the diff engine never iterates the block table.
// Adding a try-catch block adds a Block entry to the block table.
// The table is never compared or patched.
void tst_QQmlPreviewDiff::blockTableChange()
{
    const auto oldUnit = loadUnit("BlockTableOld.qml");
    const auto newUnit = loadUnit("BlockTableNew.qml");
    QVERIFY(oldUnit && newUnit);

    QVERIFY(oldUnit->unitData()->blockTableSize > 0);
    QVERIFY(newUnit->unitData()->blockTableSize > 0);

    verifyPatchProducesByteIdenticalUnit("BlockTableOld.qml", "BlockTableNew.qml");
}

// Returns the first Type_Number binding that carries the IsResolvedEnum flag, i.e. a
// folded enum value. Such a binding stores the enum value directly in resolvedEnumValue;
// it is NOT an index into the constant table.
static const Binding *findFoldedEnumBinding(const Unit *unit)
{
    const auto *qml = unit->qmlUnit();
    for (quint32 i = 0; i < qml->nObjects; ++i) {
        const Object *obj = qml->objectAt(i);
        for (quint32 b = 0; b < obj->nBindings; ++b) {
            const Binding *binding = obj->bindingTable() + b;
            if (binding->type() == Binding::Type_Number
                && binding->hasFlag(Binding::IsResolvedEnum)) {
                return binding;
            }
        }
    }
    return nullptr;
}

// A folded enum binding stores the raw enum value in resolvedEnumValue, not a constant
// table index. font.weight: Font.Black folds to 900 while the file has no numeric
// constants at all (constantTableSize == 0), so interpreting the value as a constant
// table index is out of range. Before the fix, bindingContentEqual did exactly that and
// tripped Q_ASSERT(idx < constantTableSize). The diff must instead compare the resolved
// enum values and succeed.
void tst_QQmlPreviewDiff::foldedEnumOutOfRange()
{
    const auto oldUnit = loadUnit("FoldedEnumOutOfRangeOld.qml");
    const auto newUnit = loadUnit("FoldedEnumOutOfRangeNew.qml");
    QVERIFY(oldUnit && newUnit);

    // Precondition: both units contain a folded enum whose value would be an invalid
    // index into the constant table (this is what would have tripped the assert).
    const Binding *oldEnum = findFoldedEnumBinding(oldUnit->unitData());
    const Binding *newEnum = findFoldedEnumBinding(newUnit->unitData());
    QVERIFY(oldEnum);
    QVERIFY(newEnum);
    QCOMPARE(oldEnum->value.resolvedEnumValue, newEnum->value.resolvedEnumValue);
    QVERIFY(quint32(oldEnum->value.resolvedEnumValue) >= oldUnit->unitData()->constantTableSize);
    QVERIFY(quint32(newEnum->value.resolvedEnumValue) >= newUnit->unitData()->constantTableSize);

    // Diffing must not assert or crash: the folded enum values are compared directly.
    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    verifyPatchProducesByteIdenticalUnit("FoldedEnumOutOfRangeOld.qml",
                                         "FoldedEnumOutOfRangeNew.qml");
}

// Two documents whose only meaningful difference is the folded enum value
// (font.weight: Font.Bold -> Font.Black). The diff must detect the change by comparing
// the resolved enum values and produce a patch that reconstructs the new unit exactly.
void tst_QQmlPreviewDiff::foldedEnumChange()
{
    const auto oldUnit = loadUnit("FoldedEnumChangeOld.qml");
    const auto newUnit = loadUnit("FoldedEnumChangeNew.qml");
    QVERIFY(oldUnit && newUnit);

    const Binding *oldEnum = findFoldedEnumBinding(oldUnit->unitData());
    const Binding *newEnum = findFoldedEnumBinding(newUnit->unitData());
    QVERIFY(oldEnum);
    QVERIFY(newEnum);

    // The enum values genuinely differ, and both are out of range for the constant table
    // (so the comparison cannot fall back to the constant-table path).
    QVERIFY(oldEnum->value.resolvedEnumValue != newEnum->value.resolvedEnumValue);
    QVERIFY(quint32(oldEnum->value.resolvedEnumValue) >= oldUnit->unitData()->constantTableSize);
    QVERIFY(quint32(newEnum->value.resolvedEnumValue) >= newUnit->unitData()->constantTableSize);

    const auto diff = diffCompilationUnits(oldUnit->unitData(), newUnit->unitData());
    QVERIFY(diff.success);

    // The differing folded enum must be reported as a change.
    QVERIFY(countByType(diff, ChangeType::BindingChanged)
                    + countByType(diff, ChangeType::ObjectChanged)
            > 0);

    verifyPatchProducesByteIdenticalUnit("FoldedEnumChangeOld.qml", "FoldedEnumChangeNew.qml");
}

QTEST_MAIN(tst_QQmlPreviewDiff)
#include "tst_qqmlpreviewdiff.moc"
