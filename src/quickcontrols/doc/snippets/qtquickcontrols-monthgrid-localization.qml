// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

//! [1]
MonthGrid {
    id: monthGrid
    month: Calendar.December
    year: 2015
    locale: Qt.locale("ar")
    delegate: Text {
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        opacity: model.month === monthGrid.month ? 1 : 0
        text: monthGrid.locale.toString(model.date, "d")
        font: monthGrid.font

        required property var model
    }
}
//! [1]
