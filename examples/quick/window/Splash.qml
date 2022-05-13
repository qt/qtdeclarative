// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import shared

//! [splash-properties]
Window {
    id: splash
    color: "transparent"
    title: "Splash Window"
    modality: Qt.ApplicationModal
    flags: Qt.SplashScreen
    property int timeoutInterval: 2000
    signal timeout
//! [splash-properties]
//! [screen-properties]
    x: (Screen.width - splashImage.width) / 2
    y: (Screen.height - splashImage.height) / 2
//! [screen-properties]
    width: splashImage.width
    height: splashImage.height

    Image {
        id: splashImage
        source: Images.qtLogo
        MouseArea {
            anchors.fill: parent
            onClicked: Qt.quit()
        }
    }
    //! [timer]
    Timer {
        interval: splash.timeoutInterval; running: true; repeat: false
        onTriggered: {
            splash.visible = false
            splash.timeout()
        }
    }
    //! [timer]
    Component.onCompleted: visible = true
}
