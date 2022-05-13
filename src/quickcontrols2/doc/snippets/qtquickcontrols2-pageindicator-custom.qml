// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

//! [file]
import QtQuick
import QtQuick.Controls

PageIndicator {
    id: control
    count: 5
    currentIndex: 2

    delegate: Rectangle {
        implicitWidth: 8
        implicitHeight: 8

        radius: width / 2
        color: "#21be2b"

        opacity: index === control.currentIndex ? 0.95 : pressed ? 0.7 : 0.45

        required property int index

        Behavior on opacity {
            OpacityAnimator {
                duration: 100
            }
        }
    }
}
//! [file]
