// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Item {
    QtObject {
        id: attributes
        property string name
        property int size
        property variant attributes
    }

    Text { text: attributes.name }
}
//![0]

