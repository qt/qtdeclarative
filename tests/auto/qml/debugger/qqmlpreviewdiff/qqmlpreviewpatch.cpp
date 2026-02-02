// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qqmlpreviewpatch.h"

#include <private/qv4staticvalue_p.h>

#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>

#include <cstring>
#include <numeric>
#include <optional>

//
//  W A R N I N G
//  -------------
//
// This file is AI-generated. It's only meant as test helper.
// Do not move it into the Qt libraries.
//


QT_BEGIN_NAMESPACE

namespace QV4::CompiledData {

template <typename Hunk>
struct ChangeSet
{
    QMap<quint32, Hunk> insertions;
    QHash<quint32, Hunk> replacements;
    QSet<quint32> removals;

    bool isEmpty() const
    {
        return insertions.isEmpty() && replacements.isEmpty() && removals.isEmpty();
    }
    void clear()
    {
        insertions.clear();
        replacements.clear();
        removals.clear();
    }
};

class CompilationUnitPatcher
{
public:
    CompilationUnitPatcher(const Unit *oldUnit, const CompilationUnitDiff &diff)
        : m_oldUnit(oldUnit), m_diff(diff)
    {
    }

    QByteArray apply(QString *errorMessage)
    {
        m_patchedData = QByteArray(reinterpret_cast<const char *>(m_oldUnit), m_oldUnit->unitSize);
        indexChangesByObject();
        collectChanges();

        if (!rebuildUnit()) {
            if (errorMessage)
                *errorMessage = m_errorMessage;
            return {};
        }

        if (!applyTargetLayout()) {
            if (errorMessage)
                *errorMessage = m_errorMessage;
            return {};
        }

        applyChecksum();
        return m_patchedData;
    }

private:
    void indexChangesByObject()
    {
        for (const Change &change : m_diff.changes)
            m_changesByObject[change.objectIndex].append(&change);
    }

    // ===== Offset shifting helpers =====

    template <typename ShiftFn>
    static void shiftAllHeaderOffsets(Unit *u, ShiftFn &&shift)
    {
        shift(u->offsetToFunctionTable);
        shift(u->offsetToClassTable);
        shift(u->offsetToTemplateObjectTable);
        shift(u->offsetToBlockTable);
        shift(u->offsetToJSClassTable);
        shift(u->offsetToTranslationTable);
        shift(u->offsetToLookupTable);
        shift(u->offsetToRegexpTable);
        shift(u->offsetToConstantTable);
        shift(u->offsetToLocalExportEntryTable);
        shift(u->offsetToIndirectExportEntryTable);
        shift(u->offsetToStarExportEntryTable);
        shift(u->offsetToImportEntryTable);
        shift(u->offsetToModuleRequestTable);
        shift(u->offsetToStringTable);
        shift(u->offsetToQmlUnit);
    }

    template <typename ShiftFn>
    static void shiftIndirectTableEntries(QByteArray &data, const Unit *u, ShiftFn &&shift)
    {
        auto shiftTable = [&](quint32 tableOffset, quint32 count) {
            if (tableOffset + count * sizeof(quint32_le) <= static_cast<quint64>(u->unitSize)) {
                quint32_le *offsets = reinterpret_cast<quint32_le *>(data.data() + tableOffset);
                for (quint32 i = 0; i < count; ++i)
                    shift(offsets[i]);
            }
        };
        shiftTable(u->offsetToFunctionTable, u->functionTableSize);
        shiftTable(u->offsetToClassTable, u->classTableSize);
        shiftTable(u->offsetToTemplateObjectTable, u->templateObjectTableSize);
        shiftTable(u->offsetToBlockTable, u->blockTableSize);
        shiftTable(u->offsetToJSClassTable, u->jsClassTableSize);
        shiftTable(u->offsetToStringTable, u->stringTableSize);
    }

    // Shift all offsets (header + indirect table entries) that are >= boundary.
    // Used by simple (flat-array) table rebuilds.
    static void shiftAllOffsets(QByteArray &data, qsizetype boundary, qsizetype delta)
    {
        if (delta == 0)
            return;
        Unit *u = reinterpret_cast<Unit *>(data.data());
        auto shift = [&](quint32_le &offset) {
            if (static_cast<qsizetype>(offset) >= boundary)
                offset = offset + static_cast<qint32>(delta);
        };
        shiftAllHeaderOffsets(u, shift);
        shiftIndirectTableEntries(data, u, shift);
    }

    // Which indirect offset table is being rebuilt (used to skip its own entries).
    enum class IndirectTable { Function, Class, Template, Block, JsClass };

    // Shift all offsets for an indirect table rebuild.
    // "Between" = region between old offset table end and old data start (shifted by otDelta).
    // "After"   = region past old data end (shifted by totalDelta).
    // The caller must save/restore the own table's header offset after this call.
    static void shiftOffsetsForIndirectRebuild(QByteArray &data, qsizetype oldOTEnd,
                                               qsizetype oldDataStart, qsizetype oldDataEnd,
                                               qsizetype otDelta, qsizetype totalDelta,
                                               IndirectTable ownTable)
    {
        if (totalDelta == 0)
            return;
        Unit *u = reinterpret_cast<Unit *>(data.data());

        auto updateBetween = [&](quint32_le &offset) {
            qsizetype off = static_cast<qsizetype>(offset);
            if (off >= oldOTEnd && off <= oldDataStart)
                offset = offset + static_cast<qint32>(otDelta);
        };
        auto updateAfter = [&](quint32_le &offset) {
            if (static_cast<qsizetype>(offset) >= oldDataEnd)
                offset = offset + static_cast<qint32>(totalDelta);
        };

        // All headers except own, string, and qml get updateBetween.
        if (ownTable != IndirectTable::Function)
            updateBetween(u->offsetToFunctionTable);
        if (ownTable != IndirectTable::Class)
            updateBetween(u->offsetToClassTable);
        if (ownTable != IndirectTable::Template)
            updateBetween(u->offsetToTemplateObjectTable);
        if (ownTable != IndirectTable::Block)
            updateBetween(u->offsetToBlockTable);
        if (ownTable != IndirectTable::JsClass)
            updateBetween(u->offsetToJSClassTable);
        updateBetween(u->offsetToTranslationTable);
        updateBetween(u->offsetToLookupTable);
        updateBetween(u->offsetToRegexpTable);
        updateBetween(u->offsetToConstantTable);
        updateBetween(u->offsetToLocalExportEntryTable);
        updateBetween(u->offsetToIndirectExportEntryTable);
        updateBetween(u->offsetToStarExportEntryTable);
        updateBetween(u->offsetToImportEntryTable);
        updateBetween(u->offsetToModuleRequestTable);

        // JsClass rebuild also applies updateAfter to translation + export/import headers.
        if (ownTable == IndirectTable::JsClass) {
            updateAfter(u->offsetToTranslationTable);
            updateAfter(u->offsetToLocalExportEntryTable);
            updateAfter(u->offsetToIndirectExportEntryTable);
            updateAfter(u->offsetToStarExportEntryTable);
            updateAfter(u->offsetToImportEntryTable);
            updateAfter(u->offsetToModuleRequestTable);
        }

        // String + qml always get updateAfter only.
        updateAfter(u->offsetToStringTable);
        updateAfter(u->offsetToQmlUnit);

        // All indirect table entries (except own) get both shifts sequentially.
        auto shiftEntries = [&](quint32 tableOffset, quint32 count) {
            if (tableOffset + count * sizeof(quint32_le) <= static_cast<quint64>(u->unitSize)) {
                quint32_le *offsets = reinterpret_cast<quint32_le *>(data.data() + tableOffset);
                for (quint32 i = 0; i < count; ++i) {
                    updateBetween(offsets[i]);
                    updateAfter(offsets[i]);
                }
            }
        };
        if (ownTable != IndirectTable::Function)
            shiftEntries(u->offsetToFunctionTable, u->functionTableSize);
        if (ownTable != IndirectTable::Class)
            shiftEntries(u->offsetToClassTable, u->classTableSize);
        if (ownTable != IndirectTable::Template)
            shiftEntries(u->offsetToTemplateObjectTable, u->templateObjectTableSize);
        if (ownTable != IndirectTable::Block)
            shiftEntries(u->offsetToBlockTable, u->blockTableSize);
        if (ownTable != IndirectTable::JsClass)
            shiftEntries(u->offsetToJSClassTable, u->jsClassTableSize);

        // String table entries get updateAfter only.
        if (u->offsetToStringTable + u->stringTableSize * sizeof(quint32_le)
            <= static_cast<quint64>(u->unitSize)) {
            quint32_le *stringOffsets =
                    reinterpret_cast<quint32_le *>(data.data() + u->offsetToStringTable);
            for (quint32 i = 0; i < u->stringTableSize; ++i)
                updateAfter(stringOffsets[i]);
        }
    }

    template <typename Hunk, typename VariantType = Hunk>
    static void routeChange(ChangeSet<Hunk> &cs, const Change &change, ChangeType addType,
                            ChangeType removeType)
    {
        if (change.type == addType)
            cs.insertions.insert(change.index, std::get<VariantType>(change.data));
        else if (change.type == removeType)
            cs.removals.insert(change.index);
        else
            cs.replacements.insert(change.index, std::get<VariantType>(change.data));
    }

    // ===== Merge helpers =====

    // Merge old entries with insertions/replacements/removals for indirect tables.
    // The getOldEntry callback copies and remaps a single old entry (returns nullopt to skip).
    template <typename Hunk, typename GetOldEntry>
    static QVector<Hunk> mergeIndirectEntries(const ChangeSet<Hunk> &cs, quint32 oldCount,
                                              GetOldEntry &&getOldEntry)
    {
        quint32 newCount = oldCount;
        for (auto it = cs.insertions.cbegin(); it != cs.insertions.cend(); ++it)
            newCount = std::max(newCount, it.key() + 1);
        for (auto it = cs.replacements.cbegin(); it != cs.replacements.cend(); ++it)
            newCount = std::max(newCount, it.key() + 1);

        QVector<std::optional<Hunk>> entries(newCount);
        for (quint32 i = 0; i < oldCount; ++i) {
            if (cs.removals.contains(i))
                continue;
            auto entry = getOldEntry(i);
            if (entry.has_value() && i < newCount)
                entries[i] = std::move(entry);
        }
        for (auto it = cs.replacements.cbegin(); it != cs.replacements.cend(); ++it) {
            if (it.key() < newCount)
                entries[it.key()] = it.value();
        }
        for (auto it = cs.insertions.cbegin(); it != cs.insertions.cend(); ++it) {
            if (it.key() < newCount)
                entries[it.key()] = it.value();
        }

        QVector<Hunk> result;
        for (auto &opt : entries) {
            if (opt.has_value())
                result.append(std::move(opt.value()));
        }
        return result;
    }

    // Merge old entries for simple (flat-array) tables like lookup and regexp.
    // Uses index-based insertion/replacement/removal with compaction.
    template <typename Entry, typename GetOldEntry>
    static QList<Entry> mergeSimpleEntries(const ChangeSet<Entry> &cs, quint32 oldCount,
                                           GetOldEntry &&getOldEntry)
    {
        QList<Entry> result;
        quint32 oldIdx = 0;
        quint32 newIdx = 0;
        while (cs.insertions.contains(newIdx) && oldIdx == 0) {
            result.append(cs.insertions.value(newIdx));
            ++newIdx;
        }
        while (oldIdx < oldCount) {
            if (cs.removals.contains(oldIdx)) {
                ++oldIdx;
                continue;
            }
            if (cs.replacements.contains(newIdx))
                result.append(cs.replacements.value(newIdx));
            else
                result.append(getOldEntry(oldIdx));
            ++oldIdx;
            ++newIdx;
            while (cs.insertions.contains(newIdx)) {
                result.append(cs.insertions.value(newIdx));
                ++newIdx;
            }
        }
        while (cs.insertions.contains(newIdx)) {
            result.append(cs.insertions.value(newIdx));
            ++newIdx;
        }
        return result;
    }

    // ===== Data region and offset helpers =====

