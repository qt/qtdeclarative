// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

pragma Singleton
import QtQml

QtObject {
    readonly property QtObject controls: Qt.styleHints.colorScheme === Qt.Light ? light.controls : dark.controls

    readonly property QtObject dark: QtObject {
        readonly property QtObject controls: QtObject {
            readonly property QtObject button: QtObject {
                readonly property QtObject checked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17023;8603:12521;2373:10903"
                        readonly property string filePath: "dark/images/button-background-checked.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background-checked"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2225
                        readonly property real y: 1874
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:17023;8603:12521"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem-checked"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17023;8603:12521;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon-checked"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2245
                        readonly property real y: 1881
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17023;8603:12521;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label-checked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2269
                        readonly property real y: 1879
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject checked_disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17029;8603:12527;2373:10903"
                        readonly property string filePath: "dark/images/button-background-checked-disabled.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background-checked-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2225
                        readonly property real y: 2075
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:17029;8603:12527"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem-checked-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17029;8603:12527;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2245
                        readonly property real y: 2082
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17029;8603:12527;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2269
                        readonly property real y: 2080
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject checked_hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17027;8603:12525;2373:10903"
                        readonly property string filePath: "dark/images/button-background-checked-hovered.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background-checked-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2225
                        readonly property real y: 2008
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:17027;8603:12525"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem-checked-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17027;8603:12525;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2245
                        readonly property real y: 2015
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17027;8603:12525;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2269
                        readonly property real y: 2013
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject checked_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17031;8603:12529;2373:10903"
                        readonly property string filePath: "dark/images/button-background-checked-pressed.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background-checked-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2225
                        readonly property real y: 2142
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:17031;8603:12529"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem-checked-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17031;8603:12529;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2245
                        readonly property real y: 2149
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17031;8603:12529;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2269
                        readonly property real y: 2147
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17025;8603:12523;2373:10903"
                        readonly property string filePath: "dark/images/button-background-disabled.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2225
                        readonly property real y: 1941
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:17025;8603:12523"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17025;8603:12523;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2245
                        readonly property real y: 1948
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17025;8603:12523;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2269
                        readonly property real y: 1946
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17019;8603:12517;2373:10903"
                        readonly property string filePath: "dark/images/button-background-hovered.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2225
                        readonly property real y: 1740
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:17019;8603:12517"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17019;8603:12517;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2245
                        readonly property real y: 1747
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17019;8603:12517;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2269
                        readonly property real y: 1745
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17017;8603:12515;2373:10903"
                        readonly property string filePath: "dark/images/button-background.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2227.5
                        readonly property real y: 1685
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:17017;8603:12515"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17017;8603:12515;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2247.5
                        readonly property real y: 1692
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17017;8603:12515;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2271.5
                        readonly property real y: 1690
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17021;8603:12519;2373:10903"
                        readonly property string filePath: "dark/images/button-background-pressed.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2225
                        readonly property real y: 1807
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:17021;8603:12519"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17021;8603:12519;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2245
                        readonly property real y: 1814
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17021;8603:12519;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2269
                        readonly property real y: 1812
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

            }

            readonly property QtObject checkbox: QtObject {
                readonly property QtObject checked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17040;8622:13107;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-checked"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 2838.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17040;8622:13107"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-checked"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17040;8622:13107;2425:10953"
                        readonly property string filePath: "dark/images/checkbox-indicator-checked.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-checked"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 2844.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17040;8622:13107;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-checked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 2844.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17050;8622:13117;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-checked-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 3114.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17050;8622:13117"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-checked-disabled"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17050;8622:13117;2425:10953"
                        readonly property string filePath: "dark/images/checkbox-indicator-checked-disabled.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-checked-disabled"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 3120.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17050;8622:13117;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 3120.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17054;8622:13121;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-checked-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 2976.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17054;8622:13121"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-checked-hovered"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17054;8622:13121;2425:10953"
                        readonly property string filePath: "dark/images/checkbox-indicator-checked-hovered.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-checked-hovered"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 2982.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17054;8622:13121;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 2982.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17052;8622:13119;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-checked-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 3045.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17052;8622:13119"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-checked-pressed"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17052;8622:13119;2425:10953"
                        readonly property string filePath: "dark/images/checkbox-indicator-checked-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-checked-pressed"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 3051.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17052;8622:13119;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 3051.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17056;8622:13123;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 2907.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17056;8622:13123"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-disabled"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17056;8622:13123;2425:10953"
                        readonly property string filePath: "dark/images/checkbox-indicator-disabled.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-disabled"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 2913.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17056;8622:13123;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 2913.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject disabled_partiallyChecked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17048;8622:13115;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-disabled-partiallyChecked"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 3390.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17048;8622:13115"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-disabled-partiallyChecked"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17048;8622:13115;2425:10953"
                        readonly property string filePath: "dark/images/checkbox-indicator-disabled-partiallyChecked.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-disabled-partiallyChecked"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 3396.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17048;8622:13115;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-disabled-partiallyChecked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 3396.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17036;8622:13103;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 2700.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17036;8622:13103"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-hovered"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17036;8622:13103;2425:10953"
                        readonly property string filePath: "dark/images/checkbox-indicator-hovered.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-hovered"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 2706.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17036;8622:13103;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 2706.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject hovered_partiallyChecked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17044;8622:13111;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-hovered-partiallyChecked"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 3252.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17044;8622:13111"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-hovered-partiallyChecked"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17044;8622:13111;2425:10953"
                        readonly property string filePath: "dark/images/checkbox-indicator-hovered-partiallyChecked.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-hovered-partiallyChecked"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 3258.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17044;8622:13111;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-hovered-partiallyChecked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 3258.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17034;8622:13101;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 2631.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17034;8622:13101"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17034;8622:13101;2425:10953"
                        readonly property string filePath: "dark/images/checkbox-indicator.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 2637.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17034;8622:13101;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 2637.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject partiallyChecked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17042;8622:13109;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-partiallyChecked"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 3183.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17042;8622:13109"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-partiallyChecked"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17042;8622:13109;2425:10953"
                        readonly property string filePath: "dark/images/checkbox-indicator-partiallyChecked.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-partiallyChecked"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 3189.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17042;8622:13109;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-partiallyChecked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 3189.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject partiallyChecked_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17046;8622:13113;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-partiallyChecked-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 3321.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17046;8622:13113"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-partiallyChecked-pressed"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17046;8622:13113;2425:10953"
                        readonly property string filePath: "dark/images/checkbox-indicator-partiallyChecked-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-partiallyChecked-pressed"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 3327.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17046;8622:13113;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-partiallyChecked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 3327.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17038;8622:13105;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 2769.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17038;8622:13105"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-pressed"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17038;8622:13105;2425:10953"
                        readonly property string filePath: "dark/images/checkbox-indicator-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-pressed"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 2775.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17038;8622:13105;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 2775.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

            }

            readonly property QtObject flatbutton: QtObject {
                readonly property QtObject checked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I3991:9227;3987:9104;3987:9044"
                        readonly property string filePath: "dark/images/flatbutton-background-checked.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 12
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background-checked"
                        readonly property real rightOffset: 12
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 15
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3315.5
                        readonly property real y: 2040.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9227;3987:9104"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem-checked"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9227;3987:9104;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon-checked"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3332.5
                        readonly property real y: 2045.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9227;3987:9104;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label-checked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3360.5
                        readonly property real y: 2045.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject checked_disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I3991:9230;3987:9122;3987:9044"
                        readonly property string filePath: "dark/images/flatbutton-background-checked-disabled.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 12
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background-checked-disabled"
                        readonly property real rightOffset: 12
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 15
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3315.5
                        readonly property real y: 2174.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9230;3987:9122"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem-checked-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9230;3987:9122;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3332.5
                        readonly property real y: 2179.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9230;3987:9122;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3360.5
                        readonly property real y: 2179.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject checked_hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I3991:9229;3987:9113;3987:9044"
                        readonly property string filePath: "dark/images/flatbutton-background-checked-hovered.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 12
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background-checked-hovered"
                        readonly property real rightOffset: 12
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 15
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3315.5
                        readonly property real y: 2107.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9229;3987:9113"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem-checked-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9229;3987:9113;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3332.5
                        readonly property real y: 2112.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9229;3987:9113;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3360.5
                        readonly property real y: 2112.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject checked_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I3991:9231;3987:9131;3987:9044"
                        readonly property string filePath: "dark/images/flatbutton-background-checked-pressed.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 12
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background-checked-pressed"
                        readonly property real rightOffset: 12
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 15
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3315.5
                        readonly property real y: 2241.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9231;3987:9131"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem-checked-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9231;3987:9131;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3332.5
                        readonly property real y: 2246.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9231;3987:9131;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3360.5
                        readonly property real y: 2246.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I3991:9228;3987:9095;3987:9044"
                        readonly property string filePath: "dark/images/flatbutton-background-disabled.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 12
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background-disabled"
                        readonly property real rightOffset: 12
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 15
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3315.5
                        readonly property real y: 1973.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9228;3987:9095"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9228;3987:9095;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3332.5
                        readonly property real y: 1978.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9228;3987:9095;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3360.5
                        readonly property real y: 1978.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I3991:9225;3987:9077;3987:9044"
                        readonly property string filePath: "dark/images/flatbutton-background-hovered.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 12
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background-hovered"
                        readonly property real rightOffset: 12
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 15
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3315.5
                        readonly property real y: 1839.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9225;3987:9077"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9225;3987:9077;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3332.5
                        readonly property real y: 1844.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9225;3987:9077;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3360.5
                        readonly property real y: 1844.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9224;3987:9068;3987:9044"
                        readonly property string filePath: ""
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3315.5
                        readonly property real y: 1772.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9224;3987:9068"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9224;3987:9068;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3332.5
                        readonly property real y: 1777.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9224;3987:9068;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3360.5
                        readonly property real y: 1777.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I3991:9226;3987:9086;3987:9044"
                        readonly property string filePath: "dark/images/flatbutton-background-pressed.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 12
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background-pressed"
                        readonly property real rightOffset: 12
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 15
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3315.5
                        readonly property real y: 1906.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9226;3987:9086"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9226;3987:9086;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3332.5
                        readonly property real y: 1911.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9226;3987:9086;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3360.5
                        readonly property real y: 1911.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

            }

            readonly property QtObject itemdelegate: QtObject {
                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17085;2319:9946;2399:11597"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 93
                        readonly property real x: 5917
                        readonly property real y: 2010.5
                    }

                    readonly property real bottomPadding: 8
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 8
                        readonly property string figmaId: "I2557:17085;2319:9946"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "itemdelegate-contentItem-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 12
                        readonly property real topPadding: 8
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17085;2319:9946;2411:10964"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 5924.5
                        readonly property real y: 2018.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 8
                }

                readonly property QtObject highlighted: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17087;2319:9952;2399:11597"
                        readonly property string filePath: "dark/images/itemdelegate-background-highlighted.png"
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-background-highlighted"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 93
                        readonly property real x: 5917
                        readonly property real y: 2077.5
                    }

                    readonly property real bottomPadding: 8
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 8
                        readonly property string figmaId: "I2557:17087;2319:9952"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "itemdelegate-contentItem-highlighted"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 12
                        readonly property real topPadding: 8
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17087;2319:9952;2411:10964"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-label-highlighted"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 5924.5
                        readonly property real y: 2085.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 8
                }

                readonly property QtObject highlighted_hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17089;2319:9958;2399:11597"
                        readonly property string filePath: "dark/images/itemdelegate-background-highlighted-hovered.png"
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-background-highlighted-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 93
                        readonly property real x: 5917
                        readonly property real y: 2137.5
                    }

                    readonly property real bottomPadding: 8
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 8
                        readonly property string figmaId: "I2557:17089;2319:9958"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "itemdelegate-contentItem-highlighted-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 12
                        readonly property real topPadding: 8
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17089;2319:9958;2411:10964"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-label-highlighted-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 5924.5
                        readonly property real y: 2145.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 8
                }

                readonly property QtObject highlighted_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17091;2319:9970;2399:11597"
                        readonly property string filePath: "dark/images/itemdelegate-background-highlighted-pressed.png"
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-background-highlighted-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 93
                        readonly property real x: 5917
                        readonly property real y: 2211.5
                    }

                    readonly property real bottomPadding: 8
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 8
                        readonly property string figmaId: "I2557:17091;2319:9970"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "itemdelegate-contentItem-highlighted-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 12
                        readonly property real topPadding: 8
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17091;2319:9970;2411:10964"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-label-highlighted-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 5924.5
                        readonly property real y: 2219.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 8
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17081;2319:9922;2399:11597"
                        readonly property string filePath: "dark/images/itemdelegate-background-hovered.png"
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 93
                        readonly property real x: 5917
                        readonly property real y: 1876.5
                    }

                    readonly property real bottomPadding: 8
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 8
                        readonly property string figmaId: "I2557:17081;2319:9922"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "itemdelegate-contentItem-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 12
                        readonly property real topPadding: 8
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17081;2319:9922;2411:10964"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 5924.5
                        readonly property real y: 1884.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 8
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17079;2319:9916;2399:11597"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 93
                        readonly property real x: 5917
                        readonly property real y: 1809.5
                    }

                    readonly property real bottomPadding: 8
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 8
                        readonly property string figmaId: "I2557:17079;2319:9916"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "itemdelegate-contentItem"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 12
                        readonly property real topPadding: 8
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17079;2319:9916;2411:10964"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 5924.5
                        readonly property real y: 1817.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 8
                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17083;2319:9934;2399:11597"
                        readonly property string filePath: "dark/images/itemdelegate-background-pressed.png"
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-background-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 93
                        readonly property real x: 5917
                        readonly property real y: 1943.5
                    }

                    readonly property real bottomPadding: 8
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 8
                        readonly property string figmaId: "I2557:17083;2319:9934"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "itemdelegate-contentItem-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 12
                        readonly property real topPadding: 8
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17083;2319:9934;2411:10964"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-label-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 5924.5
                        readonly property real y: 1951.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 8
                }

            }

            readonly property QtObject popup: QtObject {
                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 7
                        readonly property real bottomShadow: 25
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17074;2308:11133;2313:11247"
                        readonly property string filePath: "dark/images/popup-background.png"
                        readonly property real height: 104
                        readonly property real leftOffset: 7
                        readonly property real leftShadow: 17
                        readonly property string name: "popup-background"
                        readonly property real rightOffset: 7
                        readonly property real rightShadow: 17
                        readonly property real topOffset: 7
                        readonly property real topShadow: 9
                        readonly property real width: 116
                        readonly property real x: 7148
                        readonly property real y: 2195
                    }

                    readonly property real bottomPadding: 16
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 16
                        readonly property string figmaId: "I2557:17074;2308:11133"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 16
                        readonly property string name: "popup-contentItem"
                        readonly property real rightPadding: 16
                        readonly property real spacing: 0
                        readonly property real topPadding: 16
                    }

                    readonly property real leftPadding: 16
                    readonly property real rightPadding: 16
                    readonly property real topPadding: 16
                }

            }

            readonly property QtObject progressbar: QtObject {
                readonly property QtObject disabled: QtObject {
                    readonly property real bottomPadding: 0
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 0
                        readonly property string figmaId: "I4435:9378;4304:9328"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "progressbar-contentItem-disabled"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 0
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I4435:9378;4304:9328;4413:23724"
                        readonly property string filePath: "dark/images/progressbar-groove-disabled.png"
                        readonly property real height: 1
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-groove-disabled"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 180
                        readonly property real x: 15842
                        readonly property real y: 2059
                    }

                    readonly property real leftPadding: 0
                    readonly property real rightPadding: 0
                    readonly property real topPadding: 0
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I4435:9378;4304:9328;4267:14564"
                        readonly property real height: 3
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-track-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 48
                        readonly property real x: 15842
                        readonly property real y: 2058
                    }

                }

                readonly property QtObject disabled_indeterminate: QtObject {
                    readonly property real bottomPadding: 0
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 0
                        readonly property string figmaId: "I4435:9380;4304:9355"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "progressbar-contentItem-disabled-indeterminate"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 0
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I4435:9380;4304:9355;4350:35746"
                        readonly property string filePath: ""
                        readonly property real height: 1
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-groove-disabled-indeterminate"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 180
                        readonly property real x: 15842
                        readonly property real y: 2132
                    }

                    readonly property real leftPadding: 0
                    readonly property real rightPadding: 0
                    readonly property real topPadding: 0
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I4435:9380;4304:9355;4403:22724"
                        readonly property real height: 3
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-track-disabled-indeterminate"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 48
                        readonly property real x: 15908
                        readonly property real y: 2131
                    }

                }

                readonly property QtObject indeterminate: QtObject {
                    readonly property real bottomPadding: 0
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 0
                        readonly property string figmaId: "I4435:9376;2450:12847"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "progressbar-contentItem-indeterminate"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 0
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I4435:9376;2450:12847;4350:35746"
                        readonly property string filePath: ""
                        readonly property real height: 1
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-groove-indeterminate"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 180
                        readonly property real x: 15842
                        readonly property real y: 1986
                    }

                    readonly property real leftPadding: 0
                    readonly property real rightPadding: 0
                    readonly property real topPadding: 0
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I4435:9376;2450:12847;4403:22724"
                        readonly property real height: 3
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-track-indeterminate"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 48
                        readonly property real x: 15908
                        readonly property real y: 1985
                    }

                }

                readonly property QtObject normal: QtObject {
                    readonly property real bottomPadding: 0
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 0
                        readonly property string figmaId: "I4435:9374;2450:12841"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "progressbar-contentItem"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 0
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I4435:9374;2450:12841;4413:23724"
                        readonly property string filePath: "dark/images/progressbar-groove.png"
                        readonly property real height: 1
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-groove"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 180
                        readonly property real x: 15842
                        readonly property real y: 1913
                    }

                    readonly property real leftPadding: 0
                    readonly property real rightPadding: 0
                    readonly property real topPadding: 0
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I4435:9374;2450:12841;4267:14564"
                        readonly property real height: 3
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-track"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 48
                        readonly property real x: 15842
                        readonly property real y: 1912
                    }

                }

            }

            readonly property QtObject radiobutton: QtObject {
                readonly property QtObject checked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17135;2483:15472;2472:12869"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background-checked"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 17057.5
                        readonly property real y: 1977.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17135;2483:15472"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem-checked"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17135;2483:15472;2473:12871"
                        readonly property string filePath: "dark/images/radiobutton-indicator-checked.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator-checked"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 17061.5
                        readonly property real y: 1983.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17135;2483:15472;6758:14518"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label-checked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 17093.5
                        readonly property real y: 1985.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17141;2488:15512;2472:12869"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background-checked-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 17057.5
                        readonly property real y: 2255.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17141;2488:15512"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem-checked-disabled"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17141;2488:15512;2473:12871"
                        readonly property string filePath: "dark/images/radiobutton-indicator-checked-disabled.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator-checked-disabled"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 17061.5
                        readonly property real y: 2261.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17141;2488:15512;6758:14518"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 17093.5
                        readonly property real y: 2263.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17137;8622:14986"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background-checked-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 17057.5
                        readonly property real y: 2119.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17137;8622:14985"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem-checked-hovered"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17137;8622:14996"
                        readonly property string filePath: "dark/images/radiobutton-indicator-checked-hovered.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator-checked-hovered"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 17061.5
                        readonly property real y: 2125.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17137;8622:14988"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 17093.5
                        readonly property real y: 2127.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17139;8622:15023"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background-checked-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 17057.5
                        readonly property real y: 2186.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17139;8622:15022"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem-checked-pressed"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17139;8622:15033"
                        readonly property string filePath: "dark/images/radiobutton-indicator-checked-pressed.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator-checked-pressed"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 17061.5
                        readonly property real y: 2192.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17139;8622:15025"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 17093.5
                        readonly property real y: 2194.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17143;2483:15480;2472:12869"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 17057.5
                        readonly property real y: 2048.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17143;2483:15480"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem-disabled"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17143;2483:15480;2473:12871"
                        readonly property string filePath: "dark/images/radiobutton-indicator-disabled.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator-disabled"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 17061.5
                        readonly property real y: 2054.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17143;2483:15480;6758:14518"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 17093.5
                        readonly property real y: 2056.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17131;2473:12899;2472:12869"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 17057.5
                        readonly property real y: 1839.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17131;2473:12899"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem-hovered"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17131;2473:12899;2473:12871"
                        readonly property string filePath: "dark/images/radiobutton-indicator-hovered.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator-hovered"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 17061.5
                        readonly property real y: 1845.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17131;2473:12899;6758:14518"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 17093.5
                        readonly property real y: 1847.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17129;2473:12891;2472:12869"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 17057.5
                        readonly property real y: 1770.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17129;2473:12891"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17129;2473:12891;2473:12871"
                        readonly property string filePath: "dark/images/radiobutton-indicator.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 17061.5
                        readonly property real y: 1776.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17129;2473:12891;6758:14518"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 17093.5
                        readonly property real y: 1778.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17133;8622:15060"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 17057.5
                        readonly property real y: 1908.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17133;8622:15059"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem-pressed"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17133;8622:15070"
                        readonly property string filePath: "dark/images/radiobutton-indicator-pressed.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator-pressed"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 17061.5
                        readonly property real y: 1914.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17133;8622:15062"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 17093.5
                        readonly property real y: 1916.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

            }

            readonly property QtObject rangeslider: QtObject {
                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17152;2509:12481;2509:12419"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 200
                        readonly property real x: 17964
                        readonly property real y: 2839
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:17152;2509:12481"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "rangeslider-contentItem-disabled"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject first_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17152;2509:12481;4189:38496"
                        readonly property string filePath: "dark/images/rangeslider-first-handle-disabled.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-first-handle-disabled"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 17992
                        readonly property real y: 2839
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17152;2509:12481;4178:28261"
                        readonly property string filePath: "dark/images/rangeslider-groove-disabled.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-groove-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 184
                        readonly property real x: 17972
                        readonly property real y: 2847
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property QtObject second_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17152;2509:12481;4191:43003"
                        readonly property string filePath: "dark/images/rangeslider-second-handle-disabled.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-second-handle-disabled"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 18116
                        readonly property real y: 2839
                    }

                    readonly property real spacing: -154
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17152;2509:12481;4189:38505"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-track-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 124
                        readonly property real x: 18002
                        readonly property real y: 2847
                    }

                }

                readonly property QtObject handle_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17150;8624:14526"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-background-handle-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 200
                        readonly property real x: 17964
                        readonly property real y: 2781
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:17150;8624:14525"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "rangeslider-contentItem-handle-pressed"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject first_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17150;8624:14556"
                        readonly property string filePath: "dark/images/rangeslider-first-handle-handle-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-first-handle-handle-pressed"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 17992
                        readonly property real y: 2781
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17150;8624:14529"
                        readonly property string filePath: "dark/images/rangeslider-groove-handle-pressed.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-groove-handle-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 184
                        readonly property real x: 17972
                        readonly property real y: 2789
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property QtObject second_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17150;8624:14627"
                        readonly property string filePath: "dark/images/rangeslider-second-handle-handle-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-second-handle-handle-pressed"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 18116
                        readonly property real y: 2781
                    }

                    readonly property real spacing: -154
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17150;8624:14531"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-track-handle-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 124
                        readonly property real x: 18002
                        readonly property real y: 2789
                    }

                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17148;8624:14397"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 200
                        readonly property real x: 17964
                        readonly property real y: 2723
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:17148;8624:14396"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "rangeslider-contentItem-hovered"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject first_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17148;8624:14427"
                        readonly property string filePath: "dark/images/rangeslider-first-handle-hovered.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-first-handle-hovered"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 17992
                        readonly property real y: 2723
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17148;8624:14400"
                        readonly property string filePath: "dark/images/rangeslider-groove-hovered.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-groove-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 184
                        readonly property real x: 17972
                        readonly property real y: 2731
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property QtObject second_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17148;8624:14506"
                        readonly property string filePath: "dark/images/rangeslider-second-handle-hovered.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-second-handle-hovered"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 18116
                        readonly property real y: 2723
                    }

                    readonly property real spacing: -154
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17148;8624:14402"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-track-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 124
                        readonly property real x: 18002
                        readonly property real y: 2731
                    }

                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17146;2509:12436;2509:12419"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 200
                        readonly property real x: 17964
                        readonly property real y: 2665
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:17146;2509:12436"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "rangeslider-contentItem"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject first_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17146;2509:12436;4189:38496"
                        readonly property string filePath: "dark/images/rangeslider-first-handle.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-first-handle"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 17992
                        readonly property real y: 2665
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17146;2509:12436;4178:28261"
                        readonly property string filePath: "dark/images/rangeslider-groove.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-groove"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 184
                        readonly property real x: 17972
                        readonly property real y: 2673
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property QtObject second_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17146;2509:12436;4191:43003"
                        readonly property string filePath: "dark/images/rangeslider-second-handle.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-second-handle"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 18116
                        readonly property real y: 2665
                    }

                    readonly property real spacing: -154
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17146;2509:12436;4189:38505"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-track"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 124
                        readonly property real x: 18002
                        readonly property real y: 2673
                    }

                }

            }

            readonly property QtObject slider: QtObject {
                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17178;2506:12695;4200:48590"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 224
                        readonly property real x: 22952
                        readonly property real y: 2827.5
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:17178;2506:12695"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "slider-contentItem-disabled"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17178;2506:12695;4385:9106"
                        readonly property string filePath: "dark/images/slider-groove-disabled.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-groove-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 208
                        readonly property real x: 22960
                        readonly property real y: 2835.5
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17178;2506:12695;4200:48601"
                        readonly property string filePath: "dark/images/slider-handle-disabled.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "slider-handle-disabled"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 23123
                        readonly property real y: 2827.5
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: -208
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17178;2506:12695;4200:48597"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-track-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 173
                        readonly property real x: 22960
                        readonly property real y: 2835.5
                    }

                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17174;8624:13850"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 224
                        readonly property real x: 22952
                        readonly property real y: 2708.5
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:17174;8624:13849"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "slider-contentItem-hovered"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17174;8624:13853"
                        readonly property string filePath: "dark/images/slider-groove-hovered.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-groove-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 208
                        readonly property real x: 22960
                        readonly property real y: 2716.5
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17174;8624:13874"
                        readonly property string filePath: "dark/images/slider-handle-hovered.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "slider-handle-hovered"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 23123
                        readonly property real y: 2708.5
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: -208
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17174;8624:13855"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-track-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 173
                        readonly property real x: 22960
                        readonly property real y: 2716.5
                    }

                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17172;2506:12656;4200:48590"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 224
                        readonly property real x: 22952
                        readonly property real y: 2649.5
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:17172;2506:12656"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "slider-contentItem"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17172;2506:12656;4385:9106"
                        readonly property string filePath: "dark/images/slider-groove.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-groove"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 208
                        readonly property real x: 22960
                        readonly property real y: 2657.5
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17172;2506:12656;4200:48601"
                        readonly property string filePath: "dark/images/slider-handle.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "slider-handle"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 23123
                        readonly property real y: 2649.5
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: -208
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17172;2506:12656;4200:48597"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-track"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 173
                        readonly property real x: 22960
                        readonly property real y: 2657.5
                    }

                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17176;8624:14647"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-background-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 224
                        readonly property real x: 22952
                        readonly property real y: 2768.5
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:17176;8624:14646"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "slider-contentItem-pressed"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17176;8624:14650"
                        readonly property string filePath: "dark/images/slider-groove-pressed.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-groove-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 208
                        readonly property real x: 22960
                        readonly property real y: 2776.5
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17176;8624:14671"
                        readonly property string filePath: "dark/images/slider-handle-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "slider-handle-pressed"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 23123
                        readonly property real y: 2768.5
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: -208
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17176;8624:14652"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-track-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 173
                        readonly property real x: 22960
                        readonly property real y: 2776.5
                    }

                }

            }

            readonly property QtObject switch_: QtObject {
                readonly property QtObject checked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17204;2531:14856;4350:34538"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background-checked"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25798.5
                        readonly property real y: 2250.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17204;2531:14856"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem-checked"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 5
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17204;2531:14856;4350:34543"
                        readonly property string filePath: "dark/images/switch-handle-checked.png"
                        readonly property real height: 12
                        readonly property real leftOffset: 6
                        readonly property real leftShadow: 1
                        readonly property string name: "switch-handle-checked"
                        readonly property real rightOffset: 5
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 6
                        readonly property real topShadow: 1
                        readonly property real width: 12
                        readonly property real x: 25826.5
                        readonly property real y: 2260.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17204;2531:14856;4350:34541"
                        readonly property string filePath: "dark/images/switch-handle-background-checked.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background-checked"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25802.5
                        readonly property real y: 2256.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property string alignItems: "MAX"
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:17204;2531:14856;4350:34542"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-handle-contentItem-checked"
                        readonly property real rightPadding: 4
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17204;2531:14856;6761:23654"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label-checked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25854.5
                        readonly property real y: 2256.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17212;2531:14900;4350:34538"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background-checked-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25798.5
                        readonly property real y: 2454.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17212;2531:14900"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem-checked-disabled"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 5
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17212;2531:14900;4350:34543"
                        readonly property string filePath: "dark/images/switch-handle-checked-disabled.png"
                        readonly property real height: 12
                        readonly property real leftOffset: 6
                        readonly property real leftShadow: 1
                        readonly property string name: "switch-handle-checked-disabled"
                        readonly property real rightOffset: 5
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 6
                        readonly property real topShadow: 1
                        readonly property real width: 12
                        readonly property real x: 25826.5
                        readonly property real y: 2464.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17212;2531:14900;4350:34541"
                        readonly property string filePath: "dark/images/switch-handle-background-checked-disabled.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background-checked-disabled"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25802.5
                        readonly property real y: 2460.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property string alignItems: "MAX"
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:17212;2531:14900;4350:34542"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-handle-contentItem-checked-disabled"
                        readonly property real rightPadding: 4
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17212;2531:14900;6761:23654"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25854.5
                        readonly property real y: 2460.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17208;8664:14952"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background-checked-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25798.5
                        readonly property real y: 2352.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17208;8664:14951"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem-checked-hovered"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 6
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17208;8664:14975"
                        readonly property string filePath: "dark/images/switch-handle-checked-hovered.png"
                        readonly property real height: 14
                        readonly property real leftOffset: 7
                        readonly property real leftShadow: 1
                        readonly property string name: "switch-handle-checked-hovered"
                        readonly property real rightOffset: 6
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 7
                        readonly property real topShadow: 1
                        readonly property real width: 14
                        readonly property real x: 25825.5
                        readonly property real y: 2361.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17208;8664:14954"
                        readonly property string filePath: "dark/images/switch-handle-background-checked-hovered.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background-checked-hovered"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25802.5
                        readonly property real y: 2358.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property string alignItems: "MAX"
                        readonly property real bottomPadding: 3
                        readonly property string figmaId: "I2557:17208;8664:14955"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 3
                        readonly property string name: "switch-handle-contentItem-checked-hovered"
                        readonly property real rightPadding: 3
                        readonly property real spacing: 0
                        readonly property real topPadding: 3
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17208;8664:14957"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25854.5
                        readonly property real y: 2358.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17210;8664:14801"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background-checked-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25798.5
                        readonly property real y: 2403.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17210;8664:14800"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem-checked-pressed"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 6
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17210;8664:14824"
                        readonly property string filePath: "dark/images/switch-handle-checked-pressed.png"
                        readonly property real height: 14
                        readonly property real leftOffset: 8
                        readonly property real leftShadow: 1
                        readonly property string name: "switch-handle-checked-pressed"
                        readonly property real rightOffset: 8
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 7
                        readonly property real topShadow: 1
                        readonly property real width: 17
                        readonly property real x: 25822.5
                        readonly property real y: 2412.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17210;8664:14803"
                        readonly property string filePath: "dark/images/switch-handle-background-checked-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background-checked-pressed"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25802.5
                        readonly property real y: 2409.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property string alignItems: "MAX"
                        readonly property real bottomPadding: 3
                        readonly property string figmaId: "I2557:17210;8664:14804"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 3
                        readonly property string name: "switch-handle-contentItem-checked-pressed"
                        readonly property real rightPadding: 3
                        readonly property real spacing: 0
                        readonly property real topPadding: 3
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17210;8664:14806"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25854.5
                        readonly property real y: 2409.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17206;2531:14867;2942:5449"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25798.5
                        readonly property real y: 2301.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17206;2531:14867"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem-disabled"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 5
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17206;2531:14867;2531:14816"
                        readonly property string filePath: "dark/images/switch-handle-disabled.png"
                        readonly property real height: 12
                        readonly property real leftOffset: 6
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-disabled"
                        readonly property real rightOffset: 5
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 6
                        readonly property real topShadow: 0
                        readonly property real width: 12
                        readonly property real x: 25806.5
                        readonly property real y: 2311.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17206;2531:14867;2531:14819"
                        readonly property string filePath: "dark/images/switch-handle-background-disabled.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background-disabled"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25802.5
                        readonly property real y: 2307.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:17206;2531:14867;2531:14811"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-handle-contentItem-disabled"
                        readonly property real rightPadding: 4
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17206;2531:14867;6761:24226"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25854.5
                        readonly property real y: 2307.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17200;8664:14878"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25798.5
                        readonly property real y: 2148.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17200;8664:14877"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem-hovered"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 6
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17200;8664:14900"
                        readonly property string filePath: "dark/images/switch-handle-hovered.png"
                        readonly property real height: 14
                        readonly property real leftOffset: 7
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-hovered"
                        readonly property real rightOffset: 6
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 7
                        readonly property real topShadow: 0
                        readonly property real width: 14
                        readonly property real x: 25805.5
                        readonly property real y: 2157.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17200;8664:14880"
                        readonly property string filePath: "dark/images/switch-handle-background-hovered.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background-hovered"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25802.5
                        readonly property real y: 2154.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property real bottomPadding: 3
                        readonly property string figmaId: "I2557:17200;8664:14881"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 3
                        readonly property string name: "switch-handle-contentItem-hovered"
                        readonly property real rightPadding: 3
                        readonly property real spacing: 0
                        readonly property real topPadding: 3
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17200;8664:14883"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25854.5
                        readonly property real y: 2154.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17198;2531:14823;2942:5449"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25798.5
                        readonly property real y: 2091.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17198;2531:14823"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 5
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17198;2531:14823;2531:14816"
                        readonly property string filePath: "dark/images/switch-handle.png"
                        readonly property real height: 12
                        readonly property real leftOffset: 6
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle"
                        readonly property real rightOffset: 5
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 6
                        readonly property real topShadow: 0
                        readonly property real width: 12
                        readonly property real x: 25806.5
                        readonly property real y: 2101.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17198;2531:14823;2531:14819"
                        readonly property string filePath: "dark/images/switch-handle-background.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25802.5
                        readonly property real y: 2097.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:17198;2531:14823;2531:14811"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-handle-contentItem"
                        readonly property real rightPadding: 4
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17198;2531:14823;6761:24226"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25854.5
                        readonly property real y: 2097.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17202;8664:14715"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25798.5
                        readonly property real y: 2199.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:17202;8664:14714"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem-pressed"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 6
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17202;8664:14737"
                        readonly property string filePath: "dark/images/switch-handle-pressed.png"
                        readonly property real height: 14
                        readonly property real leftOffset: 8
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-pressed"
                        readonly property real rightOffset: 8
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 7
                        readonly property real topShadow: 0
                        readonly property real width: 17
                        readonly property real x: 25805.5
                        readonly property real y: 2208.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17202;8664:14717"
                        readonly property string filePath: "dark/images/switch-handle-background-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background-pressed"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25802.5
                        readonly property real y: 2205.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property real bottomPadding: 3
                        readonly property string figmaId: "I2557:17202;8664:14718"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 3
                        readonly property string name: "switch-handle-contentItem-pressed"
                        readonly property real rightPadding: 3
                        readonly property real spacing: 0
                        readonly property real topPadding: 3
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17202;8664:14720"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25854.5
                        readonly property real y: 2205.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

            }

            readonly property QtObject tabbar: QtObject {
                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17270;2556:17466;2556:17413"
                        readonly property string filePath: ""
                        readonly property real height: 48
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-background-disabled"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 462
                        readonly property real x: 26623.5
                        readonly property real y: 2847
                    }

                    readonly property real bottomPadding: 4
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:17270;2556:17466"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "tabbar-contentItem-disabled"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property real leftPadding: 0
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 0
                    readonly property real spacing: 0
                    readonly property QtObject tabButton1: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17270;2556:17466;2556:17415"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton1-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26623.5
                        readonly property real y: 2851
                    }

                    readonly property QtObject tabButton2: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17270;2556:17466;2556:17421"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton2-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26700.5
                        readonly property real y: 2851
                    }

                    readonly property real topPadding: 4
                }

                readonly property QtObject disabled_footer: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17274;2556:17577;2556:17534"
                        readonly property string filePath: ""
                        readonly property real height: 48
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-background-disabled-footer"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 462
                        readonly property real x: 26624
                        readonly property real y: 2977
                    }

                    readonly property real bottomPadding: 4
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:17274;2556:17577"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "tabbar-contentItem-disabled-footer"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property real leftPadding: 0
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 0
                    readonly property real spacing: 0
                    readonly property QtObject tabButton1: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17274;2556:17577;2556:17536"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton1-disabled-footer"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26624
                        readonly property real y: 2981
                    }

                    readonly property QtObject tabButton2: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17274;2556:17577;2556:17537"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton2-disabled-footer"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26701
                        readonly property real y: 2981
                    }

                    readonly property real topPadding: 4
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17268;2556:17439;2556:17413"
                        readonly property string filePath: ""
                        readonly property real height: 48
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-background"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 462
                        readonly property real x: 26624
                        readonly property real y: 2776
                    }

                    readonly property real bottomPadding: 4
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:17268;2556:17439"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "tabbar-contentItem"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property real leftPadding: 0
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 0
                    readonly property real spacing: 0
                    readonly property QtObject tabButton1: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17268;2556:17439;2556:17415"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton1"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26624
                        readonly property real y: 2780
                    }

                    readonly property QtObject tabButton2: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17268;2556:17439;2556:17421"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton2"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26701
                        readonly property real y: 2780
                    }

                    readonly property real topPadding: 4
                }

                readonly property QtObject normal_footer: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17272;2556:17555;2556:17534"
                        readonly property string filePath: ""
                        readonly property real height: 48
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-background-normal-footer"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 462
                        readonly property real x: 26624
                        readonly property real y: 2910
                    }

                    readonly property real bottomPadding: 4
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:17272;2556:17555"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "tabbar-contentItem-normal-footer"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property real leftPadding: 0
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 0
                    readonly property real spacing: 0
                    readonly property QtObject tabButton1: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17272;2556:17555;2556:17536"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton1-normal-footer"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26624
                        readonly property real y: 2914
                    }

                    readonly property QtObject tabButton2: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17272;2556:17555;2556:17537"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton2-normal-footer"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26701
                        readonly property real y: 2914
                    }

                    readonly property real topPadding: 4
                }

            }

            readonly property QtObject tabbutton: QtObject {
                readonly property QtObject checked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17257;2556:16919;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background-checked"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28285
                        readonly property real y: 1952
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:17257;2556:16919"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem-checked"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17257;2556:16919;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon-checked"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28297
                        readonly property real y: 1962
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17257;2556:16919;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label-checked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28325
                        readonly property real y: 1962
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

                readonly property QtObject checked_disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17263;2556:16934;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background-checked-disabled"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28285
                        readonly property real y: 2153
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:17263;2556:16934"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem-checked-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17263;2556:16934;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28297
                        readonly property real y: 2163
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17263;2556:16934;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28325
                        readonly property real y: 2163
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

                readonly property QtObject checked_hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17261;2556:16929;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background-checked-hovered"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28285
                        readonly property real y: 2086
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:17261;2556:16929"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem-checked-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17261;2556:16929;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28297
                        readonly property real y: 2096
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17261;2556:16929;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28325
                        readonly property real y: 2096
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

                readonly property QtObject checked_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17265;2556:16939;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background-checked-pressed"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28285
                        readonly property real y: 2220
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:17265;2556:16939"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem-checked-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17265;2556:16939;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28297
                        readonly property real y: 2230
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17265;2556:16939;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28325
                        readonly property real y: 2230
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17259;2556:16924;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background-disabled"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28285
                        readonly property real y: 2023.24
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:17259;2556:16924"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17259;2556:16924;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28297
                        readonly property real y: 2033.24
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17259;2556:16924;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28325
                        readonly property real y: 2033.24
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17253;2556:16909;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background-hovered"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28285
                        readonly property real y: 1818
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:17253;2556:16909"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17253;2556:16909;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28297
                        readonly property real y: 1828
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17253;2556:16909;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28325
                        readonly property real y: 1828
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17251;2556:16904;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28285
                        readonly property real y: 1751
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:17251;2556:16904"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17251;2556:16904;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28297
                        readonly property real y: 1761
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17251;2556:16904;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28325
                        readonly property real y: 1761
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17255;2556:16914;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background-pressed"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28285
                        readonly property real y: 1885
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:17255;2556:16914"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17255;2556:16914;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28297
                        readonly property real y: 1895
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17255;2556:16914;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28325
                        readonly property real y: 1895
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

            }

            readonly property QtObject textarea: QtObject {
                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 3
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17226;2554:13608;2554:13585"
                        readonly property string filePath: "dark/images/textarea-background-disabled.png"
                        readonly property real height: 50
                        readonly property real leftOffset: 3
                        readonly property real leftShadow: 1
                        readonly property string name: "textarea-background-disabled"
                        readonly property real rightOffset: 3
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 3
                        readonly property real topShadow: 1
                        readonly property real width: 200
                        readonly property real x: 30417.5
                        readonly property real y: 2590
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:17226;2554:13608"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 11
                        readonly property string name: "textarea-contentItem-disabled"
                        readonly property real rightPadding: 11
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17226;2554:13608;2554:13582"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "textarea-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 178
                        readonly property real x: 30428.5
                        readonly property real y: 2595
                    }

                    readonly property real leftPadding: 11
                    readonly property real rightPadding: 11
                    readonly property real topPadding: 5
                }

                readonly property QtObject focused: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 3
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2654:6248;2654:5963;2554:13585"
                        readonly property string filePath: "dark/images/textarea-background-focused.png"
                        readonly property real height: 50
                        readonly property real leftOffset: 3
                        readonly property real leftShadow: 1
                        readonly property string name: "textarea-background-focused"
                        readonly property real rightOffset: 3
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 3
                        readonly property real topShadow: 1
                        readonly property real width: 200
                        readonly property real x: 30417.5
                        readonly property real y: 2667
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2654:6248;2654:5963"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 11
                        readonly property string name: "textarea-contentItem-focused"
                        readonly property real rightPadding: 11
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2654:6248;2654:5963;2554:13582"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "textarea-label-focused"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 178
                        readonly property real x: 30428.5
                        readonly property real y: 2672
                    }

                    readonly property real leftPadding: 11
                    readonly property real rightPadding: 11
                    readonly property real topPadding: 5
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 3
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17224;2554:13603;2554:13585"
                        readonly property string filePath: "dark/images/textarea-background-hovered.png"
                        readonly property real height: 50
                        readonly property real leftOffset: 3
                        readonly property real leftShadow: 1
                        readonly property string name: "textarea-background-hovered"
                        readonly property real rightOffset: 3
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 3
                        readonly property real topShadow: 1
                        readonly property real width: 200
                        readonly property real x: 30417.5
                        readonly property real y: 2513
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:17224;2554:13603"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 11
                        readonly property string name: "textarea-contentItem-hovered"
                        readonly property real rightPadding: 11
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17224;2554:13603;2554:13582"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "textarea-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 178
                        readonly property real x: 30428.5
                        readonly property real y: 2518
                    }

                    readonly property real leftPadding: 11
                    readonly property real rightPadding: 11
                    readonly property real topPadding: 5
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 3
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17222;2554:13588;2554:13585"
                        readonly property string filePath: "dark/images/textarea-background.png"
                        readonly property real height: 50
                        readonly property real leftOffset: 3
                        readonly property real leftShadow: 1
                        readonly property string name: "textarea-background"
                        readonly property real rightOffset: 3
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 3
                        readonly property real topShadow: 1
                        readonly property real width: 200
                        readonly property real x: 30417.5
                        readonly property real y: 2436
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:17222;2554:13588"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 11
                        readonly property string name: "textarea-contentItem"
                        readonly property real rightPadding: 11
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17222;2554:13588;2554:13582"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "textarea-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 178
                        readonly property real x: 30428.5
                        readonly property real y: 2441
                    }

                    readonly property real leftPadding: 11
                    readonly property real rightPadding: 11
                    readonly property real topPadding: 5
                }

            }

            readonly property QtObject textfield: QtObject {
                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17219;2537:15922;2537:15894"
                        readonly property string filePath: "dark/images/textfield-background-disabled.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 1
                        readonly property string name: "textfield-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 4
                        readonly property real topShadow: 1
                        readonly property real width: 158
                        readonly property real x: 29552
                        readonly property real y: 1873.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:17219;2537:15922"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "textfield-contentItem-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17219;2537:15922;2537:15892"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 16
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "textfield-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 28
                        readonly property real x: 29564
                        readonly property real y: 1878.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 5
                }

                readonly property QtObject focused: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2644:5979;2644:5955;2537:15894"
                        readonly property string filePath: "dark/images/textfield-background-focused.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 1
                        readonly property string name: "textfield-background-focused"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 4
                        readonly property real topShadow: 1
                        readonly property real width: 158
                        readonly property real x: 29552
                        readonly property real y: 1942.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2644:5979;2644:5955"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "textfield-contentItem-focused"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2644:5979;2644:5955;2537:15892"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 16
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "textfield-label-focused"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 28
                        readonly property real x: 29564
                        readonly property real y: 1947.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 5
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17217;2537:15917;2537:15894"
                        readonly property string filePath: "dark/images/textfield-background-hovered.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 1
                        readonly property string name: "textfield-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 4
                        readonly property real topShadow: 1
                        readonly property real width: 158
                        readonly property real x: 29552
                        readonly property real y: 1804.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:17217;2537:15917"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "textfield-contentItem-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17217;2537:15917;2537:15892"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 16
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "textfield-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 28
                        readonly property real x: 29564
                        readonly property real y: 1809.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 5
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:17215;2537:15912;2537:15894"
                        readonly property string filePath: "dark/images/textfield-background.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 1
                        readonly property string name: "textfield-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 4
                        readonly property real topShadow: 1
                        readonly property real width: 158
                        readonly property real x: 29552
                        readonly property real y: 1735.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:17215;2537:15912"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "textfield-contentItem"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:17215;2537:15912;2537:15892"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 16
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "textfield-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 28
                        readonly property real x: 29564
                        readonly property real y: 1740.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 5
                }

            }

        }
    }
    readonly property QtObject light: QtObject {
        readonly property QtObject controls: QtObject {
            readonly property QtObject button: QtObject {
                readonly property QtObject checked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15399;2356:10516;2373:10903"
                        readonly property string filePath: "light/images/button-background-checked.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background-checked"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2225
                        readonly property real y: 2467
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:15399;2356:10516"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem-checked"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15399;2356:10516;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon-checked"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2245
                        readonly property real y: 2474
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15399;2356:10516;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label-checked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2269
                        readonly property real y: 2472
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject checked_disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15405;2356:10522;2373:10903"
                        readonly property string filePath: "light/images/button-background-checked-disabled.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background-checked-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2225
                        readonly property real y: 2668
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:15405;2356:10522"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem-checked-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15405;2356:10522;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2245
                        readonly property real y: 2675
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15405;2356:10522;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2269
                        readonly property real y: 2673
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject checked_hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15403;2356:10520;2373:10903"
                        readonly property string filePath: "light/images/button-background-checked-hovered.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background-checked-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2225
                        readonly property real y: 2601
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:15403;2356:10520"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem-checked-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15403;2356:10520;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2245
                        readonly property real y: 2608
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15403;2356:10520;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2269
                        readonly property real y: 2606
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject checked_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15407;2356:10524;2373:10903"
                        readonly property string filePath: "light/images/button-background-checked-pressed.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background-checked-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2225
                        readonly property real y: 2735
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:15407;2356:10524"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem-checked-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15407;2356:10524;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2245
                        readonly property real y: 2742
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15407;2356:10524;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2269
                        readonly property real y: 2740
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15401;2356:10518;2373:10903"
                        readonly property string filePath: "light/images/button-background-disabled.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2225
                        readonly property real y: 2534
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:15401;2356:10518"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15401;2356:10518;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2245
                        readonly property real y: 2541
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15401;2356:10518;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2269
                        readonly property real y: 2539
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15395;2356:10512;2373:10903"
                        readonly property string filePath: "light/images/button-background-hovered.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2225
                        readonly property real y: 2333
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:15395;2356:10512"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15395;2356:10512;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2245
                        readonly property real y: 2340
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15395;2356:10512;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2269
                        readonly property real y: 2338
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15393;2356:10510;2373:10903"
                        readonly property string filePath: "light/images/button-background.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2227.5
                        readonly property real y: 2278
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:15393;2356:10510"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15393;2356:10510;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2247.5
                        readonly property real y: 2285
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15393;2356:10510;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2271.5
                        readonly property real y: 2283
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15397;2356:10514;2373:10903"
                        readonly property string filePath: "light/images/button-background-pressed.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "button-background-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 98
                        readonly property real x: 2225
                        readonly property real y: 2400
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:15397;2356:10514"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "button-contentItem-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15397;2356:10514;4693:13271"
                        readonly property real height: 16
                        readonly property real leftShadow: 0
                        readonly property string name: "button-icon-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 16
                        readonly property real x: 2245
                        readonly property real y: 2407
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15397;2356:10514;2248:10452"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "button-label-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 2269
                        readonly property real y: 2405
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

            }

            readonly property QtObject checkbox: QtObject {
                readonly property QtObject checked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15416;2829:5675;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-checked"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 1941.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15416;2829:5675"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-checked"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15416;2829:5675;2425:10953"
                        readonly property string filePath: "light/images/checkbox-indicator-checked.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-checked"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 1947.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15416;2829:5675;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-checked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 1947.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15426;2427:12224;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-checked-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 2217.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15426;2427:12224"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-checked-disabled"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15426;2427:12224;2425:10953"
                        readonly property string filePath: "light/images/checkbox-indicator-checked-disabled.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-checked-disabled"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 2223.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15426;2427:12224;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 2223.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15430;2829:5737;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-checked-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 2079.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15430;2829:5737"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-checked-hovered"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15430;2829:5737;2425:10953"
                        readonly property string filePath: "light/images/checkbox-indicator-checked-hovered.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-checked-hovered"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 2085.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15430;2829:5737;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 2085.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15428;2425:12191;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-checked-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 2148.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15428;2425:12191"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-checked-pressed"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15428;2425:12191;2425:10953"
                        readonly property string filePath: "light/images/checkbox-indicator-checked-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-checked-pressed"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 2154.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15428;2425:12191;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 2154.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15432;2829:5710;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 2010.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15432;2829:5710"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-disabled"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15432;2829:5710;2425:10953"
                        readonly property string filePath: "light/images/checkbox-indicator-disabled.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-disabled"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 2016.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15432;2829:5710;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 2016.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject disabled_partiallyChecked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15424;2427:12263;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-disabled-partiallyChecked"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 2493.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15424;2427:12263"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-disabled-partiallyChecked"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15424;2427:12263;2425:10953"
                        readonly property string filePath: "light/images/checkbox-indicator-disabled-partiallyChecked.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-disabled-partiallyChecked"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 2499.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15424;2427:12263;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-disabled-partiallyChecked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 2499.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15412;2829:5612;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 1803.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15412;2829:5612"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-hovered"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15412;2829:5612;2425:10953"
                        readonly property string filePath: "light/images/checkbox-indicator-hovered.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-hovered"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 1809.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15412;2829:5612;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 1809.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject hovered_partiallyChecked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15420;2427:12244;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-hovered-partiallyChecked"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 2355.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15420;2427:12244"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-hovered-partiallyChecked"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15420;2427:12244;2425:10953"
                        readonly property string filePath: "light/images/checkbox-indicator-hovered-partiallyChecked.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-hovered-partiallyChecked"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 2361.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15420;2427:12244;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-hovered-partiallyChecked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 2361.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15410;2829:5455;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 1734.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15410;2829:5455"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15410;2829:5455;2425:10953"
                        readonly property string filePath: "light/images/checkbox-indicator.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 1740.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15410;2829:5455;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 1740.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject partiallyChecked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15418;2427:12233;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-partiallyChecked"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 2286.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15418;2427:12233"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-partiallyChecked"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15418;2427:12233;2425:10953"
                        readonly property string filePath: "light/images/checkbox-indicator-partiallyChecked.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-partiallyChecked"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 2292.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15418;2427:12233;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-partiallyChecked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 2292.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject partiallyChecked_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15422;2427:12254;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-partiallyChecked-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 2424.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15422;2427:12254"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-partiallyChecked-pressed"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15422;2427:12254;2425:10953"
                        readonly property string filePath: "light/images/checkbox-indicator-partiallyChecked-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-partiallyChecked-pressed"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 2430.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15422;2427:12254;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-partiallyChecked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 2430.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15414;2829:5648;2425:10961"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-background-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 73
                        readonly property real x: 4752.5
                        readonly property real y: 1872.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15414;2829:5648"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "checkbox-contentItem-pressed"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15414;2829:5648;2425:10953"
                        readonly property string filePath: "light/images/checkbox-indicator-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-indicator-pressed"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 4756.5
                        readonly property real y: 1878.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15414;2829:5648;6820:12339"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "checkbox-label-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 4784.5
                        readonly property real y: 1878.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

            }

            readonly property QtObject flatbutton: QtObject {
                readonly property QtObject checked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I3991:9165;3987:9104;3987:9044"
                        readonly property string filePath: "light/images/flatbutton-background-checked.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 12
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background-checked"
                        readonly property real rightOffset: 12
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 15
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3172.5
                        readonly property real y: 2040.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9165;3987:9104"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem-checked"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9165;3987:9104;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon-checked"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3189.5
                        readonly property real y: 2045.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9165;3987:9104;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label-checked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3217.5
                        readonly property real y: 2045.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject checked_disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I3991:9168;3987:9122;3987:9044"
                        readonly property string filePath: "light/images/flatbutton-background-checked-disabled.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 12
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background-checked-disabled"
                        readonly property real rightOffset: 12
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 15
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3172.5
                        readonly property real y: 2174.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9168;3987:9122"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem-checked-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9168;3987:9122;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3189.5
                        readonly property real y: 2179.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9168;3987:9122;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3217.5
                        readonly property real y: 2179.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject checked_hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I3991:9167;3987:9113;3987:9044"
                        readonly property string filePath: "light/images/flatbutton-background-checked-hovered.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 12
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background-checked-hovered"
                        readonly property real rightOffset: 12
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 15
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3172.5
                        readonly property real y: 2107.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9167;3987:9113"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem-checked-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9167;3987:9113;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3189.5
                        readonly property real y: 2112.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9167;3987:9113;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3217.5
                        readonly property real y: 2112.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject checked_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I3991:9169;3987:9131;3987:9044"
                        readonly property string filePath: "light/images/flatbutton-background-checked-pressed.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 12
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background-checked-pressed"
                        readonly property real rightOffset: 12
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 15
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3172.5
                        readonly property real y: 2241.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9169;3987:9131"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem-checked-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9169;3987:9131;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3189.5
                        readonly property real y: 2246.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9169;3987:9131;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3217.5
                        readonly property real y: 2246.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I3991:9166;3987:9095;3987:9044"
                        readonly property string filePath: "light/images/flatbutton-background-disabled.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 12
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background-disabled"
                        readonly property real rightOffset: 12
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 15
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3172.5
                        readonly property real y: 1973.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9166;3987:9095"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9166;3987:9095;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3189.5
                        readonly property real y: 1978.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9166;3987:9095;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3217.5
                        readonly property real y: 1978.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I3991:9163;3987:9077;3987:9044"
                        readonly property string filePath: "light/images/flatbutton-background-hovered.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 12
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background-hovered"
                        readonly property real rightOffset: 12
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 15
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3172.5
                        readonly property real y: 1839.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9163;3987:9077"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9163;3987:9077;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3189.5
                        readonly property real y: 1844.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9163;3987:9077;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3217.5
                        readonly property real y: 1844.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9162;3987:9068;3987:9044"
                        readonly property string filePath: ""
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3172.5
                        readonly property real y: 1772.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9162;3987:9068"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9162;3987:9068;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3189.5
                        readonly property real y: 1777.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9162;3987:9068;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3217.5
                        readonly property real y: 1777.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I3991:9164;3987:9086;3987:9044"
                        readonly property string filePath: "light/images/flatbutton-background-pressed.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 12
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-background-pressed"
                        readonly property real rightOffset: 12
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 15
                        readonly property real topShadow: 0
                        readonly property real width: 96
                        readonly property real x: 3172.5
                        readonly property real y: 1906.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I3991:9164;3987:9086"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "flatbutton-contentItem-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9164;3987:9086;4709:15937"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-icon-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 3189.5
                        readonly property real y: 1911.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I3991:9164;3987:9086;3987:9039"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "flatbutton-label-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 34
                        readonly property real x: 3217.5
                        readonly property real y: 1911.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 5
                }

            }

            readonly property QtObject itemdelegate: QtObject {
                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15461;2319:9946;2399:11597"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 93
                        readonly property real x: 5697
                        readonly property real y: 2010.5
                    }

                    readonly property real bottomPadding: 8
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 8
                        readonly property string figmaId: "I2557:15461;2319:9946"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "itemdelegate-contentItem-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 12
                        readonly property real topPadding: 8
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15461;2319:9946;2411:10964"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 5704.5
                        readonly property real y: 2018.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 8
                }

                readonly property QtObject highlighted: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15463;2319:9952;2399:11597"
                        readonly property string filePath: "light/images/itemdelegate-background-highlighted.png"
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-background-highlighted"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 93
                        readonly property real x: 5697
                        readonly property real y: 2077.5
                    }

                    readonly property real bottomPadding: 8
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 8
                        readonly property string figmaId: "I2557:15463;2319:9952"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "itemdelegate-contentItem-highlighted"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 12
                        readonly property real topPadding: 8
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15463;2319:9952;2411:10964"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-label-highlighted"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 5704.5
                        readonly property real y: 2085.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 8
                }

                readonly property QtObject highlighted_hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15465;2319:9958;2399:11597"
                        readonly property string filePath: "light/images/itemdelegate-background-highlighted-hovered.png"
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-background-highlighted-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 93
                        readonly property real x: 5697
                        readonly property real y: 2137.5
                    }

                    readonly property real bottomPadding: 8
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 8
                        readonly property string figmaId: "I2557:15465;2319:9958"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "itemdelegate-contentItem-highlighted-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 12
                        readonly property real topPadding: 8
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15465;2319:9958;2411:10964"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-label-highlighted-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 5704.5
                        readonly property real y: 2145.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 8
                }

                readonly property QtObject highlighted_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15467;2319:9970;2399:11597"
                        readonly property string filePath: "light/images/itemdelegate-background-highlighted-pressed.png"
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-background-highlighted-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 93
                        readonly property real x: 5697
                        readonly property real y: 2211.5
                    }

                    readonly property real bottomPadding: 8
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 8
                        readonly property string figmaId: "I2557:15467;2319:9970"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "itemdelegate-contentItem-highlighted-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 12
                        readonly property real topPadding: 8
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15467;2319:9970;2411:10964"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-label-highlighted-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 5704.5
                        readonly property real y: 2219.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 8
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15457;2319:9922;2399:11597"
                        readonly property string filePath: "light/images/itemdelegate-background-hovered.png"
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 93
                        readonly property real x: 5697
                        readonly property real y: 1876.5
                    }

                    readonly property real bottomPadding: 8
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 8
                        readonly property string figmaId: "I2557:15457;2319:9922"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "itemdelegate-contentItem-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 12
                        readonly property real topPadding: 8
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15457;2319:9922;2411:10964"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 5704.5
                        readonly property real y: 1884.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 8
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15455;2319:9916;2399:11597"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 93
                        readonly property real x: 5697
                        readonly property real y: 1810.5
                    }

                    readonly property real bottomPadding: 8
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 8
                        readonly property string figmaId: "I2557:15455;2319:9916"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "itemdelegate-contentItem"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 12
                        readonly property real topPadding: 8
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15455;2319:9916;2411:10964"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 5704.5
                        readonly property real y: 1818.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 8
                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15459;2319:9934;2399:11597"
                        readonly property string filePath: "light/images/itemdelegate-background-pressed.png"
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-background-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 93
                        readonly property real x: 5697
                        readonly property real y: 1943.5
                    }

                    readonly property real bottomPadding: 8
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 8
                        readonly property string figmaId: "I2557:15459;2319:9934"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "itemdelegate-contentItem-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 12
                        readonly property real topPadding: 8
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15459;2319:9934;2411:10964"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "itemdelegate-label-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 5704.5
                        readonly property real y: 1951.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 8
                }

            }

            readonly property QtObject popup: QtObject {
                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 7
                        readonly property real bottomShadow: 9
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15450;2308:11133;2313:11247"
                        readonly property string filePath: "light/images/popup-background.png"
                        readonly property real height: 104
                        readonly property real leftOffset: 7
                        readonly property real leftShadow: 5
                        readonly property string name: "popup-background"
                        readonly property real rightOffset: 7
                        readonly property real rightShadow: 5
                        readonly property real topOffset: 7
                        readonly property real topShadow: 1
                        readonly property real width: 116
                        readonly property real x: 6928
                        readonly property real y: 2195
                    }

                    readonly property real bottomPadding: 16
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 16
                        readonly property string figmaId: "I2557:15450;2308:11133"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 16
                        readonly property string name: "popup-contentItem"
                        readonly property real rightPadding: 16
                        readonly property real spacing: 0
                        readonly property real topPadding: 16
                    }

                    readonly property real leftPadding: 16
                    readonly property real rightPadding: 16
                    readonly property real topPadding: 16
                }

            }

            readonly property QtObject progressbar: QtObject {
                readonly property QtObject disabled: QtObject {
                    readonly property real bottomPadding: 0
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 0
                        readonly property string figmaId: "I4435:9316;4304:9328"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "progressbar-contentItem-disabled"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 0
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I4435:9316;4304:9328;4413:23724"
                        readonly property string filePath: "light/images/progressbar-groove-disabled.png"
                        readonly property real height: 1
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-groove-disabled"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 180
                        readonly property real x: 15598
                        readonly property real y: 2059
                    }

                    readonly property real leftPadding: 0
                    readonly property real rightPadding: 0
                    readonly property real topPadding: 0
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I4435:9316;4304:9328;4267:14564"
                        readonly property real height: 3
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-track-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 48
                        readonly property real x: 15598
                        readonly property real y: 2058
                    }

                }

                readonly property QtObject disabled_indeterminate: QtObject {
                    readonly property real bottomPadding: 0
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 0
                        readonly property string figmaId: "I4435:9318;4304:9355"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "progressbar-contentItem-disabled-indeterminate"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 0
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I4435:9318;4304:9355;4350:35746"
                        readonly property string filePath: ""
                        readonly property real height: 1
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-groove-disabled-indeterminate"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 180
                        readonly property real x: 15598
                        readonly property real y: 2132
                    }

                    readonly property real leftPadding: 0
                    readonly property real rightPadding: 0
                    readonly property real topPadding: 0
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I4435:9318;4304:9355;4403:22724"
                        readonly property real height: 3
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-track-disabled-indeterminate"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 48
                        readonly property real x: 15664
                        readonly property real y: 2131
                    }

                }

                readonly property QtObject indeterminate: QtObject {
                    readonly property real bottomPadding: 0
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 0
                        readonly property string figmaId: "I4435:9317;2450:12847"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "progressbar-contentItem-indeterminate"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 0
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I4435:9317;2450:12847;4350:35746"
                        readonly property string filePath: ""
                        readonly property real height: 1
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-groove-indeterminate"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 180
                        readonly property real x: 15598
                        readonly property real y: 1986
                    }

                    readonly property real leftPadding: 0
                    readonly property real rightPadding: 0
                    readonly property real topPadding: 0
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I4435:9317;2450:12847;4403:22724"
                        readonly property real height: 3
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-track-indeterminate"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 48
                        readonly property real x: 15664
                        readonly property real y: 1985
                    }

                }

                readonly property QtObject normal: QtObject {
                    readonly property real bottomPadding: 0
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 0
                        readonly property string figmaId: "I4435:9315;2450:12841"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "progressbar-contentItem"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 0
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I4435:9315;2450:12841;4413:23724"
                        readonly property string filePath: "light/images/progressbar-groove.png"
                        readonly property real height: 1
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-groove"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 180
                        readonly property real x: 15598
                        readonly property real y: 1913
                    }

                    readonly property real leftPadding: 0
                    readonly property real rightPadding: 0
                    readonly property real topPadding: 0
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I4435:9315;2450:12841;4267:14564"
                        readonly property real height: 3
                        readonly property real leftShadow: 0
                        readonly property string name: "progressbar-track"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 48
                        readonly property real x: 15598
                        readonly property real y: 1912
                    }

                }

            }

            readonly property QtObject radiobutton: QtObject {
                readonly property QtObject checked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15511;2483:15472;2472:12869"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background-checked"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 16867.5
                        readonly property real y: 1977.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15511;2483:15472"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem-checked"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15511;2483:15472;2473:12871"
                        readonly property string filePath: "light/images/radiobutton-indicator-checked.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator-checked"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 16871.5
                        readonly property real y: 1983.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15511;2483:15472;6758:14518"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label-checked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 16903.5
                        readonly property real y: 1985.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15517;2488:15512;2472:12869"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background-checked-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 16867.5
                        readonly property real y: 2255.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15517;2488:15512"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem-checked-disabled"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15517;2488:15512;2473:12871"
                        readonly property string filePath: "light/images/radiobutton-indicator-checked-disabled.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator-checked-disabled"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 16871.5
                        readonly property real y: 2261.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15517;2488:15512;6758:14518"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 16903.5
                        readonly property real y: 2263.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15513;8622:14986"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background-checked-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 16867.5
                        readonly property real y: 2119.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15513;8622:14985"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem-checked-hovered"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15513;8622:14996"
                        readonly property string filePath: "light/images/radiobutton-indicator-checked-hovered.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator-checked-hovered"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 16871.5
                        readonly property real y: 2125.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15513;8622:14988"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 16903.5
                        readonly property real y: 2127.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15515;8622:15023"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background-checked-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 16867.5
                        readonly property real y: 2186.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15515;8622:15022"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem-checked-pressed"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15515;8622:15033"
                        readonly property string filePath: "light/images/radiobutton-indicator-checked-pressed.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator-checked-pressed"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 16871.5
                        readonly property real y: 2192.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15515;8622:15025"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 16903.5
                        readonly property real y: 2194.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15519;2483:15480;2472:12869"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 16867.5
                        readonly property real y: 2048.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15519;2483:15480"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem-disabled"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15519;2483:15480;2473:12871"
                        readonly property string filePath: "light/images/radiobutton-indicator-disabled.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator-disabled"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 16871.5
                        readonly property real y: 2054.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15519;2483:15480;6758:14518"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 16903.5
                        readonly property real y: 2056.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15507;2473:12899;2472:12869"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 16867.5
                        readonly property real y: 1839.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15507;2473:12899"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem-hovered"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15507;2473:12899;2473:12871"
                        readonly property string filePath: "light/images/radiobutton-indicator-hovered.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator-hovered"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 16871.5
                        readonly property real y: 1845.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15507;2473:12899;6758:14518"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 16903.5
                        readonly property real y: 1847.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15505;2473:12891;2472:12869"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 16867.5
                        readonly property real y: 1770.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15505;2473:12891"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15505;2473:12891;2473:12871"
                        readonly property string filePath: "light/images/radiobutton-indicator.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 16871.5
                        readonly property real y: 1776.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15505;2473:12891;6758:14518"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 16903.5
                        readonly property real y: 1778.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15509;8622:15060"
                        readonly property string filePath: ""
                        readonly property real height: 36
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-background-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 16867.5
                        readonly property real y: 1908.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15509;8622:15059"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "radiobutton-contentItem-pressed"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 8
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject indicator: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15509;8622:15070"
                        readonly property string filePath: "light/images/radiobutton-indicator-pressed.png"
                        readonly property real height: 24
                        readonly property real leftOffset: 1
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-indicator-pressed"
                        readonly property real rightOffset: 1
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 1
                        readonly property real topShadow: 0
                        readonly property real width: 24
                        readonly property real x: 16871.5
                        readonly property real y: 1914.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15509;8622:15062"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "radiobutton-label-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 16903.5
                        readonly property real y: 1916.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: 8
                    readonly property real topPadding: 6
                }

            }

            readonly property QtObject rangeslider: QtObject {
                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15528;2509:12481;2509:12419"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 200
                        readonly property real x: 17634
                        readonly property real y: 2839
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:15528;2509:12481"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "rangeslider-contentItem-disabled"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject first_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15528;2509:12481;4189:38496"
                        readonly property string filePath: "light/images/rangeslider-first-handle-disabled.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-first-handle-disabled"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 17662
                        readonly property real y: 2839
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15528;2509:12481;4178:28261"
                        readonly property string filePath: "light/images/rangeslider-groove-disabled.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-groove-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 184
                        readonly property real x: 17642
                        readonly property real y: 2847
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property QtObject second_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15528;2509:12481;4191:43003"
                        readonly property string filePath: "light/images/rangeslider-second-handle-disabled.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-second-handle-disabled"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 17786
                        readonly property real y: 2839
                    }

                    readonly property real spacing: -154
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15528;2509:12481;4189:38505"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-track-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 124
                        readonly property real x: 17672
                        readonly property real y: 2847
                    }

                }

                readonly property QtObject handle_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15526;8624:14526"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-background-handle-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 200
                        readonly property real x: 17634
                        readonly property real y: 2781
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:15526;8624:14525"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "rangeslider-contentItem-handle-pressed"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject first_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15526;8624:14556"
                        readonly property string filePath: "light/images/rangeslider-first-handle-handle-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-first-handle-handle-pressed"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 17662
                        readonly property real y: 2781
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15526;8624:14529"
                        readonly property string filePath: "light/images/rangeslider-groove-handle-pressed.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-groove-handle-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 184
                        readonly property real x: 17642
                        readonly property real y: 2789
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property QtObject second_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15526;8624:14627"
                        readonly property string filePath: "light/images/rangeslider-second-handle-handle-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-second-handle-handle-pressed"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 17786
                        readonly property real y: 2781
                    }

                    readonly property real spacing: -154
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15526;8624:14531"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-track-handle-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 124
                        readonly property real x: 17672
                        readonly property real y: 2789
                    }

                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15524;8624:14397"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 200
                        readonly property real x: 17634
                        readonly property real y: 2723
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:15524;8624:14396"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "rangeslider-contentItem-hovered"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject first_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15524;8624:14427"
                        readonly property string filePath: "light/images/rangeslider-first-handle-hovered.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-first-handle-hovered"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 17662
                        readonly property real y: 2723
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15524;8624:14400"
                        readonly property string filePath: "light/images/rangeslider-groove-hovered.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-groove-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 184
                        readonly property real x: 17642
                        readonly property real y: 2731
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property QtObject second_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15524;8624:14506"
                        readonly property string filePath: "light/images/rangeslider-second-handle-hovered.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-second-handle-hovered"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 17786
                        readonly property real y: 2723
                    }

                    readonly property real spacing: -154
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15524;8624:14402"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-track-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 124
                        readonly property real x: 17672
                        readonly property real y: 2731
                    }

                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15522;2509:12436;2509:12419"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 200
                        readonly property real x: 17634
                        readonly property real y: 2665
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:15522;2509:12436"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "rangeslider-contentItem"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject first_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15522;2509:12436;4189:38496"
                        readonly property string filePath: "light/images/rangeslider-first-handle.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-first-handle"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 17662
                        readonly property real y: 2665
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15522;2509:12436;4178:28261"
                        readonly property string filePath: "light/images/rangeslider-groove.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-groove"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 184
                        readonly property real x: 17642
                        readonly property real y: 2673
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property QtObject second_handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15522;2509:12436;4191:43003"
                        readonly property string filePath: "light/images/rangeslider-second-handle.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "rangeslider-second-handle"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 17786
                        readonly property real y: 2665
                    }

                    readonly property real spacing: -154
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15522;2509:12436;4189:38505"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "rangeslider-track"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 124
                        readonly property real x: 17672
                        readonly property real y: 2673
                    }

                }

            }

            readonly property QtObject slider: QtObject {
                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15554;2506:12695;4200:48590"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 224
                        readonly property real x: 22622
                        readonly property real y: 2827.5
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:15554;2506:12695"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "slider-contentItem-disabled"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15554;2506:12695;4385:9106"
                        readonly property string filePath: "light/images/slider-groove-disabled.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-groove-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 208
                        readonly property real x: 22630
                        readonly property real y: 2835.5
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15554;2506:12695;4200:48601"
                        readonly property string filePath: "light/images/slider-handle-disabled.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "slider-handle-disabled"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 22793
                        readonly property real y: 2827.5
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: -208
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15554;2506:12695;4200:48597"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-track-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 173
                        readonly property real x: 22630
                        readonly property real y: 2835.5
                    }

                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15550;8624:13850"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 224
                        readonly property real x: 22622
                        readonly property real y: 2708.5
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:15550;8624:13849"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "slider-contentItem-hovered"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15550;8624:13853"
                        readonly property string filePath: "light/images/slider-groove-hovered.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-groove-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 208
                        readonly property real x: 22630
                        readonly property real y: 2716.5
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15550;8624:13874"
                        readonly property string filePath: "light/images/slider-handle-hovered.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "slider-handle-hovered"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 22793
                        readonly property real y: 2708.5
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: -208
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15550;8624:13855"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-track-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 173
                        readonly property real x: 22630
                        readonly property real y: 2716.5
                    }

                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15548;2506:12656;4200:48590"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 224
                        readonly property real x: 22622
                        readonly property real y: 2649.5
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:15548;2506:12656"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "slider-contentItem"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15548;2506:12656;4385:9106"
                        readonly property string filePath: "light/images/slider-groove.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-groove"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 208
                        readonly property real x: 22630
                        readonly property real y: 2657.5
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15548;2506:12656;4200:48601"
                        readonly property string filePath: "light/images/slider-handle.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "slider-handle"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 22793
                        readonly property real y: 2649.5
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: -208
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15548;2506:12656;4200:48597"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-track"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 173
                        readonly property real x: 22630
                        readonly property real y: 2657.5
                    }

                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15552;8624:14647"
                        readonly property string filePath: ""
                        readonly property real height: 20
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-background-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 224
                        readonly property real x: 22622
                        readonly property real y: 2768.5
                    }

                    readonly property real bottomPadding: 2
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 2
                        readonly property string figmaId: "I2557:15552;8624:14646"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 8
                        readonly property string name: "slider-contentItem-pressed"
                        readonly property real rightPadding: 8
                        readonly property real spacing: 0
                        readonly property real topPadding: 2
                    }

                    readonly property QtObject groove: QtObject {
                        readonly property real bottomOffset: 1
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15552;8624:14650"
                        readonly property string filePath: "light/images/slider-groove-pressed.png"
                        readonly property real height: 4
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-groove-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 2
                        readonly property real topShadow: 0
                        readonly property real width: 208
                        readonly property real x: 22630
                        readonly property real y: 2776.5
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15552;8624:14671"
                        readonly property string filePath: "light/images/slider-handle-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 1
                        readonly property string name: "slider-handle-pressed"
                        readonly property real rightOffset: 9
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 10
                        readonly property real topShadow: 1
                        readonly property real width: 20
                        readonly property real x: 22793
                        readonly property real y: 2768.5
                    }

                    readonly property real leftPadding: 8
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 8
                    readonly property real spacing: -208
                    readonly property real topPadding: 2
                    readonly property QtObject track: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15552;8624:14652"
                        readonly property real height: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "slider-track-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 173
                        readonly property real x: 22630
                        readonly property real y: 2776.5
                    }

                }

            }

            readonly property QtObject switch_: QtObject {
                readonly property QtObject checked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15580;2531:14856;4350:34538"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background-checked"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25618.5
                        readonly property real y: 2250.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15580;2531:14856"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem-checked"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 5
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15580;2531:14856;4350:34543"
                        readonly property string filePath: "light/images/switch-handle-checked.png"
                        readonly property real height: 12
                        readonly property real leftOffset: 6
                        readonly property real leftShadow: 1
                        readonly property string name: "switch-handle-checked"
                        readonly property real rightOffset: 5
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 6
                        readonly property real topShadow: 1
                        readonly property real width: 12
                        readonly property real x: 25646.5
                        readonly property real y: 2260.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15580;2531:14856;4350:34541"
                        readonly property string filePath: "light/images/switch-handle-background-checked.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background-checked"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25622.5
                        readonly property real y: 2256.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property string alignItems: "MAX"
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:15580;2531:14856;4350:34542"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-handle-contentItem-checked"
                        readonly property real rightPadding: 4
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15580;2531:14856;6761:23654"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label-checked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25674.5
                        readonly property real y: 2256.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15588;2531:14900;4350:34538"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background-checked-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25618.5
                        readonly property real y: 2454.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15588;2531:14900"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem-checked-disabled"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 5
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15588;2531:14900;4350:34543"
                        readonly property string filePath: "light/images/switch-handle-checked-disabled.png"
                        readonly property real height: 12
                        readonly property real leftOffset: 6
                        readonly property real leftShadow: 1
                        readonly property string name: "switch-handle-checked-disabled"
                        readonly property real rightOffset: 5
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 6
                        readonly property real topShadow: 1
                        readonly property real width: 12
                        readonly property real x: 25646.5
                        readonly property real y: 2464.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15588;2531:14900;4350:34541"
                        readonly property string filePath: "light/images/switch-handle-background-checked-disabled.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background-checked-disabled"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25622.5
                        readonly property real y: 2460.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property string alignItems: "MAX"
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:15588;2531:14900;4350:34542"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-handle-contentItem-checked-disabled"
                        readonly property real rightPadding: 4
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15588;2531:14900;6761:23654"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25674.5
                        readonly property real y: 2460.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15584;8664:14952"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background-checked-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25618.5
                        readonly property real y: 2352.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15584;8664:14951"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem-checked-hovered"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 6
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15584;8664:14975"
                        readonly property string filePath: "light/images/switch-handle-checked-hovered.png"
                        readonly property real height: 14
                        readonly property real leftOffset: 7
                        readonly property real leftShadow: 1
                        readonly property string name: "switch-handle-checked-hovered"
                        readonly property real rightOffset: 6
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 7
                        readonly property real topShadow: 1
                        readonly property real width: 14
                        readonly property real x: 25645.5
                        readonly property real y: 2361.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15584;8664:14954"
                        readonly property string filePath: "light/images/switch-handle-background-checked-hovered.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background-checked-hovered"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25622.5
                        readonly property real y: 2358.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property string alignItems: "MAX"
                        readonly property real bottomPadding: 3
                        readonly property string figmaId: "I2557:15584;8664:14955"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 3
                        readonly property string name: "switch-handle-contentItem-checked-hovered"
                        readonly property real rightPadding: 3
                        readonly property real spacing: 0
                        readonly property real topPadding: 3
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15584;8664:14957"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25674.5
                        readonly property real y: 2358.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

                readonly property QtObject checked_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15586;8664:14801"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background-checked-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25618.5
                        readonly property real y: 2403.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15586;8664:14800"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem-checked-pressed"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 6
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15586;8664:14824"
                        readonly property string filePath: "light/images/switch-handle-checked-pressed.png"
                        readonly property real height: 14
                        readonly property real leftOffset: 8
                        readonly property real leftShadow: 1
                        readonly property string name: "switch-handle-checked-pressed"
                        readonly property real rightOffset: 8
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 7
                        readonly property real topShadow: 1
                        readonly property real width: 17
                        readonly property real x: 25642.5
                        readonly property real y: 2412.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15586;8664:14803"
                        readonly property string filePath: "light/images/switch-handle-background-checked-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background-checked-pressed"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25622.5
                        readonly property real y: 2409.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property string alignItems: "MAX"
                        readonly property real bottomPadding: 3
                        readonly property string figmaId: "I2557:15586;8664:14804"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 3
                        readonly property string name: "switch-handle-contentItem-checked-pressed"
                        readonly property real rightPadding: 3
                        readonly property real spacing: 0
                        readonly property real topPadding: 3
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15586;8664:14806"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25674.5
                        readonly property real y: 2409.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15582;2531:14867;2942:5449"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25618.5
                        readonly property real y: 2301.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15582;2531:14867"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem-disabled"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 5
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15582;2531:14867;2531:14816"
                        readonly property string filePath: "light/images/switch-handle-disabled.png"
                        readonly property real height: 12
                        readonly property real leftOffset: 6
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-disabled"
                        readonly property real rightOffset: 5
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 6
                        readonly property real topShadow: 0
                        readonly property real width: 12
                        readonly property real x: 25626.5
                        readonly property real y: 2311.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15582;2531:14867;2531:14819"
                        readonly property string filePath: "light/images/switch-handle-background-disabled.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background-disabled"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25622.5
                        readonly property real y: 2307.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:15582;2531:14867;2531:14811"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-handle-contentItem-disabled"
                        readonly property real rightPadding: 4
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15582;2531:14867;6761:24226"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25674.5
                        readonly property real y: 2307.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15576;8664:14878"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25618.5
                        readonly property real y: 2148.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15576;8664:14877"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem-hovered"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 6
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15576;8664:14900"
                        readonly property string filePath: "light/images/switch-handle-hovered.png"
                        readonly property real height: 14
                        readonly property real leftOffset: 7
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-hovered"
                        readonly property real rightOffset: 6
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 7
                        readonly property real topShadow: 0
                        readonly property real width: 14
                        readonly property real x: 25625.5
                        readonly property real y: 2157.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15576;8664:14880"
                        readonly property string filePath: "light/images/switch-handle-background-hovered.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background-hovered"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25622.5
                        readonly property real y: 2154.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property real bottomPadding: 3
                        readonly property string figmaId: "I2557:15576;8664:14881"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 3
                        readonly property string name: "switch-handle-contentItem-hovered"
                        readonly property real rightPadding: 3
                        readonly property real spacing: 0
                        readonly property real topPadding: 3
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15576;8664:14883"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25674.5
                        readonly property real y: 2154.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15574;2531:14823;2942:5449"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25618.5
                        readonly property real y: 2091.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15574;2531:14823"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 5
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15574;2531:14823;2531:14816"
                        readonly property string filePath: "light/images/switch-handle.png"
                        readonly property real height: 12
                        readonly property real leftOffset: 6
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle"
                        readonly property real rightOffset: 5
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 6
                        readonly property real topShadow: 0
                        readonly property real width: 12
                        readonly property real x: 25626.5
                        readonly property real y: 2101.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15574;2531:14823;2531:14819"
                        readonly property string filePath: "light/images/switch-handle-background.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25622.5
                        readonly property real y: 2097.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:15574;2531:14823;2531:14811"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-handle-contentItem"
                        readonly property real rightPadding: 4
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15574;2531:14823;6761:24226"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25674.5
                        readonly property real y: 2097.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15578;8664:14715"
                        readonly property string filePath: ""
                        readonly property real height: 32
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-background-pressed"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 4
                        readonly property real topShadow: 0
                        readonly property real width: 99
                        readonly property real x: 25618.5
                        readonly property real y: 2199.5
                    }

                    readonly property real bottomPadding: 6
                    readonly property QtObject contentItem: QtObject {
                        readonly property real bottomPadding: 6
                        readonly property string figmaId: "I2557:15578;8664:14714"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 4
                        readonly property string name: "switch-contentItem-pressed"
                        readonly property real rightPadding: 10
                        readonly property real spacing: 12
                        readonly property real topPadding: 6
                    }

                    readonly property QtObject handle: QtObject {
                        readonly property real bottomOffset: 6
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15578;8664:14737"
                        readonly property string filePath: "light/images/switch-handle-pressed.png"
                        readonly property real height: 14
                        readonly property real leftOffset: 8
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-pressed"
                        readonly property real rightOffset: 8
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 7
                        readonly property real topShadow: 0
                        readonly property real width: 17
                        readonly property real x: 25625.5
                        readonly property real y: 2208.5
                    }

                    readonly property QtObject handle_background: QtObject {
                        readonly property real bottomOffset: 9
                        readonly property real bottomShadow: 0
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15578;8664:14717"
                        readonly property string filePath: "light/images/switch-handle-background-pressed.png"
                        readonly property real height: 20
                        readonly property real leftOffset: 10
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-handle-background-pressed"
                        readonly property real rightOffset: 10
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 10
                        readonly property real topShadow: 0
                        readonly property real width: 40
                        readonly property real x: 25622.5
                        readonly property real y: 2205.5
                    }

                    readonly property QtObject handle_contentItem: QtObject {
                        readonly property real bottomPadding: 3
                        readonly property string figmaId: "I2557:15578;8664:14718"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 3
                        readonly property string name: "switch-handle-contentItem-pressed"
                        readonly property real rightPadding: 3
                        readonly property real spacing: 0
                        readonly property real topPadding: 3
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15578;8664:14720"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "switch-label-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 2
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 33
                        readonly property real x: 25674.5
                        readonly property real y: 2205.5
                    }

                    readonly property real leftPadding: 4
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 10
                    readonly property real spacing: 12
                    readonly property real topPadding: 6
                }

            }

            readonly property QtObject tabbar: QtObject {
                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15646;2556:17466;2556:17413"
                        readonly property string filePath: ""
                        readonly property real height: 48
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-background-disabled"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 462
                        readonly property real x: 26270.5
                        readonly property real y: 2847
                    }

                    readonly property real bottomPadding: 4
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:15646;2556:17466"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "tabbar-contentItem-disabled"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property real leftPadding: 0
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 0
                    readonly property real spacing: 0
                    readonly property QtObject tabButton1: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15646;2556:17466;2556:17415"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton1-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26270.5
                        readonly property real y: 2851
                    }

                    readonly property QtObject tabButton2: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15646;2556:17466;2556:17421"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton2-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26347.5
                        readonly property real y: 2851
                    }

                    readonly property real topPadding: 4
                }

                readonly property QtObject disabled_footer: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15650;2556:17577;2556:17534"
                        readonly property string filePath: ""
                        readonly property real height: 48
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-background-disabled-footer"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 462
                        readonly property real x: 26271
                        readonly property real y: 2977
                    }

                    readonly property real bottomPadding: 4
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:15650;2556:17577"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "tabbar-contentItem-disabled-footer"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property real leftPadding: 0
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 0
                    readonly property real spacing: 0
                    readonly property QtObject tabButton1: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15650;2556:17577;2556:17536"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton1-disabled-footer"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26271
                        readonly property real y: 2981
                    }

                    readonly property QtObject tabButton2: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15650;2556:17577;2556:17537"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton2-disabled-footer"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26348
                        readonly property real y: 2981
                    }

                    readonly property real topPadding: 4
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15644;2556:17439;2556:17413"
                        readonly property string filePath: ""
                        readonly property real height: 48
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-background"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 462
                        readonly property real x: 26271
                        readonly property real y: 2776
                    }

                    readonly property real bottomPadding: 4
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:15644;2556:17439"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "tabbar-contentItem"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property real leftPadding: 0
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 0
                    readonly property real spacing: 0
                    readonly property QtObject tabButton1: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15644;2556:17439;2556:17415"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton1"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26271
                        readonly property real y: 2780
                    }

                    readonly property QtObject tabButton2: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15644;2556:17439;2556:17421"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton2"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26348
                        readonly property real y: 2780
                    }

                    readonly property real topPadding: 4
                }

                readonly property QtObject normal_footer: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15648;2556:17555;2556:17534"
                        readonly property string filePath: ""
                        readonly property real height: 48
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-background-normal-footer"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 462
                        readonly property real x: 26271
                        readonly property real y: 2910
                    }

                    readonly property real bottomPadding: 4
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 4
                        readonly property string figmaId: "I2557:15648;2556:17555"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 0
                        readonly property string name: "tabbar-contentItem-normal-footer"
                        readonly property real rightPadding: 0
                        readonly property real spacing: 0
                        readonly property real topPadding: 4
                    }

                    readonly property real leftPadding: 0
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 0
                    readonly property real spacing: 0
                    readonly property QtObject tabButton1: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15648;2556:17555;2556:17536"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton1-normal-footer"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26271
                        readonly property real y: 2914
                    }

                    readonly property QtObject tabButton2: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15648;2556:17555;2556:17537"
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbar-tabButton2-normal-footer"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 26348
                        readonly property real y: 2914
                    }

                    readonly property real topPadding: 4
                }

            }

            readonly property QtObject tabbutton: QtObject {
                readonly property QtObject checked: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15633;2556:16919;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background-checked"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28142
                        readonly property real y: 1948.5
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:15633;2556:16919"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem-checked"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15633;2556:16919;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon-checked"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28154
                        readonly property real y: 1958.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15633;2556:16919;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label-checked"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28182
                        readonly property real y: 1958.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

                readonly property QtObject checked_disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15639;2556:16934;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background-checked-disabled"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28142
                        readonly property real y: 2149.5
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:15639;2556:16934"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem-checked-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15639;2556:16934;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28154
                        readonly property real y: 2159.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15639;2556:16934;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label-checked-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28182
                        readonly property real y: 2159.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

                readonly property QtObject checked_hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15637;2556:16929;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background-checked-hovered"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28142
                        readonly property real y: 2082.5
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:15637;2556:16929"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem-checked-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15637;2556:16929;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28154
                        readonly property real y: 2092.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15637;2556:16929;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label-checked-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28182
                        readonly property real y: 2092.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

                readonly property QtObject checked_pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15641;2556:16939;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background-checked-pressed"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28142
                        readonly property real y: 2216.5
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:15641;2556:16939"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem-checked-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15641;2556:16939;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28154
                        readonly property real y: 2226.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15641;2556:16939;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label-checked-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28182
                        readonly property real y: 2226.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15635;2556:16924;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background-disabled"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28142
                        readonly property real y: 2023.24
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:15635;2556:16924"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15635;2556:16924;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon-disabled"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28154
                        readonly property real y: 2033.24
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15635;2556:16924;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28182
                        readonly property real y: 2033.24
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15629;2556:16909;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background-hovered"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28142
                        readonly property real y: 1814.5
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:15629;2556:16909"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15629;2556:16909;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon-hovered"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28154
                        readonly property real y: 1824.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15629;2556:16909;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28182
                        readonly property real y: 1824.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15627;2556:16904;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28142
                        readonly property real y: 1747.5
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:15627;2556:16904"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15627;2556:16904;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28154
                        readonly property real y: 1757.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15627;2556:16904;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28182
                        readonly property real y: 1757.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

                readonly property QtObject pressed: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 0
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15631;2556:16914;2556:16901"
                        readonly property string filePath: ""
                        readonly property real height: 40
                        readonly property real leftOffset: 0
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-background-pressed"
                        readonly property real rightOffset: 0
                        readonly property real rightShadow: 0
                        readonly property real topOffset: 0
                        readonly property real topShadow: 0
                        readonly property real width: 77
                        readonly property real x: 28142
                        readonly property real y: 1881.5
                    }

                    readonly property real bottomPadding: 10
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 10
                        readonly property string figmaId: "I2557:15631;2556:16914"
                        readonly property string layoutMode: "HORIZONTAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "tabbutton-contentItem-pressed"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 8
                        readonly property real topPadding: 10
                    }

                    readonly property QtObject icon: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15631;2556:16914;6815:11841"
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-icon-pressed"
                        readonly property real rightShadow: 0
                        readonly property real topShadow: 0
                        readonly property real width: 20
                        readonly property real x: 28154
                        readonly property real y: 1891.5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15631;2556:16914;2556:16898"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "tabbutton-label-pressed"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 4
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 25
                        readonly property real x: 28182
                        readonly property real y: 1891.5
                    }

                    readonly property real leftPadding: 12
                    readonly property bool mirrored: false
                    readonly property real rightPadding: 12
                    readonly property real spacing: 8
                    readonly property real topPadding: 10
                }

            }

            readonly property QtObject textarea: QtObject {
                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 3
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15602;2554:13608;2554:13585"
                        readonly property string filePath: "light/images/textarea-background-disabled.png"
                        readonly property real height: 50
                        readonly property real leftOffset: 3
                        readonly property real leftShadow: 1
                        readonly property string name: "textarea-background-disabled"
                        readonly property real rightOffset: 3
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 3
                        readonly property real topShadow: 1
                        readonly property real width: 200
                        readonly property real x: 30156
                        readonly property real y: 2590
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:15602;2554:13608"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 11
                        readonly property string name: "textarea-contentItem-disabled"
                        readonly property real rightPadding: 11
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15602;2554:13608;2554:13582"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "textarea-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 178
                        readonly property real x: 30167
                        readonly property real y: 2595
                    }

                    readonly property real leftPadding: 11
                    readonly property real rightPadding: 11
                    readonly property real topPadding: 5
                }

                readonly property QtObject focused: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 3
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2654:6236;2654:5963;2554:13585"
                        readonly property string filePath: "light/images/textarea-background-focused.png"
                        readonly property real height: 50
                        readonly property real leftOffset: 3
                        readonly property real leftShadow: 1
                        readonly property string name: "textarea-background-focused"
                        readonly property real rightOffset: 3
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 3
                        readonly property real topShadow: 1
                        readonly property real width: 200
                        readonly property real x: 30156
                        readonly property real y: 2667
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2654:6236;2654:5963"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 11
                        readonly property string name: "textarea-contentItem-focused"
                        readonly property real rightPadding: 11
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2654:6236;2654:5963;2554:13582"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "textarea-label-focused"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 178
                        readonly property real x: 30167
                        readonly property real y: 2672
                    }

                    readonly property real leftPadding: 11
                    readonly property real rightPadding: 11
                    readonly property real topPadding: 5
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 3
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15600;2554:13603;2554:13585"
                        readonly property string filePath: "light/images/textarea-background-hovered.png"
                        readonly property real height: 50
                        readonly property real leftOffset: 3
                        readonly property real leftShadow: 1
                        readonly property string name: "textarea-background-hovered"
                        readonly property real rightOffset: 3
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 3
                        readonly property real topShadow: 1
                        readonly property real width: 200
                        readonly property real x: 30156
                        readonly property real y: 2513
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:15600;2554:13603"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 11
                        readonly property string name: "textarea-contentItem-hovered"
                        readonly property real rightPadding: 11
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15600;2554:13603;2554:13582"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "textarea-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 178
                        readonly property real x: 30167
                        readonly property real y: 2518
                    }

                    readonly property real leftPadding: 11
                    readonly property real rightPadding: 11
                    readonly property real topPadding: 5
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 3
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15598;2554:13588;2554:13585"
                        readonly property string filePath: "light/images/textarea-background.png"
                        readonly property real height: 50
                        readonly property real leftOffset: 3
                        readonly property real leftShadow: 1
                        readonly property string name: "textarea-background"
                        readonly property real rightOffset: 3
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 3
                        readonly property real topShadow: 1
                        readonly property real width: 200
                        readonly property real x: 30156
                        readonly property real y: 2436
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:15598;2554:13588"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 11
                        readonly property string name: "textarea-contentItem"
                        readonly property real rightPadding: 11
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15598;2554:13588;2554:13582"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 14
                        readonly property real height: 40
                        readonly property real leftShadow: 0
                        readonly property string name: "textarea-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 32
                        readonly property real topShadow: 0
                        readonly property real width: 178
                        readonly property real x: 30167
                        readonly property real y: 2441
                    }

                    readonly property real leftPadding: 11
                    readonly property real rightPadding: 11
                    readonly property real topPadding: 5
                }

            }

            readonly property QtObject textfield: QtObject {
                readonly property QtObject disabled: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15595;2537:15922;2537:15894"
                        readonly property string filePath: "light/images/textfield-background-disabled.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 1
                        readonly property string name: "textfield-background-disabled"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 4
                        readonly property real topShadow: 1
                        readonly property real width: 158
                        readonly property real x: 29362
                        readonly property real y: 1873.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:15595;2537:15922"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "textfield-contentItem-disabled"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15595;2537:15922;2537:15892"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 16
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "textfield-label-disabled"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 28
                        readonly property real x: 29374
                        readonly property real y: 1878.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 5
                }

                readonly property QtObject focused: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2644:5967;2644:5955;2537:15894"
                        readonly property string filePath: "light/images/textfield-background-focused.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 1
                        readonly property string name: "textfield-background-focused"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 4
                        readonly property real topShadow: 1
                        readonly property real width: 158
                        readonly property real x: 29362
                        readonly property real y: 1942.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2644:5967;2644:5955"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "textfield-contentItem-focused"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2644:5967;2644:5955;2537:15892"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 16
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "textfield-label-focused"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 28
                        readonly property real x: 29374
                        readonly property real y: 1947.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 5
                }

                readonly property QtObject hovered: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15593;2537:15917;2537:15894"
                        readonly property string filePath: "light/images/textfield-background-hovered.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 1
                        readonly property string name: "textfield-background-hovered"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 4
                        readonly property real topShadow: 1
                        readonly property real width: 158
                        readonly property real x: 29362
                        readonly property real y: 1804.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:15593;2537:15917"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "textfield-contentItem-hovered"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15593;2537:15917;2537:15892"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 16
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "textfield-label-hovered"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 28
                        readonly property real x: 29374
                        readonly property real y: 1809.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 5
                }

                readonly property QtObject normal: QtObject {
                    readonly property QtObject background: QtObject {
                        readonly property real bottomOffset: 4
                        readonly property real bottomShadow: 1
                        readonly property string exportType: "image"
                        readonly property string figmaId: "I2557:15591;2537:15912;2537:15894"
                        readonly property string filePath: "light/images/textfield-background.png"
                        readonly property real height: 30
                        readonly property real leftOffset: 4
                        readonly property real leftShadow: 1
                        readonly property string name: "textfield-background"
                        readonly property real rightOffset: 4
                        readonly property real rightShadow: 1
                        readonly property real topOffset: 4
                        readonly property real topShadow: 1
                        readonly property real width: 158
                        readonly property real x: 29362
                        readonly property real y: 1735.5
                    }

                    readonly property real bottomPadding: 5
                    readonly property QtObject contentItem: QtObject {
                        readonly property string alignItems: "CENTER"
                        readonly property real bottomPadding: 5
                        readonly property string figmaId: "I2557:15591;2537:15912"
                        readonly property string layoutMode: "VERTICAL"
                        readonly property real leftPadding: 12
                        readonly property string name: "textfield-contentItem"
                        readonly property real rightPadding: 12
                        readonly property real spacing: 0
                        readonly property real topPadding: 5
                    }

                    readonly property QtObject label: QtObject {
                        readonly property real bottomShadow: 0
                        readonly property string figmaId: "I2557:15591;2537:15912;2537:15892"
                        readonly property string fontFamily: "Segoe UI"
                        readonly property real fontSize: 16
                        readonly property real height: 20
                        readonly property real leftShadow: 0
                        readonly property string name: "textfield-label"
                        readonly property real rightShadow: 0
                        readonly property real textHAlignment: 1
                        readonly property real textVAlignment: 128
                        readonly property real topShadow: 0
                        readonly property real width: 28
                        readonly property real x: 29374
                        readonly property real y: 1740.5
                    }

                    readonly property real leftPadding: 12
                    readonly property real rightPadding: 12
                    readonly property real topPadding: 5
                }

            }

        }
    }
}
