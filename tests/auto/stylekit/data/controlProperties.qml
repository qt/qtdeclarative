import QtQuick
import Qt.labs.StyleKit

Style {
    // Every property in the button is given a unique value, both in the
    // normal state, in each individual control state, and in one nested state. See
    // tst_StyleKit::controlProperties() for how the values are constructed.
    button {
        spacing: 1
        padding: 2
        leftPadding: 3
        rightPadding: 4
        topPadding: 5
        bottomPadding: 6
        transition: Transition { objectName: "transition" }

        background {
            width: 101
            height: 102
            minimumWidth: 103
            minimumHeight: 104
            margins: 105
            leftMargin: 106
            rightMargin: 107
            topMargin: 108
            bottomMargin: 109
            radius: 110
            topLeftRadius: 111
            topRightRadius: 112
            bottomLeftRadius: 113
            bottomRightRadius: 114
            rotation: 115
            scale: 1.013
            opacity: 0.011
            color: "#130a25"
            alignment: Qt.AlignLeft | Qt.AlignTop
            fillWidth: false
            fillHeight: true
            visible: true
            clip: false
            gradient: Gradient { objectName: "background.gradient" }
            data: QtObject { objectName: "background.data" }
            delegate: Component { Item { objectName: "background.delegate" } }
            border {
                width: 116
                color: "#133c80"
            }
            shadow {
                color: "#136edb"
                opacity: 0.012
                scale: 1.014
                verticalOffset: 117
                horizontalOffset: 118
                blur: 119
                visible: true
                delegate: Component { Item { objectName: "background.shadow.delegate" } }
            }
            image {
                source: "background.png"
                color: "#13a036"
                fillMode: Image.Stretch
            }
        }

        handle {
            width: 201
            height: 202
            minimumWidth: 203
            minimumHeight: 204
            margins: 205
            leftMargin: 206
            rightMargin: 207
            topMargin: 208
            bottomMargin: 209
            radius: 210
            topLeftRadius: 211
            topRightRadius: 212
            bottomLeftRadius: 213
            bottomRightRadius: 214
            rotation: 215
            scale: 1.023
            opacity: 0.021
            color: "#260a4a"
            alignment: Qt.AlignRight | Qt.AlignBottom
            fillWidth: true
            fillHeight: false
            visible: true
            clip: false
            gradient: Gradient { objectName: "handle.gradient" }
            data: QtObject { objectName: "handle.data" }
            delegate: Component { Item { objectName: "handle.delegate" } }
            border {
                width: 216
                color: "#263ca5"
            }
            shadow {
                color: "#266e00"
                opacity: 0.022
                scale: 1.024
                verticalOffset: 217
                horizontalOffset: 218
                blur: 219
                visible: false
                delegate: Component { Item { objectName: "handle.shadow.delegate" } }
            }
            image {
                source: "handle.png"
                color: "#26a05b"
                fillMode: Image.PreserveAspectFit
            }

            first {
                width: 301
                height: 302
                minimumWidth: 303
                minimumHeight: 304
                margins: 305
                leftMargin: 306
                rightMargin: 307
                topMargin: 308
                bottomMargin: 309
                radius: 310
                topLeftRadius: 311
                topRightRadius: 312
                bottomLeftRadius: 313
                bottomRightRadius: 314
                rotation: 315
                scale: 1.033
                opacity: 0.031
                color: "#390a6f"
                alignment: Qt.AlignHCenter | Qt.AlignVCenter
                fillWidth: false
                fillHeight: true
                visible: false
                clip: true
                gradient: Gradient { objectName: "handle.first.gradient" }
                data: QtObject { objectName: "handle.first.data" }
                delegate: Component { Item { objectName: "handle.first.delegate" } }
                border {
                    width: 316
                    color: "#393cca"
                }
                shadow {
                    color: "#396e25"
                    opacity: 0.032
                    scale: 1.034
                    verticalOffset: 317
                    horizontalOffset: 318
                    blur: 319
                    visible: false
                    delegate: Component { Item { objectName: "handle.first.shadow.delegate" } }
                }
                image {
                    source: "handle.first.png"
                    color: "#39a080"
                    fillMode: Image.PreserveAspectCrop
                }
            }

            second {
                width: 401
                height: 402
                minimumWidth: 403
                minimumHeight: 404
                margins: 405
                leftMargin: 406
                rightMargin: 407
                topMargin: 408
                bottomMargin: 409
                radius: 410
                topLeftRadius: 411
                topRightRadius: 412
                bottomLeftRadius: 413
                bottomRightRadius: 414
                rotation: 415
                scale: 1.043
                opacity: 0.041
                color: "#4c0a94"
                alignment: Qt.AlignJustify | Qt.AlignBaseline
                fillWidth: true
                fillHeight: false
                visible: true
                clip: false
                gradient: Gradient { objectName: "handle.second.gradient" }
                data: QtObject { objectName: "handle.second.data" }
                delegate: Component { Item { objectName: "handle.second.delegate" } }
                border {
                    width: 416
                    color: "#4c3cef"
                }
                shadow {
                    color: "#4c6e4a"
                    opacity: 0.042
                    scale: 1.044
                    verticalOffset: 417
                    horizontalOffset: 418
                    blur: 419
                    visible: true
                    delegate: Component { Item { objectName: "handle.second.shadow.delegate" } }
                }
                image {
                    source: "handle.second.png"
                    color: "#4ca0a5"
                    fillMode: Image.Tile
                }
            }
        }

        indicator {
            width: 501
            height: 502
            minimumWidth: 503
            minimumHeight: 504
            margins: 505
            leftMargin: 506
            rightMargin: 507
            topMargin: 508
            bottomMargin: 509
            radius: 510
            topLeftRadius: 511
            topRightRadius: 512
            bottomLeftRadius: 513
            bottomRightRadius: 514
            rotation: 515
            scale: 1.053
            opacity: 0.051
            color: "#5f0ab9"
            alignment: Qt.AlignLeft | Qt.AlignBottom
            fillWidth: false
            fillHeight: true
            visible: true
            clip: false
            gradient: Gradient { objectName: "indicator.gradient" }
            data: QtObject { objectName: "indicator.data" }
            delegate: Component { Item { objectName: "indicator.delegate" } }
            border {
                width: 516
                color: "#5f3c14"
            }
            shadow {
                color: "#5f6e6f"
                opacity: 0.052
                scale: 1.054
                verticalOffset: 517
                horizontalOffset: 518
                blur: 519
                visible: true
                delegate: Component { Item { objectName: "indicator.shadow.delegate" } }
            }
            image {
                source: "indicator.png"
                color: "#5fa0ca"
                fillMode: Image.TileVertically
            }

            foreground {
                width: 601
                height: 602
                minimumWidth: 603
                minimumHeight: 604
                margins: 605
                leftMargin: 606
                rightMargin: 607
                topMargin: 608
                bottomMargin: 609
                radius: 610
                topLeftRadius: 611
                topRightRadius: 612
                bottomLeftRadius: 613
                bottomRightRadius: 614
                rotation: 615
                scale: 1.063
                opacity: 0.061
                color: "#720ade"
                alignment: Qt.AlignRight | Qt.AlignVCenter
                fillWidth: true
                fillHeight: false
                visible: false
                clip: true
                gradient: Gradient { objectName: "indicator.foreground.gradient" }
                data: QtObject { objectName: "indicator.foreground.data" }
                delegate: Component { Item { objectName: "indicator.foreground.delegate" } }
                border {
                    width: 616
                    color: "#723c39"
                }
                shadow {
                    color: "#726e94"
                    opacity: 0.062
                    scale: 1.064
                    verticalOffset: 617
                    horizontalOffset: 618
                    blur: 619
                    visible: false
                    delegate: Component { Item { objectName: "indicator.foreground.shadow.delegate" } }
                }
                image {
                    source: "indicator.foreground.png"
                    color: "#72a0ef"
                    fillMode: Image.TileHorizontally
                }
            }

            first {
                width: 701
                height: 702
                minimumWidth: 703
                minimumHeight: 704
                margins: 705
                leftMargin: 706
                rightMargin: 707
                topMargin: 708
                bottomMargin: 709
                radius: 710
                topLeftRadius: 711
                topRightRadius: 712
                bottomLeftRadius: 713
                bottomRightRadius: 714
                rotation: 715
                scale: 1.073
                opacity: 0.071
                color: "#850a03"
                alignment: Qt.AlignHCenter | Qt.AlignBaseline
                fillWidth: false
                fillHeight: true
                visible: true
                clip: false
                gradient: Gradient { objectName: "indicator.first.gradient" }
                data: QtObject { objectName: "indicator.first.data" }
                delegate: Component { Item { objectName: "indicator.first.delegate" } }
                border {
                    width: 716
                    color: "#853c5e"
                }
                shadow {
                    color: "#856eb9"
                    opacity: 0.072
                    scale: 1.074
                    verticalOffset: 717
                    horizontalOffset: 718
                    blur: 719
                    visible: false
                    delegate: Component { Item { objectName: "indicator.first.shadow.delegate" } }
                }
                image {
                    source: "indicator.first.png"
                    color: "#85a014"
                    fillMode: Image.Pad
                }

                foreground {
                    width: 801
                    height: 802
                    minimumWidth: 803
                    minimumHeight: 804
                    margins: 805
                    leftMargin: 806
                    rightMargin: 807
                    topMargin: 808
                    bottomMargin: 809
                    radius: 810
                    topLeftRadius: 811
                    topRightRadius: 812
                    bottomLeftRadius: 813
                    bottomRightRadius: 814
                    rotation: 815
                    scale: 1.083
                    opacity: 0.081
                    color: "#980a28"
                    alignment: Qt.AlignJustify | Qt.AlignTop
                    fillWidth: true
                    fillHeight: false
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "indicator.first.foreground.gradient" }
                    data: QtObject { objectName: "indicator.first.foreground.data" }
                    delegate: Component { Item { objectName: "indicator.first.foreground.delegate" } }
                    border {
                        width: 816
                        color: "#983c83"
                    }
                    shadow {
                        color: "#986ede"
                        opacity: 0.082
                        scale: 1.084
                        verticalOffset: 817
                        horizontalOffset: 818
                        blur: 819
                        visible: true
                        delegate: Component { Item { objectName: "indicator.first.foreground.shadow.delegate" } }
                    }
                    image {
                        source: "indicator.first.foreground.png"
                        color: "#98a039"
                        fillMode: Image.Stretch
                    }
                }
            }

            second {
                width: 901
                height: 902
                minimumWidth: 903
                minimumHeight: 904
                margins: 905
                leftMargin: 906
                rightMargin: 907
                topMargin: 908
                bottomMargin: 909
                radius: 910
                topLeftRadius: 911
                topRightRadius: 912
                bottomLeftRadius: 913
                bottomRightRadius: 914
                rotation: 915
                scale: 1.093
                opacity: 0.091
                color: "#ab0a4d"
                alignment: Qt.AlignLeft | Qt.AlignVCenter
                fillWidth: false
                fillHeight: true
                visible: false
                clip: true
                gradient: Gradient { objectName: "indicator.second.gradient" }
                data: QtObject { objectName: "indicator.second.data" }
                delegate: Component { Item { objectName: "indicator.second.delegate" } }
                border {
                    width: 916
                    color: "#ab3ca8"
                }
                shadow {
                    color: "#ab6e03"
                    opacity: 0.092
                    scale: 1.094
                    verticalOffset: 917
                    horizontalOffset: 918
                    blur: 919
                    visible: true
                    delegate: Component { Item { objectName: "indicator.second.shadow.delegate" } }
                }
                image {
                    source: "indicator.second.png"
                    color: "#aba05e"
                    fillMode: Image.PreserveAspectFit
                }

                foreground {
                    width: 1001
                    height: 1002
                    minimumWidth: 1003
                    minimumHeight: 1004
                    margins: 1005
                    leftMargin: 1006
                    rightMargin: 1007
                    topMargin: 1008
                    bottomMargin: 1009
                    radius: 1010
                    topLeftRadius: 1011
                    topRightRadius: 1012
                    bottomLeftRadius: 1013
                    bottomRightRadius: 1014
                    rotation: 1015
                    scale: 1.103
                    opacity: 0.101
                    color: "#be0a72"
                    alignment: Qt.AlignRight | Qt.AlignBaseline
                    fillWidth: true
                    fillHeight: false
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "indicator.second.foreground.gradient" }
                    data: QtObject { objectName: "indicator.second.foreground.data" }
                    delegate: Component { Item { objectName: "indicator.second.foreground.delegate" } }
                    border {
                        width: 1016
                        color: "#be3ccd"
                    }
                    shadow {
                        color: "#be6e28"
                        opacity: 0.102
                        scale: 1.104
                        verticalOffset: 1017
                        horizontalOffset: 1018
                        blur: 1019
                        visible: false
                        delegate: Component { Item { objectName: "indicator.second.foreground.shadow.delegate" } }
                    }
                    image {
                        source: "indicator.second.foreground.png"
                        color: "#bea083"
                        fillMode: Image.PreserveAspectCrop
                    }
                }
            }
        }

        text {
            color: "#d10a97"
            alignment: Qt.AlignHCenter | Qt.AlignTop
            bold: false
            italic: true
            pointSize: 1101
            padding: 1102
            leftPadding: 1103
            rightPadding: 1104
            topPadding: 1105
            bottomPadding: 1106

            first {
                color: "#e40abc"
                alignment: Qt.AlignJustify | Qt.AlignBottom
                bold: true
                italic: false
                pointSize: 1201
                padding: 1202
                leftPadding: 1203
                rightPadding: 1204
                topPadding: 1205
                bottomPadding: 1206
            }

            second {
                color: "#f70ae1"
                alignment: Qt.AlignLeft | Qt.AlignBaseline
                bold: false
                italic: true
                pointSize: 1301
                padding: 1302
                leftPadding: 1303
                rightPadding: 1304
                topPadding: 1305
                bottomPadding: 1306
            }
        }

        pressed {
            spacing: 10001
            padding: 10002
            leftPadding: 10003
            rightPadding: 10004
            topPadding: 10005
            bottomPadding: 10006
            transition: Transition { objectName: "pressed.transition" }

            background {
                width: 10101
                height: 10102
                minimumWidth: 10103
                minimumHeight: 10104
                margins: 10105
                leftMargin: 10106
                rightMargin: 10107
                topMargin: 10108
                bottomMargin: 10109
                radius: 10110
                topLeftRadius: 10111
                topRightRadius: 10112
                bottomLeftRadius: 10113
                bottomRightRadius: 10114
                rotation: 10115
                scale: 1.113
                opacity: 0.111
                color: "#130f25"
                alignment: Qt.AlignRight | Qt.AlignBottom
                fillWidth: true
                fillHeight: false
                visible: false
                clip: true
                gradient: Gradient { objectName: "pressed.background.gradient" }
                data: QtObject { objectName: "pressed.background.data" }
                delegate: Component { Item { objectName: "pressed.background.delegate" } }
                border {
                    width: 10116
                    color: "#134180"
                }
                shadow {
                    color: "#1373db"
                    opacity: 0.112
                    scale: 1.114
                    verticalOffset: 10117
                    horizontalOffset: 10118
                    blur: 10119
                    visible: false
                    delegate: Component { Item { objectName: "pressed.background.shadow.delegate" } }
                }
                image {
                    source: "pressed.background.png"
                    color: "#13a536"
                    fillMode: Image.PreserveAspectFit
                }
            }

            handle {
                width: 10201
                height: 10202
                minimumWidth: 10203
                minimumHeight: 10204
                margins: 10205
                leftMargin: 10206
                rightMargin: 10207
                topMargin: 10208
                bottomMargin: 10209
                radius: 10210
                topLeftRadius: 10211
                topRightRadius: 10212
                bottomLeftRadius: 10213
                bottomRightRadius: 10214
                rotation: 10215
                scale: 1.123
                opacity: 0.121
                color: "#260f4a"
                alignment: Qt.AlignHCenter | Qt.AlignVCenter
                fillWidth: false
                fillHeight: true
                visible: false
                clip: true
                gradient: Gradient { objectName: "pressed.handle.gradient" }
                data: QtObject { objectName: "pressed.handle.data" }
                delegate: Component { Item { objectName: "pressed.handle.delegate" } }
                border {
                    width: 10216
                    color: "#2641a5"
                }
                shadow {
                    color: "#267300"
                    opacity: 0.122
                    scale: 1.124
                    verticalOffset: 10217
                    horizontalOffset: 10218
                    blur: 10219
                    visible: true
                    delegate: Component { Item { objectName: "pressed.handle.shadow.delegate" } }
                }
                image {
                    source: "pressed.handle.png"
                    color: "#26a55b"
                    fillMode: Image.PreserveAspectCrop
                }

                first {
                    width: 10301
                    height: 10302
                    minimumWidth: 10303
                    minimumHeight: 10304
                    margins: 10305
                    leftMargin: 10306
                    rightMargin: 10307
                    topMargin: 10308
                    bottomMargin: 10309
                    radius: 10310
                    topLeftRadius: 10311
                    topRightRadius: 10312
                    bottomLeftRadius: 10313
                    bottomRightRadius: 10314
                    rotation: 10315
                    scale: 1.133
                    opacity: 0.131
                    color: "#390f6f"
                    alignment: Qt.AlignJustify | Qt.AlignBaseline
                    fillWidth: true
                    fillHeight: false
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "pressed.handle.first.gradient" }
                    data: QtObject { objectName: "pressed.handle.first.data" }
                    delegate: Component { Item { objectName: "pressed.handle.first.delegate" } }
                    border {
                        width: 10316
                        color: "#3941ca"
                    }
                    shadow {
                        color: "#397325"
                        opacity: 0.132
                        scale: 1.134
                        verticalOffset: 10317
                        horizontalOffset: 10318
                        blur: 10319
                        visible: true
                        delegate: Component { Item { objectName: "pressed.handle.first.shadow.delegate" } }
                    }
                    image {
                        source: "pressed.handle.first.png"
                        color: "#39a580"
                        fillMode: Image.Tile
                    }
                }

                second {
                    width: 10401
                    height: 10402
                    minimumWidth: 10403
                    minimumHeight: 10404
                    margins: 10405
                    leftMargin: 10406
                    rightMargin: 10407
                    topMargin: 10408
                    bottomMargin: 10409
                    radius: 10410
                    topLeftRadius: 10411
                    topRightRadius: 10412
                    bottomLeftRadius: 10413
                    bottomRightRadius: 10414
                    rotation: 10415
                    scale: 1.143
                    opacity: 0.141
                    color: "#4c0f94"
                    alignment: Qt.AlignLeft | Qt.AlignBottom
                    fillWidth: false
                    fillHeight: true
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "pressed.handle.second.gradient" }
                    data: QtObject { objectName: "pressed.handle.second.data" }
                    delegate: Component { Item { objectName: "pressed.handle.second.delegate" } }
                    border {
                        width: 10416
                        color: "#4c41ef"
                    }
                    shadow {
                        color: "#4c734a"
                        opacity: 0.142
                        scale: 1.144
                        verticalOffset: 10417
                        horizontalOffset: 10418
                        blur: 10419
                        visible: false
                        delegate: Component { Item { objectName: "pressed.handle.second.shadow.delegate" } }
                    }
                    image {
                        source: "pressed.handle.second.png"
                        color: "#4ca5a5"
                        fillMode: Image.TileVertically
                    }
                }
            }

            indicator {
                width: 10501
                height: 10502
                minimumWidth: 10503
                minimumHeight: 10504
                margins: 10505
                leftMargin: 10506
                rightMargin: 10507
                topMargin: 10508
                bottomMargin: 10509
                radius: 10510
                topLeftRadius: 10511
                topRightRadius: 10512
                bottomLeftRadius: 10513
                bottomRightRadius: 10514
                rotation: 10515
                scale: 1.153
                opacity: 0.151
                color: "#5f0fb9"
                alignment: Qt.AlignRight | Qt.AlignVCenter
                fillWidth: true
                fillHeight: false
                visible: false
                clip: true
                gradient: Gradient { objectName: "pressed.indicator.gradient" }
                data: QtObject { objectName: "pressed.indicator.data" }
                delegate: Component { Item { objectName: "pressed.indicator.delegate" } }
                border {
                    width: 10516
                    color: "#5f4114"
                }
                shadow {
                    color: "#5f736f"
                    opacity: 0.152
                    scale: 1.154
                    verticalOffset: 10517
                    horizontalOffset: 10518
                    blur: 10519
                    visible: false
                    delegate: Component { Item { objectName: "pressed.indicator.shadow.delegate" } }
                }
                image {
                    source: "pressed.indicator.png"
                    color: "#5fa5ca"
                    fillMode: Image.TileHorizontally
                }

                foreground {
                    width: 10601
                    height: 10602
                    minimumWidth: 10603
                    minimumHeight: 10604
                    margins: 10605
                    leftMargin: 10606
                    rightMargin: 10607
                    topMargin: 10608
                    bottomMargin: 10609
                    radius: 10610
                    topLeftRadius: 10611
                    topRightRadius: 10612
                    bottomLeftRadius: 10613
                    bottomRightRadius: 10614
                    rotation: 10615
                    scale: 1.163
                    opacity: 0.161
                    color: "#720fde"
                    alignment: Qt.AlignHCenter | Qt.AlignBaseline
                    fillWidth: false
                    fillHeight: true
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "pressed.indicator.foreground.gradient" }
                    data: QtObject { objectName: "pressed.indicator.foreground.data" }
                    delegate: Component { Item { objectName: "pressed.indicator.foreground.delegate" } }
                    border {
                        width: 10616
                        color: "#724139"
                    }
                    shadow {
                        color: "#727394"
                        opacity: 0.162
                        scale: 1.164
                        verticalOffset: 10617
                        horizontalOffset: 10618
                        blur: 10619
                        visible: true
                        delegate: Component { Item { objectName: "pressed.indicator.foreground.shadow.delegate" } }
                    }
                    image {
                        source: "pressed.indicator.foreground.png"
                        color: "#72a5ef"
                        fillMode: Image.Pad
                    }
                }

                first {
                    width: 10701
                    height: 10702
                    minimumWidth: 10703
                    minimumHeight: 10704
                    margins: 10705
                    leftMargin: 10706
                    rightMargin: 10707
                    topMargin: 10708
                    bottomMargin: 10709
                    radius: 10710
                    topLeftRadius: 10711
                    topRightRadius: 10712
                    bottomLeftRadius: 10713
                    bottomRightRadius: 10714
                    rotation: 10715
                    scale: 1.173
                    opacity: 0.171
                    color: "#850f03"
                    alignment: Qt.AlignJustify | Qt.AlignTop
                    fillWidth: true
                    fillHeight: false
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "pressed.indicator.first.gradient" }
                    data: QtObject { objectName: "pressed.indicator.first.data" }
                    delegate: Component { Item { objectName: "pressed.indicator.first.delegate" } }
                    border {
                        width: 10716
                        color: "#85415e"
                    }
                    shadow {
                        color: "#8573b9"
                        opacity: 0.172
                        scale: 1.174
                        verticalOffset: 10717
                        horizontalOffset: 10718
                        blur: 10719
                        visible: true
                        delegate: Component { Item { objectName: "pressed.indicator.first.shadow.delegate" } }
                    }
                    image {
                        source: "pressed.indicator.first.png"
                        color: "#85a514"
                        fillMode: Image.Stretch
                    }

                    foreground {
                        width: 10801
                        height: 10802
                        minimumWidth: 10803
                        minimumHeight: 10804
                        margins: 10805
                        leftMargin: 10806
                        rightMargin: 10807
                        topMargin: 10808
                        bottomMargin: 10809
                        radius: 10810
                        topLeftRadius: 10811
                        topRightRadius: 10812
                        bottomLeftRadius: 10813
                        bottomRightRadius: 10814
                        rotation: 10815
                        scale: 1.183
                        opacity: 0.181
                        color: "#980f28"
                        alignment: Qt.AlignLeft | Qt.AlignVCenter
                        fillWidth: false
                        fillHeight: true
                        visible: false
                        clip: true
                        gradient: Gradient { objectName: "pressed.indicator.first.foreground.gradient" }
                        data: QtObject { objectName: "pressed.indicator.first.foreground.data" }
                        delegate: Component { Item { objectName: "pressed.indicator.first.foreground.delegate" } }
                        border {
                            width: 10816
                            color: "#984183"
                        }
                        shadow {
                            color: "#9873de"
                            opacity: 0.182
                            scale: 1.184
                            verticalOffset: 10817
                            horizontalOffset: 10818
                            blur: 10819
                            visible: false
                            delegate: Component { Item { objectName: "pressed.indicator.first.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "pressed.indicator.first.foreground.png"
                            color: "#98a539"
                            fillMode: Image.PreserveAspectFit
                        }
                    }
                }

                second {
                    width: 10901
                    height: 10902
                    minimumWidth: 10903
                    minimumHeight: 10904
                    margins: 10905
                    leftMargin: 10906
                    rightMargin: 10907
                    topMargin: 10908
                    bottomMargin: 10909
                    radius: 10910
                    topLeftRadius: 10911
                    topRightRadius: 10912
                    bottomLeftRadius: 10913
                    bottomRightRadius: 10914
                    rotation: 10915
                    scale: 1.193
                    opacity: 0.191
                    color: "#ab0f4d"
                    alignment: Qt.AlignRight | Qt.AlignBaseline
                    fillWidth: true
                    fillHeight: false
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "pressed.indicator.second.gradient" }
                    data: QtObject { objectName: "pressed.indicator.second.data" }
                    delegate: Component { Item { objectName: "pressed.indicator.second.delegate" } }
                    border {
                        width: 10916
                        color: "#ab41a8"
                    }
                    shadow {
                        color: "#ab7303"
                        opacity: 0.192
                        scale: 1.194
                        verticalOffset: 10917
                        horizontalOffset: 10918
                        blur: 10919
                        visible: false
                        delegate: Component { Item { objectName: "pressed.indicator.second.shadow.delegate" } }
                    }
                    image {
                        source: "pressed.indicator.second.png"
                        color: "#aba55e"
                        fillMode: Image.PreserveAspectCrop
                    }

                    foreground {
                        width: 11001
                        height: 11002
                        minimumWidth: 11003
                        minimumHeight: 11004
                        margins: 11005
                        leftMargin: 11006
                        rightMargin: 11007
                        topMargin: 11008
                        bottomMargin: 11009
                        radius: 11010
                        topLeftRadius: 11011
                        topRightRadius: 11012
                        bottomLeftRadius: 11013
                        bottomRightRadius: 11014
                        rotation: 11015
                        scale: 1.203
                        opacity: 0.201
                        color: "#be0f72"
                        alignment: Qt.AlignHCenter | Qt.AlignTop
                        fillWidth: false
                        fillHeight: true
                        visible: false
                        clip: true
                        gradient: Gradient { objectName: "pressed.indicator.second.foreground.gradient" }
                        data: QtObject { objectName: "pressed.indicator.second.foreground.data" }
                        delegate: Component { Item { objectName: "pressed.indicator.second.foreground.delegate" } }
                        border {
                            width: 11016
                            color: "#be41cd"
                        }
                        shadow {
                            color: "#be7328"
                            opacity: 0.202
                            scale: 1.204
                            verticalOffset: 11017
                            horizontalOffset: 11018
                            blur: 11019
                            visible: true
                            delegate: Component { Item { objectName: "pressed.indicator.second.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "pressed.indicator.second.foreground.png"
                            color: "#bea583"
                            fillMode: Image.Tile
                        }
                    }
                }
            }

            text {
                color: "#d10f97"
                alignment: Qt.AlignJustify | Qt.AlignBottom
                bold: true
                italic: false
                pointSize: 11101
                padding: 11102
                leftPadding: 11103
                rightPadding: 11104
                topPadding: 11105
                bottomPadding: 11106

                first {
                    color: "#e40fbc"
                    alignment: Qt.AlignLeft | Qt.AlignBaseline
                    bold: false
                    italic: true
                    pointSize: 11201
                    padding: 11202
                    leftPadding: 11203
                    rightPadding: 11204
                    topPadding: 11205
                    bottomPadding: 11206
                }

                second {
                    color: "#f70fe1"
                    alignment: Qt.AlignRight | Qt.AlignTop
                    bold: true
                    italic: false
                    pointSize: 11301
                    padding: 11302
                    leftPadding: 11303
                    rightPadding: 11304
                    topPadding: 11305
                    bottomPadding: 11306
                }
            }
        }

        hovered {
            spacing: 20001
            padding: 20002
            leftPadding: 20003
            rightPadding: 20004
            topPadding: 20005
            bottomPadding: 20006
            transition: Transition { objectName: "hovered.transition" }

            background {
                width: 20101
                height: 20102
                minimumWidth: 20103
                minimumHeight: 20104
                margins: 20105
                leftMargin: 20106
                rightMargin: 20107
                topMargin: 20108
                bottomMargin: 20109
                radius: 20110
                topLeftRadius: 20111
                topRightRadius: 20112
                bottomLeftRadius: 20113
                bottomRightRadius: 20114
                rotation: 20115
                scale: 1.213
                opacity: 0.211
                color: "#131425"
                alignment: Qt.AlignHCenter | Qt.AlignVCenter
                fillWidth: false
                fillHeight: true
                visible: true
                clip: false
                gradient: Gradient { objectName: "hovered.background.gradient" }
                data: QtObject { objectName: "hovered.background.data" }
                delegate: Component { Item { objectName: "hovered.background.delegate" } }
                border {
                    width: 20116
                    color: "#134680"
                }
                shadow {
                    color: "#1378db"
                    opacity: 0.212
                    scale: 1.214
                    verticalOffset: 20117
                    horizontalOffset: 20118
                    blur: 20119
                    visible: true
                    delegate: Component { Item { objectName: "hovered.background.shadow.delegate" } }
                }
                image {
                    source: "hovered.background.png"
                    color: "#13aa36"
                    fillMode: Image.PreserveAspectCrop
                }
            }

            handle {
                width: 20201
                height: 20202
                minimumWidth: 20203
                minimumHeight: 20204
                margins: 20205
                leftMargin: 20206
                rightMargin: 20207
                topMargin: 20208
                bottomMargin: 20209
                radius: 20210
                topLeftRadius: 20211
                topRightRadius: 20212
                bottomLeftRadius: 20213
                bottomRightRadius: 20214
                rotation: 20215
                scale: 1.223
                opacity: 0.221
                color: "#26144a"
                alignment: Qt.AlignJustify | Qt.AlignBaseline
                fillWidth: true
                fillHeight: false
                visible: true
                clip: false
                gradient: Gradient { objectName: "hovered.handle.gradient" }
                data: QtObject { objectName: "hovered.handle.data" }
                delegate: Component { Item { objectName: "hovered.handle.delegate" } }
                border {
                    width: 20216
                    color: "#2646a5"
                }
                shadow {
                    color: "#267800"
                    opacity: 0.222
                    scale: 1.224
                    verticalOffset: 20217
                    horizontalOffset: 20218
                    blur: 20219
                    visible: false
                    delegate: Component { Item { objectName: "hovered.handle.shadow.delegate" } }
                }
                image {
                    source: "hovered.handle.png"
                    color: "#26aa5b"
                    fillMode: Image.Tile
                }

                first {
                    width: 20301
                    height: 20302
                    minimumWidth: 20303
                    minimumHeight: 20304
                    margins: 20305
                    leftMargin: 20306
                    rightMargin: 20307
                    topMargin: 20308
                    bottomMargin: 20309
                    radius: 20310
                    topLeftRadius: 20311
                    topRightRadius: 20312
                    bottomLeftRadius: 20313
                    bottomRightRadius: 20314
                    rotation: 20315
                    scale: 1.233
                    opacity: 0.231
                    color: "#39146f"
                    alignment: Qt.AlignLeft | Qt.AlignBottom
                    fillWidth: false
                    fillHeight: true
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "hovered.handle.first.gradient" }
                    data: QtObject { objectName: "hovered.handle.first.data" }
                    delegate: Component { Item { objectName: "hovered.handle.first.delegate" } }
                    border {
                        width: 20316
                        color: "#3946ca"
                    }
                    shadow {
                        color: "#397825"
                        opacity: 0.232
                        scale: 1.234
                        verticalOffset: 20317
                        horizontalOffset: 20318
                        blur: 20319
                        visible: false
                        delegate: Component { Item { objectName: "hovered.handle.first.shadow.delegate" } }
                    }
                    image {
                        source: "hovered.handle.first.png"
                        color: "#39aa80"
                        fillMode: Image.TileVertically
                    }
                }

                second {
                    width: 20401
                    height: 20402
                    minimumWidth: 20403
                    minimumHeight: 20404
                    margins: 20405
                    leftMargin: 20406
                    rightMargin: 20407
                    topMargin: 20408
                    bottomMargin: 20409
                    radius: 20410
                    topLeftRadius: 20411
                    topRightRadius: 20412
                    bottomLeftRadius: 20413
                    bottomRightRadius: 20414
                    rotation: 20415
                    scale: 1.243
                    opacity: 0.241
                    color: "#4c1494"
                    alignment: Qt.AlignRight | Qt.AlignVCenter
                    fillWidth: true
                    fillHeight: false
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "hovered.handle.second.gradient" }
                    data: QtObject { objectName: "hovered.handle.second.data" }
                    delegate: Component { Item { objectName: "hovered.handle.second.delegate" } }
                    border {
                        width: 20416
                        color: "#4c46ef"
                    }
                    shadow {
                        color: "#4c784a"
                        opacity: 0.242
                        scale: 1.244
                        verticalOffset: 20417
                        horizontalOffset: 20418
                        blur: 20419
                        visible: true
                        delegate: Component { Item { objectName: "hovered.handle.second.shadow.delegate" } }
                    }
                    image {
                        source: "hovered.handle.second.png"
                        color: "#4caaa5"
                        fillMode: Image.TileHorizontally
                    }
                }
            }

            indicator {
                width: 20501
                height: 20502
                minimumWidth: 20503
                minimumHeight: 20504
                margins: 20505
                leftMargin: 20506
                rightMargin: 20507
                topMargin: 20508
                bottomMargin: 20509
                radius: 20510
                topLeftRadius: 20511
                topRightRadius: 20512
                bottomLeftRadius: 20513
                bottomRightRadius: 20514
                rotation: 20515
                scale: 1.253
                opacity: 0.251
                color: "#5f14b9"
                alignment: Qt.AlignHCenter | Qt.AlignBaseline
                fillWidth: false
                fillHeight: true
                visible: true
                clip: false
                gradient: Gradient { objectName: "hovered.indicator.gradient" }
                data: QtObject { objectName: "hovered.indicator.data" }
                delegate: Component { Item { objectName: "hovered.indicator.delegate" } }
                border {
                    width: 20516
                    color: "#5f4614"
                }
                shadow {
                    color: "#5f786f"
                    opacity: 0.252
                    scale: 1.254
                    verticalOffset: 20517
                    horizontalOffset: 20518
                    blur: 20519
                    visible: true
                    delegate: Component { Item { objectName: "hovered.indicator.shadow.delegate" } }
                }
                image {
                    source: "hovered.indicator.png"
                    color: "#5faaca"
                    fillMode: Image.Pad
                }

                foreground {
                    width: 20601
                    height: 20602
                    minimumWidth: 20603
                    minimumHeight: 20604
                    margins: 20605
                    leftMargin: 20606
                    rightMargin: 20607
                    topMargin: 20608
                    bottomMargin: 20609
                    radius: 20610
                    topLeftRadius: 20611
                    topRightRadius: 20612
                    bottomLeftRadius: 20613
                    bottomRightRadius: 20614
                    rotation: 20615
                    scale: 1.263
                    opacity: 0.261
                    color: "#7214de"
                    alignment: Qt.AlignJustify | Qt.AlignTop
                    fillWidth: true
                    fillHeight: false
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "hovered.indicator.foreground.gradient" }
                    data: QtObject { objectName: "hovered.indicator.foreground.data" }
                    delegate: Component { Item { objectName: "hovered.indicator.foreground.delegate" } }
                    border {
                        width: 20616
                        color: "#724639"
                    }
                    shadow {
                        color: "#727894"
                        opacity: 0.262
                        scale: 1.264
                        verticalOffset: 20617
                        horizontalOffset: 20618
                        blur: 20619
                        visible: false
                        delegate: Component { Item { objectName: "hovered.indicator.foreground.shadow.delegate" } }
                    }
                    image {
                        source: "hovered.indicator.foreground.png"
                        color: "#72aaef"
                        fillMode: Image.Stretch
                    }
                }

                first {
                    width: 20701
                    height: 20702
                    minimumWidth: 20703
                    minimumHeight: 20704
                    margins: 20705
                    leftMargin: 20706
                    rightMargin: 20707
                    topMargin: 20708
                    bottomMargin: 20709
                    radius: 20710
                    topLeftRadius: 20711
                    topRightRadius: 20712
                    bottomLeftRadius: 20713
                    bottomRightRadius: 20714
                    rotation: 20715
                    scale: 1.273
                    opacity: 0.271
                    color: "#851403"
                    alignment: Qt.AlignLeft | Qt.AlignVCenter
                    fillWidth: false
                    fillHeight: true
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "hovered.indicator.first.gradient" }
                    data: QtObject { objectName: "hovered.indicator.first.data" }
                    delegate: Component { Item { objectName: "hovered.indicator.first.delegate" } }
                    border {
                        width: 20716
                        color: "#85465e"
                    }
                    shadow {
                        color: "#8578b9"
                        opacity: 0.272
                        scale: 1.274
                        verticalOffset: 20717
                        horizontalOffset: 20718
                        blur: 20719
                        visible: false
                        delegate: Component { Item { objectName: "hovered.indicator.first.shadow.delegate" } }
                    }
                    image {
                        source: "hovered.indicator.first.png"
                        color: "#85aa14"
                        fillMode: Image.PreserveAspectFit
                    }

                    foreground {
                        width: 20801
                        height: 20802
                        minimumWidth: 20803
                        minimumHeight: 20804
                        margins: 20805
                        leftMargin: 20806
                        rightMargin: 20807
                        topMargin: 20808
                        bottomMargin: 20809
                        radius: 20810
                        topLeftRadius: 20811
                        topRightRadius: 20812
                        bottomLeftRadius: 20813
                        bottomRightRadius: 20814
                        rotation: 20815
                        scale: 1.283
                        opacity: 0.281
                        color: "#981428"
                        alignment: Qt.AlignRight | Qt.AlignBaseline
                        fillWidth: true
                        fillHeight: false
                        visible: true
                        clip: false
                        gradient: Gradient { objectName: "hovered.indicator.first.foreground.gradient" }
                        data: QtObject { objectName: "hovered.indicator.first.foreground.data" }
                        delegate: Component { Item { objectName: "hovered.indicator.first.foreground.delegate" } }
                        border {
                            width: 20816
                            color: "#984683"
                        }
                        shadow {
                            color: "#9878de"
                            opacity: 0.282
                            scale: 1.284
                            verticalOffset: 20817
                            horizontalOffset: 20818
                            blur: 20819
                            visible: true
                            delegate: Component { Item { objectName: "hovered.indicator.first.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "hovered.indicator.first.foreground.png"
                            color: "#98aa39"
                            fillMode: Image.PreserveAspectCrop
                        }
                    }
                }

                second {
                    width: 20901
                    height: 20902
                    minimumWidth: 20903
                    minimumHeight: 20904
                    margins: 20905
                    leftMargin: 20906
                    rightMargin: 20907
                    topMargin: 20908
                    bottomMargin: 20909
                    radius: 20910
                    topLeftRadius: 20911
                    topRightRadius: 20912
                    bottomLeftRadius: 20913
                    bottomRightRadius: 20914
                    rotation: 20915
                    scale: 1.293
                    opacity: 0.291
                    color: "#ab144d"
                    alignment: Qt.AlignHCenter | Qt.AlignTop
                    fillWidth: false
                    fillHeight: true
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "hovered.indicator.second.gradient" }
                    data: QtObject { objectName: "hovered.indicator.second.data" }
                    delegate: Component { Item { objectName: "hovered.indicator.second.delegate" } }
                    border {
                        width: 20916
                        color: "#ab46a8"
                    }
                    shadow {
                        color: "#ab7803"
                        opacity: 0.292
                        scale: 1.294
                        verticalOffset: 20917
                        horizontalOffset: 20918
                        blur: 20919
                        visible: true
                        delegate: Component { Item { objectName: "hovered.indicator.second.shadow.delegate" } }
                    }
                    image {
                        source: "hovered.indicator.second.png"
                        color: "#abaa5e"
                        fillMode: Image.Tile
                    }

                    foreground {
                        width: 21001
                        height: 21002
                        minimumWidth: 21003
                        minimumHeight: 21004
                        margins: 21005
                        leftMargin: 21006
                        rightMargin: 21007
                        topMargin: 21008
                        bottomMargin: 21009
                        radius: 21010
                        topLeftRadius: 21011
                        topRightRadius: 21012
                        bottomLeftRadius: 21013
                        bottomRightRadius: 21014
                        rotation: 21015
                        scale: 1.303
                        opacity: 0.301
                        color: "#be1472"
                        alignment: Qt.AlignJustify | Qt.AlignBottom
                        fillWidth: true
                        fillHeight: false
                        visible: true
                        clip: false
                        gradient: Gradient { objectName: "hovered.indicator.second.foreground.gradient" }
                        data: QtObject { objectName: "hovered.indicator.second.foreground.data" }
                        delegate: Component { Item { objectName: "hovered.indicator.second.foreground.delegate" } }
                        border {
                            width: 21016
                            color: "#be46cd"
                        }
                        shadow {
                            color: "#be7828"
                            opacity: 0.302
                            scale: 1.304
                            verticalOffset: 21017
                            horizontalOffset: 21018
                            blur: 21019
                            visible: false
                            delegate: Component { Item { objectName: "hovered.indicator.second.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "hovered.indicator.second.foreground.png"
                            color: "#beaa83"
                            fillMode: Image.TileVertically
                        }
                    }
                }
            }

            text {
                color: "#d11497"
                alignment: Qt.AlignLeft | Qt.AlignBaseline
                bold: false
                italic: true
                pointSize: 21101
                padding: 21102
                leftPadding: 21103
                rightPadding: 21104
                topPadding: 21105
                bottomPadding: 21106

                first {
                    color: "#e414bc"
                    alignment: Qt.AlignRight | Qt.AlignTop
                    bold: true
                    italic: false
                    pointSize: 21201
                    padding: 21202
                    leftPadding: 21203
                    rightPadding: 21204
                    topPadding: 21205
                    bottomPadding: 21206
                }

                second {
                    color: "#f714e1"
                    alignment: Qt.AlignHCenter | Qt.AlignBottom
                    bold: false
                    italic: true
                    pointSize: 21301
                    padding: 21302
                    leftPadding: 21303
                    rightPadding: 21304
                    topPadding: 21305
                    bottomPadding: 21306
                }
            }
        }

        focused {
            spacing: 30001
            padding: 30002
            leftPadding: 30003
            rightPadding: 30004
            topPadding: 30005
            bottomPadding: 30006
            transition: Transition { objectName: "focused.transition" }

            background {
                width: 30101
                height: 30102
                minimumWidth: 30103
                minimumHeight: 30104
                margins: 30105
                leftMargin: 30106
                rightMargin: 30107
                topMargin: 30108
                bottomMargin: 30109
                radius: 30110
                topLeftRadius: 30111
                topRightRadius: 30112
                bottomLeftRadius: 30113
                bottomRightRadius: 30114
                rotation: 30115
                scale: 1.313
                opacity: 0.311
                color: "#131925"
                alignment: Qt.AlignJustify | Qt.AlignBaseline
                fillWidth: true
                fillHeight: false
                visible: false
                clip: true
                gradient: Gradient { objectName: "focused.background.gradient" }
                data: QtObject { objectName: "focused.background.data" }
                delegate: Component { Item { objectName: "focused.background.delegate" } }
                border {
                    width: 30116
                    color: "#134b80"
                }
                shadow {
                    color: "#137ddb"
                    opacity: 0.312
                    scale: 1.314
                    verticalOffset: 30117
                    horizontalOffset: 30118
                    blur: 30119
                    visible: false
                    delegate: Component { Item { objectName: "focused.background.shadow.delegate" } }
                }
                image {
                    source: "focused.background.png"
                    color: "#13af36"
                    fillMode: Image.Tile
                }
            }

            handle {
                width: 30201
                height: 30202
                minimumWidth: 30203
                minimumHeight: 30204
                margins: 30205
                leftMargin: 30206
                rightMargin: 30207
                topMargin: 30208
                bottomMargin: 30209
                radius: 30210
                topLeftRadius: 30211
                topRightRadius: 30212
                bottomLeftRadius: 30213
                bottomRightRadius: 30214
                rotation: 30215
                scale: 1.323
                opacity: 0.321
                color: "#26194a"
                alignment: Qt.AlignLeft | Qt.AlignBottom
                fillWidth: false
                fillHeight: true
                visible: false
                clip: true
                gradient: Gradient { objectName: "focused.handle.gradient" }
                data: QtObject { objectName: "focused.handle.data" }
                delegate: Component { Item { objectName: "focused.handle.delegate" } }
                border {
                    width: 30216
                    color: "#264ba5"
                }
                shadow {
                    color: "#267d00"
                    opacity: 0.322
                    scale: 1.324
                    verticalOffset: 30217
                    horizontalOffset: 30218
                    blur: 30219
                    visible: true
                    delegate: Component { Item { objectName: "focused.handle.shadow.delegate" } }
                }
                image {
                    source: "focused.handle.png"
                    color: "#26af5b"
                    fillMode: Image.TileVertically
                }

                first {
                    width: 30301
                    height: 30302
                    minimumWidth: 30303
                    minimumHeight: 30304
                    margins: 30305
                    leftMargin: 30306
                    rightMargin: 30307
                    topMargin: 30308
                    bottomMargin: 30309
                    radius: 30310
                    topLeftRadius: 30311
                    topRightRadius: 30312
                    bottomLeftRadius: 30313
                    bottomRightRadius: 30314
                    rotation: 30315
                    scale: 1.333
                    opacity: 0.331
                    color: "#39196f"
                    alignment: Qt.AlignRight | Qt.AlignVCenter
                    fillWidth: true
                    fillHeight: false
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "focused.handle.first.gradient" }
                    data: QtObject { objectName: "focused.handle.first.data" }
                    delegate: Component { Item { objectName: "focused.handle.first.delegate" } }
                    border {
                        width: 30316
                        color: "#394bca"
                    }
                    shadow {
                        color: "#397d25"
                        opacity: 0.332
                        scale: 1.334
                        verticalOffset: 30317
                        horizontalOffset: 30318
                        blur: 30319
                        visible: true
                        delegate: Component { Item { objectName: "focused.handle.first.shadow.delegate" } }
                    }
                    image {
                        source: "focused.handle.first.png"
                        color: "#39af80"
                        fillMode: Image.TileHorizontally
                    }
                }

                second {
                    width: 30401
                    height: 30402
                    minimumWidth: 30403
                    minimumHeight: 30404
                    margins: 30405
                    leftMargin: 30406
                    rightMargin: 30407
                    topMargin: 30408
                    bottomMargin: 30409
                    radius: 30410
                    topLeftRadius: 30411
                    topRightRadius: 30412
                    bottomLeftRadius: 30413
                    bottomRightRadius: 30414
                    rotation: 30415
                    scale: 1.343
                    opacity: 0.341
                    color: "#4c1994"
                    alignment: Qt.AlignHCenter | Qt.AlignBaseline
                    fillWidth: false
                    fillHeight: true
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "focused.handle.second.gradient" }
                    data: QtObject { objectName: "focused.handle.second.data" }
                    delegate: Component { Item { objectName: "focused.handle.second.delegate" } }
                    border {
                        width: 30416
                        color: "#4c4bef"
                    }
                    shadow {
                        color: "#4c7d4a"
                        opacity: 0.342
                        scale: 1.344
                        verticalOffset: 30417
                        horizontalOffset: 30418
                        blur: 30419
                        visible: false
                        delegate: Component { Item { objectName: "focused.handle.second.shadow.delegate" } }
                    }
                    image {
                        source: "focused.handle.second.png"
                        color: "#4cafa5"
                        fillMode: Image.Pad
                    }
                }
            }

            indicator {
                width: 30501
                height: 30502
                minimumWidth: 30503
                minimumHeight: 30504
                margins: 30505
                leftMargin: 30506
                rightMargin: 30507
                topMargin: 30508
                bottomMargin: 30509
                radius: 30510
                topLeftRadius: 30511
                topRightRadius: 30512
                bottomLeftRadius: 30513
                bottomRightRadius: 30514
                rotation: 30515
                scale: 1.353
                opacity: 0.351
                color: "#5f19b9"
                alignment: Qt.AlignJustify | Qt.AlignTop
                fillWidth: true
                fillHeight: false
                visible: false
                clip: true
                gradient: Gradient { objectName: "focused.indicator.gradient" }
                data: QtObject { objectName: "focused.indicator.data" }
                delegate: Component { Item { objectName: "focused.indicator.delegate" } }
                border {
                    width: 30516
                    color: "#5f4b14"
                }
                shadow {
                    color: "#5f7d6f"
                    opacity: 0.352
                    scale: 1.354
                    verticalOffset: 30517
                    horizontalOffset: 30518
                    blur: 30519
                    visible: false
                    delegate: Component { Item { objectName: "focused.indicator.shadow.delegate" } }
                }
                image {
                    source: "focused.indicator.png"
                    color: "#5fafca"
                    fillMode: Image.Stretch
                }

                foreground {
                    width: 30601
                    height: 30602
                    minimumWidth: 30603
                    minimumHeight: 30604
                    margins: 30605
                    leftMargin: 30606
                    rightMargin: 30607
                    topMargin: 30608
                    bottomMargin: 30609
                    radius: 30610
                    topLeftRadius: 30611
                    topRightRadius: 30612
                    bottomLeftRadius: 30613
                    bottomRightRadius: 30614
                    rotation: 30615
                    scale: 1.363
                    opacity: 0.361
                    color: "#7219de"
                    alignment: Qt.AlignLeft | Qt.AlignVCenter
                    fillWidth: false
                    fillHeight: true
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "focused.indicator.foreground.gradient" }
                    data: QtObject { objectName: "focused.indicator.foreground.data" }
                    delegate: Component { Item { objectName: "focused.indicator.foreground.delegate" } }
                    border {
                        width: 30616
                        color: "#724b39"
                    }
                    shadow {
                        color: "#727d94"
                        opacity: 0.362
                        scale: 1.364
                        verticalOffset: 30617
                        horizontalOffset: 30618
                        blur: 30619
                        visible: true
                        delegate: Component { Item { objectName: "focused.indicator.foreground.shadow.delegate" } }
                    }
                    image {
                        source: "focused.indicator.foreground.png"
                        color: "#72afef"
                        fillMode: Image.PreserveAspectFit
                    }
                }

                first {
                    width: 30701
                    height: 30702
                    minimumWidth: 30703
                    minimumHeight: 30704
                    margins: 30705
                    leftMargin: 30706
                    rightMargin: 30707
                    topMargin: 30708
                    bottomMargin: 30709
                    radius: 30710
                    topLeftRadius: 30711
                    topRightRadius: 30712
                    bottomLeftRadius: 30713
                    bottomRightRadius: 30714
                    rotation: 30715
                    scale: 1.373
                    opacity: 0.371
                    color: "#851903"
                    alignment: Qt.AlignRight | Qt.AlignBaseline
                    fillWidth: true
                    fillHeight: false
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "focused.indicator.first.gradient" }
                    data: QtObject { objectName: "focused.indicator.first.data" }
                    delegate: Component { Item { objectName: "focused.indicator.first.delegate" } }
                    border {
                        width: 30716
                        color: "#854b5e"
                    }
                    shadow {
                        color: "#857db9"
                        opacity: 0.372
                        scale: 1.374
                        verticalOffset: 30717
                        horizontalOffset: 30718
                        blur: 30719
                        visible: true
                        delegate: Component { Item { objectName: "focused.indicator.first.shadow.delegate" } }
                    }
                    image {
                        source: "focused.indicator.first.png"
                        color: "#85af14"
                        fillMode: Image.PreserveAspectCrop
                    }

                    foreground {
                        width: 30801
                        height: 30802
                        minimumWidth: 30803
                        minimumHeight: 30804
                        margins: 30805
                        leftMargin: 30806
                        rightMargin: 30807
                        topMargin: 30808
                        bottomMargin: 30809
                        radius: 30810
                        topLeftRadius: 30811
                        topRightRadius: 30812
                        bottomLeftRadius: 30813
                        bottomRightRadius: 30814
                        rotation: 30815
                        scale: 1.383
                        opacity: 0.381
                        color: "#981928"
                        alignment: Qt.AlignHCenter | Qt.AlignTop
                        fillWidth: false
                        fillHeight: true
                        visible: false
                        clip: true
                        gradient: Gradient { objectName: "focused.indicator.first.foreground.gradient" }
                        data: QtObject { objectName: "focused.indicator.first.foreground.data" }
                        delegate: Component { Item { objectName: "focused.indicator.first.foreground.delegate" } }
                        border {
                            width: 30816
                            color: "#984b83"
                        }
                        shadow {
                            color: "#987dde"
                            opacity: 0.382
                            scale: 1.384
                            verticalOffset: 30817
                            horizontalOffset: 30818
                            blur: 30819
                            visible: false
                            delegate: Component { Item { objectName: "focused.indicator.first.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "focused.indicator.first.foreground.png"
                            color: "#98af39"
                            fillMode: Image.Tile
                        }
                    }
                }

                second {
                    width: 30901
                    height: 30902
                    minimumWidth: 30903
                    minimumHeight: 30904
                    margins: 30905
                    leftMargin: 30906
                    rightMargin: 30907
                    topMargin: 30908
                    bottomMargin: 30909
                    radius: 30910
                    topLeftRadius: 30911
                    topRightRadius: 30912
                    bottomLeftRadius: 30913
                    bottomRightRadius: 30914
                    rotation: 30915
                    scale: 1.393
                    opacity: 0.391
                    color: "#ab194d"
                    alignment: Qt.AlignJustify | Qt.AlignBottom
                    fillWidth: true
                    fillHeight: false
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "focused.indicator.second.gradient" }
                    data: QtObject { objectName: "focused.indicator.second.data" }
                    delegate: Component { Item { objectName: "focused.indicator.second.delegate" } }
                    border {
                        width: 30916
                        color: "#ab4ba8"
                    }
                    shadow {
                        color: "#ab7d03"
                        opacity: 0.392
                        scale: 1.394
                        verticalOffset: 30917
                        horizontalOffset: 30918
                        blur: 30919
                        visible: false
                        delegate: Component { Item { objectName: "focused.indicator.second.shadow.delegate" } }
                    }
                    image {
                        source: "focused.indicator.second.png"
                        color: "#abaf5e"
                        fillMode: Image.TileVertically
                    }

                    foreground {
                        width: 31001
                        height: 31002
                        minimumWidth: 31003
                        minimumHeight: 31004
                        margins: 31005
                        leftMargin: 31006
                        rightMargin: 31007
                        topMargin: 31008
                        bottomMargin: 31009
                        radius: 31010
                        topLeftRadius: 31011
                        topRightRadius: 31012
                        bottomLeftRadius: 31013
                        bottomRightRadius: 31014
                        rotation: 31015
                        scale: 1.403
                        opacity: 0.401
                        color: "#be1972"
                        alignment: Qt.AlignLeft | Qt.AlignBaseline
                        fillWidth: false
                        fillHeight: true
                        visible: false
                        clip: true
                        gradient: Gradient { objectName: "focused.indicator.second.foreground.gradient" }
                        data: QtObject { objectName: "focused.indicator.second.foreground.data" }
                        delegate: Component { Item { objectName: "focused.indicator.second.foreground.delegate" } }
                        border {
                            width: 31016
                            color: "#be4bcd"
                        }
                        shadow {
                            color: "#be7d28"
                            opacity: 0.402
                            scale: 1.404
                            verticalOffset: 31017
                            horizontalOffset: 31018
                            blur: 31019
                            visible: true
                            delegate: Component { Item { objectName: "focused.indicator.second.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "focused.indicator.second.foreground.png"
                            color: "#beaf83"
                            fillMode: Image.TileHorizontally
                        }
                    }
                }
            }

            text {
                color: "#d11997"
                alignment: Qt.AlignRight | Qt.AlignTop
                bold: true
                italic: false
                pointSize: 31101
                padding: 31102
                leftPadding: 31103
                rightPadding: 31104
                topPadding: 31105
                bottomPadding: 31106

                first {
                    color: "#e419bc"
                    alignment: Qt.AlignHCenter | Qt.AlignBottom
                    bold: false
                    italic: true
                    pointSize: 31201
                    padding: 31202
                    leftPadding: 31203
                    rightPadding: 31204
                    topPadding: 31205
                    bottomPadding: 31206
                }

                second {
                    color: "#f719e1"
                    alignment: Qt.AlignJustify | Qt.AlignVCenter
                    bold: true
                    italic: false
                    pointSize: 31301
                    padding: 31302
                    leftPadding: 31303
                    rightPadding: 31304
                    topPadding: 31305
                    bottomPadding: 31306
                }
            }
        }

        checked {
            spacing: 40001
            padding: 40002
            leftPadding: 40003
            rightPadding: 40004
            topPadding: 40005
            bottomPadding: 40006
            transition: Transition { objectName: "checked.transition" }

            background {
                width: 40101
                height: 40102
                minimumWidth: 40103
                minimumHeight: 40104
                margins: 40105
                leftMargin: 40106
                rightMargin: 40107
                topMargin: 40108
                bottomMargin: 40109
                radius: 40110
                topLeftRadius: 40111
                topRightRadius: 40112
                bottomLeftRadius: 40113
                bottomRightRadius: 40114
                rotation: 40115
                scale: 1.413
                opacity: 0.411
                color: "#131e25"
                alignment: Qt.AlignLeft | Qt.AlignBottom
                fillWidth: false
                fillHeight: true
                visible: true
                clip: false
                gradient: Gradient { objectName: "checked.background.gradient" }
                data: QtObject { objectName: "checked.background.data" }
                delegate: Component { Item { objectName: "checked.background.delegate" } }
                border {
                    width: 40116
                    color: "#135080"
                }
                shadow {
                    color: "#1382db"
                    opacity: 0.412
                    scale: 1.414
                    verticalOffset: 40117
                    horizontalOffset: 40118
                    blur: 40119
                    visible: true
                    delegate: Component { Item { objectName: "checked.background.shadow.delegate" } }
                }
                image {
                    source: "checked.background.png"
                    color: "#13b436"
                    fillMode: Image.TileVertically
                }
            }

            handle {
                width: 40201
                height: 40202
                minimumWidth: 40203
                minimumHeight: 40204
                margins: 40205
                leftMargin: 40206
                rightMargin: 40207
                topMargin: 40208
                bottomMargin: 40209
                radius: 40210
                topLeftRadius: 40211
                topRightRadius: 40212
                bottomLeftRadius: 40213
                bottomRightRadius: 40214
                rotation: 40215
                scale: 1.423
                opacity: 0.421
                color: "#261e4a"
                alignment: Qt.AlignRight | Qt.AlignVCenter
                fillWidth: true
                fillHeight: false
                visible: true
                clip: false
                gradient: Gradient { objectName: "checked.handle.gradient" }
                data: QtObject { objectName: "checked.handle.data" }
                delegate: Component { Item { objectName: "checked.handle.delegate" } }
                border {
                    width: 40216
                    color: "#2650a5"
                }
                shadow {
                    color: "#268200"
                    opacity: 0.422
                    scale: 1.424
                    verticalOffset: 40217
                    horizontalOffset: 40218
                    blur: 40219
                    visible: false
                    delegate: Component { Item { objectName: "checked.handle.shadow.delegate" } }
                }
                image {
                    source: "checked.handle.png"
                    color: "#26b45b"
                    fillMode: Image.TileHorizontally
                }

                first {
                    width: 40301
                    height: 40302
                    minimumWidth: 40303
                    minimumHeight: 40304
                    margins: 40305
                    leftMargin: 40306
                    rightMargin: 40307
                    topMargin: 40308
                    bottomMargin: 40309
                    radius: 40310
                    topLeftRadius: 40311
                    topRightRadius: 40312
                    bottomLeftRadius: 40313
                    bottomRightRadius: 40314
                    rotation: 40315
                    scale: 1.433
                    opacity: 0.431
                    color: "#391e6f"
                    alignment: Qt.AlignHCenter | Qt.AlignBaseline
                    fillWidth: false
                    fillHeight: true
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "checked.handle.first.gradient" }
                    data: QtObject { objectName: "checked.handle.first.data" }
                    delegate: Component { Item { objectName: "checked.handle.first.delegate" } }
                    border {
                        width: 40316
                        color: "#3950ca"
                    }
                    shadow {
                        color: "#398225"
                        opacity: 0.432
                        scale: 1.434
                        verticalOffset: 40317
                        horizontalOffset: 40318
                        blur: 40319
                        visible: false
                        delegate: Component { Item { objectName: "checked.handle.first.shadow.delegate" } }
                    }
                    image {
                        source: "checked.handle.first.png"
                        color: "#39b480"
                        fillMode: Image.Pad
                    }
                }

                second {
                    width: 40401
                    height: 40402
                    minimumWidth: 40403
                    minimumHeight: 40404
                    margins: 40405
                    leftMargin: 40406
                    rightMargin: 40407
                    topMargin: 40408
                    bottomMargin: 40409
                    radius: 40410
                    topLeftRadius: 40411
                    topRightRadius: 40412
                    bottomLeftRadius: 40413
                    bottomRightRadius: 40414
                    rotation: 40415
                    scale: 1.443
                    opacity: 0.441
                    color: "#4c1e94"
                    alignment: Qt.AlignJustify | Qt.AlignTop
                    fillWidth: true
                    fillHeight: false
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "checked.handle.second.gradient" }
                    data: QtObject { objectName: "checked.handle.second.data" }
                    delegate: Component { Item { objectName: "checked.handle.second.delegate" } }
                    border {
                        width: 40416
                        color: "#4c50ef"
                    }
                    shadow {
                        color: "#4c824a"
                        opacity: 0.442
                        scale: 1.444
                        verticalOffset: 40417
                        horizontalOffset: 40418
                        blur: 40419
                        visible: true
                        delegate: Component { Item { objectName: "checked.handle.second.shadow.delegate" } }
                    }
                    image {
                        source: "checked.handle.second.png"
                        color: "#4cb4a5"
                        fillMode: Image.Stretch
                    }
                }
            }

            indicator {
                width: 40501
                height: 40502
                minimumWidth: 40503
                minimumHeight: 40504
                margins: 40505
                leftMargin: 40506
                rightMargin: 40507
                topMargin: 40508
                bottomMargin: 40509
                radius: 40510
                topLeftRadius: 40511
                topRightRadius: 40512
                bottomLeftRadius: 40513
                bottomRightRadius: 40514
                rotation: 40515
                scale: 1.453
                opacity: 0.451
                color: "#5f1eb9"
                alignment: Qt.AlignLeft | Qt.AlignVCenter
                fillWidth: false
                fillHeight: true
                visible: true
                clip: false
                gradient: Gradient { objectName: "checked.indicator.gradient" }
                data: QtObject { objectName: "checked.indicator.data" }
                delegate: Component { Item { objectName: "checked.indicator.delegate" } }
                border {
                    width: 40516
                    color: "#5f5014"
                }
                shadow {
                    color: "#5f826f"
                    opacity: 0.452
                    scale: 1.454
                    verticalOffset: 40517
                    horizontalOffset: 40518
                    blur: 40519
                    visible: true
                    delegate: Component { Item { objectName: "checked.indicator.shadow.delegate" } }
                }
                image {
                    source: "checked.indicator.png"
                    color: "#5fb4ca"
                    fillMode: Image.PreserveAspectFit
                }

                foreground {
                    width: 40601
                    height: 40602
                    minimumWidth: 40603
                    minimumHeight: 40604
                    margins: 40605
                    leftMargin: 40606
                    rightMargin: 40607
                    topMargin: 40608
                    bottomMargin: 40609
                    radius: 40610
                    topLeftRadius: 40611
                    topRightRadius: 40612
                    bottomLeftRadius: 40613
                    bottomRightRadius: 40614
                    rotation: 40615
                    scale: 1.463
                    opacity: 0.461
                    color: "#721ede"
                    alignment: Qt.AlignRight | Qt.AlignBaseline
                    fillWidth: true
                    fillHeight: false
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "checked.indicator.foreground.gradient" }
                    data: QtObject { objectName: "checked.indicator.foreground.data" }
                    delegate: Component { Item { objectName: "checked.indicator.foreground.delegate" } }
                    border {
                        width: 40616
                        color: "#725039"
                    }
                    shadow {
                        color: "#728294"
                        opacity: 0.462
                        scale: 1.464
                        verticalOffset: 40617
                        horizontalOffset: 40618
                        blur: 40619
                        visible: false
                        delegate: Component { Item { objectName: "checked.indicator.foreground.shadow.delegate" } }
                    }
                    image {
                        source: "checked.indicator.foreground.png"
                        color: "#72b4ef"
                        fillMode: Image.PreserveAspectCrop
                    }
                }

                first {
                    width: 40701
                    height: 40702
                    minimumWidth: 40703
                    minimumHeight: 40704
                    margins: 40705
                    leftMargin: 40706
                    rightMargin: 40707
                    topMargin: 40708
                    bottomMargin: 40709
                    radius: 40710
                    topLeftRadius: 40711
                    topRightRadius: 40712
                    bottomLeftRadius: 40713
                    bottomRightRadius: 40714
                    rotation: 40715
                    scale: 1.473
                    opacity: 0.471
                    color: "#851e03"
                    alignment: Qt.AlignHCenter | Qt.AlignTop
                    fillWidth: false
                    fillHeight: true
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "checked.indicator.first.gradient" }
                    data: QtObject { objectName: "checked.indicator.first.data" }
                    delegate: Component { Item { objectName: "checked.indicator.first.delegate" } }
                    border {
                        width: 40716
                        color: "#85505e"
                    }
                    shadow {
                        color: "#8582b9"
                        opacity: 0.472
                        scale: 1.474
                        verticalOffset: 40717
                        horizontalOffset: 40718
                        blur: 40719
                        visible: false
                        delegate: Component { Item { objectName: "checked.indicator.first.shadow.delegate" } }
                    }
                    image {
                        source: "checked.indicator.first.png"
                        color: "#85b414"
                        fillMode: Image.Tile
                    }

                    foreground {
                        width: 40801
                        height: 40802
                        minimumWidth: 40803
                        minimumHeight: 40804
                        margins: 40805
                        leftMargin: 40806
                        rightMargin: 40807
                        topMargin: 40808
                        bottomMargin: 40809
                        radius: 40810
                        topLeftRadius: 40811
                        topRightRadius: 40812
                        bottomLeftRadius: 40813
                        bottomRightRadius: 40814
                        rotation: 40815
                        scale: 1.483
                        opacity: 0.481
                        color: "#981e28"
                        alignment: Qt.AlignJustify | Qt.AlignBottom
                        fillWidth: true
                        fillHeight: false
                        visible: true
                        clip: false
                        gradient: Gradient { objectName: "checked.indicator.first.foreground.gradient" }
                        data: QtObject { objectName: "checked.indicator.first.foreground.data" }
                        delegate: Component { Item { objectName: "checked.indicator.first.foreground.delegate" } }
                        border {
                            width: 40816
                            color: "#985083"
                        }
                        shadow {
                            color: "#9882de"
                            opacity: 0.482
                            scale: 1.484
                            verticalOffset: 40817
                            horizontalOffset: 40818
                            blur: 40819
                            visible: true
                            delegate: Component { Item { objectName: "checked.indicator.first.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "checked.indicator.first.foreground.png"
                            color: "#98b439"
                            fillMode: Image.TileVertically
                        }
                    }
                }

                second {
                    width: 40901
                    height: 40902
                    minimumWidth: 40903
                    minimumHeight: 40904
                    margins: 40905
                    leftMargin: 40906
                    rightMargin: 40907
                    topMargin: 40908
                    bottomMargin: 40909
                    radius: 40910
                    topLeftRadius: 40911
                    topRightRadius: 40912
                    bottomLeftRadius: 40913
                    bottomRightRadius: 40914
                    rotation: 40915
                    scale: 1.493
                    opacity: 0.491
                    color: "#ab1e4d"
                    alignment: Qt.AlignLeft | Qt.AlignBaseline
                    fillWidth: false
                    fillHeight: true
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "checked.indicator.second.gradient" }
                    data: QtObject { objectName: "checked.indicator.second.data" }
                    delegate: Component { Item { objectName: "checked.indicator.second.delegate" } }
                    border {
                        width: 40916
                        color: "#ab50a8"
                    }
                    shadow {
                        color: "#ab8203"
                        opacity: 0.492
                        scale: 1.494
                        verticalOffset: 40917
                        horizontalOffset: 40918
                        blur: 40919
                        visible: true
                        delegate: Component { Item { objectName: "checked.indicator.second.shadow.delegate" } }
                    }
                    image {
                        source: "checked.indicator.second.png"
                        color: "#abb45e"
                        fillMode: Image.TileHorizontally
                    }

                    foreground {
                        width: 41001
                        height: 41002
                        minimumWidth: 41003
                        minimumHeight: 41004
                        margins: 41005
                        leftMargin: 41006
                        rightMargin: 41007
                        topMargin: 41008
                        bottomMargin: 41009
                        radius: 41010
                        topLeftRadius: 41011
                        topRightRadius: 41012
                        bottomLeftRadius: 41013
                        bottomRightRadius: 41014
                        rotation: 41015
                        scale: 1.503
                        opacity: 0.501
                        color: "#be1e72"
                        alignment: Qt.AlignRight | Qt.AlignTop
                        fillWidth: true
                        fillHeight: false
                        visible: true
                        clip: false
                        gradient: Gradient { objectName: "checked.indicator.second.foreground.gradient" }
                        data: QtObject { objectName: "checked.indicator.second.foreground.data" }
                        delegate: Component { Item { objectName: "checked.indicator.second.foreground.delegate" } }
                        border {
                            width: 41016
                            color: "#be50cd"
                        }
                        shadow {
                            color: "#be8228"
                            opacity: 0.502
                            scale: 1.504
                            verticalOffset: 41017
                            horizontalOffset: 41018
                            blur: 41019
                            visible: false
                            delegate: Component { Item { objectName: "checked.indicator.second.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "checked.indicator.second.foreground.png"
                            color: "#beb483"
                            fillMode: Image.Pad
                        }
                    }
                }
            }

            text {
                color: "#d11e97"
                alignment: Qt.AlignHCenter | Qt.AlignBottom
                bold: false
                italic: true
                pointSize: 41101
                padding: 41102
                leftPadding: 41103
                rightPadding: 41104
                topPadding: 41105
                bottomPadding: 41106

                first {
                    color: "#e41ebc"
                    alignment: Qt.AlignJustify | Qt.AlignVCenter
                    bold: true
                    italic: false
                    pointSize: 41201
                    padding: 41202
                    leftPadding: 41203
                    rightPadding: 41204
                    topPadding: 41205
                    bottomPadding: 41206
                }

                second {
                    color: "#f71ee1"
                    alignment: Qt.AlignLeft | Qt.AlignTop
                    bold: false
                    italic: true
                    pointSize: 41301
                    padding: 41302
                    leftPadding: 41303
                    rightPadding: 41304
                    topPadding: 41305
                    bottomPadding: 41306
                }
            }
        }

        disabled {
            spacing: 50001
            padding: 50002
            leftPadding: 50003
            rightPadding: 50004
            topPadding: 50005
            bottomPadding: 50006
            transition: Transition { objectName: "disabled.transition" }

            background {
                width: 50101
                height: 50102
                minimumWidth: 50103
                minimumHeight: 50104
                margins: 50105
                leftMargin: 50106
                rightMargin: 50107
                topMargin: 50108
                bottomMargin: 50109
                radius: 50110
                topLeftRadius: 50111
                topRightRadius: 50112
                bottomLeftRadius: 50113
                bottomRightRadius: 50114
                rotation: 50115
                scale: 1.513
                opacity: 0.511
                color: "#132325"
                alignment: Qt.AlignRight | Qt.AlignVCenter
                fillWidth: true
                fillHeight: false
                visible: false
                clip: true
                gradient: Gradient { objectName: "disabled.background.gradient" }
                data: QtObject { objectName: "disabled.background.data" }
                delegate: Component { Item { objectName: "disabled.background.delegate" } }
                border {
                    width: 50116
                    color: "#135580"
                }
                shadow {
                    color: "#1387db"
                    opacity: 0.512
                    scale: 1.514
                    verticalOffset: 50117
                    horizontalOffset: 50118
                    blur: 50119
                    visible: false
                    delegate: Component { Item { objectName: "disabled.background.shadow.delegate" } }
                }
                image {
                    source: "disabled.background.png"
                    color: "#13b936"
                    fillMode: Image.TileHorizontally
                }
            }

            handle {
                width: 50201
                height: 50202
                minimumWidth: 50203
                minimumHeight: 50204
                margins: 50205
                leftMargin: 50206
                rightMargin: 50207
                topMargin: 50208
                bottomMargin: 50209
                radius: 50210
                topLeftRadius: 50211
                topRightRadius: 50212
                bottomLeftRadius: 50213
                bottomRightRadius: 50214
                rotation: 50215
                scale: 1.523
                opacity: 0.521
                color: "#26234a"
                alignment: Qt.AlignHCenter | Qt.AlignBaseline
                fillWidth: false
                fillHeight: true
                visible: false
                clip: true
                gradient: Gradient { objectName: "disabled.handle.gradient" }
                data: QtObject { objectName: "disabled.handle.data" }
                delegate: Component { Item { objectName: "disabled.handle.delegate" } }
                border {
                    width: 50216
                    color: "#2655a5"
                }
                shadow {
                    color: "#268700"
                    opacity: 0.522
                    scale: 1.524
                    verticalOffset: 50217
                    horizontalOffset: 50218
                    blur: 50219
                    visible: true
                    delegate: Component { Item { objectName: "disabled.handle.shadow.delegate" } }
                }
                image {
                    source: "disabled.handle.png"
                    color: "#26b95b"
                    fillMode: Image.Pad
                }

                first {
                    width: 50301
                    height: 50302
                    minimumWidth: 50303
                    minimumHeight: 50304
                    margins: 50305
                    leftMargin: 50306
                    rightMargin: 50307
                    topMargin: 50308
                    bottomMargin: 50309
                    radius: 50310
                    topLeftRadius: 50311
                    topRightRadius: 50312
                    bottomLeftRadius: 50313
                    bottomRightRadius: 50314
                    rotation: 50315
                    scale: 1.533
                    opacity: 0.531
                    color: "#39236f"
                    alignment: Qt.AlignJustify | Qt.AlignTop
                    fillWidth: true
                    fillHeight: false
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "disabled.handle.first.gradient" }
                    data: QtObject { objectName: "disabled.handle.first.data" }
                    delegate: Component { Item { objectName: "disabled.handle.first.delegate" } }
                    border {
                        width: 50316
                        color: "#3955ca"
                    }
                    shadow {
                        color: "#398725"
                        opacity: 0.532
                        scale: 1.534
                        verticalOffset: 50317
                        horizontalOffset: 50318
                        blur: 50319
                        visible: true
                        delegate: Component { Item { objectName: "disabled.handle.first.shadow.delegate" } }
                    }
                    image {
                        source: "disabled.handle.first.png"
                        color: "#39b980"
                        fillMode: Image.Stretch
                    }
                }

                second {
                    width: 50401
                    height: 50402
                    minimumWidth: 50403
                    minimumHeight: 50404
                    margins: 50405
                    leftMargin: 50406
                    rightMargin: 50407
                    topMargin: 50408
                    bottomMargin: 50409
                    radius: 50410
                    topLeftRadius: 50411
                    topRightRadius: 50412
                    bottomLeftRadius: 50413
                    bottomRightRadius: 50414
                    rotation: 50415
                    scale: 1.543
                    opacity: 0.541
                    color: "#4c2394"
                    alignment: Qt.AlignLeft | Qt.AlignVCenter
                    fillWidth: false
                    fillHeight: true
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "disabled.handle.second.gradient" }
                    data: QtObject { objectName: "disabled.handle.second.data" }
                    delegate: Component { Item { objectName: "disabled.handle.second.delegate" } }
                    border {
                        width: 50416
                        color: "#4c55ef"
                    }
                    shadow {
                        color: "#4c874a"
                        opacity: 0.542
                        scale: 1.544
                        verticalOffset: 50417
                        horizontalOffset: 50418
                        blur: 50419
                        visible: false
                        delegate: Component { Item { objectName: "disabled.handle.second.shadow.delegate" } }
                    }
                    image {
                        source: "disabled.handle.second.png"
                        color: "#4cb9a5"
                        fillMode: Image.PreserveAspectFit
                    }
                }
            }

            indicator {
                width: 50501
                height: 50502
                minimumWidth: 50503
                minimumHeight: 50504
                margins: 50505
                leftMargin: 50506
                rightMargin: 50507
                topMargin: 50508
                bottomMargin: 50509
                radius: 50510
                topLeftRadius: 50511
                topRightRadius: 50512
                bottomLeftRadius: 50513
                bottomRightRadius: 50514
                rotation: 50515
                scale: 1.553
                opacity: 0.551
                color: "#5f23b9"
                alignment: Qt.AlignRight | Qt.AlignBaseline
                fillWidth: true
                fillHeight: false
                visible: false
                clip: true
                gradient: Gradient { objectName: "disabled.indicator.gradient" }
                data: QtObject { objectName: "disabled.indicator.data" }
                delegate: Component { Item { objectName: "disabled.indicator.delegate" } }
                border {
                    width: 50516
                    color: "#5f5514"
                }
                shadow {
                    color: "#5f876f"
                    opacity: 0.552
                    scale: 1.554
                    verticalOffset: 50517
                    horizontalOffset: 50518
                    blur: 50519
                    visible: false
                    delegate: Component { Item { objectName: "disabled.indicator.shadow.delegate" } }
                }
                image {
                    source: "disabled.indicator.png"
                    color: "#5fb9ca"
                    fillMode: Image.PreserveAspectCrop
                }

                foreground {
                    width: 50601
                    height: 50602
                    minimumWidth: 50603
                    minimumHeight: 50604
                    margins: 50605
                    leftMargin: 50606
                    rightMargin: 50607
                    topMargin: 50608
                    bottomMargin: 50609
                    radius: 50610
                    topLeftRadius: 50611
                    topRightRadius: 50612
                    bottomLeftRadius: 50613
                    bottomRightRadius: 50614
                    rotation: 50615
                    scale: 1.563
                    opacity: 0.561
                    color: "#7223de"
                    alignment: Qt.AlignHCenter | Qt.AlignTop
                    fillWidth: false
                    fillHeight: true
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "disabled.indicator.foreground.gradient" }
                    data: QtObject { objectName: "disabled.indicator.foreground.data" }
                    delegate: Component { Item { objectName: "disabled.indicator.foreground.delegate" } }
                    border {
                        width: 50616
                        color: "#725539"
                    }
                    shadow {
                        color: "#728794"
                        opacity: 0.562
                        scale: 1.564
                        verticalOffset: 50617
                        horizontalOffset: 50618
                        blur: 50619
                        visible: true
                        delegate: Component { Item { objectName: "disabled.indicator.foreground.shadow.delegate" } }
                    }
                    image {
                        source: "disabled.indicator.foreground.png"
                        color: "#72b9ef"
                        fillMode: Image.Tile
                    }
                }

                first {
                    width: 50701
                    height: 50702
                    minimumWidth: 50703
                    minimumHeight: 50704
                    margins: 50705
                    leftMargin: 50706
                    rightMargin: 50707
                    topMargin: 50708
                    bottomMargin: 50709
                    radius: 50710
                    topLeftRadius: 50711
                    topRightRadius: 50712
                    bottomLeftRadius: 50713
                    bottomRightRadius: 50714
                    rotation: 50715
                    scale: 1.573
                    opacity: 0.571
                    color: "#852303"
                    alignment: Qt.AlignJustify | Qt.AlignBottom
                    fillWidth: true
                    fillHeight: false
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "disabled.indicator.first.gradient" }
                    data: QtObject { objectName: "disabled.indicator.first.data" }
                    delegate: Component { Item { objectName: "disabled.indicator.first.delegate" } }
                    border {
                        width: 50716
                        color: "#85555e"
                    }
                    shadow {
                        color: "#8587b9"
                        opacity: 0.572
                        scale: 1.574
                        verticalOffset: 50717
                        horizontalOffset: 50718
                        blur: 50719
                        visible: true
                        delegate: Component { Item { objectName: "disabled.indicator.first.shadow.delegate" } }
                    }
                    image {
                        source: "disabled.indicator.first.png"
                        color: "#85b914"
                        fillMode: Image.TileVertically
                    }

                    foreground {
                        width: 50801
                        height: 50802
                        minimumWidth: 50803
                        minimumHeight: 50804
                        margins: 50805
                        leftMargin: 50806
                        rightMargin: 50807
                        topMargin: 50808
                        bottomMargin: 50809
                        radius: 50810
                        topLeftRadius: 50811
                        topRightRadius: 50812
                        bottomLeftRadius: 50813
                        bottomRightRadius: 50814
                        rotation: 50815
                        scale: 1.583
                        opacity: 0.581
                        color: "#982328"
                        alignment: Qt.AlignLeft | Qt.AlignBaseline
                        fillWidth: false
                        fillHeight: true
                        visible: false
                        clip: true
                        gradient: Gradient { objectName: "disabled.indicator.first.foreground.gradient" }
                        data: QtObject { objectName: "disabled.indicator.first.foreground.data" }
                        delegate: Component { Item { objectName: "disabled.indicator.first.foreground.delegate" } }
                        border {
                            width: 50816
                            color: "#985583"
                        }
                        shadow {
                            color: "#9887de"
                            opacity: 0.582
                            scale: 1.584
                            verticalOffset: 50817
                            horizontalOffset: 50818
                            blur: 50819
                            visible: false
                            delegate: Component { Item { objectName: "disabled.indicator.first.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "disabled.indicator.first.foreground.png"
                            color: "#98b939"
                            fillMode: Image.TileHorizontally
                        }
                    }
                }

                second {
                    width: 50901
                    height: 50902
                    minimumWidth: 50903
                    minimumHeight: 50904
                    margins: 50905
                    leftMargin: 50906
                    rightMargin: 50907
                    topMargin: 50908
                    bottomMargin: 50909
                    radius: 50910
                    topLeftRadius: 50911
                    topRightRadius: 50912
                    bottomLeftRadius: 50913
                    bottomRightRadius: 50914
                    rotation: 50915
                    scale: 1.593
                    opacity: 0.591
                    color: "#ab234d"
                    alignment: Qt.AlignRight | Qt.AlignTop
                    fillWidth: true
                    fillHeight: false
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "disabled.indicator.second.gradient" }
                    data: QtObject { objectName: "disabled.indicator.second.data" }
                    delegate: Component { Item { objectName: "disabled.indicator.second.delegate" } }
                    border {
                        width: 50916
                        color: "#ab55a8"
                    }
                    shadow {
                        color: "#ab8703"
                        opacity: 0.592
                        scale: 1.594
                        verticalOffset: 50917
                        horizontalOffset: 50918
                        blur: 50919
                        visible: false
                        delegate: Component { Item { objectName: "disabled.indicator.second.shadow.delegate" } }
                    }
                    image {
                        source: "disabled.indicator.second.png"
                        color: "#abb95e"
                        fillMode: Image.Pad
                    }

                    foreground {
                        width: 51001
                        height: 51002
                        minimumWidth: 51003
                        minimumHeight: 51004
                        margins: 51005
                        leftMargin: 51006
                        rightMargin: 51007
                        topMargin: 51008
                        bottomMargin: 51009
                        radius: 51010
                        topLeftRadius: 51011
                        topRightRadius: 51012
                        bottomLeftRadius: 51013
                        bottomRightRadius: 51014
                        rotation: 51015
                        scale: 1.603
                        opacity: 0.601
                        color: "#be2372"
                        alignment: Qt.AlignHCenter | Qt.AlignBottom
                        fillWidth: false
                        fillHeight: true
                        visible: false
                        clip: true
                        gradient: Gradient { objectName: "disabled.indicator.second.foreground.gradient" }
                        data: QtObject { objectName: "disabled.indicator.second.foreground.data" }
                        delegate: Component { Item { objectName: "disabled.indicator.second.foreground.delegate" } }
                        border {
                            width: 51016
                            color: "#be55cd"
                        }
                        shadow {
                            color: "#be8728"
                            opacity: 0.602
                            scale: 1.604
                            verticalOffset: 51017
                            horizontalOffset: 51018
                            blur: 51019
                            visible: true
                            delegate: Component { Item { objectName: "disabled.indicator.second.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "disabled.indicator.second.foreground.png"
                            color: "#beb983"
                            fillMode: Image.Stretch
                        }
                    }
                }
            }

            text {
                color: "#d12397"
                alignment: Qt.AlignJustify | Qt.AlignVCenter
                bold: true
                italic: false
                pointSize: 51101
                padding: 51102
                leftPadding: 51103
                rightPadding: 51104
                topPadding: 51105
                bottomPadding: 51106

                first {
                    color: "#e423bc"
                    alignment: Qt.AlignLeft | Qt.AlignTop
                    bold: false
                    italic: true
                    pointSize: 51201
                    padding: 51202
                    leftPadding: 51203
                    rightPadding: 51204
                    topPadding: 51205
                    bottomPadding: 51206
                }

                second {
                    color: "#f723e1"
                    alignment: Qt.AlignRight | Qt.AlignBottom
                    bold: true
                    italic: false
                    pointSize: 51301
                    padding: 51302
                    leftPadding: 51303
                    rightPadding: 51304
                    topPadding: 51305
                    bottomPadding: 51306
                }
            }
        }

        highlighted {
            spacing: 60001
            padding: 60002
            leftPadding: 60003
            rightPadding: 60004
            topPadding: 60005
            bottomPadding: 60006
            transition: Transition { objectName: "highlighted.transition" }

            background {
                width: 60101
                height: 60102
                minimumWidth: 60103
                minimumHeight: 60104
                margins: 60105
                leftMargin: 60106
                rightMargin: 60107
                topMargin: 60108
                bottomMargin: 60109
                radius: 60110
                topLeftRadius: 60111
                topRightRadius: 60112
                bottomLeftRadius: 60113
                bottomRightRadius: 60114
                rotation: 60115
                scale: 1.613
                opacity: 0.611
                color: "#132825"
                alignment: Qt.AlignHCenter | Qt.AlignBaseline
                fillWidth: false
                fillHeight: true
                visible: true
                clip: false
                gradient: Gradient { objectName: "highlighted.background.gradient" }
                data: QtObject { objectName: "highlighted.background.data" }
                delegate: Component { Item { objectName: "highlighted.background.delegate" } }
                border {
                    width: 60116
                    color: "#135a80"
                }
                shadow {
                    color: "#138cdb"
                    opacity: 0.612
                    scale: 1.614
                    verticalOffset: 60117
                    horizontalOffset: 60118
                    blur: 60119
                    visible: true
                    delegate: Component { Item { objectName: "highlighted.background.shadow.delegate" } }
                }
                image {
                    source: "highlighted.background.png"
                    color: "#13be36"
                    fillMode: Image.Pad
                }
            }

            handle {
                width: 60201
                height: 60202
                minimumWidth: 60203
                minimumHeight: 60204
                margins: 60205
                leftMargin: 60206
                rightMargin: 60207
                topMargin: 60208
                bottomMargin: 60209
                radius: 60210
                topLeftRadius: 60211
                topRightRadius: 60212
                bottomLeftRadius: 60213
                bottomRightRadius: 60214
                rotation: 60215
                scale: 1.623
                opacity: 0.621
                color: "#26284a"
                alignment: Qt.AlignJustify | Qt.AlignTop
                fillWidth: true
                fillHeight: false
                visible: true
                clip: false
                gradient: Gradient { objectName: "highlighted.handle.gradient" }
                data: QtObject { objectName: "highlighted.handle.data" }
                delegate: Component { Item { objectName: "highlighted.handle.delegate" } }
                border {
                    width: 60216
                    color: "#265aa5"
                }
                shadow {
                    color: "#268c00"
                    opacity: 0.622
                    scale: 1.624
                    verticalOffset: 60217
                    horizontalOffset: 60218
                    blur: 60219
                    visible: false
                    delegate: Component { Item { objectName: "highlighted.handle.shadow.delegate" } }
                }
                image {
                    source: "highlighted.handle.png"
                    color: "#26be5b"
                    fillMode: Image.Stretch
                }

                first {
                    width: 60301
                    height: 60302
                    minimumWidth: 60303
                    minimumHeight: 60304
                    margins: 60305
                    leftMargin: 60306
                    rightMargin: 60307
                    topMargin: 60308
                    bottomMargin: 60309
                    radius: 60310
                    topLeftRadius: 60311
                    topRightRadius: 60312
                    bottomLeftRadius: 60313
                    bottomRightRadius: 60314
                    rotation: 60315
                    scale: 1.633
                    opacity: 0.631
                    color: "#39286f"
                    alignment: Qt.AlignLeft | Qt.AlignVCenter
                    fillWidth: false
                    fillHeight: true
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "highlighted.handle.first.gradient" }
                    data: QtObject { objectName: "highlighted.handle.first.data" }
                    delegate: Component { Item { objectName: "highlighted.handle.first.delegate" } }
                    border {
                        width: 60316
                        color: "#395aca"
                    }
                    shadow {
                        color: "#398c25"
                        opacity: 0.632
                        scale: 1.634
                        verticalOffset: 60317
                        horizontalOffset: 60318
                        blur: 60319
                        visible: false
                        delegate: Component { Item { objectName: "highlighted.handle.first.shadow.delegate" } }
                    }
                    image {
                        source: "highlighted.handle.first.png"
                        color: "#39be80"
                        fillMode: Image.PreserveAspectFit
                    }
                }

                second {
                    width: 60401
                    height: 60402
                    minimumWidth: 60403
                    minimumHeight: 60404
                    margins: 60405
                    leftMargin: 60406
                    rightMargin: 60407
                    topMargin: 60408
                    bottomMargin: 60409
                    radius: 60410
                    topLeftRadius: 60411
                    topRightRadius: 60412
                    bottomLeftRadius: 60413
                    bottomRightRadius: 60414
                    rotation: 60415
                    scale: 1.643
                    opacity: 0.641
                    color: "#4c2894"
                    alignment: Qt.AlignRight | Qt.AlignBaseline
                    fillWidth: true
                    fillHeight: false
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "highlighted.handle.second.gradient" }
                    data: QtObject { objectName: "highlighted.handle.second.data" }
                    delegate: Component { Item { objectName: "highlighted.handle.second.delegate" } }
                    border {
                        width: 60416
                        color: "#4c5aef"
                    }
                    shadow {
                        color: "#4c8c4a"
                        opacity: 0.642
                        scale: 1.644
                        verticalOffset: 60417
                        horizontalOffset: 60418
                        blur: 60419
                        visible: true
                        delegate: Component { Item { objectName: "highlighted.handle.second.shadow.delegate" } }
                    }
                    image {
                        source: "highlighted.handle.second.png"
                        color: "#4cbea5"
                        fillMode: Image.PreserveAspectCrop
                    }
                }
            }

            indicator {
                width: 60501
                height: 60502
                minimumWidth: 60503
                minimumHeight: 60504
                margins: 60505
                leftMargin: 60506
                rightMargin: 60507
                topMargin: 60508
                bottomMargin: 60509
                radius: 60510
                topLeftRadius: 60511
                topRightRadius: 60512
                bottomLeftRadius: 60513
                bottomRightRadius: 60514
                rotation: 60515
                scale: 1.653
                opacity: 0.651
                color: "#5f28b9"
                alignment: Qt.AlignHCenter | Qt.AlignTop
                fillWidth: false
                fillHeight: true
                visible: true
                clip: false
                gradient: Gradient { objectName: "highlighted.indicator.gradient" }
                data: QtObject { objectName: "highlighted.indicator.data" }
                delegate: Component { Item { objectName: "highlighted.indicator.delegate" } }
                border {
                    width: 60516
                    color: "#5f5a14"
                }
                shadow {
                    color: "#5f8c6f"
                    opacity: 0.652
                    scale: 1.654
                    verticalOffset: 60517
                    horizontalOffset: 60518
                    blur: 60519
                    visible: true
                    delegate: Component { Item { objectName: "highlighted.indicator.shadow.delegate" } }
                }
                image {
                    source: "highlighted.indicator.png"
                    color: "#5fbeca"
                    fillMode: Image.Tile
                }

                foreground {
                    width: 60601
                    height: 60602
                    minimumWidth: 60603
                    minimumHeight: 60604
                    margins: 60605
                    leftMargin: 60606
                    rightMargin: 60607
                    topMargin: 60608
                    bottomMargin: 60609
                    radius: 60610
                    topLeftRadius: 60611
                    topRightRadius: 60612
                    bottomLeftRadius: 60613
                    bottomRightRadius: 60614
                    rotation: 60615
                    scale: 1.663
                    opacity: 0.661
                    color: "#7228de"
                    alignment: Qt.AlignJustify | Qt.AlignBottom
                    fillWidth: true
                    fillHeight: false
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "highlighted.indicator.foreground.gradient" }
                    data: QtObject { objectName: "highlighted.indicator.foreground.data" }
                    delegate: Component { Item { objectName: "highlighted.indicator.foreground.delegate" } }
                    border {
                        width: 60616
                        color: "#725a39"
                    }
                    shadow {
                        color: "#728c94"
                        opacity: 0.662
                        scale: 1.664
                        verticalOffset: 60617
                        horizontalOffset: 60618
                        blur: 60619
                        visible: false
                        delegate: Component { Item { objectName: "highlighted.indicator.foreground.shadow.delegate" } }
                    }
                    image {
                        source: "highlighted.indicator.foreground.png"
                        color: "#72beef"
                        fillMode: Image.TileVertically
                    }
                }

                first {
                    width: 60701
                    height: 60702
                    minimumWidth: 60703
                    minimumHeight: 60704
                    margins: 60705
                    leftMargin: 60706
                    rightMargin: 60707
                    topMargin: 60708
                    bottomMargin: 60709
                    radius: 60710
                    topLeftRadius: 60711
                    topRightRadius: 60712
                    bottomLeftRadius: 60713
                    bottomRightRadius: 60714
                    rotation: 60715
                    scale: 1.673
                    opacity: 0.671
                    color: "#852803"
                    alignment: Qt.AlignLeft | Qt.AlignBaseline
                    fillWidth: false
                    fillHeight: true
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "highlighted.indicator.first.gradient" }
                    data: QtObject { objectName: "highlighted.indicator.first.data" }
                    delegate: Component { Item { objectName: "highlighted.indicator.first.delegate" } }
                    border {
                        width: 60716
                        color: "#855a5e"
                    }
                    shadow {
                        color: "#858cb9"
                        opacity: 0.672
                        scale: 1.674
                        verticalOffset: 60717
                        horizontalOffset: 60718
                        blur: 60719
                        visible: false
                        delegate: Component { Item { objectName: "highlighted.indicator.first.shadow.delegate" } }
                    }
                    image {
                        source: "highlighted.indicator.first.png"
                        color: "#85be14"
                        fillMode: Image.TileHorizontally
                    }

                    foreground {
                        width: 60801
                        height: 60802
                        minimumWidth: 60803
                        minimumHeight: 60804
                        margins: 60805
                        leftMargin: 60806
                        rightMargin: 60807
                        topMargin: 60808
                        bottomMargin: 60809
                        radius: 60810
                        topLeftRadius: 60811
                        topRightRadius: 60812
                        bottomLeftRadius: 60813
                        bottomRightRadius: 60814
                        rotation: 60815
                        scale: 1.683
                        opacity: 0.681
                        color: "#982828"
                        alignment: Qt.AlignRight | Qt.AlignTop
                        fillWidth: true
                        fillHeight: false
                        visible: true
                        clip: false
                        gradient: Gradient { objectName: "highlighted.indicator.first.foreground.gradient" }
                        data: QtObject { objectName: "highlighted.indicator.first.foreground.data" }
                        delegate: Component { Item { objectName: "highlighted.indicator.first.foreground.delegate" } }
                        border {
                            width: 60816
                            color: "#985a83"
                        }
                        shadow {
                            color: "#988cde"
                            opacity: 0.682
                            scale: 1.684
                            verticalOffset: 60817
                            horizontalOffset: 60818
                            blur: 60819
                            visible: true
                            delegate: Component { Item { objectName: "highlighted.indicator.first.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "highlighted.indicator.first.foreground.png"
                            color: "#98be39"
                            fillMode: Image.Pad
                        }
                    }
                }

                second {
                    width: 60901
                    height: 60902
                    minimumWidth: 60903
                    minimumHeight: 60904
                    margins: 60905
                    leftMargin: 60906
                    rightMargin: 60907
                    topMargin: 60908
                    bottomMargin: 60909
                    radius: 60910
                    topLeftRadius: 60911
                    topRightRadius: 60912
                    bottomLeftRadius: 60913
                    bottomRightRadius: 60914
                    rotation: 60915
                    scale: 1.693
                    opacity: 0.691
                    color: "#ab284d"
                    alignment: Qt.AlignHCenter | Qt.AlignBottom
                    fillWidth: false
                    fillHeight: true
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "highlighted.indicator.second.gradient" }
                    data: QtObject { objectName: "highlighted.indicator.second.data" }
                    delegate: Component { Item { objectName: "highlighted.indicator.second.delegate" } }
                    border {
                        width: 60916
                        color: "#ab5aa8"
                    }
                    shadow {
                        color: "#ab8c03"
                        opacity: 0.692
                        scale: 1.694
                        verticalOffset: 60917
                        horizontalOffset: 60918
                        blur: 60919
                        visible: true
                        delegate: Component { Item { objectName: "highlighted.indicator.second.shadow.delegate" } }
                    }
                    image {
                        source: "highlighted.indicator.second.png"
                        color: "#abbe5e"
                        fillMode: Image.Stretch
                    }

                    foreground {
                        width: 61001
                        height: 61002
                        minimumWidth: 61003
                        minimumHeight: 61004
                        margins: 61005
                        leftMargin: 61006
                        rightMargin: 61007
                        topMargin: 61008
                        bottomMargin: 61009
                        radius: 61010
                        topLeftRadius: 61011
                        topRightRadius: 61012
                        bottomLeftRadius: 61013
                        bottomRightRadius: 61014
                        rotation: 61015
                        scale: 1.703
                        opacity: 0.701
                        color: "#be2872"
                        alignment: Qt.AlignJustify | Qt.AlignVCenter
                        fillWidth: true
                        fillHeight: false
                        visible: true
                        clip: false
                        gradient: Gradient { objectName: "highlighted.indicator.second.foreground.gradient" }
                        data: QtObject { objectName: "highlighted.indicator.second.foreground.data" }
                        delegate: Component { Item { objectName: "highlighted.indicator.second.foreground.delegate" } }
                        border {
                            width: 61016
                            color: "#be5acd"
                        }
                        shadow {
                            color: "#be8c28"
                            opacity: 0.702
                            scale: 1.704
                            verticalOffset: 61017
                            horizontalOffset: 61018
                            blur: 61019
                            visible: false
                            delegate: Component { Item { objectName: "highlighted.indicator.second.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "highlighted.indicator.second.foreground.png"
                            color: "#bebe83"
                            fillMode: Image.PreserveAspectFit
                        }
                    }
                }
            }

            text {
                color: "#d12897"
                alignment: Qt.AlignLeft | Qt.AlignTop
                bold: false
                italic: true
                pointSize: 61101
                padding: 61102
                leftPadding: 61103
                rightPadding: 61104
                topPadding: 61105
                bottomPadding: 61106

                first {
                    color: "#e428bc"
                    alignment: Qt.AlignRight | Qt.AlignBottom
                    bold: true
                    italic: false
                    pointSize: 61201
                    padding: 61202
                    leftPadding: 61203
                    rightPadding: 61204
                    topPadding: 61205
                    bottomPadding: 61206
                }

                second {
                    color: "#f728e1"
                    alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    bold: false
                    italic: true
                    pointSize: 61301
                    padding: 61302
                    leftPadding: 61303
                    rightPadding: 61304
                    topPadding: 61305
                    bottomPadding: 61306
                }
            }
        }

        vertical {
            spacing: 70001
            padding: 70002
            leftPadding: 70003
            rightPadding: 70004
            topPadding: 70005
            bottomPadding: 70006
            transition: Transition { objectName: "vertical.transition" }

            background {
                width: 70101
                height: 70102
                minimumWidth: 70103
                minimumHeight: 70104
                margins: 70105
                leftMargin: 70106
                rightMargin: 70107
                topMargin: 70108
                bottomMargin: 70109
                radius: 70110
                topLeftRadius: 70111
                topRightRadius: 70112
                bottomLeftRadius: 70113
                bottomRightRadius: 70114
                rotation: 70115
                scale: 1.713
                opacity: 0.711
                color: "#132d25"
                alignment: Qt.AlignJustify | Qt.AlignTop
                fillWidth: true
                fillHeight: false
                visible: false
                clip: true
                gradient: Gradient { objectName: "vertical.background.gradient" }
                data: QtObject { objectName: "vertical.background.data" }
                delegate: Component { Item { objectName: "vertical.background.delegate" } }
                border {
                    width: 70116
                    color: "#135f80"
                }
                shadow {
                    color: "#1391db"
                    opacity: 0.712
                    scale: 1.714
                    verticalOffset: 70117
                    horizontalOffset: 70118
                    blur: 70119
                    visible: false
                    delegate: Component { Item { objectName: "vertical.background.shadow.delegate" } }
                }
                image {
                    source: "vertical.background.png"
                    color: "#13c336"
                    fillMode: Image.Stretch
                }
            }

            handle {
                width: 70201
                height: 70202
                minimumWidth: 70203
                minimumHeight: 70204
                margins: 70205
                leftMargin: 70206
                rightMargin: 70207
                topMargin: 70208
                bottomMargin: 70209
                radius: 70210
                topLeftRadius: 70211
                topRightRadius: 70212
                bottomLeftRadius: 70213
                bottomRightRadius: 70214
                rotation: 70215
                scale: 1.723
                opacity: 0.721
                color: "#262d4a"
                alignment: Qt.AlignLeft | Qt.AlignVCenter
                fillWidth: false
                fillHeight: true
                visible: false
                clip: true
                gradient: Gradient { objectName: "vertical.handle.gradient" }
                data: QtObject { objectName: "vertical.handle.data" }
                delegate: Component { Item { objectName: "vertical.handle.delegate" } }
                border {
                    width: 70216
                    color: "#265fa5"
                }
                shadow {
                    color: "#269100"
                    opacity: 0.722
                    scale: 1.724
                    verticalOffset: 70217
                    horizontalOffset: 70218
                    blur: 70219
                    visible: true
                    delegate: Component { Item { objectName: "vertical.handle.shadow.delegate" } }
                }
                image {
                    source: "vertical.handle.png"
                    color: "#26c35b"
                    fillMode: Image.PreserveAspectFit
                }

                first {
                    width: 70301
                    height: 70302
                    minimumWidth: 70303
                    minimumHeight: 70304
                    margins: 70305
                    leftMargin: 70306
                    rightMargin: 70307
                    topMargin: 70308
                    bottomMargin: 70309
                    radius: 70310
                    topLeftRadius: 70311
                    topRightRadius: 70312
                    bottomLeftRadius: 70313
                    bottomRightRadius: 70314
                    rotation: 70315
                    scale: 1.733
                    opacity: 0.731
                    color: "#392d6f"
                    alignment: Qt.AlignRight | Qt.AlignBaseline
                    fillWidth: true
                    fillHeight: false
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "vertical.handle.first.gradient" }
                    data: QtObject { objectName: "vertical.handle.first.data" }
                    delegate: Component { Item { objectName: "vertical.handle.first.delegate" } }
                    border {
                        width: 70316
                        color: "#395fca"
                    }
                    shadow {
                        color: "#399125"
                        opacity: 0.732
                        scale: 1.734
                        verticalOffset: 70317
                        horizontalOffset: 70318
                        blur: 70319
                        visible: true
                        delegate: Component { Item { objectName: "vertical.handle.first.shadow.delegate" } }
                    }
                    image {
                        source: "vertical.handle.first.png"
                        color: "#39c380"
                        fillMode: Image.PreserveAspectCrop
                    }
                }

                second {
                    width: 70401
                    height: 70402
                    minimumWidth: 70403
                    minimumHeight: 70404
                    margins: 70405
                    leftMargin: 70406
                    rightMargin: 70407
                    topMargin: 70408
                    bottomMargin: 70409
                    radius: 70410
                    topLeftRadius: 70411
                    topRightRadius: 70412
                    bottomLeftRadius: 70413
                    bottomRightRadius: 70414
                    rotation: 70415
                    scale: 1.743
                    opacity: 0.741
                    color: "#4c2d94"
                    alignment: Qt.AlignHCenter | Qt.AlignTop
                    fillWidth: false
                    fillHeight: true
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "vertical.handle.second.gradient" }
                    data: QtObject { objectName: "vertical.handle.second.data" }
                    delegate: Component { Item { objectName: "vertical.handle.second.delegate" } }
                    border {
                        width: 70416
                        color: "#4c5fef"
                    }
                    shadow {
                        color: "#4c914a"
                        opacity: 0.742
                        scale: 1.744
                        verticalOffset: 70417
                        horizontalOffset: 70418
                        blur: 70419
                        visible: false
                        delegate: Component { Item { objectName: "vertical.handle.second.shadow.delegate" } }
                    }
                    image {
                        source: "vertical.handle.second.png"
                        color: "#4cc3a5"
                        fillMode: Image.Tile
                    }
                }
            }

            indicator {
                width: 70501
                height: 70502
                minimumWidth: 70503
                minimumHeight: 70504
                margins: 70505
                leftMargin: 70506
                rightMargin: 70507
                topMargin: 70508
                bottomMargin: 70509
                radius: 70510
                topLeftRadius: 70511
                topRightRadius: 70512
                bottomLeftRadius: 70513
                bottomRightRadius: 70514
                rotation: 70515
                scale: 1.753
                opacity: 0.751
                color: "#5f2db9"
                alignment: Qt.AlignJustify | Qt.AlignBottom
                fillWidth: true
                fillHeight: false
                visible: false
                clip: true
                gradient: Gradient { objectName: "vertical.indicator.gradient" }
                data: QtObject { objectName: "vertical.indicator.data" }
                delegate: Component { Item { objectName: "vertical.indicator.delegate" } }
                border {
                    width: 70516
                    color: "#5f5f14"
                }
                shadow {
                    color: "#5f916f"
                    opacity: 0.752
                    scale: 1.754
                    verticalOffset: 70517
                    horizontalOffset: 70518
                    blur: 70519
                    visible: false
                    delegate: Component { Item { objectName: "vertical.indicator.shadow.delegate" } }
                }
                image {
                    source: "vertical.indicator.png"
                    color: "#5fc3ca"
                    fillMode: Image.TileVertically
                }

                foreground {
                    width: 70601
                    height: 70602
                    minimumWidth: 70603
                    minimumHeight: 70604
                    margins: 70605
                    leftMargin: 70606
                    rightMargin: 70607
                    topMargin: 70608
                    bottomMargin: 70609
                    radius: 70610
                    topLeftRadius: 70611
                    topRightRadius: 70612
                    bottomLeftRadius: 70613
                    bottomRightRadius: 70614
                    rotation: 70615
                    scale: 1.763
                    opacity: 0.761
                    color: "#722dde"
                    alignment: Qt.AlignLeft | Qt.AlignBaseline
                    fillWidth: false
                    fillHeight: true
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "vertical.indicator.foreground.gradient" }
                    data: QtObject { objectName: "vertical.indicator.foreground.data" }
                    delegate: Component { Item { objectName: "vertical.indicator.foreground.delegate" } }
                    border {
                        width: 70616
                        color: "#725f39"
                    }
                    shadow {
                        color: "#729194"
                        opacity: 0.762
                        scale: 1.764
                        verticalOffset: 70617
                        horizontalOffset: 70618
                        blur: 70619
                        visible: true
                        delegate: Component { Item { objectName: "vertical.indicator.foreground.shadow.delegate" } }
                    }
                    image {
                        source: "vertical.indicator.foreground.png"
                        color: "#72c3ef"
                        fillMode: Image.TileHorizontally
                    }
                }

                first {
                    width: 70701
                    height: 70702
                    minimumWidth: 70703
                    minimumHeight: 70704
                    margins: 70705
                    leftMargin: 70706
                    rightMargin: 70707
                    topMargin: 70708
                    bottomMargin: 70709
                    radius: 70710
                    topLeftRadius: 70711
                    topRightRadius: 70712
                    bottomLeftRadius: 70713
                    bottomRightRadius: 70714
                    rotation: 70715
                    scale: 1.773
                    opacity: 0.771
                    color: "#852d03"
                    alignment: Qt.AlignRight | Qt.AlignTop
                    fillWidth: true
                    fillHeight: false
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "vertical.indicator.first.gradient" }
                    data: QtObject { objectName: "vertical.indicator.first.data" }
                    delegate: Component { Item { objectName: "vertical.indicator.first.delegate" } }
                    border {
                        width: 70716
                        color: "#855f5e"
                    }
                    shadow {
                        color: "#8591b9"
                        opacity: 0.772
                        scale: 1.774
                        verticalOffset: 70717
                        horizontalOffset: 70718
                        blur: 70719
                        visible: true
                        delegate: Component { Item { objectName: "vertical.indicator.first.shadow.delegate" } }
                    }
                    image {
                        source: "vertical.indicator.first.png"
                        color: "#85c314"
                        fillMode: Image.Pad
                    }

                    foreground {
                        width: 70801
                        height: 70802
                        minimumWidth: 70803
                        minimumHeight: 70804
                        margins: 70805
                        leftMargin: 70806
                        rightMargin: 70807
                        topMargin: 70808
                        bottomMargin: 70809
                        radius: 70810
                        topLeftRadius: 70811
                        topRightRadius: 70812
                        bottomLeftRadius: 70813
                        bottomRightRadius: 70814
                        rotation: 70815
                        scale: 1.783
                        opacity: 0.781
                        color: "#982d28"
                        alignment: Qt.AlignHCenter | Qt.AlignBottom
                        fillWidth: false
                        fillHeight: true
                        visible: false
                        clip: true
                        gradient: Gradient { objectName: "vertical.indicator.first.foreground.gradient" }
                        data: QtObject { objectName: "vertical.indicator.first.foreground.data" }
                        delegate: Component { Item { objectName: "vertical.indicator.first.foreground.delegate" } }
                        border {
                            width: 70816
                            color: "#985f83"
                        }
                        shadow {
                            color: "#9891de"
                            opacity: 0.782
                            scale: 1.784
                            verticalOffset: 70817
                            horizontalOffset: 70818
                            blur: 70819
                            visible: false
                            delegate: Component { Item { objectName: "vertical.indicator.first.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "vertical.indicator.first.foreground.png"
                            color: "#98c339"
                            fillMode: Image.Stretch
                        }
                    }
                }

                second {
                    width: 70901
                    height: 70902
                    minimumWidth: 70903
                    minimumHeight: 70904
                    margins: 70905
                    leftMargin: 70906
                    rightMargin: 70907
                    topMargin: 70908
                    bottomMargin: 70909
                    radius: 70910
                    topLeftRadius: 70911
                    topRightRadius: 70912
                    bottomLeftRadius: 70913
                    bottomRightRadius: 70914
                    rotation: 70915
                    scale: 1.793
                    opacity: 0.791
                    color: "#ab2d4d"
                    alignment: Qt.AlignJustify | Qt.AlignVCenter
                    fillWidth: true
                    fillHeight: false
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "vertical.indicator.second.gradient" }
                    data: QtObject { objectName: "vertical.indicator.second.data" }
                    delegate: Component { Item { objectName: "vertical.indicator.second.delegate" } }
                    border {
                        width: 70916
                        color: "#ab5fa8"
                    }
                    shadow {
                        color: "#ab9103"
                        opacity: 0.792
                        scale: 1.794
                        verticalOffset: 70917
                        horizontalOffset: 70918
                        blur: 70919
                        visible: false
                        delegate: Component { Item { objectName: "vertical.indicator.second.shadow.delegate" } }
                    }
                    image {
                        source: "vertical.indicator.second.png"
                        color: "#abc35e"
                        fillMode: Image.PreserveAspectFit
                    }

                    foreground {
                        width: 71001
                        height: 71002
                        minimumWidth: 71003
                        minimumHeight: 71004
                        margins: 71005
                        leftMargin: 71006
                        rightMargin: 71007
                        topMargin: 71008
                        bottomMargin: 71009
                        radius: 71010
                        topLeftRadius: 71011
                        topRightRadius: 71012
                        bottomLeftRadius: 71013
                        bottomRightRadius: 71014
                        rotation: 71015
                        scale: 1.803
                        opacity: 0.801
                        color: "#be2d72"
                        alignment: Qt.AlignLeft | Qt.AlignTop
                        fillWidth: false
                        fillHeight: true
                        visible: false
                        clip: true
                        gradient: Gradient { objectName: "vertical.indicator.second.foreground.gradient" }
                        data: QtObject { objectName: "vertical.indicator.second.foreground.data" }
                        delegate: Component { Item { objectName: "vertical.indicator.second.foreground.delegate" } }
                        border {
                            width: 71016
                            color: "#be5fcd"
                        }
                        shadow {
                            color: "#be9128"
                            opacity: 0.802
                            scale: 1.804
                            verticalOffset: 71017
                            horizontalOffset: 71018
                            blur: 71019
                            visible: true
                            delegate: Component { Item { objectName: "vertical.indicator.second.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "vertical.indicator.second.foreground.png"
                            color: "#bec383"
                            fillMode: Image.PreserveAspectCrop
                        }
                    }
                }
            }

            text {
                color: "#d12d97"
                alignment: Qt.AlignRight | Qt.AlignBottom
                bold: true
                italic: false
                pointSize: 71101
                padding: 71102
                leftPadding: 71103
                rightPadding: 71104
                topPadding: 71105
                bottomPadding: 71106

                first {
                    color: "#e42dbc"
                    alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    bold: false
                    italic: true
                    pointSize: 71201
                    padding: 71202
                    leftPadding: 71203
                    rightPadding: 71204
                    topPadding: 71205
                    bottomPadding: 71206
                }

                second {
                    color: "#f72de1"
                    alignment: Qt.AlignJustify | Qt.AlignBaseline
                    bold: true
                    italic: false
                    pointSize: 71301
                    padding: 71302
                    leftPadding: 71303
                    rightPadding: 71304
                    topPadding: 71305
                    bottomPadding: 71306
                }
            }
        }

        pressed.hovered.focused.checked.highlighted.vertical {
            spacing: 80001
            padding: 80002
            leftPadding: 80003
            rightPadding: 80004
            topPadding: 80005
            bottomPadding: 80006
            transition: Transition { objectName: "nested.transition" }

            background {
                width: 80101
                height: 80102
                minimumWidth: 80103
                minimumHeight: 80104
                margins: 80105
                leftMargin: 80106
                rightMargin: 80107
                topMargin: 80108
                bottomMargin: 80109
                radius: 80110
                topLeftRadius: 80111
                topRightRadius: 80112
                bottomLeftRadius: 80113
                bottomRightRadius: 80114
                rotation: 80115
                scale: 1.813
                opacity: 0.811
                color: "#133225"
                alignment: Qt.AlignLeft | Qt.AlignVCenter
                fillWidth: false
                fillHeight: true
                visible: true
                clip: false
                gradient: Gradient { objectName: "nested.background.gradient" }
                data: QtObject { objectName: "nested.background.data" }
                delegate: Component { Item { objectName: "nested.background.delegate" } }
                border {
                    width: 80116
                    color: "#136480"
                }
                shadow {
                    color: "#1396db"
                    opacity: 0.812
                    scale: 1.814
                    verticalOffset: 80117
                    horizontalOffset: 80118
                    blur: 80119
                    visible: true
                    delegate: Component { Item { objectName: "nested.background.shadow.delegate" } }
                }
                image {
                    source: "nested.background.png"
                    color: "#13c836"
                    fillMode: Image.PreserveAspectFit
                }
            }

            handle {
                width: 80201
                height: 80202
                minimumWidth: 80203
                minimumHeight: 80204
                margins: 80205
                leftMargin: 80206
                rightMargin: 80207
                topMargin: 80208
                bottomMargin: 80209
                radius: 80210
                topLeftRadius: 80211
                topRightRadius: 80212
                bottomLeftRadius: 80213
                bottomRightRadius: 80214
                rotation: 80215
                scale: 1.823
                opacity: 0.821
                color: "#26324a"
                alignment: Qt.AlignRight | Qt.AlignBaseline
                fillWidth: true
                fillHeight: false
                visible: true
                clip: false
                gradient: Gradient { objectName: "nested.handle.gradient" }
                data: QtObject { objectName: "nested.handle.data" }
                delegate: Component { Item { objectName: "nested.handle.delegate" } }
                border {
                    width: 80216
                    color: "#2664a5"
                }
                shadow {
                    color: "#269600"
                    opacity: 0.822
                    scale: 1.824
                    verticalOffset: 80217
                    horizontalOffset: 80218
                    blur: 80219
                    visible: false
                    delegate: Component { Item { objectName: "nested.handle.shadow.delegate" } }
                }
                image {
                    source: "nested.handle.png"
                    color: "#26c85b"
                    fillMode: Image.PreserveAspectCrop
                }

                first {
                    width: 80301
                    height: 80302
                    minimumWidth: 80303
                    minimumHeight: 80304
                    margins: 80305
                    leftMargin: 80306
                    rightMargin: 80307
                    topMargin: 80308
                    bottomMargin: 80309
                    radius: 80310
                    topLeftRadius: 80311
                    topRightRadius: 80312
                    bottomLeftRadius: 80313
                    bottomRightRadius: 80314
                    rotation: 80315
                    scale: 1.833
                    opacity: 0.831
                    color: "#39326f"
                    alignment: Qt.AlignHCenter | Qt.AlignTop
                    fillWidth: false
                    fillHeight: true
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "nested.handle.first.gradient" }
                    data: QtObject { objectName: "nested.handle.first.data" }
                    delegate: Component { Item { objectName: "nested.handle.first.delegate" } }
                    border {
                        width: 80316
                        color: "#3964ca"
                    }
                    shadow {
                        color: "#399625"
                        opacity: 0.832
                        scale: 1.834
                        verticalOffset: 80317
                        horizontalOffset: 80318
                        blur: 80319
                        visible: false
                        delegate: Component { Item { objectName: "nested.handle.first.shadow.delegate" } }
                    }
                    image {
                        source: "nested.handle.first.png"
                        color: "#39c880"
                        fillMode: Image.Tile
                    }
                }

                second {
                    width: 80401
                    height: 80402
                    minimumWidth: 80403
                    minimumHeight: 80404
                    margins: 80405
                    leftMargin: 80406
                    rightMargin: 80407
                    topMargin: 80408
                    bottomMargin: 80409
                    radius: 80410
                    topLeftRadius: 80411
                    topRightRadius: 80412
                    bottomLeftRadius: 80413
                    bottomRightRadius: 80414
                    rotation: 80415
                    scale: 1.843
                    opacity: 0.841
                    color: "#4c3294"
                    alignment: Qt.AlignJustify | Qt.AlignBottom
                    fillWidth: true
                    fillHeight: false
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "nested.handle.second.gradient" }
                    data: QtObject { objectName: "nested.handle.second.data" }
                    delegate: Component { Item { objectName: "nested.handle.second.delegate" } }
                    border {
                        width: 80416
                        color: "#4c64ef"
                    }
                    shadow {
                        color: "#4c964a"
                        opacity: 0.842
                        scale: 1.844
                        verticalOffset: 80417
                        horizontalOffset: 80418
                        blur: 80419
                        visible: true
                        delegate: Component { Item { objectName: "nested.handle.second.shadow.delegate" } }
                    }
                    image {
                        source: "nested.handle.second.png"
                        color: "#4cc8a5"
                        fillMode: Image.TileVertically
                    }
                }
            }

            indicator {
                width: 80501
                height: 80502
                minimumWidth: 80503
                minimumHeight: 80504
                margins: 80505
                leftMargin: 80506
                rightMargin: 80507
                topMargin: 80508
                bottomMargin: 80509
                radius: 80510
                topLeftRadius: 80511
                topRightRadius: 80512
                bottomLeftRadius: 80513
                bottomRightRadius: 80514
                rotation: 80515
                scale: 1.853
                opacity: 0.851
                color: "#5f32b9"
                alignment: Qt.AlignLeft | Qt.AlignBaseline
                fillWidth: false
                fillHeight: true
                visible: true
                clip: false
                gradient: Gradient { objectName: "nested.indicator.gradient" }
                data: QtObject { objectName: "nested.indicator.data" }
                delegate: Component { Item { objectName: "nested.indicator.delegate" } }
                border {
                    width: 80516
                    color: "#5f6414"
                }
                shadow {
                    color: "#5f966f"
                    opacity: 0.852
                    scale: 1.854
                    verticalOffset: 80517
                    horizontalOffset: 80518
                    blur: 80519
                    visible: true
                    delegate: Component { Item { objectName: "nested.indicator.shadow.delegate" } }
                }
                image {
                    source: "nested.indicator.png"
                    color: "#5fc8ca"
                    fillMode: Image.TileHorizontally
                }

                foreground {
                    width: 80601
                    height: 80602
                    minimumWidth: 80603
                    minimumHeight: 80604
                    margins: 80605
                    leftMargin: 80606
                    rightMargin: 80607
                    topMargin: 80608
                    bottomMargin: 80609
                    radius: 80610
                    topLeftRadius: 80611
                    topRightRadius: 80612
                    bottomLeftRadius: 80613
                    bottomRightRadius: 80614
                    rotation: 80615
                    scale: 1.863
                    opacity: 0.861
                    color: "#7232de"
                    alignment: Qt.AlignRight | Qt.AlignTop
                    fillWidth: true
                    fillHeight: false
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "nested.indicator.foreground.gradient" }
                    data: QtObject { objectName: "nested.indicator.foreground.data" }
                    delegate: Component { Item { objectName: "nested.indicator.foreground.delegate" } }
                    border {
                        width: 80616
                        color: "#726439"
                    }
                    shadow {
                        color: "#729694"
                        opacity: 0.862
                        scale: 1.864
                        verticalOffset: 80617
                        horizontalOffset: 80618
                        blur: 80619
                        visible: false
                        delegate: Component { Item { objectName: "nested.indicator.foreground.shadow.delegate" } }
                    }
                    image {
                        source: "nested.indicator.foreground.png"
                        color: "#72c8ef"
                        fillMode: Image.Pad
                    }
                }

                first {
                    width: 80701
                    height: 80702
                    minimumWidth: 80703
                    minimumHeight: 80704
                    margins: 80705
                    leftMargin: 80706
                    rightMargin: 80707
                    topMargin: 80708
                    bottomMargin: 80709
                    radius: 80710
                    topLeftRadius: 80711
                    topRightRadius: 80712
                    bottomLeftRadius: 80713
                    bottomRightRadius: 80714
                    rotation: 80715
                    scale: 1.873
                    opacity: 0.871
                    color: "#853203"
                    alignment: Qt.AlignHCenter | Qt.AlignBottom
                    fillWidth: false
                    fillHeight: true
                    visible: true
                    clip: false
                    gradient: Gradient { objectName: "nested.indicator.first.gradient" }
                    data: QtObject { objectName: "nested.indicator.first.data" }
                    delegate: Component { Item { objectName: "nested.indicator.first.delegate" } }
                    border {
                        width: 80716
                        color: "#85645e"
                    }
                    shadow {
                        color: "#8596b9"
                        opacity: 0.872
                        scale: 1.874
                        verticalOffset: 80717
                        horizontalOffset: 80718
                        blur: 80719
                        visible: false
                        delegate: Component { Item { objectName: "nested.indicator.first.shadow.delegate" } }
                    }
                    image {
                        source: "nested.indicator.first.png"
                        color: "#85c814"
                        fillMode: Image.Stretch
                    }

                    foreground {
                        width: 80801
                        height: 80802
                        minimumWidth: 80803
                        minimumHeight: 80804
                        margins: 80805
                        leftMargin: 80806
                        rightMargin: 80807
                        topMargin: 80808
                        bottomMargin: 80809
                        radius: 80810
                        topLeftRadius: 80811
                        topRightRadius: 80812
                        bottomLeftRadius: 80813
                        bottomRightRadius: 80814
                        rotation: 80815
                        scale: 1.883
                        opacity: 0.881
                        color: "#983228"
                        alignment: Qt.AlignJustify | Qt.AlignVCenter
                        fillWidth: true
                        fillHeight: false
                        visible: true
                        clip: false
                        gradient: Gradient { objectName: "nested.indicator.first.foreground.gradient" }
                        data: QtObject { objectName: "nested.indicator.first.foreground.data" }
                        delegate: Component { Item { objectName: "nested.indicator.first.foreground.delegate" } }
                        border {
                            width: 80816
                            color: "#986483"
                        }
                        shadow {
                            color: "#9896de"
                            opacity: 0.882
                            scale: 1.884
                            verticalOffset: 80817
                            horizontalOffset: 80818
                            blur: 80819
                            visible: true
                            delegate: Component { Item { objectName: "nested.indicator.first.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "nested.indicator.first.foreground.png"
                            color: "#98c839"
                            fillMode: Image.PreserveAspectFit
                        }
                    }
                }

                second {
                    width: 80901
                    height: 80902
                    minimumWidth: 80903
                    minimumHeight: 80904
                    margins: 80905
                    leftMargin: 80906
                    rightMargin: 80907
                    topMargin: 80908
                    bottomMargin: 80909
                    radius: 80910
                    topLeftRadius: 80911
                    topRightRadius: 80912
                    bottomLeftRadius: 80913
                    bottomRightRadius: 80914
                    rotation: 80915
                    scale: 1.893
                    opacity: 0.891
                    color: "#ab324d"
                    alignment: Qt.AlignLeft | Qt.AlignTop
                    fillWidth: false
                    fillHeight: true
                    visible: false
                    clip: true
                    gradient: Gradient { objectName: "nested.indicator.second.gradient" }
                    data: QtObject { objectName: "nested.indicator.second.data" }
                    delegate: Component { Item { objectName: "nested.indicator.second.delegate" } }
                    border {
                        width: 80916
                        color: "#ab64a8"
                    }
                    shadow {
                        color: "#ab9603"
                        opacity: 0.892
                        scale: 1.894
                        verticalOffset: 80917
                        horizontalOffset: 80918
                        blur: 80919
                        visible: true
                        delegate: Component { Item { objectName: "nested.indicator.second.shadow.delegate" } }
                    }
                    image {
                        source: "nested.indicator.second.png"
                        color: "#abc85e"
                        fillMode: Image.PreserveAspectCrop
                    }

                    foreground {
                        width: 81001
                        height: 81002
                        minimumWidth: 81003
                        minimumHeight: 81004
                        margins: 81005
                        leftMargin: 81006
                        rightMargin: 81007
                        topMargin: 81008
                        bottomMargin: 81009
                        radius: 81010
                        topLeftRadius: 81011
                        topRightRadius: 81012
                        bottomLeftRadius: 81013
                        bottomRightRadius: 81014
                        rotation: 81015
                        scale: 1.903
                        opacity: 0.901
                        color: "#be3272"
                        alignment: Qt.AlignRight | Qt.AlignBottom
                        fillWidth: true
                        fillHeight: false
                        visible: true
                        clip: false
                        gradient: Gradient { objectName: "nested.indicator.second.foreground.gradient" }
                        data: QtObject { objectName: "nested.indicator.second.foreground.data" }
                        delegate: Component { Item { objectName: "nested.indicator.second.foreground.delegate" } }
                        border {
                            width: 81016
                            color: "#be64cd"
                        }
                        shadow {
                            color: "#be9628"
                            opacity: 0.902
                            scale: 1.904
                            verticalOffset: 81017
                            horizontalOffset: 81018
                            blur: 81019
                            visible: false
                            delegate: Component { Item { objectName: "nested.indicator.second.foreground.shadow.delegate" } }
                        }
                        image {
                            source: "nested.indicator.second.foreground.png"
                            color: "#bec883"
                            fillMode: Image.Tile
                        }
                    }
                }
            }

            text {
                color: "#d13297"
                alignment: Qt.AlignHCenter | Qt.AlignVCenter
                bold: false
                italic: true
                pointSize: 81101
                padding: 81102
                leftPadding: 81103
                rightPadding: 81104
                topPadding: 81105
                bottomPadding: 81106

                first {
                    color: "#e432bc"
                    alignment: Qt.AlignJustify | Qt.AlignBaseline
                    bold: true
                    italic: false
                    pointSize: 81201
                    padding: 81202
                    leftPadding: 81203
                    rightPadding: 81204
                    topPadding: 81205
                    bottomPadding: 81206
                }

                second {
                    color: "#f732e1"
                    alignment: Qt.AlignLeft | Qt.AlignBottom
                    bold: false
                    italic: true
                    pointSize: 81301
                    padding: 81302
                    leftPadding: 81303
                    rightPadding: 81304
                    topPadding: 81305
                    bottomPadding: 81306
                }
            }
        }
    }
}