    // Validate an indirect (offset-based) table: check that the OT and all entries fit
    // within the unit. SizeFunc: (const EntryType *) → int.
    template <typename EntryType, typename SizeFunc>
    bool validateIndirectTable(quint32 tableOffset, quint32 tableSize, SizeFunc &&calcSize)
    {
        const qsizetype unitSz =
                reinterpret_cast<const Unit *>(m_patchedData.constData())->unitSize;
        const qsizetype otStart = static_cast<qsizetype>(tableOffset);
        const qsizetype otEnd = otStart + qsizetype(tableSize) * qsizetype(sizeof(quint32_le));
        if (otStart < 0 || otEnd > unitSz || otStart > unitSz) {
            m_errorMessage = QStringLiteral("Corrupted offset table in table rebuild");
            return false;
        }
        if (tableSize > 0) {
            const quint32_le *offsets =
                    reinterpret_cast<const quint32_le *>(m_patchedData.constData() + otStart);
            for (quint32 i = 0; i < tableSize; ++i) {
                qsizetype off = static_cast<qsizetype>(offsets[i]);
                if (off < 0 || off + qsizetype(sizeof(EntryType)) > unitSz) {
                    m_errorMessage = QStringLiteral("Corrupted offset entry in table rebuild");
                    return false;
                }
                const EntryType *entry =
                        reinterpret_cast<const EntryType *>(m_patchedData.constData() + off);
                if (off + calcSize(entry) > unitSz) {
                    m_errorMessage = QStringLiteral("Corrupted data entry in table rebuild");
                    return false;
                }
            }
        }
        return true;
    }

    // Find the min/max byte range of entries in an indirect (offset-based) table.
    template <typename T, typename SizeFunc>
    static bool findDataRegionBounds(const QByteArray &data, quint32 offsetTableAddr,
                                     quint32 tableSize, SizeFunc &&calcSize, quint32 &outStart,
                                     quint32 &outEnd)
    {
        if (tableSize == 0)
            return false;
        const quint32_le *offsets =
                reinterpret_cast<const quint32_le *>(data.constData() + offsetTableAddr);
        outStart = std::numeric_limits<quint32>::max();
        outEnd = 0;
        for (quint32 i = 0; i < tableSize; ++i) {
            const T *entry = reinterpret_cast<const T *>(data.constData() + offsets[i]);
            quint32 sz = calcSize(entry);
            if (offsets[i] < outStart)
                outStart = offsets[i];
            if (offsets[i] + sz > outEnd)
                outEnd = offsets[i] + sz;
        }
        return true;
    }

    // Shift all entries in an offset table by delta.
    static void applyOffsetDelta(QByteArray &data, quint32 tableOffset, quint32 tableSize,
                                 qint32 delta)
    {
        if (tableSize == 0 || delta == 0)
            return;
        quint32_le *offsets = reinterpret_cast<quint32_le *>(data.data() + tableOffset);
        for (quint32 i = 0; i < tableSize; ++i)
            offsets[i] = offsets[i] + delta;
    }

    // Merge entries for flat tables where replacements are keyed by old index (constants,
    // translations). ConvertFn converts Hunk → Entry; identity when Hunk == Entry.
    template <typename Entry, typename Hunk, typename GetOldEntry, typename ConvertFn>
    static QList<Entry> mergeEntriesOldKeyed(const ChangeSet<Hunk> &cs, quint32 oldCount,
                                             GetOldEntry &&getOldEntry, ConvertFn &&convert)
    {
        struct Indexed
        {
            Hunk value;
            quint32 index;
        };
        QList<Indexed> sorted;
        for (auto it = cs.insertions.cbegin(); it != cs.insertions.cend(); ++it)
            sorted.append({ it.value(), it.key() });
        std::sort(sorted.begin(), sorted.end(),
                  [](const auto &a, const auto &b) { return a.index < b.index; });

        QList<Entry> result;
        quint32 newIdx = 0, insIdx = 0, oldIdx = 0;
        while (oldIdx < oldCount || insIdx < static_cast<quint32>(sorted.size())) {
            while (insIdx < static_cast<quint32>(sorted.size()) && sorted[insIdx].index == newIdx) {
                result.append(convert(sorted[insIdx].value));
                ++newIdx;
                ++insIdx;
            }
            if (oldIdx < oldCount) {
                if (cs.replacements.contains(oldIdx)) {
                    result.append(convert(cs.replacements[oldIdx]));
                    ++newIdx;
                } else if (!cs.removals.contains(oldIdx)) {
                    result.append(getOldEntry(oldIdx));
                    ++newIdx;
                }
                ++oldIdx;
            }
        }
        while (insIdx < static_cast<quint32>(sorted.size())) {
            result.append(convert(sorted[insIdx].value));
            ++insIdx;
        }
        return result;
    }

    static TranslationData toTranslationData(const TranslationDataHunk &d)
    {
        TranslationData t;
        t.stringIndex = d.stringIndex;
        t.commentIndex = d.commentIndex;
        t.number = d.number;
        t.contextIndex = d.contextIndex;
        return t;
    }

    // Splice a flat (contiguous) table into m_patchedData: replace oldCount entries at
    // oldTableOffset with newEntries, shift all downstream offsets, and update the Unit
    // header's count/offset fields identified by byte offsets countFieldOff/offsetFieldOff.
    template <typename Entry>
    void spliceFlatTable(const QList<Entry> &newEntries, quint32 oldTableOffset, quint32 oldCount,
                         size_t countFieldOff, size_t offsetFieldOff)
    {
        const quint32 newCount = static_cast<quint32>(newEntries.size());
        const qsizetype oldTableSize = static_cast<qsizetype>(oldCount) * sizeof(Entry);
        const qsizetype newTableSize = static_cast<qsizetype>(newCount) * sizeof(Entry);
        const qsizetype delta = newTableSize - oldTableSize;

        auto unitField = [](char *base, size_t off) -> quint32_le & {
            return *reinterpret_cast<quint32_le *>(base + off);
        };

        auto writeEntries = [&](char *base) {
            Entry *table = reinterpret_cast<Entry *>(base + oldTableOffset);
            for (qsizetype i = 0; i < newEntries.size(); ++i)
                table[i] = newEntries[i];
        };

        if (delta == 0) {
            writeEntries(m_patchedData.data());
            unitField(m_patchedData.data(), countFieldOff) = newCount;
            return;
        }

        const quint32 oldTableEnd = oldTableOffset + static_cast<quint32>(oldTableSize);
        const qsizetype oldUnitSize =
                reinterpret_cast<const Unit *>(m_patchedData.constData())->unitSize;
        const qsizetype newUnitSize = oldUnitSize + delta;
        QByteArray newData(newUnitSize, '\0');

        std::memcpy(newData.data(), m_patchedData.constData(), oldTableOffset);
        writeEntries(newData.data());

        const qsizetype newTableEnd = oldTableOffset + newTableSize;
        const qsizetype remaining = oldUnitSize - oldTableEnd;
        if (remaining > 0)
            std::memcpy(newData.data() + newTableEnd, m_patchedData.constData() + oldTableEnd,
                        remaining);

        Unit *newUnit = reinterpret_cast<Unit *>(newData.data());
        newUnit->unitSize = static_cast<quint32>(newUnitSize);
        unitField(newData.data(), countFieldOff) = newCount;

        quint32 savedOffset = unitField(newData.data(), offsetFieldOff);
        shiftAllOffsets(newData, oldTableEnd, delta);
        unitField(newData.data(), offsetFieldOff) = savedOffset;

        m_patchedData = newData;
    }

    // ===== Splice-and-shift helper for indirect table rebuilds =====

    // Performs the common splice (memcpy before/OT/between/data/after), header update,
    // and offset shifting for indirect table rebuilds (class, template, jsClass, block).
    // The caller provides pre-serialized entry data and sizes. Returns false on validation error.
    bool spliceAndShiftIndirectTable(qsizetype oldOTStart, qsizetype oldOTSize,
                                     qsizetype oldDataStart, qsizetype oldDataEnd,
                                     quint32 actualCount, const QByteArray &entryData,
                                     const QVector<qsizetype> &entrySizes, IndirectTable ownTable,
                                     size_t tableSizeFieldOffset, size_t tableOffsetFieldOffset)
    {
        const Unit *oldUnit = reinterpret_cast<const Unit *>(m_patchedData.constData());

        qsizetype newOTSize = actualCount * sizeof(quint32_le);
        qsizetype otDelta = newOTSize - oldOTSize;
        qsizetype dataDelta = entryData.size() - (oldDataEnd - oldDataStart);
        qsizetype totalDelta = otDelta + dataDelta;

        qsizetype newUnitSize = oldUnit->unitSize + totalDelta;
        if (newUnitSize <= 0 || oldOTStart > newUnitSize
            || oldOTStart > static_cast<qsizetype>(oldUnit->unitSize)
            || oldDataStart > static_cast<qsizetype>(oldUnit->unitSize)
            || oldDataEnd > static_cast<qsizetype>(oldUnit->unitSize)
            || oldDataStart > oldDataEnd) {
            m_errorMessage = QStringLiteral("Invalid unit size in table rebuild");
            return false;
        }
        QByteArray newData(newUnitSize, '\0');

        // Copy before offset table
        std::memcpy(newData.data(), m_patchedData.constData(), oldOTStart);

        // Build new offset table
        quint32_le *offsetTable = reinterpret_cast<quint32_le *>(newData.data() + oldOTStart);

        // Copy between offset table end and data region
        qsizetype oldOTEnd = oldOTStart + oldOTSize;
        qsizetype newOTEnd = oldOTStart + newOTSize;
        qsizetype betweenSize = oldDataStart - oldOTEnd;
        if (betweenSize > 0) {
            std::memcpy(newData.data() + newOTEnd, m_patchedData.constData() + oldOTEnd,
                        betweenSize);
        }

        // Write entry data with offset table
        qsizetype newDataStart = oldDataStart + otDelta;
        quint32 currentOffset = static_cast<quint32>(newDataStart);
        qsizetype entryPos = 0;
        for (quint32 i = 0; i < actualCount; ++i) {
            offsetTable[i] = currentOffset;
            qsizetype sz = entrySizes[i];
            std::memcpy(newData.data() + currentOffset, entryData.constData() + entryPos, sz);
            currentOffset += static_cast<quint32>(sz);
            entryPos += sz;
        }

        // Copy after old data region
        qsizetype newDataEnd = newDataStart + entryData.size();
        qsizetype remaining = oldUnit->unitSize - oldDataEnd;
        if (remaining > 0) {
            std::memcpy(newData.data() + newDataEnd, m_patchedData.constData() + oldDataEnd,
                        remaining);
        }

        // Update header
        Unit *newUnit = reinterpret_cast<Unit *>(newData.data());
        newUnit->unitSize = static_cast<quint32>(newUnitSize);
        *reinterpret_cast<quint32_le *>(reinterpret_cast<char *>(newUnit) + tableSizeFieldOffset) =
                actualCount;

        // Save/restore own offset around shift
        quint32_le &ownOffsetRef = *reinterpret_cast<quint32_le *>(reinterpret_cast<char *>(newUnit)
                                                                   + tableOffsetFieldOffset);
        quint32 savedOffset = ownOffsetRef;
        shiftOffsetsForIndirectRebuild(newData, oldOTEnd, oldDataStart, oldDataEnd, otDelta,
                                       totalDelta, ownTable);
        ownOffsetRef = savedOffset;

        m_patchedData = newData;
        return true;
    }

    void collectChanges()
    {
        for (const Change &change : m_diff.changes) {
            switch (change.type) {
            case ChangeType::StringDataAdded:
            case ChangeType::StringDataChanged:
            case ChangeType::StringDataRemoved:
                routeChange(m_strings, change, ChangeType::StringDataAdded, ChangeType::StringDataRemoved);
                break;
            case ChangeType::ConstantAdded:
            case ChangeType::ConstantChanged:
            case ChangeType::ConstantRemoved:
                routeChange<quint64, ConstantHunk>(m_constants, change, ChangeType::ConstantAdded,
                                                   ChangeType::ConstantRemoved);
                break;
            case ChangeType::FunctionAdded:
            case ChangeType::FunctionChanged:
            case ChangeType::FunctionLocationChanged:
            case ChangeType::FunctionRemoved:
                routeChange(m_functions, change, ChangeType::FunctionAdded,
                            ChangeType::FunctionRemoved);
                break;
            case ChangeType::LookupAdded:
            case ChangeType::LookupChanged:
            case ChangeType::LookupRemoved:
                routeChange(m_lookups, change, ChangeType::LookupAdded, ChangeType::LookupRemoved);
                break;
            case ChangeType::TranslationDataAdded:
            case ChangeType::TranslationDataChanged:
            case ChangeType::TranslationDataRemoved:
                routeChange(m_translations, change, ChangeType::TranslationDataAdded,
                            ChangeType::TranslationDataRemoved);
                break;
            case ChangeType::UnitMetadataChanged:
                m_unitMetadata = std::get<UnitHunk>(change.data);
                break;
            case ChangeType::RegExpAdded:
            case ChangeType::RegExpChanged:
            case ChangeType::RegExpRemoved:
                routeChange(m_regexps, change, ChangeType::RegExpAdded, ChangeType::RegExpRemoved);
                break;
            case ChangeType::ClassAdded:
            case ChangeType::ClassChanged:
            case ChangeType::ClassRemoved:
                routeChange(m_classes, change, ChangeType::ClassAdded, ChangeType::ClassRemoved);
                break;
            case ChangeType::TemplateObjectAdded:
            case ChangeType::TemplateObjectChanged:
            case ChangeType::TemplateObjectRemoved:
                routeChange(m_templateObjects, change, ChangeType::TemplateObjectAdded,
                            ChangeType::TemplateObjectRemoved);
                break;
            case ChangeType::JSClassAdded:
            case ChangeType::JSClassChanged:
            case ChangeType::JSClassRemoved:
                routeChange(m_jsClasses, change, ChangeType::JSClassAdded,
                            ChangeType::JSClassRemoved);
                break;
            case ChangeType::BlockAdded:
            case ChangeType::BlockChanged:
            case ChangeType::BlockRemoved:
                routeChange(m_blocks, change, ChangeType::BlockAdded, ChangeType::BlockRemoved);
                break;
            default:
                break;
            }
        }
    }

