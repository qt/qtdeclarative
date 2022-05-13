// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.LocalStorage
import "Database.js" as JS

ListModel {
    id: listModel
    Component.onCompleted: JS.dbReadAll()
}
