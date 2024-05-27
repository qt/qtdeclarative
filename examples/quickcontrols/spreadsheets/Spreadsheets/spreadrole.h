// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SPREADROLE_H
#define SPREADROLE_H

#include <Qt>

namespace spread {
enum Role {
    // data roles
    BeginRole = Qt::DisplayRole,  // begin of data roles
    Display = Qt::DisplayRole,
    Edit = Qt::EditRole,
    Hightlight = Qt::UserRole + 1,
    EndRole,  // end of data roles
    // non-data roles
    ColumnName,
    RowName,
};
}

#endif // SPREADROLE_H