    bool applyMetadataChange()
    {
        if (!m_unitMetadata.has_value())
            return true;
        Unit *unit = reinterpret_cast<Unit *>(m_patchedData.data());
        unit->sourceTimeStamp = m_unitMetadata->sourceTimeStamp;
        unit->sourceFileIndex = m_unitMetadata->sourceFileIndex;
        unit->finalUrlIndex = m_unitMetadata->finalUrlIndex;
        unit->flags = m_unitMetadata->flags;

        return true;
    }

    bool getOrAddString(const QString &str, quint32 *outIndex)
    {
        Unit *unit = reinterpret_cast<Unit *>(m_patchedData.data());

        // First, check if the string exists in the current (possibly rebuilt) unit
        for (quint32 i = 0; i < unit->stringTableSize; ++i) {
            QString s = unit->stringAtInternal(i);
            if (s == str) {
                *outIndex = i;
                return true;
            }
        }

        // String not found - this should not happen if we did rebuildStringTable correctly
        m_errorMessage = QStringLiteral("String not found after rebuild: ") + str;
        return false;
    }

    quint32 remapStringIndex(quint32 oldIdx) const
    {
        if (m_oldToNewStringIndex.isEmpty())
            return oldIdx;
        return m_oldToNewStringIndex.value(oldIdx, oldIdx);
    }

    ParameterType remapParameterType(ParameterType pt) const
    {
        if (!pt.indexIsCommonType()) {
            ParameterType::Flags flags;
            if (pt.isList())
                flags |= ParameterType::List;
            pt.set(flags, remapStringIndex(pt.typeNameIndexOrCommonType()));
        }
        return pt;
    }

    Parameter remapParameter(Parameter p) const
    {
        p.nameIndex = remapStringIndex(p.nameIndex);
        p.type = remapParameterType(p.type);
        return p;
    }

    bool rebuildUnit()
    {
        if (!rebuildStringTable())
            return false;

        if (!rebuildConstantTableWithAdditions())
            return false;

        if (!rebuildFunctionTable())
            return false;

        if (!rebuildLookupTable())
            return false;

        if (!rebuildTranslationTable())
            return false;

        if (!rebuildRegExpTable())
            return false;

        if (!rebuildClassTable())
            return false;

        if (!rebuildTemplateObjectTable())
            return false;

        if (!rebuildJsClassTable())
            return false;

        if (!rebuildBlockTable())
            return false;

        if (!rebuildAllObjects())
            return false;

        if (!applyMetadataChange())
            return false;

        return true;
    }

    template <typename T>
    static void writeEntries(char *base, quint32 offset, const QList<T> &items)
    {
        T *table = reinterpret_cast<T *>(base + offset);
        for (qsizetype i = 0; i < items.size(); ++i)
            table[i] = items[i];
    }

    static void applyImport(Import &imp, const ImportHunk &id)
    {
        imp = Import();
        imp.type = id.type;
        imp.uriIndex = id.uriIndex;
        imp.qualifierIndex = id.qualifierIndex;
        imp.location = id.location;
        imp.version = id.version;
    }

    bool rebuildAllObjects()
    {
        // Get the QmlUnit from the patched data (tables already rebuilt)
        const Unit *patchedUnit = reinterpret_cast<const Unit *>(m_patchedData.constData());
        const QmlUnit *oldQml = patchedUnit->qmlUnit();

        // Collect object changes
        QSet<quint32> removedObjects;
        QList<const Change *> addedObjects;
        for (const Change &change : m_diff.changes) {
            if (change.type == ChangeType::ObjectRemoved) {
                removedObjects.insert(static_cast<quint32>(change.index));
            } else if (change.type == ChangeType::ObjectAdded) {
                addedObjects.append(&change);
            }
        }

        // Sort added objects by index
        std::sort(addedObjects.begin(), addedObjects.end(),
                  [](const Change *a, const Change *b) { return a->index < b->index; });

        // Build the new object list
        QList<QByteArray> newObjects;
        quint32 addedIdx = 0;
        quint32 oldIdx = 0;
        quint32 newObjIdx = 0;

        while (oldIdx < oldQml->nObjects || addedIdx < static_cast<quint32>(addedObjects.size())) {
            // Check if there's an object to add at this position
            while (addedIdx < static_cast<quint32>(addedObjects.size())
                   && addedObjects[addedIdx]->index == newObjIdx) {
                QByteArray objData =
                        buildNewObject(std::get<ObjectHunk>(addedObjects[addedIdx]->data));
                if (objData.isEmpty()) {
                    return false;
                }
                newObjects.append(objData);
                ++addedIdx;
                ++newObjIdx;
            }

            // Process old object if not removed
            if (oldIdx < oldQml->nObjects) {
                if (!removedObjects.contains(oldIdx)) {
                    const Object *oldObj = oldQml->objectAt(oldIdx);
                    QByteArray objData = rebuildObject(oldIdx, oldObj);
                    if (objData.isEmpty()) {
                        return false;
                    }
                    newObjects.append(objData);
                    ++newObjIdx;
                }
                ++oldIdx;
            }
        }

        // Handle any remaining added objects
        while (addedIdx < static_cast<quint32>(addedObjects.size())) {
            QByteArray objData = buildNewObject(std::get<ObjectHunk>(addedObjects[addedIdx]->data));
            if (objData.isEmpty()) {
                return false;
            }
            newObjects.append(objData);
            ++addedIdx;
        }

        // Collect import changes
        QHash<quint32, const Change *> changedImports;
        QSet<quint32> removedImports;
        QList<const Change *> addedImports;
        for (const Change &change : m_diff.changes) {
            if (change.type == ChangeType::ImportChanged
                || change.type == ChangeType::ImportLocationChanged) {
                changedImports[change.index] = &change;
            } else if (change.type == ChangeType::ImportRemoved) {
                removedImports.insert(change.index);
            } else if (change.type == ChangeType::ImportAdded) {
                addedImports.append(&change);
            }
        }

        // Calculate new import count
        quint32 newImportCount = oldQml->nImports - removedImports.size() + addedImports.size();

        // Calculate new QmlUnit size
        qsizetype qmlUnitHeaderSize = 16; // nImports, offsetToImports, nObjects, offsetToObjects
        qsizetype importsSize = static_cast<qsizetype>(newImportCount) * sizeof(Import);
        qsizetype objectOffsetsSize =
                static_cast<qsizetype>(newObjects.size()) * sizeof(quint32_le);

        qsizetype totalObjectsSize = 0;
        for (const QByteArray &objData : newObjects) {
            totalObjectsSize += objData.size();
        }

        // Build new QmlUnit
        QByteArray newQmlUnitData;
        newQmlUnitData.resize(qmlUnitHeaderSize + importsSize + objectOffsetsSize
                              + totalObjectsSize);

        QmlUnit *newQmlUnit = reinterpret_cast<QmlUnit *>(newQmlUnitData.data());
        newQmlUnit->nImports = newImportCount;
        newQmlUnit->offsetToImports = static_cast<quint32>(qmlUnitHeaderSize);
        newQmlUnit->nObjects = static_cast<quint32>(newObjects.size());
        newQmlUnit->offsetToObjects = static_cast<quint32>(qmlUnitHeaderSize + importsSize);

        // Write imports (copy from old, apply changes)
        if (newImportCount > 0) {
            Import *newImports =
                    reinterpret_cast<Import *>(newQmlUnitData.data() + qmlUnitHeaderSize);
            quint32 newIdx = 0;
            for (quint32 oldIdx = 0; oldIdx < oldQml->nImports; ++oldIdx) {
                if (removedImports.contains(oldIdx))
                    continue;

                if (changedImports.contains(oldIdx)) {
                    const Change *change = changedImports[oldIdx];
                    const ImportHunk &id = std::get<ImportHunk>(change->data);
                    applyImport(newImports[newIdx], id);
                } else {
                    // Copy from old - indices remain the same (string table is position-rebuilt)
                    const Import *oldImport = oldQml->importAt(oldIdx);
                    Import &imp = newImports[newIdx];
                    std::memcpy(&imp, oldImport, sizeof(Import));
                }
                ++newIdx;
            }
            // Add new imports
            for (const Change *change : addedImports) {
                const ImportHunk &id = std::get<ImportHunk>(change->data);
                applyImport(newImports[newIdx], id);
                ++newIdx;
            }
        }

        // Write object offsets and data
        quint32_le *objectOffsets = reinterpret_cast<quint32_le *>(
                newQmlUnitData.data() + qmlUnitHeaderSize + importsSize);
        quint32 currentOffset =
                static_cast<quint32>(qmlUnitHeaderSize + importsSize + objectOffsetsSize);

        for (qsizetype i = 0; i < newObjects.size(); ++i) {
            objectOffsets[i] = currentOffset;
            std::memcpy(newQmlUnitData.data() + currentOffset, newObjects[i].constData(),
                        newObjects[i].size());
            currentOffset += static_cast<quint32>(newObjects[i].size());
        }

        return rebuildUnitInPlace(newQmlUnitData);
    }

    QByteArray buildNewObject(const ObjectHunk &objData)
    {
        // Build an Object binary from ObjectData (from ObjectAdded)
        // The object's string indices need to be looked up in the patched string table

        int objSize = Object::calculateSizeExcludingSignalsAndEnums(
                objData.data.nFunctions, objData.data.nProperties, objData.data.nAliases,
                objData.data.nEnums, objData.data.nSignals, objData.data.nBindings,
                objData.data.nNamedObjectsInComponent, objData.data.nInlineComponents,
                objData.data.nRequiredPropertyExtraData);

        QByteArray result(objSize, '\0');
        Object *newObj = reinterpret_cast<Object *>(result.data());

        *newObj = objData.data;

        // Calculate offsets within the object
        quint32 currentOffset = sizeof(Object);

        newObj->offsetToFunctions = currentOffset;
        currentOffset += objData.functionIndices.size() * sizeof(quint32_le);

        newObj->offsetToProperties = currentOffset;
        writeEntries<Property>(result.data(), currentOffset, objData.properties);
        currentOffset += objData.data.nProperties * sizeof(Property);

        newObj->offsetToAliases = currentOffset;
        writeEntries<Alias>(result.data(), currentOffset, objData.aliases);
        currentOffset += objData.data.nAliases * sizeof(Alias);

        newObj->offsetToEnums = currentOffset;
        newObj->offsetToSignals = currentOffset;

        newObj->offsetToBindings = currentOffset;
        writeEntries<Binding>(result.data(), currentOffset, objData.bindings);
        currentOffset += objData.data.nBindings * sizeof(Binding);

        newObj->offsetToNamedObjectsInComponent = currentOffset;
        writeEntries<quint32_le>(result.data(), currentOffset,
                                 objData.namedObjectsInComponentIndices);
        currentOffset += objData.data.nNamedObjectsInComponent * sizeof(quint32_le);

        newObj->offsetToInlineComponents = currentOffset;
        newObj->offsetToRequiredPropertyExtraData = currentOffset;

        return result;
    }

    BindingHunk extractDataFromOld(const Binding *b) const
    {
        BindingHunk bd = *b;
        bd.propertyNameIndex = remapStringIndex(b->propertyNameIndex);
        bd.stringIndex = remapStringIndex(b->stringIndex);
        return bd;
    }

    PropertyHunk extractDataFromOld(const Property *p) const
    {
        PropertyHunk pd = *p;
        pd.setNameIndex(remapStringIndex(p->nameIndex()));
        if (!p->isCommonType())
            pd.setTypeNameIndex(remapStringIndex(p->commonTypeOrTypeNameIndex()));
        return pd;
    }

