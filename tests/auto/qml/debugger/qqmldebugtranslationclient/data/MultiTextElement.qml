/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick Designer Components.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
