// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

//! [1]
ColumnLayout {
    DayOfWeekRow {
        locale: grid.locale
        Layout.fillWidth: true
    }

    MonthGrid {
        id: grid
        month: Calendar.December
        year: 2015
        locale: Qt.locale("en_US")
        Layout.fillWidth: true
    }
}
//! [1]