    AliasHunk extractDataFromOld(const Alias *a) const
    {
        AliasHunk ad = *a;
        ad.setNameIndex(remapStringIndex(a->nameIndex()));
        ad.setIdIndex(remapStringIndex(a->idIndex()));
        ad.setPropertyNameIndex(remapStringIndex(a->propertyNameIndex()));
        return ad;
    }

    EnumHunk extractDataFromOld(Object::EnumIterator e) const
    {
        EnumHunk data;
        data.data = *e;
        data.data.nameIndex = remapStringIndex(e->nameIndex);
        for (const EnumValue *v = e->enumValuesBegin(); v != e->enumValuesEnd(); ++v) {
            EnumValue ev = *v;
            ev.nameIndex = remapStringIndex(v->nameIndex);
            data.values.append(ev);
        }
        return data;
    }

    SignalHunk extractDataFromOld(Object::SignalIterator s) const
    {
        SignalHunk data;
        data.data = *s;
        data.data.nameIndex = remapStringIndex(s->nameIndex);
        for (quint32 i = 0; i < s->nParameters; ++i)
            data.parameters.append(remapParameter(*s->parameterAt(i)));
        return data;
    }

    InlineComponentHunk extractDataFromOld(Object::InlineComponentIterator ic) const
    {
        InlineComponentHunk data;
        data.objectIndex = ic->objectIndex;
        data.nameIndex = remapStringIndex(ic->nameIndex);
        data.location = ic->location;
        return data;
    }

    RequiredPropertyExtraDataHunk
    extractDataFromOld(Object::RequiredPropertyExtraDataIterator rped) const
    {
        RequiredPropertyExtraDataHunk data;
        data.nameIndex = remapStringIndex(rped->nameIndex);
        return data;
    }

    template <typename Hunk, typename Iterator>
    QList<Hunk> rebuildObjectAttributes(ChangeSet<Hunk> &&cs, Iterator oldBegin,
                                        Iterator oldEnd) const
    {
        QList<Hunk> result;

        quint32 newIdx = 0;
        Iterator oldItem = oldBegin;

        while (oldItem != oldEnd || !cs.insertions.isEmpty()) {
            while (cs.insertions.contains(newIdx)) {
                result.append(cs.insertions.take(newIdx));
                ++newIdx;
            }

            if (oldItem != oldEnd) {
                const int oldIndex = oldItem - oldBegin;
                if (!cs.removals.contains(oldIndex)) {
                    const auto it = cs.replacements.find(oldIndex);
                    result.append(it == cs.replacements.end() ? extractDataFromOld(oldItem) : *it);
                    ++newIdx;
                }
                ++oldItem;
            }
        }

        for (auto it = cs.insertions.begin(); it != cs.insertions.end(); ++it)
            result.append(it.value());

        return result;
    }

    QByteArray rebuildObject(quint32 objIdx, const Object *oldObj)
    {
        ChangeSet<BindingHunk> bindings;
        ChangeSet<PropertyHunk> properties;
        ChangeSet<AliasHunk> aliases;
        ChangeSet<RequiredPropertyExtraDataHunk> rpeds;
        ChangeSet<EnumHunk> enums;
        ChangeSet<SignalHunk> signalChanges;
        ChangeSet<InlineComponentHunk> ics;

        const QList<const Change *> &changes = m_changesByObject.value(objIdx);
        for (const Change *c : changes) {
            switch (c->type) {
            case ChangeType::BindingAdded:
            case ChangeType::BindingChanged:
            case ChangeType::BindingLocationChanged:
            case ChangeType::BindingRemoved:
                routeChange(bindings, *c, ChangeType::BindingAdded, ChangeType::BindingRemoved);
                break;
            case ChangeType::PropertyAdded:
            case ChangeType::PropertyChanged:
            case ChangeType::PropertyLocationChanged:
            case ChangeType::PropertyRemoved:
                routeChange(properties, *c, ChangeType::PropertyAdded,
                            ChangeType::PropertyRemoved);
                break;
            case ChangeType::AliasAdded:
            case ChangeType::AliasChanged:
            case ChangeType::AliasLocationChanged:
            case ChangeType::AliasRemoved:
                routeChange(aliases, *c, ChangeType::AliasAdded, ChangeType::AliasRemoved);
                break;
            case ChangeType::RequiredPropertyExtraDataAdded:
            case ChangeType::RequiredPropertyExtraDataChanged:
            case ChangeType::RequiredPropertyExtraDataRemoved:
                routeChange(rpeds, *c, ChangeType::RequiredPropertyExtraDataAdded,
                            ChangeType::RequiredPropertyExtraDataRemoved);
                break;
            case ChangeType::EnumAdded:
            case ChangeType::EnumChanged:
            case ChangeType::EnumLocationChanged:
            case ChangeType::EnumRemoved:
                routeChange(enums, *c, ChangeType::EnumAdded, ChangeType::EnumRemoved);
                break;
            case ChangeType::SignalAdded:
            case ChangeType::SignalChanged:
            case ChangeType::SignalLocationChanged:
            case ChangeType::SignalRemoved:
                routeChange(signalChanges, *c, ChangeType::SignalAdded, ChangeType::SignalRemoved);
                break;
            case ChangeType::InlineComponentAdded:
            case ChangeType::InlineComponentChanged:
            case ChangeType::InlineComponentLocationChanged:
            case ChangeType::InlineComponentRemoved:
                routeChange(ics, *c, ChangeType::InlineComponentAdded,
                            ChangeType::InlineComponentRemoved);
                break;
            default:
                break;
            }
        }

        const auto newBindings = rebuildObjectAttributes<BindingHunk>(
                std::move(bindings), oldObj->bindingsBegin(), oldObj->bindingsEnd());
        const auto newProperties = rebuildObjectAttributes<PropertyHunk>(
                std::move(properties), oldObj->propertiesBegin(), oldObj->propertiesEnd());
        const auto newAliases = rebuildObjectAttributes<AliasHunk>(
                std::move(aliases), oldObj->aliasesBegin(), oldObj->aliasesEnd());
        const auto newEnums = rebuildObjectAttributes<EnumHunk>(
                std::move(enums), oldObj->enumsBegin(), oldObj->enumsEnd());
        const auto newSignals = rebuildObjectAttributes<SignalHunk>(
                std::move(signalChanges), oldObj->signalsBegin(), oldObj->signalsEnd());
        const auto newInlineComponents = rebuildObjectAttributes<InlineComponentHunk>(
                std::move(ics), oldObj->inlineComponentsBegin(), oldObj->inlineComponentsEnd());
        const auto newRequiredPropExtraData =
                rebuildObjectAttributes<RequiredPropertyExtraDataHunk>(
                        std::move(rpeds), oldObj->requiredPropertyExtraDataBegin(),
                        oldObj->requiredPropertyExtraDataEnd());

        // Check for ObjectChanged to get updated nNamedObjectsInComponent and other fields
        quint32 nNamedObjects = oldObj->nNamedObjectsInComponent;
        std::optional<QList<quint32_le>> newNamedObjects;
        quint32 newNFunctions = oldObj->nFunctions;
        QList<quint32_le> newFunctionIndices;
        // Initialize from old object
        if (oldObj->nFunctions > 0) {
            const quint32_le *oldFuncs = oldObj->functionOffsetTable();
            for (quint32 i = 0; i < oldObj->nFunctions; ++i)
                newFunctionIndices.append(oldFuncs[i]);
        }

        // ObjectChanged fields (initialized from old object, updated from change if present)
        quint32 newInheritedTypeNameIndex = remapStringIndex(oldObj->inheritedTypeNameIndex);
        quint32 newIdNameIndex = remapStringIndex(oldObj->idNameIndex);
        Location newLocation = oldObj->location;
        Location newLocationOfIdProperty = oldObj->locationOfIdProperty;
        qint32 newIndexOfDefaultPropertyOrAlias = oldObj->indexOfDefaultPropertyOrAlias;
        Object::Flags newFlags = oldObj->flags();
        bool newHasAliasAsDefaultProperty = oldObj->hasAliasAsDefaultProperty();
        qint32 newObjectId = oldObj->objectId();

        for (const Change &change : m_diff.changes) {
            if (change.index != objIdx)
                continue;

            if (change.type == ChangeType::ObjectChanged) {
                const ObjectHunk &newObject = std::get<ObjectHunk>(change.data);
                nNamedObjects = newObject.namedObjectsInComponentIndices.size();
                newNamedObjects = newObject.namedObjectsInComponentIndices;
                newInheritedTypeNameIndex = newObject.data.inheritedTypeNameIndex;
                newIdNameIndex = newObject.data.idNameIndex;
                newIndexOfDefaultPropertyOrAlias = newObject.data.indexOfDefaultPropertyOrAlias;
                newFlags = newObject.data.flags();
                newHasAliasAsDefaultProperty = newObject.data.hasAliasAsDefaultProperty();
                newObjectId = newObject.data.objectId();
                newNFunctions = newObject.functionIndices.size();
                newFunctionIndices = newObject.functionIndices;
                newLocation = newObject.data.location;
                newLocationOfIdProperty = newObject.data.locationOfIdProperty;
            } else if (change.type == ChangeType::ObjectLocationChanged) {
                const ObjectHunk &newObject = std::get<ObjectHunk>(change.data);
                newLocation = newObject.data.location;
                newLocationOfIdProperty = newObject.data.locationOfIdProperty;
            }
        }

        // Calculate size for each enum (variable-sized)
        qsizetype totalEnumsDataSize = 0;
        for (const EnumHunk &ed : newEnums)
            totalEnumsDataSize += Enum::calculateSize(ed.values.size());
        qsizetype enumsOffsetTableSize =
                static_cast<qsizetype>(newEnums.size()) * sizeof(quint32_le);

        // Calculate size for each signal (variable-sized)
        qsizetype totalSignalsDataSize = 0;
        for (const SignalHunk &sd : newSignals)
            totalSignalsDataSize += Signal::calculateSize(sd.parameters.size());
        qsizetype signalsOffsetTableSize =
                static_cast<qsizetype>(newSignals.size()) * sizeof(quint32_le);

        // Calculate size needed for fixed-size tables
        qsizetype baseSize = sizeof(Object);
        qsizetype functionsSize = static_cast<qsizetype>(newNFunctions) * sizeof(quint32_le);
        qsizetype propertiesSize = static_cast<qsizetype>(newProperties.size()) * sizeof(Property);
        qsizetype aliasesSize = static_cast<qsizetype>(newAliases.size()) * sizeof(Alias);
        qsizetype bindingsSize = static_cast<qsizetype>(newBindings.size()) * sizeof(Binding);
        qsizetype namedObjectsSize = static_cast<qsizetype>(nNamedObjects) * sizeof(quint32_le);
        qsizetype inlineComponentsSize =
                static_cast<qsizetype>(newInlineComponents.size()) * sizeof(InlineComponent);
        qsizetype requiredPropertyExtraDataSize =
                static_cast<qsizetype>(newRequiredPropExtraData.size())
                * sizeof(RequiredPropertyExtraData);

        qsizetype totalSize = baseSize + functionsSize + propertiesSize + aliasesSize
                + enumsOffsetTableSize + signalsOffsetTableSize + bindingsSize + namedObjectsSize
                + inlineComponentsSize + requiredPropertyExtraDataSize + totalEnumsDataSize
                + totalSignalsDataSize;

        // Align to 8 bytes (Qt requires this for Object alignment)
        totalSize = (totalSize + 7) & ~7;

        QByteArray result(totalSize, '\0');
        Object *newObj = reinterpret_cast<Object *>(result.data());

        newObj->inheritedTypeNameIndex = newInheritedTypeNameIndex;
        newObj->idNameIndex = newIdNameIndex;

        newObj->indexOfDefaultPropertyOrAlias = newIndexOfDefaultPropertyOrAlias;
        // Set the flags/id using proper accessors
        newObj->setFlags(newFlags);
        newObj->setHasAliasAsDefaultProperty(newHasAliasAsDefaultProperty);
        newObj->setObjectId(newObjectId);
        newObj->nFunctions = newNFunctions;
        newObj->nProperties = static_cast<quint32>(newProperties.size());
        newObj->nAliases = static_cast<quint32>(newAliases.size());
        newObj->nEnums = static_cast<quint16>(newEnums.size());
        newObj->nSignals = static_cast<quint16>(newSignals.size());
        newObj->nBindings = static_cast<quint32>(newBindings.size());
        newObj->nNamedObjectsInComponent = nNamedObjects;
        newObj->nInlineComponents = static_cast<quint16>(newInlineComponents.size());
        newObj->nRequiredPropertyExtraData = static_cast<quint16>(newRequiredPropExtraData.size());
        newObj->location = newLocation;
        newObj->locationOfIdProperty = newLocationOfIdProperty;

        quint32 currentOffset = static_cast<quint32>(baseSize);

        newObj->offsetToFunctions = currentOffset;
        currentOffset += static_cast<quint32>(functionsSize);

        newObj->offsetToProperties = currentOffset;
        currentOffset += static_cast<quint32>(propertiesSize);

        newObj->offsetToAliases = currentOffset;
        currentOffset += static_cast<quint32>(aliasesSize);

        newObj->offsetToEnums = currentOffset;
        currentOffset += static_cast<quint32>(enumsOffsetTableSize);

        newObj->offsetToSignals = currentOffset;
        currentOffset += static_cast<quint32>(signalsOffsetTableSize);

        newObj->offsetToBindings = currentOffset;
        currentOffset += static_cast<quint32>(bindingsSize);

        newObj->offsetToNamedObjectsInComponent = currentOffset;
        currentOffset += static_cast<quint32>(namedObjectsSize);

        newObj->offsetToInlineComponents = currentOffset;
        currentOffset += static_cast<quint32>(inlineComponentsSize);

        newObj->offsetToRequiredPropertyExtraData = currentOffset;
        currentOffset += static_cast<quint32>(requiredPropertyExtraDataSize);

        quint32 enumDataStartOffset = currentOffset;
        currentOffset += static_cast<quint32>(totalEnumsDataSize);

        quint32 signalDataStartOffset = currentOffset;
        currentOffset += static_cast<quint32>(totalSignalsDataSize);

        writeEntries<quint32_le>(result.data(), newObj->offsetToFunctions, newFunctionIndices);
        writeEntries<Property>(result.data(), newObj->offsetToProperties, newProperties);
        writeEntries<Alias>(result.data(), newObj->offsetToAliases, newAliases);

        // Write enums (variable-sized with offset table)
        quint32_le *enumOffsetTable =
                reinterpret_cast<quint32_le *>(result.data() + newObj->offsetToEnums);
        char *enumDataPtr = result.data() + enumDataStartOffset;
        for (qsizetype i = 0; i < newEnums.size(); ++i) {
            const EnumHunk &ed = newEnums[i];
            enumOffsetTable[i] = static_cast<quint32>(enumDataPtr - result.data());
            Enum *e = reinterpret_cast<Enum *>(enumDataPtr);
            *e = ed.data;
            EnumValue *values = reinterpret_cast<EnumValue *>(e + 1);
            for (qsizetype j = 0; j < ed.values.size(); ++j)
                values[j] = ed.values[j];
            enumDataPtr += Enum::calculateSize(ed.values.size());
        }

        // Write signals (variable-sized with offset table)
        quint32_le *signalOffsetTable =
                reinterpret_cast<quint32_le *>(result.data() + newObj->offsetToSignals);
        char *signalDataPtr = result.data() + signalDataStartOffset;
        for (qsizetype i = 0; i < newSignals.size(); ++i) {
            const SignalHunk &sd = newSignals[i];
            signalOffsetTable[i] = static_cast<quint32>(signalDataPtr - result.data());
            Signal *s = reinterpret_cast<Signal *>(signalDataPtr);
            *s = sd.data;
            Parameter *params = reinterpret_cast<Parameter *>(s + 1);
            for (qsizetype j = 0; j < sd.parameters.size(); ++j)
                params[j] = sd.parameters[j];
            signalDataPtr += Signal::calculateSize(sd.parameters.size());
        }

        writeEntries<Binding>(result.data(), newObj->offsetToBindings, newBindings);

        quint32_le *namedObjectsTable = reinterpret_cast<quint32_le *>(
                result.data() + newObj->offsetToNamedObjectsInComponent);
        if (newNamedObjects.has_value()) {
            writeEntries<quint32_le>(result.data(), newObj->offsetToNamedObjectsInComponent,
                                     *newNamedObjects);
        } else {
            const quint32_le *oldNamedObjs = oldObj->namedObjectsInComponentTable();
            std::memcpy(namedObjectsTable, oldNamedObjs, namedObjectsSize);
        }

        writeEntries<InlineComponent>(result.data(), newObj->offsetToInlineComponents,
                                      newInlineComponents);
        writeEntries<RequiredPropertyExtraData>(
                result.data(), newObj->offsetToRequiredPropertyExtraData, newRequiredPropExtraData);

        return result;
    }

