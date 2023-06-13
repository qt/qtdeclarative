// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [entire]
import QtQuick

Window {
    id: mainWindow
    title: "Main Window"
    color: "#456"
    property real defaultSpacing: 10

    property Splash splash: Splash {
        onTimeout: mainWindow.show()
    }

    component Splash: Window {
        id: splash

        // a splash screen has no titlebar
        flags: Qt.SplashScreen
        // the transparent color lets background behind the image edges show through
        color: "transparent"
        modality: Qt.ApplicationModal // in case another application window is showing
        title: "Splash Window" // for the taskbar/dock, task switcher etc.
        visible: true

        // here we use the Screen attached property to center the splash window
        //! [screen-properties]
        x: (Screen.width - splashImage.width) / 2
        y: (Screen.height - splashImage.height) / 2
        //! [screen-properties]
        width: splashImage.width
        height: splashImage.height

        property int timeoutInterval: 2000
        signal timeout

        Image {
            id: splashImage
            source: "images/qt-logo.png"
        }

        TapHandler {
            onTapped: splash.timeout()
        }

        Timer {
            interval: splash.timeoutInterval; running: true; repeat: false
            onTriggered: {
                splash.visible = false
                splash.timeout()
            }
        }
    }
}
//! [entire]
