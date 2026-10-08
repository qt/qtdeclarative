// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml.Models
import QtQuick

ListView {
    width: 200
    height: 200

    model: ObjectModel {
        id: objectModel

        Rectangle {
            width: 200
            height: 200
            color: "red"
        }
        Rectangle {
            width: 200
            height: 200
            color: "green"
        }
        Rectangle {
            width: 200
            height: 200
            color: "blue"
        }
    }
}
