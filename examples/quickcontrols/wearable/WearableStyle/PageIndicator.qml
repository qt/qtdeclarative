// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.PageIndicator {
    id: control

    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding

    spacing: 6
    padding: 6
    bottomPadding: 7

    delegate: Rectangle {
        required property int index

        implicitWidth: 16
        implicitHeight: 16

        radius: width / 2
        color: index === control.currentIndex ? UIStyle.buttonProgress : UIStyle.pageIndicatorColor
        border.color: UIStyle.indicatorOutlineColor

        Behavior on opacity {
            OpacityAnimator {
                duration: 100
            }
        }
    }

    contentItem: Row {
        spacing: control.spacing

        Repeater {
            model: control.count
            delegate: control.delegate
        }
    }
}
