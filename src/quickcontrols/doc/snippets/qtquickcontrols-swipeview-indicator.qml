// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

Item {
    width: 200
    height: 320

    //! [1]
    SwipeView {
        id: view

        currentIndex: 1
        anchors.fill: parent

        Item {
            id: firstPage
        }
        Item {
            id: secondPage
        }
        Item {
            id: thirdPage
        }
    }

    PageIndicator {
        id: indicator

        count: view.count
        currentIndex: view.currentIndex

        anchors.bottom: view.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
    //! [1]
}