    bool getOrAddConstantForRebuild(double value, quint32 *outIndex)
    {
        // Check if constant already exists in the patched unit (may have been rebuilt)
        const Unit *patchedUnit = reinterpret_cast<const Unit *>(m_patchedData.constData());
        const StaticValue *constants =
                reinterpret_cast<const StaticValue *>(patchedUnit->constants());
        for (quint32 i = 0; i < patchedUnit->constantTableSize; ++i) {
            double existingValue = constants[i].doubleValue();

            if (qFuzzyCompare(existingValue, value) || (qIsNaN(existingValue) && qIsNaN(value))) {
                *outIndex = i;
                return true;
            }
        }

        // Constant not found - this should not happen if addConstantIfNeeded() was complete
        m_errorMessage = QStringLiteral("Constant not found in table: %1").arg(value);
        return false;
    }

    bool rebuildUnitInPlace(const QByteArray &newQmlUnitData)
    {
        // The old unit layout stays mostly the same, just with the new QmlUnit section

        // Use the patched unit's offset (may have been shifted by rebuildStringTable)
        const Unit *patchedUnit = reinterpret_cast<const Unit *>(m_patchedData.constData());
        quint64 qmlUnitOffset = patchedUnit->offsetToQmlUnit;
        quint64 oldQmlUnitSize = patchedUnit->unitSize - qmlUnitOffset;

        // If the new QmlUnit is the same size, we can replace in-place
        if (static_cast<quint64>(newQmlUnitData.size()) == oldQmlUnitSize) {
            std::memcpy(m_patchedData.data() + qmlUnitOffset, newQmlUnitData.constData(),
                        newQmlUnitData.size());
            return true;
        }

        // Different size - need to rebuild the entire unit
        qint64 sizeDiff = newQmlUnitData.size() - static_cast<qint64>(oldQmlUnitSize);
        qint64 newUnitSize = static_cast<qint64>(patchedUnit->unitSize) + sizeDiff;

        QByteArray newUnitData;
        newUnitData.resize(newUnitSize);

        // Copy everything before QmlUnit
        std::memcpy(newUnitData.data(), m_patchedData.constData(), qmlUnitOffset);

        // Copy the new QmlUnit
        std::memcpy(newUnitData.data() + qmlUnitOffset, newQmlUnitData.constData(),
                    newQmlUnitData.size());

        // Update the Unit header with new size
        Unit *newUnit = reinterpret_cast<Unit *>(newUnitData.data());
        newUnit->unitSize = static_cast<quint32>(newUnitSize);

        m_patchedData = newUnitData;
        return true;
    }

    bool rebuildStringTable()
    {
        const Unit *oldUnit = m_oldUnit;

        QList<QString> allStrings = mergeEntriesOldKeyed<QString>(
                m_strings, oldUnit->stringTableSize,
                [&](quint32 i) { return oldUnit->stringAtInternal(i); },
                [](const QString &s) { return s; });

        // Build content-based string index remap: for each old string index,
        // find where the same string content ended up in the new table.
        // The positional mapping (m_oldToNewStringIndex built above) is wrong
        // when strings are replaced (StringChanged swaps content at an index),
        // so we need to find by content instead.
        {
            QHash<QString, quint32> newStringToIndex;
            for (qsizetype i = 0; i < allStrings.size(); ++i)
                newStringToIndex.insert(allStrings[i], static_cast<quint32>(i));

            m_oldToNewStringIndex.clear();
            for (quint32 i = 0; i < oldUnit->stringTableSize; ++i) {
                const QString oldStr = oldUnit->stringAtInternal(i);
                auto it = newStringToIndex.constFind(oldStr);
                if (it != newStringToIndex.constEnd())
                    m_oldToNewStringIndex[i] = it.value();
                // If the old string was removed entirely, there's no mapping;
                // remapStringIndex() will return the old index as-is (shouldn't
                // be referenced by any surviving element).
            }
        }

        quint32 newStringTableSize = static_cast<quint32>(allStrings.size());

        // Calculate sizes
        // Offset table: newStringTableSize * sizeof(quint32_le), rounded up to 8 bytes
        qsizetype rawOffsetTableSize =
                static_cast<qsizetype>(newStringTableSize) * sizeof(quint32_le);
        qsizetype offsetTableSize = (rawOffsetTableSize + 7) & ~7; // Round up to 8-byte alignment

        // Calculate string data size
        qsizetype stringDataSize = 0;
        for (const QString &str : std::as_const(allStrings))
            stringDataSize += CompiledData::String::calculateSize(str);

        qsizetype newStringTableTotalSize = offsetTableSize + stringDataSize;
        qsizetype oldStringTableTotalSize = calculateOldStringTableSize();
        qsizetype stringTableDelta = newStringTableTotalSize - oldStringTableTotalSize;

        // Create new unit with adjusted size, zero-initialized
        qsizetype newUnitSize = oldUnit->unitSize + stringTableDelta;
        QByteArray newData(newUnitSize, '\0');

        // Copy header (everything before string table)
        qsizetype headerSize = oldUnit->offsetToStringTable;
        std::memcpy(newData.data(), m_patchedData.constData(), headerSize);

        // Build new string table
        char *stringTableStart = newData.data() + headerSize;
        quint32_le *offsetTablePtr = reinterpret_cast<quint32_le *>(stringTableStart);
        char *stringData = stringTableStart + offsetTableSize;

        quint32 currentStringOffset = static_cast<quint32>(
                headerSize + offsetTableSize); // Absolute offset from unit start

        for (qsizetype i = 0; i < allStrings.size(); ++i) {
            const QString &str = allStrings[i];

            // Write offset (absolute from unit start)
            offsetTablePtr[i] = currentStringOffset;

            // Write string data
            CompiledData::String *s = reinterpret_cast<CompiledData::String *>(stringData);
            s->size = str.size();

            // Write UTF-16 data (little-endian)
            quint16 *uc = reinterpret_cast<quint16 *>(stringData + sizeof(CompiledData::String));
            for (int j = 0; j < str.size(); ++j) {
                uc[j] = str.at(j).unicode();
            }
            uc[str.size()] = 0; // Null terminator

            qsizetype strEntrySize = CompiledData::String::calculateSize(str);
            stringData += strEntrySize;
            currentStringOffset += static_cast<quint32>(strEntrySize);
        }

        // Copy everything after the old string table
        qsizetype afterOldStringTable = oldUnit->offsetToStringTable + oldStringTableTotalSize;
        qsizetype afterNewStringTable = headerSize + newStringTableTotalSize;
        qsizetype remainingSize = oldUnit->unitSize - afterOldStringTable;

        if (remainingSize > 0) {
            std::memcpy(newData.data() + afterNewStringTable,
                        m_patchedData.constData() + afterOldStringTable, remainingSize);
        }

        // Update Unit header
        Unit *newUnit = reinterpret_cast<Unit *>(newData.data());
        newUnit->stringTableSize = newStringTableSize;
        newUnit->unitSize = static_cast<quint32>(newUnitSize);

        // Update all offsets that were after the string table
        if (stringTableDelta != 0) {
            auto shift = [&](quint32_le &offset) {
                if (static_cast<qsizetype>(offset)
                    > static_cast<qsizetype>(oldUnit->offsetToStringTable))
                    offset = offset + static_cast<qint32>(stringTableDelta);
            };
            shiftAllHeaderOffsets(newUnit, shift);
        }

        m_patchedData = newData;
        return true;
    }

    bool rebuildConstantTableWithAdditions()
    {
        const Unit *oldUnit = reinterpret_cast<const Unit *>(m_patchedData.constData());

        const quint64 *oldConstants = oldUnit->constantTableSize > 0
                ? reinterpret_cast<const quint64 *>(reinterpret_cast<const char *>(oldUnit)
                                                    + oldUnit->offsetToConstantTable)
                : nullptr;

        QList<quint64> allConstants = mergeEntriesOldKeyed<quint64>(
                m_constants, oldUnit->constantTableSize, [&](quint32 i) { return oldConstants[i]; },
                [](quint64 v) { return v; });

        spliceFlatTable<quint64>(allConstants, oldUnit->offsetToConstantTable,
                                 oldUnit->constantTableSize, offsetof(Unit, constantTableSize),
                                 offsetof(Unit, offsetToConstantTable));

        m_constantTableRebuilt = true;
        m_constants.clear();
        return true;
    }

