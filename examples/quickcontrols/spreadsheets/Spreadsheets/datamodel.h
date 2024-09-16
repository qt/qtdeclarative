// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DATAMODEL_H
#define DATAMODEL_H

#include "spreadcell.h"
#include "spreadkey.h"

/**********************************************************
 * The DataModel struct manages the binding of data, keys,
 * and ids. There are some special functionalities that are
 * only related to data and keys, like
 *   shifting columns and rows
 *   inserting column and row
 *   removing columns and rows, and
 *   managing only the data.
 * This struct is extracted from the SpreadModel, and the
 * intention is to simplify the SpreadModel class and also
 * encapsulate any data-related concepts.
 **********************************************************/
struct DataModel
{
    bool empty() const;

    /******************************************************
     * Unsets highlight of highlighted data.
     * Returns a pair of top-left and bottom-right keys of updated cells
     ******************************************************/
    std::pair<SpreadKey, SpreadKey> clearHighlight();
    /******************************************************
     * Sets highlight role of data.
     * Returns true if any cell updated, otherwise, false.
     ******************************************************/
    bool setHighlight(const SpreadKey &key, bool highlight);

    QVariant getData(int id, int role) const;
    QVariant getData(const SpreadKey &key, int role) const;
    bool setData(const SpreadKey &key, const QVariant &value, int role);
    bool clearData(const SpreadKey &key);

    void shiftColumns(int from, int count);
    void removeColumnCells(int column);

    void shiftRows(int from, int count);
    void removeRowCells(int row);

    /******************************************************
     * If the key already exists in the model, returns the
     * id; otherwise, adds the key, assignes an id, and
     * returns the id.
     ******************************************************/
    int createId(const SpreadKey &key);
    int getId(const SpreadKey &key) const;
    SpreadKey getKey(int id) const;

private:
    uint lastId = 0;
    QMap<SpreadKey, SpreadCell> m_cells;
    QMap<int, SpreadKey> m_keys;
};

#endif // DATAMODEL_H
