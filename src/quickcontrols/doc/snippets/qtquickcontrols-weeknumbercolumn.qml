// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

//! [1]
WeekNumberColumn {
    month: Calendar.December
    year: 2015
    locale: Qt.locale("en_US")
}
//! [1]
