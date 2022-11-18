// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

//! [1]
ListView {
    id: listview

    width: 200; height: 200
    snapMode: ListView.SnapOneItem
    orientation: ListView.Horizontal
    highlightRangeMode: ListView.StrictlyEnforceRange

    model: CalendarModel {
        from: new Date(2015, 0, 1)
        to: new Date(2015, 11, 31)
    }

    delegate: MonthGrid {
        width: listview.width
        height: listview.height

        month: model.month
        year: model.year
        locale: Qt.locale("en_US")
    }

    ScrollIndicator.horizontal: ScrollIndicator { }
}
//! [1]