    bool rebuildTranslationTable()
    {
        if (m_translations.isEmpty())
            return true;

        const Unit *oldUnit = reinterpret_cast<const Unit *>(m_patchedData.constData());

        const TranslationData *oldEntries =
                oldUnit->translationTableSize > 0 ? oldUnit->translations() : nullptr;

        QList<TranslationData> allEntries = mergeEntriesOldKeyed<TranslationData>(
                m_translations, oldUnit->translationTableSize,
                [&](quint32 i) { return oldEntries[i]; }, toTranslationData);

        qsizetype oldTableSize = oldUnit->translationTableSize * sizeof(TranslationData);
        qsizetype newTableSize = allEntries.size() * sizeof(TranslationData);

        // When the source unit has no translation table entries, the offset is either
        // a valid compiler-allocated slot (if non-zero) or absent (zero). Use the
        // existing non-zero offset if present, otherwise fall back to the target layout.
        qsizetype oldTableStart;
        if (oldUnit->translationTableSize == 0) {
            if (static_cast<quint32>(oldUnit->offsetToTranslationTable) != 0) {
                oldTableStart = oldUnit->offsetToTranslationTable;
            } else if (m_unitMetadata.has_value()
                       && m_unitMetadata.value().offsetToTranslationTable != 0) {
                oldTableStart = m_unitMetadata.value().offsetToTranslationTable;
            } else {
                return true; // No valid position to insert translation entries
            }
        } else {
            oldTableStart = oldUnit->offsetToTranslationTable;
        }

        // The compiler appends a 4-byte translation context ID and aligns to 8 bytes
        // after the translation entries whenever there is at least one entry.
        auto trailingBytes = [oldTableStart](qsizetype entryCount) -> qsizetype {
            if (entryCount == 0)
                return 0;
            qsizetype entriesEnd = oldTableStart + entryCount * sizeof(TranslationData);
            qsizetype withContextId = entriesEnd + sizeof(quint32_le);
            qsizetype aligned = (withContextId + 7) & ~7;
            return aligned - entriesEnd;
        };

        qsizetype oldTrailingBytes = trailingBytes(oldUnit->translationTableSize);
        qsizetype newTrailingBytes = trailingBytes(allEntries.size());
        qsizetype oldTableEnd = oldTableStart + oldTableSize + oldTrailingBytes;
        qsizetype delta = (newTableSize + newTrailingBytes) - (oldTableSize + oldTrailingBytes);

        qsizetype newUnitSize = oldUnit->unitSize + delta;
        QByteArray newData(newUnitSize, '\0');

        std::memcpy(newData.data(), m_patchedData.constData(), oldTableStart);

        TranslationData *dest = reinterpret_cast<TranslationData *>(newData.data() + oldTableStart);
        for (qsizetype i = 0; i < allEntries.size(); ++i)
            dest[i] = allEntries[i];
        // Trailing bytes (context ID + padding) are left as zeros in newData.

        qsizetype newTableEnd = oldTableStart + newTableSize + newTrailingBytes;
        qsizetype remainingSize = oldUnit->unitSize - oldTableEnd;
        if (remainingSize > 0)
            std::memcpy(newData.data() + newTableEnd, m_patchedData.constData() + oldTableEnd,
                        remainingSize);

        Unit *newUnit = reinterpret_cast<Unit *>(newData.data());
        newUnit->unitSize = static_cast<quint32>(newUnitSize);
        newUnit->translationTableSize = static_cast<quint32>(allEntries.size());
        // When the source had no translation table, we inserted at the target position.
        // Update the header to reflect the new location. In all other cases the position
        // is left unchanged for applyTargetLayout to fix up.
        if (oldUnit->translationTableSize == 0) {
            newUnit->offsetToTranslationTable = static_cast<quint32>(oldTableStart);
        }

        quint32 savedTranslationOffset = newUnit->offsetToTranslationTable;
        shiftAllOffsets(newData, oldTableEnd, delta);
        newUnit->offsetToTranslationTable = savedTranslationOffset;

        m_patchedData = newData;
        m_translations.clear();
        return true;
    }

    bool rebuildLookupTable()
    {
        const Unit *oldUnit = reinterpret_cast<const Unit *>(m_patchedData.constData());

        QList<Lookup> allLookups =
                mergeSimpleEntries<Lookup>(m_lookups, oldUnit->lookupTableSize,
                                           [&](quint32 i) { return oldUnit->lookupTable()[i]; });

        spliceFlatTable<Lookup>(allLookups, oldUnit->offsetToLookupTable, oldUnit->lookupTableSize,
                                offsetof(Unit, lookupTableSize),
                                offsetof(Unit, offsetToLookupTable));

        m_lookups.clear();
        return true;
    }

    bool rebuildRegExpTable()
    {
        if (m_regexps.isEmpty())
            return true;

        const Unit *oldUnit = reinterpret_cast<const Unit *>(m_patchedData.constData());

        QList<RegExp> allRegExps =
                mergeSimpleEntries<RegExp>(m_regexps, oldUnit->regexpTableSize, [&](quint32 i) {
                    RegExp re = *oldUnit->regexpAt(i);
                    quint32 newStrIdx = remapStringIndex(re.stringIndex());
                    return RegExp(re.flags(), newStrIdx);
                });

        spliceFlatTable<RegExp>(allRegExps, oldUnit->offsetToRegexpTable, oldUnit->regexpTableSize,
                                offsetof(Unit, regexpTableSize),
                                offsetof(Unit, offsetToRegexpTable));

        m_regexps.clear();
        return true;
    }

    // Generic helper for all indirect (offset-table + variable-sized data) table rebuilds.
    // SizeCalc:    (const EntryType *) → int  — binary entry size
    // ExtractOldFn: (quint32 idx)       → std::optional<Hunk>
    // SerializeFn:  (const Hunk &)      → QByteArray
    template <typename EntryType, typename Hunk, typename SizeCalc, typename ExtractOldFn,
              typename SerializeFn>
    bool rebuildIndirectTableGeneric(ChangeSet<Hunk> &changes, quint32 tableOffset,
                                     quint32 tableSize, const SizeCalc &calcSize,
                                     ExtractOldFn &&extractOld, SerializeFn &&serialize,
                                     IndirectTable tableEnum, size_t countFieldOff,
                                     size_t offsetFieldOff)
    {
        if (changes.isEmpty())
            return true;

        if (!validateIndirectTable<EntryType>(tableOffset, tableSize, calcSize))
            return false;

        QVector<Hunk> finalEntries = mergeIndirectEntries<Hunk>(
                changes, tableSize, std::forward<ExtractOldFn>(extractOld));

        QByteArray entryData;
        QVector<qsizetype> entrySizes;
        for (const Hunk &h : std::as_const(finalEntries)) {
            QByteArray buf = serialize(h);
            entrySizes.append(buf.size());
            entryData.append(buf);
        }

        const Unit *oldUnit = reinterpret_cast<const Unit *>(m_patchedData.constData());
        quint32 dataStart, dataEnd;
        if (!findDataRegionBounds<EntryType>(m_patchedData, tableOffset, tableSize, calcSize,
                                             dataStart, dataEnd)) {
            dataStart = dataEnd = oldUnit->offsetToStringTable;
        }

        if (!spliceAndShiftIndirectTable(tableOffset, tableSize * sizeof(quint32_le), dataStart,
                                         dataEnd, static_cast<quint32>(finalEntries.size()),
                                         entryData, entrySizes, tableEnum, countFieldOff,
                                         offsetFieldOff))
            return false;

        changes.clear();
        return true;
    }

    bool rebuildClassTable()
    {
        const Unit *u = reinterpret_cast<const Unit *>(m_patchedData.constData());
        return rebuildIndirectTableGeneric<Class>(
                m_classes, u->offsetToClassTable, u->classTableSize,
                [](const Class *c) { return Class::calculateSize(c->nStaticMethods, c->nMethods); },
                [&](quint32 i) -> std::optional<ClassHunk> {
                    const Class *old = u->classAt(i);
                    ClassHunk cd;
                    cd.data = *old;
                    cd.data.nameIndex = remapStringIndex(old->nameIndex);
                    quint32 total = old->nStaticMethods + old->nMethods;
                    const Method *m = old->methodTable();
                    for (quint32 j = 0; j < total; ++j) {
                        Method me = m[j];
                        me.name = remapStringIndex(me.name);
                        cd.methods.append(me);
                    }
                    return cd;
                },
                [](const ClassHunk &cd) -> QByteArray {
                    quint32 total = cd.data.nStaticMethods + cd.data.nMethods;
                    QByteArray buf(Class::calculateSize(cd.data.nStaticMethods, cd.data.nMethods),
                                   '\0');
                    Class *cls = reinterpret_cast<Class *>(buf.data());
                    *cls = cd.data;
                    cls->methodTableOffset = sizeof(Class);
                    Method *out = reinterpret_cast<Method *>(buf.data() + sizeof(Class));
                    for (quint32 j = 0; j < total && j < quint32(cd.methods.size()); ++j)
                        out[j] = cd.methods[j];
                    return buf;
                },
                IndirectTable::Class, offsetof(Unit, classTableSize),
                offsetof(Unit, offsetToClassTable));
    }

    bool rebuildTemplateObjectTable()
    {
        const Unit *u = reinterpret_cast<const Unit *>(m_patchedData.constData());
        return rebuildIndirectTableGeneric<TemplateObject>(
                m_templateObjects, u->offsetToTemplateObjectTable, u->templateObjectTableSize,
                [](const TemplateObject *t) { return TemplateObject::calculateSize(t->size); },
                [&](quint32 i) -> std::optional<TemplateObjectHunk> {
                    const TemplateObject *old = u->templateObjectAt(i);
                    TemplateObjectHunk td;
                    td.size = old->size;
                    const quint32_le *st = old->stringTable();
                    for (quint32 j = 0; j < 2 * old->size; ++j) {
                        quint32_le val;
                        val = remapStringIndex(st[j]);
                        td.strings.append(val);
                    }
                    return td;
                },
                [](const TemplateObjectHunk &td) -> QByteArray {
                    QByteArray buf(TemplateObject::calculateSize(td.size), '\0');
                    TemplateObject *to = reinterpret_cast<TemplateObject *>(buf.data());
                    to->size = td.size;
                    quint32_le *out =
                            reinterpret_cast<quint32_le *>(buf.data() + sizeof(TemplateObject));
                    for (quint32 j = 0; j < 2 * td.size && j < quint32(td.strings.size()); ++j)
                        out[j] = td.strings[j];
                    return buf;
                },
                IndirectTable::Template, offsetof(Unit, templateObjectTableSize),
                offsetof(Unit, offsetToTemplateObjectTable));
    }

    bool rebuildJsClassTable()
    {
        const Unit *u = reinterpret_cast<const Unit *>(m_patchedData.constData());
        return rebuildIndirectTableGeneric<JSClass>(
                m_jsClasses, u->offsetToJSClassTable, u->jsClassTableSize,
                [](const JSClass *k) { return JSClass::calculateSize(k->nMembers); },
                [&](quint32 i) -> std::optional<JsClassHunk> {
                    int n = 0;
                    const JSClassMember *old = u->jsClassAt(i, &n);
                    JsClassHunk jd;
                    jd.nMembers = static_cast<quint32>(n);
                    for (int j = 0; j < n; ++j) {
                        JSClassMember m = old[j];
                        m.set(remapStringIndex(m.nameOffset()), m.isAccessor());
                        jd.members.append(m);
                    }
                    return jd;
                },
                [](const JsClassHunk &jd) -> QByteArray {
                    QByteArray buf(JSClass::calculateSize(jd.nMembers), '\0');
                    JSClass *jsc = reinterpret_cast<JSClass *>(buf.data());
                    jsc->nMembers = jd.nMembers;
                    JSClassMember *out =
                            reinterpret_cast<JSClassMember *>(buf.data() + sizeof(JSClass));
                    for (quint32 j = 0; j < jd.nMembers && j < quint32(jd.members.size()); ++j)
                        out[j] = jd.members[j];
                    return buf;
                },
                IndirectTable::JsClass, offsetof(Unit, jsClassTableSize),
                offsetof(Unit, offsetToJSClassTable));
    }

