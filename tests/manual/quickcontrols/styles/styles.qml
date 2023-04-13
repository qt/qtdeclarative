// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
    This manual test is used to take screenshots of each style for use in the
    documentation:

    Step 1

    Take screenshots of the app in the relevant styles in both light and dark
    themes (where applicable) using the commented-out window hints.

    The screenshots are usually taken on a MacBook with a DPI of 2.
    On macOS, Cmd+Shift+4 starts the screenshot process, Space allows selecting
    a window, holding Option before left-clicking takes a screenshot of the
    window without the default drop shadow border.

    If updating all styles, it can be helpful to have a script to open an
    instance of the app for each style:

        #! /bin/bash

        app=$1

        set -e

        usageExample="Usage example:\run-with-all-controls-styles.sh ./tst_manual_qqc_styles"

        if [ ! -f "$app" ]; then
            echo "app \"$app\" doesn't exist in \"$PWD\"; aborting"
            echo $usageExample
            exit
        fi

        # For dark mode.
        export QT_QUICK_CONTROLS_MATERIAL_THEME=Dark
        export QT_QUICK_CONTROLS_UNIVERSAL_THEME=Dark

        QT_QUICK_CONTROLS_STYLE=Basic $app 0 0 &
        QT_QUICK_CONTROLS_STYLE=Fusion $app 400 0 &
        QT_QUICK_CONTROLS_STYLE=macOS $app 800 0 &
        QT_QUICK_CONTROLS_STYLE=Material $app 1200 0 &
        QT_QUICK_CONTROLS_STYLE=Imagine $app 0 400 &
        QT_QUICK_CONTROLS_STYLE=iOS $app 400 0 &
        QT_QUICK_CONTROLS_STYLE=Universal $app 800 400 &
        #QT_QUICK_CONTROLS_STYLE=Windows $app 1200 400 &

    Step 2

    Rename images accordingly.

    Step 3

    Until QTBUG-63366 is solved, and if taken on a display with a DPR > 1,
    reduce the size of the images so that they are their natural (1 DPI) size.
    For example, if taken on a display with a DPR of 2, halve them:

        mogrify -resize 50% qtquickcontrols-basic.png
        mogrify -resize 50% qtquickcontrols-fusion-light.png
        mogrify -resize 50% qtquickcontrols-fusion-dark.png
        mogrify -resize 50% qtquickcontrols-imagine.png
        mogrify -resize 50% qtquickcontrols-ios-light.png
        mogrify -resize 50% qtquickcontrols-ios-dark.png
        mogrify -resize 50% qtquickcontrols-macos-light.png
        mogrify -resize 50% qtquickcontrols-macos-dark.png
        mogrify -resize 50% qtquickcontrols-material-light.png
        mogrify -resize 50% qtquickcontrols-material-dark.png
        mogrify -resize 50% qtquickcontrols-universal-light.png
        mogrify -resize 50% qtquickcontrols-universal-dark.png
        mogrify -resize 50% qtquickcontrols-windows.png

    Step 4

    Run "optipng -o 7 -strip all" on each image to reduce their file size.

    Step 5

    There may be extra screenshot images that need to be updated, depending
    on which style is being updated. For example, the Material style has
    screenshots that can be generated using tst_snippets:

        SCREENSHOTS=1 QT_QUICK_CONTROLS_STYLE=Material ./tst_snippets verify:qtquickcontrols-material-accent verify:qtquickcontrols-material-attributes verify:qtquickcontrols-material-background verify:qtquickcontrols-material-elevation verify:qtquickcontrols-material-foreground verify:qtquickcontrols-material-theme verify:qtquickcontrols-material-variant
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ApplicationWindow {
    id: window

    visible: true
    // Add some extra width and height to give the content a little more room.
    minimumWidth: column.implicitWidth + column.anchors.margins + 120
    minimumHeight: column.implicitHeight + column.anchors.margins + 20
    title: "Qt Quick Controls - Styles"

    // for taking frameless screenshots:
//    flags: Qt.Window | Qt.FramelessWindowHint

    ColumnLayout {
        id: column
        spacing: 20
        anchors.fill: parent
        anchors.margins: 40

        GroupBox {
            title: "Font Size"
            topPadding: 30
            contentWidth: fontColumnLayout.implicitWidth + fontColumnLayout.anchors.leftMargin
            background.visible: false

            ColumnLayout {
                id: fontColumnLayout
                anchors.fill: parent
                anchors.leftMargin: 20

                RadioButton {
                    leftPadding: 0
                    text: "Small"
                }
                RadioButton {
                    leftPadding: 0
                    text: "Medium"
                    checked: true
                }
                RadioButton {
                    leftPadding: 0
                    text: "Large"
                }
            }
        }

        GroupBox {
            title: "Audio"
            topPadding: 30
            contentWidth: audioGridLayout.implicitWidth + audioGridLayout.anchors.leftMargin
            background.visible: false

            Layout.fillWidth: true

            GridLayout {
                id: audioGridLayout
                columns: 2
                columnSpacing: 30
                anchors.fill: parent
                anchors.leftMargin: 20

                Label {
                    text: "Volume"
                }
                Slider {
                    value: 1.0
                    Layout.fillWidth: true
                }

                Label {
                    text: "Bass"
                }
                Slider {
                    value: 0.75
                    Layout.fillWidth: true
                }

                Label {
                    text: "Treble"
                }
                Slider {
                    value: 0.5
                    Layout.fillWidth: true
                }
            }
        }

        Button {
            text: "Save"
            Layout.alignment: Qt.AlignRight
        }

        Item { Layout.fillHeight: true }
    }
}
