/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.14
import QtQuick.Shapes 1.14

Rectangle {
    id: i
    width: parent.width * 0.5
    height: parent.height * 0.5
    anchors.centerIn: parent
    color: "transparent"
    border.color: "red"

    Shape {
        id: pathLineMove
        vendorExtensionsEnabled: false
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.verticalCenter
            right: parent.horizontalCenter
        }

        visible: true

        ShapePath {
            id: c_sp1
            strokeWidth: -1
            fillColor: Qt.rgba(1,0,1,1.0)
            scale: Qt.size(pathLineMove.width - 1, pathLineMove.height - 1)

            startX: 0.5;
            startY: 1

            PathLine {
                x: 0
                y: 1
            }
            PathLine {
                x: 0.5
                y: 0
            }
            PathLine {
                x: 1
                y: 1
            }
            PathLine {
                x: c_sp1.startX
                y: c_sp1.startY
            }

            // Inner shape
            PathMove {
                x: 0.5
                y: 0.25
            }
            PathLine {
                x: 0.8
                y: 0.8
            }
            PathLine {
                x: 0.2
                y: 0.8
            }
            PathLine {
                x: 0.5
                y: 0.25
            }
        }
    }
    Shape {
        id: pathCurveArcQuad
        vendorExtensionsEnabled: false
        anchors {
            left: parent.horizontalCenter
            top: parent.top
            bottom: parent.verticalCenter
            right: parent.right
        }

        visible: true

        ShapePath {
            strokeWidth: 1
            fillColor: "transparent"
            strokeColor: "goldenrod"
            scale: Qt.size(pathCurveArcQuad.width - 1, pathCurveArcQuad.height - 1)

            startX: 0/400; startY: 100/400

            PathCurve { x: 75/400; y: 75/400 }
            PathCurve { x: 200/400; y: 150/400 }
            PathCurve { x: 325/400; y: 25/400 }
            PathCurve { x: 400/400; y: 100/400 }
            PathMove { x: 0.5; y: 0 }
            PathArc {
                x: 0; y: 100 / 200
                radiusX: 100 / 200; radiusY: 100 / 200
                useLargeArc: true
            }
            PathMove { x: 0; y: 0.5 }
            PathQuad { x: 1; y: 0.5; controlX: 0.5; controlY: 1 }
        }
    }


    Shape {
        id: pathCubicAngleArc
        vendorExtensionsEnabled: false
        anchors {
            left: parent.left
            top: parent.verticalCenter
            bottom: parent.bottom
            right: parent.horizontalCenter
        }

        visible: true

        ShapePath {
            strokeWidth: 1
            fillColor: "transparent"
            strokeColor: "deepskyblue"
            scale: Qt.size(pathCubicAngleArc.width - 1, pathCubicAngleArc.height - 1)

            startX: 20/200; startY: 0

            PathCubic {
                x: 180/200; y: 0
                control1X: -10/200; control1Y: 90/200
                control2X: 210/200; control2Y: 90/200
            }

            PathAngleArc {
                centerX: 0.5; centerY: 0.5
                radiusX: 0.45; radiusY: 0.45
                startAngle: -180
                sweepAngle: 234
                moveToStart: true
            }
        }
    }
    Shape {
        id: pathSvg
        vendorExtensionsEnabled: false
        anchors {
            left: parent.horizontalCenter
            top: parent.verticalCenter
            bottom: parent.bottom
            right: parent.right
        }

        visible: true

        ShapePath {
            strokeWidth: 5
            fillColor: "transparent"
            strokeColor: "coral"
            scale: Qt.size((pathSvg.width - 1), (pathSvg.height - 1))
            startX: .25; startY: .25
            PathSvg { path: "L .75 .25 L .5 .75 z" }
        }

        ShapePath {
            strokeWidth: 1
            fillColor: "transparent"
            strokeColor: "black"
            scale: Qt.size((pathSvg.width - 1) / 200, (pathSvg.height - 1) / 200)
            startX: 50; startY: 50
            PathSvg { path: "L 150 50 L 100 150 z" }
        }

        ShapePath {
            strokeColor: "red"
            strokeWidth: 4
            fillColor: "transparent"
            scale: Qt.size((pathSvg.width - 1) / 500, (pathSvg.height - 1) / 500)
            PathSvg {
                path: "m 325.03711,0.5
                    c -26.61408,6.4494547 -49.95197,2.1018066 -76.21132,1.0771669
                    -22.26577,7.6817151 -47.96405,9.3627181 -65.67832,25.8497861
                    -15.74718,12.80008 -41.1564,19.605644 -45.74903,40.600391
                    -12.46933,17.76181 -25.36105,35.720146 -29.20117,57.999996
                    -18.709864,3.10961 -16.347355,30.83801 -22.385143,46.675
                    -6.848711,11.2677 11.07278,24.69174 -8.514666,27.97383
                    -10.266901,5.61543 -12.859313,28.96588 -13.732346,5.78143
                    0.940083,-11.53398 -13.486195,-38.30626 -16.81701,-34.20231
                    14.608079,7.8234 21.299281,50.52979 11.380052,48.14418
                    -3.406456,-15.12428 -26.181106,-38.29457 -31.849471,-35.62945
                    16.851912,6.41472 35.569884,31.75215 28.172486,47.93115
                    -7.906485,-15.42757 -37.758959,-35.53783 -44.275447,-31.28685
                    18.975831,1.7428 37.986009,20.68109 42.87115,37.14427 C
                    42.279655,225.774 9.879724,213.57795 4.7080253,219.04989
                    20.780803,212.57418 55.055919,239.88547 49.602579,241.25683
                    38.186641,230.40078 6.6930104,222.77983 2.5752529,228.41774 c
                    13.6045481,-8.33065 49.4437901,14.89041 43.5525671,14.2358
                    -9.759981,-7.96123 -43.5842921,7.36937 -17.554974,-1.20248
                    9.464499,-3.73452 40.555672,12.80659 16.398749,5.14121
                    -9.1987,-7.28225 -39.0013156,3.37352 -14.121965,-2.12828
                    13.244874,-0.0206 35.758428,14.62706 10.562447,6.42228
                    -10.780465,-8.4873 -47.8282254,11.10651 -21.027329,-0.003
                    11.640859,-4.82877 52.615601,10.74471 24.234828,8.2659
                    -10.695834,-7.03902 -42.9384162,8.93905 -34.227854,5.58373
                    9.077539,-8.56443 49.068801,-5.28097 43.06838,0.45546
                    -10.900893,-0.7118 -27.449619,17.27258 -10.00187,3.46526
                    15.705191,-9.18198 18.344231,9.31645 1.10807,8.73907
                    -9.908444,1.77856 -21.108189,20.66671 -7.974821,4.92019
                    15.750746,-14.10374 34.01348,2.07267 9.796961,8.69337
                    -8.17128,5.49929 -12.642664,19.13654 -3.994573,4.19708
                    9.044753,-8.7077 23.850399,-13.64552 21.404959,4.02329
                    12.509737,17.12562 51.158782,11.0442 45.106112,43.34009
                    -0.65006,10.05318 -3.79228,13.95389 1.62128,14.30064
                    -4.30913,8.82737 -14.652714,37.9591 2.92144,17.46024
                    7.37972,-3.68333 -7.62399,16.24161 -7.98007,23.83761
                    -9.336865,18.77418 19.74873,-18.55943 6.62229,5.46195
                    5.46464,-3.7389 36.23886,-19.41901 14.78167,0.58987
                    -8.59505,4.55644 29.29441,-2.99423 8.95489,6.47134 -9.22562,5.54437
                    -24.09765,26.79976 -11.73274,22.20385 -0.81685,5.4936
                    -1.58629,21.47626 2.34158,9.14886 1.61237,14.67029
                    -2.38384,25.22225 12.26908,15.1741 -4.40761,8.01039
                    -8.23679,36.91214 5.12235,17.92578 1.53454,2.99551 9.37569,3.1726
                    7.15304,14.93579 3.51234,-11.31873 18.4607,-29.83809
                    12.36869,-6.48005 -0.22629,16.26174 5.44303,-7.24791
                    6.56926,10.49819 12.45412,28.9931 3.40908,-41.89883
                    17.52051,-9.19238 3.23093,11.1924 6.53006,29.46941 7.55984,5.1249
                    15.37236,-19.52583 4.09776,20.07416 12.64063,1.48215
                    18.11247,-24.55068 -8.92586,38.39355 6.73828,6.62225
                    4.55353,-6.91007 15.35028,-38.88977 12.55806,-13.78666
                    1.05309,27.02664 11.54743,-24.40259 12.40657,6.86306
                    -1.72561,13.28253 11.85393,-24.15909 13.85568,-1.38002
                    3.12455,8.33539 8.76536,26.46432 8.73882,5.09231 3.57025,-10.37352
                    -16.025,-37.75672 0.20707,-22.5788 -1.2458,-14.17213
                    -2.38918,-16.90145 10.85489,-6.71468 -16.57629,-17.22152
                    0.19706,-26.08949 5.7751,-19.14889 -14.91681,-16.1674
                    19.74174,7.19334 2.31875,-9.86869 -4.32508,-15.23278
                    27.25228,29.12341 20.27514,18.81172 -11.97527,-18.92603
                    -17.96305,-45.80333 11.70099,-51.52566 17.19069,-9.57351
                    31.17452,21.93154 38.50541,1.56304 16.26048,-4.6633
                    22.3749,38.26516 24.86349,9.11316 5.94153,-9.9731 30.14313,6.97379
                    36.34294,4.75012 7.07435,18.27732 8.06778,14.78971 11.04264,3.86016
                    2.73754,-15.85945 28.7269,10.06391 28.09146,25.96561 3.00672,2.4754
                    6.55025,-22.10264 11.23552,-14.43872 2.84155,-11.4823
                    -3.28976,-27.88574 4.24895,-25.5189 -0.61494,-11.53957
                    22.83611,0.11011 10.64648,-15.28756 -6.5587,-21.38598
                    9.32959,-3.0159 13.5107,-4.69375 -1.38592,-16.74533
                    -8.66673,-31.83316 -1.90087,-41.0875 2.39623,-15.14303
                    -12.50533,-44.45478 -4.70573,-48.49375 15.08472,3.42779
                    -20.39159,-42.17451 -1.69776,-40.85728 24.07272,21.63552
                    -3.65989,-30.10299 2.27233,-33.17152 16.90643,17.53071
                    -12.7383,-38.42821 6.79531,-21.57013 -4.50946,-21.08135
                    -2.53357,-37.43561 -15.5535,-55.59527 -11.0035,-12.40086
                    -1.87775,-7.12745 1.34831,-8.11755 C 468.27562,118.9774
                    451.40746,102.656 430.98897,92.119168 439.06192,78.203836
                    455.88012,60.123881 457.38638,40.337815 463.2373,23.183067
                    450.82861,4.7342783 435.04883,22.626367 409.5188,28.206712
                    386.3569,24.131269 365.63904,8.0954152 352.788,2.8857182
                    338.88892,0.40735091 325.03711,0.5 Z m -219.0625,357.04297
                    -0.97656,0.88476 z"
             }
        }
    }
}
