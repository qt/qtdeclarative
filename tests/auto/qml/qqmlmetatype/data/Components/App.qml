// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml 2.0

import Components 1.0

QtObject {
    id: mainRect

    property int appState: App.AppState.Blue
    property string color: "blue"

    enum AppState {
        Red,
        Green,
        Blue
    }

    onAppStateChanged: {
        if (appState === App.AppState.Green)
            mainRect.color = "green"
        else if (appState === App.AppState.Red)
            mainRect.color = "red"
    }

    property Timer timer: Timer {
        onTriggered: appState = App.AppState.Green
        running: true
        interval: 100
    }
}
