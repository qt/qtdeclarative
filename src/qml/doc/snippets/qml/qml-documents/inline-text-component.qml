// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [document]
import QtQuick

Rectangle {
    width: 240; height: 320;

    ListView {
        anchors.fill: parent
        model: contactModel
        delegate: Text {
            text: modelData.firstName + " " + modelData.lastName
        }
    }
}
//! [document]