    bool rebuildBlockTable()
    {
        const Unit *u = reinterpret_cast<const Unit *>(m_patchedData.constData());
        return rebuildIndirectTableGeneric<Block>(
                m_blocks, u->offsetToBlockTable, u->blockTableSize,
                [](const Block *b) { return Block::calculateSize(b->nLocals); },
                [&](quint32 i) -> std::optional<BlockHunk> {
                    const Block *old = u->blockAt(i);
                    BlockHunk bd;
                    bd.data = *old;
                    const quint32_le *loc = old->localsTable();
                    for (quint32 j = 0; j < old->nLocals; ++j) {
                        quint32_le val;
                        val = remapStringIndex(loc[j]);
                        bd.locals.append(val);
                    }
                    return bd;
                },
                [](const BlockHunk &bd) -> QByteArray {
                    constexpr quint32 hdr = (sizeof(Block) + 7) & ~7;
                    QByteArray buf(Block::calculateSize(bd.data.nLocals), '\0');
                    Block *blk = reinterpret_cast<Block *>(buf.data());
                    *blk = bd.data;
                    blk->localsOffset = hdr;
                    quint32_le *out = reinterpret_cast<quint32_le *>(buf.data() + hdr);
                    for (quint32 j = 0; j < bd.data.nLocals && j < quint32(bd.locals.size()); ++j)
                        out[j] = bd.locals[j];
                    return buf;
                },
                IndirectTable::Block, offsetof(Unit, blockTableSize),
                offsetof(Unit, offsetToBlockTable));
    }

    bool applyTargetLayout()
    {
        // Reorganize inline tables to match the target layout
        // This removes gaps and positions tables at their target offsets
        //
        // Inline tables are placed right after the Unit header (sizeof(Unit) = 200 bytes):
        // - Function offset table (functionTableSize * 4 bytes)
        // - Lookup table (lookupTableSize * sizeof(Lookup) bytes)
        // - Regexp table (regexpTableSize * sizeof(RegExp) bytes)
        // - Constant table (constantTableSize * sizeof(quint64) bytes)
        // - JSClass table (jsClassTableSize * sizeof(JSClassMember) * entries)
        // - Translation table (translationTableSize * sizeof(TranslationData) bytes)
        //
        // After inline tables: function DATA, then string table, then QmlUnit

        if (!m_unitMetadata.has_value())
            return true;

        const Unit *oldUnit = reinterpret_cast<const Unit *>(m_patchedData.constData());
        const UnitHunk &target = m_unitMetadata.value();

        // Calculate the current inline table region boundaries
        // The inline tables start at sizeof(Unit) = 200
        constexpr quint32 headerSize = sizeof(Unit);

        // Find where inline tables end in the old unit (before variable-size data or string table).
        // The inline tables end at min(offsetToStringTable, first data position of any type).
        quint32 oldInlineTablesEnd = oldUnit->offsetToStringTable;
        auto lowerBound = [&](quint32 offset) {
            if (offset > 0 && offset < oldInlineTablesEnd)
                oldInlineTablesEnd = offset;
        };
        auto firstIndirectEntry = [&](quint32 tableSize, quint32 tableOffset) {
            if (tableSize > 0) {
                const quint32_le *off = reinterpret_cast<const quint32_le *>(
                        m_patchedData.constData() + tableOffset);
                lowerBound(off[0]);
            }
        };
        firstIndirectEntry(oldUnit->functionTableSize, oldUnit->offsetToFunctionTable);
        firstIndirectEntry(oldUnit->classTableSize, oldUnit->offsetToClassTable);
        firstIndirectEntry(oldUnit->templateObjectTableSize, oldUnit->offsetToTemplateObjectTable);
        firstIndirectEntry(oldUnit->blockTableSize, oldUnit->offsetToBlockTable);

        // The target inline tables end will be calculated below based on actual table data

        // Extract inline table data from old unit
        auto extractTable = [&](quint32 size, quint32 offset, size_t elemSize) -> QByteArray {
            if (size == 0)
                return {};
            return QByteArray(m_patchedData.constData() + offset,
                              static_cast<qsizetype>(size) * elemSize);
        };
        QByteArray functionOffsets = extractTable(
                oldUnit->functionTableSize, oldUnit->offsetToFunctionTable, sizeof(quint32_le));
        QByteArray lookupData = extractTable(oldUnit->lookupTableSize, oldUnit->offsetToLookupTable,
                                             sizeof(Lookup));
        QByteArray regexpData = extractTable(oldUnit->regexpTableSize, oldUnit->offsetToRegexpTable,
                                             sizeof(RegExp));
        QByteArray constantData = extractTable(oldUnit->constantTableSize,
                                               oldUnit->offsetToConstantTable, sizeof(quint64));
        QByteArray classOffsetData = extractTable(oldUnit->classTableSize,
                                                  oldUnit->offsetToClassTable, sizeof(quint32_le));
        QByteArray templateObjectOffsetData =
                extractTable(oldUnit->templateObjectTableSize, oldUnit->offsetToTemplateObjectTable,
                             sizeof(quint32_le));
        QByteArray blockOffsetData = extractTable(oldUnit->blockTableSize,
                                                  oldUnit->offsetToBlockTable, sizeof(quint32_le));

        // Extract translation table entries and recompute trailing bytes for the TARGET
        // position. The trailing bytes (context ID slot + alignment) are alignment-dependent,
        // so using the source position for the calculation would yield the wrong size when
        // the source and target table positions differ.
        QByteArray translationData;
        qsizetype translationDataSize = 0;
        if (oldUnit->translationTableSize > 0 && target.offsetToTranslationTable != 0) {
            qsizetype entriesSize =
                    static_cast<qsizetype>(oldUnit->translationTableSize) * sizeof(TranslationData);
            // Compute trailing for TARGET position
            qsizetype targetEntriesEnd = target.offsetToTranslationTable + entriesSize;
            qsizetype targetWithContextId =
                    targetEntriesEnd + static_cast<qsizetype>(sizeof(quint32_le));
            qsizetype targetAligned = (targetWithContextId + 7) & ~7;
            qsizetype targetTrailing = targetAligned - targetEntriesEnd;
            translationDataSize = entriesSize + targetTrailing;
            // Copy only the entries; trailing bytes stay zero (context ID + padding)
            translationData = QByteArray(translationDataSize, '\0');
            std::memcpy(translationData.data(),
                        m_patchedData.constData() + oldUnit->offsetToTranslationTable, entriesSize);
        }

        QByteArray jsClassOffsetData = extractTable(
                oldUnit->jsClassTableSize, oldUnit->offsetToJSClassTable, sizeof(quint32_le));
        QByteArray jsClassEntryData;
        quint32 oldJsClassEntryStart = 0;
        quint32 oldJsClassEntryEnd = 0;
        if (findDataRegionBounds<JSClass>(
                    m_patchedData, oldUnit->offsetToJSClassTable, oldUnit->jsClassTableSize,
                    [](const JSClass *jsc) -> quint32 {
                        return JSClass::calculateSize(jsc->nMembers);
                    },
                    oldJsClassEntryStart, oldJsClassEntryEnd)) {
            jsClassEntryData = QByteArray(m_patchedData.constData() + oldJsClassEntryStart,
                                          oldJsClassEntryEnd - oldJsClassEntryStart);
        }

        // Calculate where the inline tables end in the target layout
        quint32 targetInlineEnd = headerSize;
        struct InlineTableEnd
        {
            quint32 offset;
            quint32 size;
        };
        const InlineTableEnd inlineEnds[] = {
            { target.offsetToFunctionTable,
              static_cast<quint32>(oldUnit->functionTableSize * sizeof(quint32_le)) },
            { target.offsetToLookupTable,
              static_cast<quint32>(oldUnit->lookupTableSize * sizeof(Lookup)) },
            { target.offsetToRegexpTable,
              static_cast<quint32>(oldUnit->regexpTableSize * sizeof(RegExp)) },
            { target.offsetToConstantTable,
              static_cast<quint32>(oldUnit->constantTableSize * sizeof(quint64)) },
            { target.offsetToClassTable,
              static_cast<quint32>(target.classTableSize * sizeof(quint32_le)) },
            { target.offsetToTemplateObjectTable,
              static_cast<quint32>(target.templateObjectTableSize * sizeof(quint32_le)) },
            { target.offsetToBlockTable,
              static_cast<quint32>(target.blockTableSize * sizeof(quint32_le)) },
            { target.offsetToJSClassTable,
              static_cast<quint32>(target.jsClassTableSize * sizeof(quint32_le)) },
            { target.offsetToTranslationTable, static_cast<quint32>(translationDataSize) },
        };
        for (const auto &e : inlineEnds)
            targetInlineEnd = qMax(targetInlineEnd, e.offset + e.size);
        // JSClass ENTRY data is inline (right after the offset table)
        quint32 jsClassEntryTargetStart =
                target.offsetToJSClassTable + target.jsClassTableSize * sizeof(quint32_le);
        targetInlineEnd =
                qMax(targetInlineEnd,
                     jsClassEntryTargetStart + static_cast<quint32>(jsClassEntryData.size()));

        // Calculate size change.
        // If jsClass entry data is moving from the tail to the inline region,
        // the tail shrinks by that amount, so net size change is reduced.
        bool jsClassWillMoveFromTail =
                (oldUnit->jsClassTableSize > 0 && oldJsClassEntryStart >= oldInlineTablesEnd);
        qint32 inlineRegionDelta =
                static_cast<qint32>(targetInlineEnd) - static_cast<qint32>(oldInlineTablesEnd);
        qsizetype tailShrink =
                jsClassWillMoveFromTail ? static_cast<qsizetype>(jsClassEntryData.size()) : 0;

        qsizetype newUnitSize = oldUnit->unitSize + inlineRegionDelta - tailShrink;
        QByteArray newData(newUnitSize, '\0');

        // Copy header (unchanged)
        std::memcpy(newData.data(), m_patchedData.constData(), headerSize);

        // Write inline tables at their target positions
        auto writeToTarget = [&](const QByteArray &data, quint32 offset) {
            if (!data.isEmpty())
                std::memcpy(newData.data() + offset, data.constData(), data.size());
        };
        writeToTarget(functionOffsets, target.offsetToFunctionTable);
        writeToTarget(lookupData, target.offsetToLookupTable);
        writeToTarget(regexpData, target.offsetToRegexpTable);
        writeToTarget(constantData, target.offsetToConstantTable);
        writeToTarget(translationData, target.offsetToTranslationTable);
        writeToTarget(classOffsetData, target.offsetToClassTable);
        writeToTarget(templateObjectOffsetData, target.offsetToTemplateObjectTable);
        writeToTarget(blockOffsetData, target.offsetToBlockTable);
        writeToTarget(jsClassOffsetData, target.offsetToJSClassTable);

        // ===== Extract and reorder post-inline data regions =====
        // After sequential rebuilds, the data regions (function DATA, class DATA,
        // templateObj DATA, block DATA) may be in a different order than the compiler
        // produces. We extract each region and reassemble them in the correct order:
        //   [func DATA][class DATA][templateObj DATA][block DATA][string table+QmlUnit]

        struct DataRegion
        {
            quint32 oldStart;
            quint32 size;
            int targetOrder; // 0=func, 1=class, 2=template, 3=block
        };
        QVector<DataRegion> tailRegions;

        // Use findDataRegionBounds for each table type to find tail data regions
        {
            quint32 start, end;
            if (findDataRegionBounds<Function>(
                        m_patchedData, oldUnit->offsetToFunctionTable, oldUnit->functionTableSize,
                        [](const Function *f) -> quint32 {
                            return Function::calculateSize(f->nFormals, f->nLocals,
                                                           f->nLineAndStatementNumbers,
                                                           f->nLabelInfos, f->codeSize);
                        },
                        start, end)
                && start >= oldInlineTablesEnd) {
                tailRegions.append({ start, end - start, 0 });
            }
            if (findDataRegionBounds<Class>(
                        m_patchedData, oldUnit->offsetToClassTable, oldUnit->classTableSize,
                        [](const Class *c) -> quint32 {
                            return Class::calculateSize(c->nStaticMethods, c->nMethods);
                        },
                        start, end)
                && start >= oldInlineTablesEnd) {
                tailRegions.append({ start, end - start, 1 });
            }
            if (findDataRegionBounds<TemplateObject>(
                        m_patchedData, oldUnit->offsetToTemplateObjectTable,
                        oldUnit->templateObjectTableSize,
                        [](const TemplateObject *t) -> quint32 {
                            return TemplateObject::calculateSize(t->size);
                        },
                        start, end)
                && start >= oldInlineTablesEnd) {
                tailRegions.append({ start, end - start, 2 });
            }
            if (findDataRegionBounds<Block>(
                        m_patchedData, oldUnit->offsetToBlockTable, oldUnit->blockTableSize,
                        [](const Block *b) -> quint32 { return Block::calculateSize(b->nLocals); },
                        start, end)
                && start >= oldInlineTablesEnd) {
                tailRegions.append({ start, end - start, 3 });
            }
        }

        // Sort regions by their current position
        std::sort(tailRegions.begin(), tailRegions.end(),
                  [](const DataRegion &a, const DataRegion &b) { return a.oldStart < b.oldStart; });

        // Extract each data region from the old unit, excluding jsClass entries if
        // they overlap (jsClass entries belong in the inline section).
        bool jsClassInTail =
                (oldUnit->jsClassTableSize > 0 && oldJsClassEntryStart >= oldInlineTablesEnd);
        QVector<QByteArray> regionData;
        for (auto &r : tailRegions) {
            if (jsClassInTail && oldJsClassEntryStart >= r.oldStart
                && oldJsClassEntryStart < r.oldStart + r.size) {
                quint32 preSize = oldJsClassEntryStart - r.oldStart;
                quint32 postSize = (r.oldStart + r.size) - oldJsClassEntryEnd;
                QByteArray data(preSize + postSize, '\0');
                if (preSize > 0) {
                    std::memcpy(data.data(), m_patchedData.constData() + r.oldStart, preSize);
                }
                if (postSize > 0) {
                    std::memcpy(data.data() + preSize,
                                m_patchedData.constData() + oldJsClassEntryEnd, postSize);
                }
                r.size = preSize + postSize;
                regionData.append(data);
            } else {
                regionData.append(QByteArray(m_patchedData.constData() + r.oldStart, r.size));
            }
        }

        // Extract suffix (string table + QmlUnit)
        quint32 suffixStart = oldUnit->offsetToStringTable;
        quint32 suffixSize = oldUnit->unitSize - suffixStart;
        QByteArray suffixData;
        if (jsClassInTail && oldJsClassEntryStart >= suffixStart) {
            quint32 preSize = oldJsClassEntryStart - suffixStart;
            quint32 postSize = oldUnit->unitSize - oldJsClassEntryEnd;
            suffixData.resize(preSize + postSize);
            if (preSize > 0) {
                std::memcpy(suffixData.data(), m_patchedData.constData() + suffixStart, preSize);
            }
            if (postSize > 0) {
                std::memcpy(suffixData.data() + preSize,
                            m_patchedData.constData() + oldJsClassEntryEnd, postSize);
            }
            suffixSize = preSize + postSize;
        } else {
            suffixData = QByteArray(m_patchedData.constData() + suffixStart, suffixSize);
        }

        // Sort regions by target order (func=0, class=1, template=2, block=3)
        QVector<int> sortOrder(tailRegions.size());
        std::iota(sortOrder.begin(), sortOrder.end(), 0);
        std::sort(sortOrder.begin(), sortOrder.end(), [&](int a, int b) {
            return tailRegions[a].targetOrder < tailRegions[b].targetOrder;
        });

        // Write data regions in the correct order starting at targetInlineEnd
        quint32 writePos = targetInlineEnd;
        QMap<int, qint32> regionDeltas; // targetOrder -> (newPos - oldPos)
        for (int idx : sortOrder) {
            qint32 delta =
                    static_cast<qint32>(writePos) - static_cast<qint32>(tailRegions[idx].oldStart);
            regionDeltas[tailRegions[idx].targetOrder] = delta;
            std::memcpy(newData.data() + writePos, regionData[idx].constData(),
                        regionData[idx].size());
            writePos += tailRegions[idx].size;
        }

        // Write jsClass entry data in the inline region at the target position
        if (!jsClassEntryData.isEmpty()) {
            std::memcpy(newData.data() + jsClassEntryTargetStart, jsClassEntryData.constData(),
                        jsClassEntryData.size());
        }

        // Write suffix (string table + QmlUnit)
        quint32 suffixNewStart = writePos;
        std::memcpy(newData.data() + writePos, suffixData.constData(), suffixData.size());
        qint32 suffixDelta = static_cast<qint32>(suffixNewStart) - static_cast<qint32>(suffixStart);

        // Update the Unit header with new offsets
        Unit *newUnit = reinterpret_cast<Unit *>(newData.data());
        newUnit->unitSize = static_cast<quint32>(newUnitSize);
        newUnit->offsetToFunctionTable = target.offsetToFunctionTable;
        newUnit->offsetToLookupTable = target.offsetToLookupTable;
        newUnit->offsetToRegexpTable = target.offsetToRegexpTable;
        newUnit->offsetToConstantTable = target.offsetToConstantTable;
        newUnit->offsetToJSClassTable = target.offsetToJSClassTable;
        newUnit->offsetToTranslationTable = target.offsetToTranslationTable;
        newUnit->offsetToClassTable = target.offsetToClassTable;
        newUnit->offsetToBlockTable = target.offsetToBlockTable;
        newUnit->offsetToTemplateObjectTable = target.offsetToTemplateObjectTable;
        newUnit->offsetToLocalExportEntryTable = target.offsetToLocalExportEntryTable;
        newUnit->offsetToIndirectExportEntryTable = target.offsetToIndirectExportEntryTable;
        newUnit->offsetToStarExportEntryTable = target.offsetToStarExportEntryTable;
        newUnit->offsetToImportEntryTable = target.offsetToImportEntryTable;
        newUnit->offsetToModuleRequestTable = target.offsetToModuleRequestTable;

        // Update string table and QmlUnit offsets (they moved with the suffix)
        newUnit->offsetToStringTable = suffixNewStart;
        newUnit->offsetToQmlUnit = suffixNewStart + (oldUnit->offsetToQmlUnit - suffixStart);

        // Update data offset table entries to point to new positions
        if (regionDeltas.contains(0))
            applyOffsetDelta(newData, target.offsetToFunctionTable, oldUnit->functionTableSize,
                             regionDeltas[0]);
        if (regionDeltas.contains(1))
            applyOffsetDelta(newData, target.offsetToClassTable, oldUnit->classTableSize,
                             regionDeltas[1]);
        if (regionDeltas.contains(2))
            applyOffsetDelta(newData, target.offsetToTemplateObjectTable,
                             oldUnit->templateObjectTableSize, regionDeltas[2]);
        if (regionDeltas.contains(3))
            applyOffsetDelta(newData, target.offsetToBlockTable, oldUnit->blockTableSize,
                             regionDeltas[3]);

        // Update jsClass offset table entries — rebase to new inline position
        applyOffsetDelta(newData, target.offsetToJSClassTable, oldUnit->jsClassTableSize,
                         static_cast<qint32>(jsClassEntryTargetStart)
                                 - static_cast<qint32>(oldJsClassEntryStart));

        // Update string offsets (absolute positions within the unit)
        applyOffsetDelta(newData, newUnit->offsetToStringTable, newUnit->stringTableSize,
                         suffixDelta);

        m_patchedData = newData;
        return true;
    }

