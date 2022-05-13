// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQml.Models 2.15
import QtQuick 2.15

DelegateModel {
    model: ListModel {
        ListElement {
            name: "Item 0"
        }
        ListElement {
            name: "Item 1"
        }
        ListElement {
            name: "Item 2"
        }
    }
    delegate: Item {}
}
