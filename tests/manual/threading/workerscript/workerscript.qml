// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    width: 320; height: 480

    WorkerScript {
        id: myWorker
        source: "workerscript.mjs"

        onMessage: (messageObject) => {
            if (messageObject.row == rowSpinner.value && messageObject.column == columnSpinner.value){ //Not an old result
                if (messageObject.result == -1)
                    resultText.text = "Column must be <= Row";
                else
                    resultText.text = messageObject.result;
            }
        }
    }

    Row {
        y: 24
        spacing: 24
        anchors.horizontalCenter: parent.horizontalCenter

        Spinner {
            id: rowSpinner
            label: "Row"
            onValueChanged: {
                resultText.text = "Loading...";
                myWorker.sendMessage( { row: rowSpinner.value, column: columnSpinner.value } );
            }
        }

        Spinner {
            id: columnSpinner
            label: "Column"
            onValueChanged: {
                resultText.text = "Loading...";
                myWorker.sendMessage( { row: rowSpinner.value, column: columnSpinner.value } );
            }
        }
    }

    Text {
        id: resultText
        y: 180
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        font.pixelSize: 32
    }

    Text {
        text: "Pascal's Triangle Calculator"
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 50 }
    }
}
