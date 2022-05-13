// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml 2.15
import QtQuick 2.15

Text {
    id: root

    property real descent: fontMetrics.descent
    property real leading: fontMetrics.leading
    property real fontHeight: fontMetrics.height

    property real baselineOffset: -999

    //lineHeight: root.fontHeight - root.descent + root.baselineOffset - root.leading

    Binding on lineHeight {
        when: root.baselineOffset !== -999
        value: root.fontHeight - root.descent + root.baselineOffset - root.leading
    }

    onLineHeightChanged: {
        print("lh")
        print(root.baseLineOffset)
    }

    FontMetrics {
        id: fontMetrics
        font: root.font
    }

    lineHeightMode: root.baselineOffset !== -999 ? Text.FixedHeight : Text.ProportionalHeight

    width: visible ? implicitWidth : 0
    height: visible ? implicitHeight : 0


    property Text __backupText: Text {
        id: backupText
        visible: false
    }

    property Text languageExceptionItem: backupText
    onLanguageExceptionItemChanged: {
        if (root.__completed)
            root.assignException()
    }

    property bool __completed: false

    Component.onCompleted: {
        root.__backupText.font = root.font
        root.__backupText.text = root.text
        root.__backupText.color = root.color
        root.__backupText.lineHeight = root.lineHeight
        root.__backupText.lineHeightMode = root.lineHeightMode

        root.__completed = true
        print("start " + root.languageExceptionItem)
        root.assignException()
    }

    function assignException() {
        print("assign")
        print(root.languageExceptionItem)
        root.font = root.languageExceptionItem.font
        root.text = root.languageExceptionItem.text
        root.color = root.languageExceptionItem.color
        root.lineHeight = root.languageExceptionItem.lineHeight
        root.lineHeightMode = root.languageExceptionItem.lineHeightMode
    }

}
