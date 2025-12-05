// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

//! [currentIndex]
SearchField {
    id: searchField
    suggestionModel: ListModel {
        ListElement { value: "123,456" }
    }
    textRole: "value"

    Component.onCompleted: {
        if (suggestionModel.count > 0) {
           text = suggestionModel.get(0).value
           currentIndex = 0
       }
    }
}
//! [currentIndex]
