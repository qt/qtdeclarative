// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "datamodel.h"
#include "spreadrole.h"

bool DataModel::empty() const
{
    return m_keys.empty();
}

std::pair<SpreadKey, SpreadKey> DataModel::clearHighlight()
{
    SpreadKey top_left{INT_MAX, INT_MAX};
    SpreadKey bottom_right{-1, -1};

    for (auto it = m_cells.begin(); it != m_cells.end();) {
        it.value().set(spread::Role::Hightlight, false);
        if (it.key().first < top_left.first)
            top_left.first = it.key().first;
        if (it.key().second < top_left.second)
            top_left.second = it.key().second;
        if (bottom_right.first < it.key().first)
            bottom_right.first = it.key().first;
        if (bottom_right.second < it.key().second)
            bottom_right.second = it.key().second;
        if (it.value().isNull()) {
            m_keys.remove(it.value().id);
            auto cit = spread::make_const(m_cells, it);
            it = m_cells.erase(cit);
        } else {
            ++it;
        }
    }

    return std::make_pair(top_left, bottom_right);
}

bool DataModel::setHighlight(const SpreadKey &key, bool highlight)
{
    if (auto it = m_cells.find(key); it != m_cells.end()) {
        it.value().set(spread::Role::Hightlight, highlight);
        if (it.value().isNull()) {
            m_keys.remove(it.value().id);
            auto cit = spread::make_const(m_cells, it);
            m_cells.erase(cit);
        }
        return true;
    }
    if (highlight) {
        SpreadCell cell;
        cell.set(spread::Role::Hightlight, true);
        cell.id = ++lastId;
        m_cells.insert(key, cell);
        m_keys.insert(cell.id, key);
        return true;
    }
    // we skipped false highlight for non-existing cell
    // because we don't store cells with only false hightlight data
    // to save memory
    return false;
}

QVariant DataModel::getData(int id, int role) const
{
    auto it_key = m_keys.find(id);
    if (it_key == m_keys.end())
        return QVariant{};
    const SpreadKey &key = it_key.value();
    auto it_cell = m_cells.find(key);
    if (it_cell == m_cells.end())
        return QVariant{};
    return it_cell.value().get(role);
}

QVariant DataModel::getData(const SpreadKey &key, int role) const
{
    auto it = m_cells.find(key);
    return it == m_cells.end() ? QVariant{} : it.value().get(role);
}

bool DataModel::setData(const SpreadKey &key, const QVariant &value, int role)
{
    // special roles
    switch (role) {
    case spread::Role::Hightlight:
        return setHighlight(key, value.toBool());
    default: break;
    }

           // no special handling for the role
    if (auto it = m_cells.find(key); it != m_cells.end()) {
        it.value().set(role, value);
        if (it.value().isNull()) {
            clearData(key);
            return true;
        }
    } else {
        SpreadCell cell;
        cell.set(role, value);
        cell.id = ++lastId;
        if (!cell.isNull()) {
            m_cells.insert(key, cell);
            m_keys.insert(cell.id, key);
        }
    }

    return true;
}

bool DataModel::clearData(const SpreadKey &key)
{
    auto find_key = [&key](const auto &i) { return i == key; };
    auto it = std::find_if(m_keys.cbegin(), m_keys.cend(), find_key);
    if (it == m_keys.cend())
        return 0;
    m_keys.erase(it);
    return m_cells.remove(key) > 0;
}

void DataModel::shiftColumns(int from, int count)
{
    if (count > 0) {
        // the reason for reverse iteration is because of the coverage of
        // the updated keys (bigger keys) and existing keys (next keys)
        QMapIterator i(m_cells);
        i.toBack();
        while (i.hasPrevious()) {
            i.previous();
            if (i.key().second >= from) {
                SpreadKey key = i.key();
                SpreadCell cell = i.value();
                m_cells.remove(key);
                key.second += count;
                m_cells.insert(key, cell);
            }
        }
    } else if (count < 0) {
        // the reason for normal iteration is because of the coverage of
        // the updated keys (smaller keys) and existing keys (previous keys)
        for (auto it = m_cells.begin(); it != m_cells.end(); ++it) {
            if (it.key().second >= from) {
                SpreadKey key = it.key();
                SpreadCell cell = it.value();
                m_cells.remove(key);
                key.second += count;
                m_cells.insert(key, cell);
            }
        }
    }

    if (count != 0) {
        for (auto it = m_keys.begin(); it != m_keys.end(); ++it) {
            SpreadKey &key = it.value();
            if (key.second >= from)
                key.second += count;
        }
    }
}

void DataModel::removeColumnCells(int column)
{
    for (auto it = m_cells.begin(); it != m_cells.end(); ) {
        if (it.key().second == column) {
            auto cit = spread::make_const(m_cells, it);
            it = m_cells.erase(cit);
        } else {
            ++it;
        }
    }

    for (auto it = m_keys.begin(); it != m_keys.end(); ) {
        if (it.value().second == column) {
            auto cit = spread::make_const(m_keys, it);
            it = m_keys.erase(cit);
        } else {
            ++it;
        }
    }
}

void DataModel::shiftRows(int from, int count)
{
    if (count > 0) {
        // the reason for reverse iteration is because of the coverage of
        // the updated keys (bigger keys) and existing keys (next keys)
        QMapIterator i(m_cells);
        i.toBack();
        while (i.hasPrevious()) {
            i.previous();
            if (i.key().first < from)
                break;
            SpreadKey key = i.key();
            SpreadCell cell = i.value();
            m_cells.remove(key);
            key.first += count;
            m_cells.insert(key, cell);
        }
    } else if (count < 0) {
        // the reason for normal iteration is because of the coverage of
        // the updated keys (smaller keys) and existing keys (previous keys)
        for (auto it = m_cells.begin(); it != m_cells.end(); ++it) {
            if (it.key().first >= from) {
                SpreadKey key = it.key();
                SpreadCell cell = it.value();
                m_cells.remove(key);
                key.first += count;
                m_cells.insert(key, cell);
            }
        }
    }

    if (count != 0) {
        for (auto it = m_keys.begin(); it != m_keys.end(); ++it) {
            SpreadKey &key = it.value();
            if (key.first >= from)
                key.first += count;
        }
    }
}

void DataModel::removeRowCells(int row)
{
    for (auto it = m_cells.begin(); it != m_cells.end(); ) {
        if (it.key().first == row) {
            auto cit = spread::make_const(m_cells, it);
            it = m_cells.erase(cit);
        } else {
            ++it;
        }
    }

    for (auto it = m_keys.begin(); it != m_keys.end(); ) {
        if (it.value().first == row) {
            auto cit = spread::make_const(m_keys, it);
            it = m_keys.erase(cit);
        } else {
            ++it;
        }
    }
}

int DataModel::createId(const SpreadKey &key)
{
    auto find_key = [&key](const auto &elem) { return key == elem; };
    auto it = std::find_if(m_keys.begin(), m_keys.end(), find_key);
    if (it != m_keys.end())
        return it.key();
    const int id = ++lastId;
    m_keys.insert(id, key);
    return id;
}

int DataModel::getId(const SpreadKey &key) const
{
    auto find_key = [&key](const auto &elem) { return key == elem; };
    auto it = std::find_if(m_keys.begin(), m_keys.end(), find_key);
    if (it == m_keys.end())
        return 0;
    return it.key();
}

SpreadKey DataModel::getKey(int id) const
{
    auto it = m_keys.find(id);
    return it == m_keys.end() ? SpreadKey{-1, -1} : it.value();
}
