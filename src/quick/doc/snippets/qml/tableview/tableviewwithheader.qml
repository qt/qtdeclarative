// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
TableView {
    id: tableView

    topMargin: header.implicitHeight

    Text {
        id: header
        text: "A table header"
    }
}
//![0]
