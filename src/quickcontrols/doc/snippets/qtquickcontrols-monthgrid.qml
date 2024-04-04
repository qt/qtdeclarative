// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

//! [1]
MonthGrid {
    month: Calendar.December
    year: 2015
    locale: Qt.locale("en_US")
}
//! [1]
