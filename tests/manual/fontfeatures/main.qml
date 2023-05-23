// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    width: 800
    height: 600
    title: "Font features"
    visible: true
    color: "white"

    function handleFont(font)
    {
        var map = font.features
        for (var i = 0; i < listView.count; ++i) {
            var item = listView.itemAtIndex(i)
            if (item !== null) {
                if (item.checkState === Qt.Checked)
                    map[item.text] = true
                else if (item.checkState === Qt.Unchecked)
                    map[item.text] = false
            }
        }

        font.features = map
        return font
    }

    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            Text {
                text: "Font:"
            }
            Text {
                text: sampleText.font.family
            }
            ToolButton {
                text: "..."
                onClicked: fontDialog.visible = true
            }
        }
        TextField {
            id: sampleText
            text: "This text is fine"
            font.family: "Calibri"
            font.pixelSize: 20
        }
        TextField {
            id: smcpText
            text: "Small caps"
            font: {
                "family": sampleText.font.family,
                "pixelSize": sampleText.font.pixelSize,
                "features": { "smcp": 1 }
            }
        }
        TextField {
            id: noLigaturesOrKerning
            text: "This text is fine"
            font.family: sampleText.font.family
            font.pixelSize: sampleText.font.pixelSize
            font.features: { "liga": 0, "dlig": 0, "clig": 0, "hlig": 0, "kern": 0 }
        }

        ListView {
            id: listView
            height: window.height / 2
            width: window.width
            model: [ "aalt",
                "abvf",
                "abvm",
                "abvs",
                "afrc",
                "akhn",
                "blwf",
                "blwm",
                "blws",
                "calt",
                "case",
                "ccmp",
                "cfar",
                "chws",
                "cjct",
                "clig",
                "cpct",
                "cpsp",
                "cswh",
                "curs",
                "cv01",
                "c2pc",
                "c2sc",
                "dist",
                "dlig",
                "dnom",
                "dtls",
                "expt",
                "falt",
                "fin2",
                "fin3",
                "fina",
                "flac",
                "frac",
                "fwid",
                "half",
                "haln",
                "halt",
                "hist",
                "hkna",
                "hlig",
                "hngl",
                "hojo",
                "hwid",
                "init",
                "isol",
                "ital",
                "jalt",
                "jp78",
                "jp83",
                "jp90",
                "jp04",
                "kern",
                "lfbd",
                "liga",
                "ljmo",
                "lnum",
                "locl",
                "ltra",
                "ltrm",
                "mark",
                "med2",
                "medi",
                "mgrk",
                "mkmk",
                "mset",
                "nalt",
                "nlck",
                "nukt",
                "numr",
                "onum",
                "opbd",
                "ordn",
                "ornm",
                "palt",
                "pcap",
                "pkna",
                "pnum",
                "pref",
                "pres",
                "pstf",
                "psts",
                "pwid",
                "qwid",
                "rand",
                "rclt",
                "rkrf",
                "rlig",
                "rphf",
                "rtbd",
                "rtla",
                "rtlm",
                "ruby",
                "rvrn",
                "salt",
                "sinf",
                "size",
                "smcp",
                "smpl",
                "ss01",
                "ss02",
                "ss03",
                "ss04",
                "ss05",
                "ss06",
                "ss07",
                "ss08",
                "ss09",
                "ss10",
                "ss11",
                "ss12",
                "ss13",
                "ss14",
                "ss15",
                "ss16",
                "ss17",
                "ss18",
                "ss19",
                "ss20",
                "ssty",
                "stch",
                "subs",
                "sups",
                "swsh",
                "titl",
                "tjmo",
                "tnam",
                "tnum",
                "trad",
                "twid",
                "unic",
                "valt",
                "vatu",
                "vchw",
                "vert",
                "vhal",
                "vjmo",
                "vkna",
                "vkrn",
                "vpal",
                "vrt2",
                "vrtr",
                "zero"
            ]

            delegate: CheckBox {
                id: checkBox
                text: modelData
                tristate: true
                checkState: Qt.PartiallyChecked
                onCheckedChanged: {
                    sampleText.font = handleFont(fontDialog.currentFont)
                }
            }
        }
    }

    FontDialog {
        id: fontDialog
        visible: false
        title: "Please select a font"
        onAccepted: {
            sampleText.font = handleFont(fontDialog.currentFont)
        }
    }
}