    bool rebuildFunctionTable()
    {
        const Unit *u = reinterpret_cast<const Unit *>(m_patchedData.constData());
        auto funcSize = [](const Function *f) {
            return Function::calculateSize(f->nFormals, f->nLocals, f->nLineAndStatementNumbers,
                                           f->nLabelInfos, f->codeSize);
        };
        return rebuildIndirectTableGeneric<Function>(
                m_functions, u->offsetToFunctionTable, u->functionTableSize, funcSize,
                [&](quint32 i) -> std::optional<FunctionHunk> {
                    const Function &old = *u->functionAt(i);
                    FunctionHunk fd;
                    fd.data = old;
                    fd.data.nameIndex = remapStringIndex(old.nameIndex);
                    fd.data.returnType = remapParameterType(old.returnType);
                    for (quint16 j = 0, end = old.nFormals; j != end; ++j)
                        fd.formals.append(remapParameter(old.formalsTable()[j]));
                    for (quint16 j = 0, end = old.nLocals; j != end; ++j) {
                        quint32_le val;
                        val = remapStringIndex(old.localsTable()[j]);
                        fd.locals.append(val);
                    }
                    for (quint16 j = 0, end = old.nLineAndStatementNumbers; j != end; ++j)
                        fd.lineAndStatementNumbers.append(old.lineAndStatementNumberTable()[j]);
                    for (quint32 j = 0, end = old.nLabelInfos; j != end; ++j)
                        fd.labelInfos.append(old.labelInfoTable()[j]);
                    fd.code = QByteArray(old.code(), old.codeSize);
                    return fd;
                },
                [](const FunctionHunk &fd) -> QByteArray {
                    int sz = Function::calculateSize(fd.data.nFormals, fd.data.nLocals,
                                                     fd.data.nLineAndStatementNumbers,
                                                     fd.data.nLabelInfos, fd.data.codeSize);
                    QByteArray buf(sz, '\0');
                    Function *f = reinterpret_cast<Function *>(buf.data());
                    *f = fd.data;
                    auto *formals = reinterpret_cast<Parameter *>(buf.data() + f->formalsOffset);
                    for (qsizetype j = 0; j < fd.formals.size(); ++j)
                        formals[j] = fd.formals[j];
                    auto *locals = reinterpret_cast<quint32_le *>(buf.data() + f->localsOffset);
                    for (qsizetype j = 0; j < fd.locals.size(); ++j)
                        locals[j] = fd.locals[j];
                    auto *lsn = reinterpret_cast<CodeOffsetToLineAndStatement *>(
                            buf.data() + f->lineAndStatementNumberOffset());
                    for (qsizetype j = 0; j < fd.lineAndStatementNumbers.size(); ++j)
                        lsn[j] = fd.lineAndStatementNumbers[j];
                    auto *labels =
                            reinterpret_cast<quint32_le *>(buf.data() + f->labelInfosOffset());
                    for (qsizetype j = 0; j < fd.labelInfos.size(); ++j)
                        labels[j] = fd.labelInfos[j];
                    memcpy(buf.data() + f->codeOffset, fd.code.constData(), fd.code.size());
                    return buf;
                },
                IndirectTable::Function, offsetof(Unit, functionTableSize),
                offsetof(Unit, offsetToFunctionTable));
    }

    qsizetype calculateOldStringTableSize() const
    {
        // Calculate the total size of the old string table (offset table + string data +
        // any alignment/gap bytes before the QML unit).
        // We use offsetToQmlUnit as the actual end of the string section, since the QML unit
        // immediately follows the string table in compiled QML units.
        const Unit *unit = m_oldUnit;

        if (unit->stringTableSize == 0)
            return 0;

        return static_cast<qsizetype>(unit->offsetToQmlUnit)
                - static_cast<qsizetype>(unit->offsetToStringTable);
    }

    void applyChecksum()
    {
        // Find the UnitMetadataChanged change that contains the new checksum
        for (const Change &change : m_diff.changes) {
            if (change.type == ChangeType::UnitMetadataChanged) {
                const UnitHunk &newMetadata = std::get<UnitHunk>(change.data);
                Unit *unit = reinterpret_cast<Unit *>(m_patchedData.data());
                std::memcpy(unit->md5Checksum, newMetadata.md5Checksum,
                            sizeof(newMetadata.md5Checksum));
                std::memcpy(unit->dependencyMD5Checksum, newMetadata.dependencyMD5Checksum,
                            sizeof(newMetadata.dependencyMD5Checksum));
                return;
            }
        }
        // No checksum change found - keep the original
    }

    const Unit *m_oldUnit;
    const CompilationUnitDiff &m_diff;
    QByteArray m_patchedData;
    QString m_errorMessage;

    QHash<int, QList<const Change *>> m_changesByObject;

    ChangeSet<QString> m_strings;
    QHash<quint32, quint32> m_oldToNewStringIndex; // old string index -> new string index

    ChangeSet<quint64> m_constants;
    ChangeSet<FunctionHunk> m_functions;
    ChangeSet<LookupHunk> m_lookups;
    ChangeSet<TranslationDataHunk> m_translations;
    ChangeSet<RegExpHunk> m_regexps;
    ChangeSet<ClassHunk> m_classes;
    ChangeSet<TemplateObjectHunk> m_templateObjects;
    ChangeSet<JsClassHunk> m_jsClasses;
    ChangeSet<BlockHunk> m_blocks;

    std::optional<UnitHunk> m_unitMetadata;
    bool m_constantTableRebuilt = false;
};

QByteArray patchCompilationUnit(const Unit *oldUnit, const CompilationUnitDiff &diff,
                                QString *errorMessage)
{
    if (!diff.success) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Diff was not successful");
        return {};
    }

    const QmlUnit *oldQml = oldUnit->qmlUnit();

    if (!oldQml) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Invalid QML unit");
        return {};
    }

    CompilationUnitPatcher patcher(oldUnit, diff);
    return patcher.apply(errorMessage);
}

} // namespace QV4::CompiledData

QT_END_NAMESPACE
