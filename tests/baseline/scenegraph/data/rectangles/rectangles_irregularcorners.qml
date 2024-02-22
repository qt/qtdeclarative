// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: r_0000
    width: (standardRectWidth+1) * 10 + 3
    height: (standardRectHeight+1) * 10 + 3
    color: "white"

    property int standardRectWidth: 32
    property int standardRectHeight: 48
    property int yPlacementRow_0: 2
    property int yPlacementRow_1: 2 + (standardRectHeight+1)
    property int yPlacementRow_2: 2 + (standardRectHeight+1)*2
    property int yPlacementRow_3: 2 + (standardRectHeight+1)*3
    property int yPlacementRow_4: 2 + (standardRectHeight+1)*4
    property int yPlacementRow_5: 2 + (standardRectHeight+1)*5
    property int yPlacementRow_6: 2 + (standardRectHeight+1)*6
    property int yPlacementRow_7: 2 + (standardRectHeight+1)*7
    property int yPlacementRow_8: 2 + (standardRectHeight+1)*8
    property int yPlacementRow_9: 2 + (standardRectHeight+1)*9
    property int xPlacementCol_0: 2
    property int xPlacementCol_1: 2 + (standardRectWidth+1)
    property int xPlacementCol_2: 2 + (standardRectWidth+1)*2
    property int xPlacementCol_3: 2 + (standardRectWidth+1)*3
    property int xPlacementCol_4: 2 + (standardRectWidth+1)*4
    property int xPlacementCol_5: 2 + (standardRectWidth+1)*5
    property int xPlacementCol_6: 2 + (standardRectWidth+1)*6
    property int xPlacementCol_7: 2 + (standardRectWidth+1)*7
    property int xPlacementCol_8: 2 + (standardRectWidth+1)*8
    property int xPlacementCol_9: 2 + (standardRectWidth+1)*9

    Rectangle{
        id: r_0001
        x: xPlacementCol_0
        y: yPlacementRow_0
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 10
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0002
        x: xPlacementCol_1
        y: yPlacementRow_0
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 0
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0003
        x: xPlacementCol_2
        y: yPlacementRow_0
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 10
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0004
        x: xPlacementCol_3
        y: yPlacementRow_0
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 10
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0005
        x: xPlacementCol_4
        y: yPlacementRow_0
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 0
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0006
        x: xPlacementCol_5
        y: yPlacementRow_0
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 10
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0007
        x: xPlacementCol_6
        y: yPlacementRow_0
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 10
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0008
        x: xPlacementCol_7
        y: yPlacementRow_0
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 0
        bottomLeftRadius: 10
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0009
        x: xPlacementCol_8
        y: yPlacementRow_0
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 10
        bottomLeftRadius: 0
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0010
        x: xPlacementCol_9
        y: yPlacementRow_0
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 0
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        color: "red"
    }

    /*

      OUTLINE AS THICK AS THE RADIUS

    */

    Rectangle{
        id: r_0011
        x: xPlacementCol_0
        y: yPlacementRow_1
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 10
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 10
        color: "red"
    }
    Rectangle{
        id: r_0012
        x: xPlacementCol_1
        y: yPlacementRow_1
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 0
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 10
        color: "red"
    }
    Rectangle{
        id: r_0013
        x: xPlacementCol_2
        y: yPlacementRow_1
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 10
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 10
        color: "red"
    }
    Rectangle{
        id: r_0014
        x: xPlacementCol_3
        y: yPlacementRow_1
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 10
        bottomRightRadius: 0
        border.color: "black"
        border.width: 10
        color: "red"
    }
    Rectangle{
        id: r_00015
        x: xPlacementCol_4
        y: yPlacementRow_1
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 0
        bottomRightRadius: 10
        border.color: "black"
        border.width: 10
        color: "red"
    }
    Rectangle{
        id: r_0016
        x: xPlacementCol_5
        y: yPlacementRow_1
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 10
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 10
        color: "red"
    }
    Rectangle{
        id: r_0017
        x: xPlacementCol_6
        y: yPlacementRow_1
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 10
        bottomRightRadius: 10
        border.color: "black"
        border.width: 10
        color: "red"
    }
    Rectangle{
        id: r_0018
        x: xPlacementCol_7
        y: yPlacementRow_1
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 0
        bottomLeftRadius: 10
        bottomRightRadius: 0
        border.color: "black"
        border.width: 10
        color: "red"
    }
    Rectangle{
        id: r_0019
        x: xPlacementCol_8
        y: yPlacementRow_1
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 10
        bottomLeftRadius: 0
        bottomRightRadius: 10
        border.color: "black"
        border.width: 10
        color: "red"
    }
    Rectangle{
        id: r_0020
        x: xPlacementCol_9
        y: yPlacementRow_1
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 0
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 10
        color: "red"
    }

    /*

        OUTLINE THICKER THAN RADIUS

    */

    Rectangle{
        id: r_0021
        x: xPlacementCol_0
        y: yPlacementRow_2
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 10
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 15
        color: "red"
    }
    Rectangle{
        id: r_0022
        x: xPlacementCol_1
        y: yPlacementRow_2
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 0
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 15
        color: "red"
    }
    Rectangle{
        id: r_0023
        x: xPlacementCol_2
        y: yPlacementRow_2
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 10
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 15
        color: "red"
    }
    Rectangle{
        id: r_0024
        x: xPlacementCol_3
        y: yPlacementRow_2
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 10
        bottomRightRadius: 0
        border.color: "black"
        border.width: 15
        color: "red"
    }
    Rectangle{
        id: r_00025
        x: xPlacementCol_4
        y: yPlacementRow_2
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 0
        bottomRightRadius: 10
        border.color: "black"
        border.width: 15
        color: "red"
    }
    Rectangle{
        id: r_0026
        x: xPlacementCol_5
        y: yPlacementRow_2
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 10
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 15
        color: "red"
    }
    Rectangle{
        id: r_0027
        x: xPlacementCol_6
        y: yPlacementRow_2
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 10
        bottomRightRadius: 10
        border.color: "black"
        border.width: 15
        color: "red"
    }
    Rectangle{
        id: r_0028
        x: xPlacementCol_7
        y: yPlacementRow_2
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 0
        bottomLeftRadius: 10
        bottomRightRadius: 0
        border.color: "black"
        border.width: 15
        color: "red"
    }
    Rectangle{
        id: r_0029
        x: xPlacementCol_8
        y: yPlacementRow_2
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 10
        bottomLeftRadius: 0
        bottomRightRadius: 10
        border.color: "black"
        border.width: 15
        color: "red"
    }
    Rectangle{
        id: r_0030
        x: xPlacementCol_9
        y: yPlacementRow_2
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 0
        topLeftRadius: 0
        topRightRadius: 0
        bottomLeftRadius: 0
        bottomRightRadius: 0
        border.color: "black"
        border.width: 15
        color: "red"
    }

    /*

        DIFFERENT RADII

    */

    Rectangle{
        id: r_0031
        x: xPlacementCol_0
        y: yPlacementRow_3
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 10
        topLeftRadius: 0
        topRightRadius: 5
        bottomLeftRadius: 10
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0032
        x: xPlacementCol_1
        y: yPlacementRow_3
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 15
        topRightRadius: 0
        bottomLeftRadius: 5
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0033
        x: xPlacementCol_2
        y: yPlacementRow_3
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 15
        bottomLeftRadius: 0
        bottomRightRadius: 5
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0034
        x: xPlacementCol_3
        y: yPlacementRow_3
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 5
        topRightRadius: 10
        bottomLeftRadius: 15
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0035
        x: xPlacementCol_4
        y: yPlacementRow_3
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 15
        topRightRadius: 10
        bottomLeftRadius: 5
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0036
        x: xPlacementCol_5
        y: yPlacementRow_3
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 15
        bottomLeftRadius: 10
        bottomRightRadius: 5
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0037
        x: xPlacementCol_6
        y: yPlacementRow_3
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 5
        topRightRadius: 0
        bottomLeftRadius: 15
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0038
        x: xPlacementCol_7
        y: yPlacementRow_3
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 5
        bottomLeftRadius: 0
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0039
        x: xPlacementCol_8
        y: yPlacementRow_3
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 0
        bottomLeftRadius: 5
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        color: "red"
    }
    Rectangle{
        id: r_0040
        x: xPlacementCol_9
        y: yPlacementRow_3
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 0
        topLeftRadius: 5
        topRightRadius: 15
        bottomLeftRadius: 0
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        color: "red"
    }

    /*

        DIFFERENT RADII; NO OUTLINE

    */

    Rectangle{
        id: r_0041
        x: xPlacementCol_0
        y: yPlacementRow_4
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 10
        topLeftRadius: 0
        topRightRadius: 5
        bottomLeftRadius: 10
        bottomRightRadius: 15
        color: "red"
    }
    Rectangle{
        id: r_0042
        x: xPlacementCol_1
        y: yPlacementRow_4
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 15
        topRightRadius: 0
        bottomLeftRadius: 5
        bottomRightRadius: 10
        color: "red"
    }
    Rectangle{
        id: r_0043
        x: xPlacementCol_2
        y: yPlacementRow_4
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 15
        bottomLeftRadius: 0
        bottomRightRadius: 5
        color: "red"
    }
    Rectangle{
        id: r_0044
        x: xPlacementCol_3
        y: yPlacementRow_4
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 5
        topRightRadius: 10
        bottomLeftRadius: 15
        bottomRightRadius: 0
        color: "red"
    }
    Rectangle{
        id: r_0045
        x: xPlacementCol_4
        y: yPlacementRow_4
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 15
        topRightRadius: 10
        bottomLeftRadius: 5
        bottomRightRadius: 0
        color: "red"
    }
    Rectangle{
        id: r_0046
        x: xPlacementCol_5
        y: yPlacementRow_4
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 15
        bottomLeftRadius: 10
        bottomRightRadius: 5
        color: "red"
    }
    Rectangle{
        id: r_0047
        x: xPlacementCol_6
        y: yPlacementRow_4
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 5
        topRightRadius: 0
        bottomLeftRadius: 15
        bottomRightRadius: 10
        color: "red"
    }
    Rectangle{
        id: r_0048
        x: xPlacementCol_7
        y: yPlacementRow_4
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 5
        bottomLeftRadius: 0
        bottomRightRadius: 15
        color: "red"
    }
    Rectangle{
        id: r_0049
        x: xPlacementCol_8
        y: yPlacementRow_4
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 0
        bottomLeftRadius: 5
        bottomRightRadius: 15
        color: "red"
    }
    Rectangle{
        id: r_0050
        x: xPlacementCol_9
        y: yPlacementRow_4
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 0
        topLeftRadius: 5
        topRightRadius: 15
        bottomLeftRadius: 0
        bottomRightRadius: 10
        color: "red"
    }

    /*

        DIFFERENT RADII; LOW FREQ GRAD HORIZONTAL

    */

    Gradient {
        id: lowFGradH
        orientation: Qt.Horizontal
        GradientStop { position: 0.0; color: "green" }
        GradientStop { position: 0.33; color: "blue" }
        GradientStop { position: 0.66; color: "red" }
        GradientStop { position: 1.0; color: "yellow" }
    }

    Rectangle{
        id: r_0051
        x: xPlacementCol_0
        y: yPlacementRow_5
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 10
        topLeftRadius: 0
        topRightRadius: 5
        bottomLeftRadius: 10
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
    }
    Rectangle{
        id: r_0052
        x: xPlacementCol_1
        y: yPlacementRow_5
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 15
        topRightRadius: 0
        bottomLeftRadius: 5
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
    }
    Rectangle{
        id: r_0053
        x: xPlacementCol_2
        y: yPlacementRow_5
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 15
        bottomLeftRadius: 0
        bottomRightRadius: 5
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
    }
    Rectangle{
        id: r_0054
        x: xPlacementCol_3
        y: yPlacementRow_5
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 5
        topRightRadius: 10
        bottomLeftRadius: 15
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
    }
    Rectangle{
        id: r_0055
        x: xPlacementCol_4
        y: yPlacementRow_5
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 15
        topRightRadius: 10
        bottomLeftRadius: 5
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
    }
    Rectangle{
        id: r_0056
        x: xPlacementCol_5
        y: yPlacementRow_5
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 15
        bottomLeftRadius: 10
        bottomRightRadius: 5
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
    }
    Rectangle{
        id: r_0057
        x: xPlacementCol_6
        y: yPlacementRow_5
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 5
        topRightRadius: 0
        bottomLeftRadius: 15
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
    }
    Rectangle{
        id: r_0058
        x: xPlacementCol_7
        y: yPlacementRow_5
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 5
        bottomLeftRadius: 0
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
    }
    Rectangle{
        id: r_0059
        x: xPlacementCol_8
        y: yPlacementRow_5
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 0
        bottomLeftRadius: 5
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
    }
    Rectangle{
        id: r_0060
        x: xPlacementCol_9
        y: yPlacementRow_5
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 0
        topLeftRadius: 5
        topRightRadius: 15
        bottomLeftRadius: 0
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
    }

    /*

        DIFFERENT RADII; LOW FREQ GRAD VERTICAL

    */

    Gradient {
        id: lowFGradV
        orientation: Qt.Vertical
        GradientStop { position: 0.0; color: "green" }
        GradientStop { position: 0.33; color: "blue" }
        GradientStop { position: 0.66; color: "red" }
        GradientStop { position: 1.0; color: "yellow" }
    }

    Rectangle{
        id: r_0061
        x: xPlacementCol_0
        y: yPlacementRow_6
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 10
        topLeftRadius: 0
        topRightRadius: 5
        bottomLeftRadius: 10
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: lowFGradV
    }
    Rectangle{
        id: r_0062
        x: xPlacementCol_1
        y: yPlacementRow_6
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 15
        topRightRadius: 0
        bottomLeftRadius: 5
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: lowFGradV
    }
    Rectangle{
        id: r_0063
        x: xPlacementCol_2
        y: yPlacementRow_6
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 15
        bottomLeftRadius: 0
        bottomRightRadius: 5
        border.color: "black"
        border.width: 2
        gradient: lowFGradV
    }
    Rectangle{
        id: r_0064
        x: xPlacementCol_3
        y: yPlacementRow_6
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 5
        topRightRadius: 10
        bottomLeftRadius: 15
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        gradient: lowFGradV
    }
    Rectangle{
        id: r_0065
        x: xPlacementCol_4
        y: yPlacementRow_6
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 15
        topRightRadius: 10
        bottomLeftRadius: 5
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        gradient: lowFGradV
    }
    Rectangle{
        id: r_0066
        x: xPlacementCol_5
        y: yPlacementRow_6
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 15
        bottomLeftRadius: 10
        bottomRightRadius: 5
        border.color: "black"
        border.width: 2
        gradient: lowFGradV
    }
    Rectangle{
        id: r_0067
        x: xPlacementCol_6
        y: yPlacementRow_6
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 5
        topRightRadius: 0
        bottomLeftRadius: 15
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: lowFGradV
    }
    Rectangle{
        id: r_0068
        x: xPlacementCol_7
        y: yPlacementRow_6
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 5
        bottomLeftRadius: 0
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: lowFGradV
    }
    Rectangle{
        id: r_0069
        x: xPlacementCol_8
        y: yPlacementRow_6
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 0
        bottomLeftRadius: 5
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: lowFGradV
    }
    Rectangle{
        id: r_0070
        x: xPlacementCol_9
        y: yPlacementRow_6
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 0
        topLeftRadius: 5
        topRightRadius: 15
        bottomLeftRadius: 0
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: lowFGradV
    }

    /*

        DIFFERENT RADII; HIGH FREQ GRAD HORIZONTAL

    */

    Gradient {
        id: highFGradH
        orientation: Qt.Horizontal
        GradientStop { position: 0.00; color: "yellow" }
        GradientStop { position: 0.05; color: "red" }
        GradientStop { position: 0.10; color: "yellow" }
        GradientStop { position: 0.15; color: "red" }
        GradientStop { position: 0.20; color: "yellow" }
        GradientStop { position: 0.25; color: "red" }
        GradientStop { position: 0.30; color: "yellow" }
        GradientStop { position: 0.35; color: "red" }
        GradientStop { position: 0.40; color: "yellow" }
        GradientStop { position: 0.45; color: "red" }
        GradientStop { position: 0.50; color: "yellow" }
        GradientStop { position: 0.55; color: "red" }
        GradientStop { position: 0.60; color: "yellow" }
        GradientStop { position: 0.65; color: "red" }
        GradientStop { position: 0.70; color: "yellow" }
        GradientStop { position: 0.75; color: "red" }
        GradientStop { position: 0.80; color: "yellow" }
        GradientStop { position: 0.85; color: "red" }
        GradientStop { position: 0.90; color: "yellow" }
        GradientStop { position: 0.95; color: "red" }
        GradientStop { position: 1.00; color: "yellow" }
    }

    Rectangle{
        id: r_0071
        x: xPlacementCol_0
        y: yPlacementRow_7
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 10
        topLeftRadius: 0
        topRightRadius: 5
        bottomLeftRadius: 10
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: highFGradH
    }
    Rectangle{
        id: r_0072
        x: xPlacementCol_1
        y: yPlacementRow_7
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 15
        topRightRadius: 0
        bottomLeftRadius: 5
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: highFGradH
    }
    Rectangle{
        id: r_0073
        x: xPlacementCol_2
        y: yPlacementRow_7
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 15
        bottomLeftRadius: 0
        bottomRightRadius: 5
        border.color: "black"
        border.width: 2
        gradient: highFGradH
    }
    Rectangle{
        id: r_0074
        x: xPlacementCol_3
        y: yPlacementRow_7
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 5
        topRightRadius: 10
        bottomLeftRadius: 15
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        gradient: highFGradH
    }
    Rectangle{
        id: r_0075
        x: xPlacementCol_4
        y: yPlacementRow_7
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 15
        topRightRadius: 10
        bottomLeftRadius: 5
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        gradient: highFGradH
    }
    Rectangle{
        id: r_0076
        x: xPlacementCol_5
        y: yPlacementRow_7
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 15
        bottomLeftRadius: 10
        bottomRightRadius: 5
        border.color: "black"
        border.width: 2
        gradient: highFGradH
    }
    Rectangle{
        id: r_0077
        x: xPlacementCol_6
        y: yPlacementRow_7
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 5
        topRightRadius: 0
        bottomLeftRadius: 15
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: highFGradH
    }
    Rectangle{
        id: r_0078
        x: xPlacementCol_7
        y: yPlacementRow_7
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 5
        bottomLeftRadius: 0
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: highFGradH
    }
    Rectangle{
        id: r_0079
        x: xPlacementCol_8
        y: yPlacementRow_7
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 0
        bottomLeftRadius: 5
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: highFGradH
    }
    Rectangle{
        id: r_0080
        x: xPlacementCol_9
        y: yPlacementRow_7
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 0
        topLeftRadius: 5
        topRightRadius: 15
        bottomLeftRadius: 0
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: highFGradH
    }

    /*

        DIFFERENT RADII; HIGH FREQ GRAD VERTICAL

    */

    Gradient {
        id: highFGradV
        orientation: Qt.Vertical
        GradientStop { position: 0.00; color: "yellow" }
        GradientStop { position: 0.05; color: "red" }
        GradientStop { position: 0.10; color: "yellow" }
        GradientStop { position: 0.15; color: "red" }
        GradientStop { position: 0.20; color: "yellow" }
        GradientStop { position: 0.25; color: "red" }
        GradientStop { position: 0.30; color: "yellow" }
        GradientStop { position: 0.35; color: "red" }
        GradientStop { position: 0.40; color: "yellow" }
        GradientStop { position: 0.45; color: "red" }
        GradientStop { position: 0.50; color: "yellow" }
        GradientStop { position: 0.55; color: "red" }
        GradientStop { position: 0.60; color: "yellow" }
        GradientStop { position: 0.65; color: "red" }
        GradientStop { position: 0.70; color: "yellow" }
        GradientStop { position: 0.75; color: "red" }
        GradientStop { position: 0.80; color: "yellow" }
        GradientStop { position: 0.85; color: "red" }
        GradientStop { position: 0.90; color: "yellow" }
        GradientStop { position: 0.95; color: "red" }
        GradientStop { position: 1.00; color: "yellow" }
    }

    Rectangle{
        id: r_0081
        x: xPlacementCol_0
        y: yPlacementRow_8
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 10
        topLeftRadius: 0
        topRightRadius: 5
        bottomLeftRadius: 10
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: highFGradV
    }
    Rectangle{
        id: r_0082
        x: xPlacementCol_1
        y: yPlacementRow_8
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 15
        topRightRadius: 0
        bottomLeftRadius: 5
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: highFGradV
    }
    Rectangle{
        id: r_0083
        x: xPlacementCol_2
        y: yPlacementRow_8
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 15
        bottomLeftRadius: 0
        bottomRightRadius: 5
        border.color: "black"
        border.width: 2
        gradient: highFGradV
    }
    Rectangle{
        id: r_0084
        x: xPlacementCol_3
        y: yPlacementRow_8
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 5
        topRightRadius: 10
        bottomLeftRadius: 15
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        gradient: highFGradV
    }
    Rectangle{
        id: r_0085
        x: xPlacementCol_4
        y: yPlacementRow_8
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 15
        topRightRadius: 10
        bottomLeftRadius: 5
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        gradient: highFGradV
    }
    Rectangle{
        id: r_0086
        x: xPlacementCol_5
        y: yPlacementRow_8
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 15
        bottomLeftRadius: 10
        bottomRightRadius: 5
        border.color: "black"
        border.width: 2
        gradient: highFGradV
    }
    Rectangle{
        id: r_0087
        x: xPlacementCol_6
        y: yPlacementRow_8
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 5
        topRightRadius: 0
        bottomLeftRadius: 15
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: highFGradV
    }
    Rectangle{
        id: r_0088
        x: xPlacementCol_7
        y: yPlacementRow_8
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 5
        bottomLeftRadius: 0
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: highFGradV
    }
    Rectangle{
        id: r_0089
        x: xPlacementCol_8
        y: yPlacementRow_8
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 0
        bottomLeftRadius: 5
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: highFGradV
    }
    Rectangle{
        id: r_0090
        x: xPlacementCol_9
        y: yPlacementRow_8
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 0
        topLeftRadius: 5
        topRightRadius: 15
        bottomLeftRadius: 0
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: highFGradV
    }

    /*

        TRANSPARENCY

    */

    Rectangle{
        id: r_0091
        x: xPlacementCol_0
        y: yPlacementRow_9
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 10
        topLeftRadius: 0
        topRightRadius: 5
        bottomLeftRadius: 10
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
        opacity: 0.3
    }
    Rectangle{
        id: r_0092
        x: xPlacementCol_1
        y: yPlacementRow_9
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 15
        topRightRadius: 0
        bottomLeftRadius: 5
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
        opacity: 0.3
    }
    Rectangle{
        id: r_0093
        x: xPlacementCol_2
        y: yPlacementRow_9
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 15
        bottomLeftRadius: 0
        bottomRightRadius: 5
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
        opacity: 0.3
    }
    Rectangle{
        id: r_0094
        x: xPlacementCol_3
        y: yPlacementRow_9
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 5
        topRightRadius: 10
        bottomLeftRadius: 15
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
        opacity: 0.3
    }
    Rectangle{
        id: r_0095
        x: xPlacementCol_4
        y: yPlacementRow_9
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 15
        topRightRadius: 10
        bottomLeftRadius: 5
        bottomRightRadius: 0
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
        opacity: 0.3
    }
    Rectangle{
        id: r_0096
        x: xPlacementCol_5
        y: yPlacementRow_9
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 0
        topRightRadius: 15
        bottomLeftRadius: 10
        bottomRightRadius: 5
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
        opacity: 0.3
    }
    Rectangle{
        id: r_0097
        x: xPlacementCol_6
        y: yPlacementRow_9
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 5
        topRightRadius: 0
        bottomLeftRadius: 15
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
        opacity: 0.3
    }
    Rectangle{
        id: r_0098
        x: xPlacementCol_7
        y: yPlacementRow_9
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 5
        bottomLeftRadius: 0
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
        opacity: 0.3
    }
    Rectangle{
        id: r_0099
        x: xPlacementCol_8
        y: yPlacementRow_9
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        topLeftRadius: 10
        topRightRadius: 0
        bottomLeftRadius: 5
        bottomRightRadius: 15
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
        opacity: 0.3
    }
    Rectangle{
        id: r_0100
        x: xPlacementCol_9
        y: yPlacementRow_9
        width: r_0000.standardRectWidth
        height: r_0000.standardRectHeight
        radius: 0
        topLeftRadius: 5
        topRightRadius: 15
        bottomLeftRadius: 0
        bottomRightRadius: 10
        border.color: "black"
        border.width: 2
        gradient: lowFGradH
        opacity: 0.3
    }
}

