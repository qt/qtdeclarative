/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.2
import QtTest 1.0
import Qt.labs.controls 1.0
import Qt.labs.templates 1.0 as T

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "Control"

    Component {
        id: component
        T.Control { }
    }

    SignalSpy {
        id: layoutDirectionSpy
        signalName: "layoutDirectionChanged"
    }

    SignalSpy {
        id: effectiveLayoutDirectionSpy
        signalName: "effectiveLayoutDirectionChanged"
    }

    SignalSpy {
        id: mirroredSpy
        signalName: "mirroredChanged"
    }

    SignalSpy {
        id: availableWidthSpy
        signalName: "availableWidthChanged"
    }

    SignalSpy {
        id: availableHeightSpy
        signalName: "availableHeightChanged"
    }

    function test_padding() {
        var control = component.createObject(testCase)
        verify(control)

        compare(control.padding, 0)
        compare(control.topPadding, 0)
        compare(control.leftPadding, 0)
        compare(control.rightPadding, 0)
        compare(control.bottomPadding, 0)
        compare(control.availableWidth, 0)
        compare(control.availableHeight, 0)

        control.width = 100
        control.height = 100

        control.padding = 10
        compare(control.padding, 10)
        compare(control.topPadding, 10)
        compare(control.leftPadding, 10)
        compare(control.rightPadding, 10)
        compare(control.bottomPadding, 10)

        control.topPadding = 20
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 10)
        compare(control.rightPadding, 10)
        compare(control.bottomPadding, 10)

        control.leftPadding = 30
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 10)
        compare(control.bottomPadding, 10)

        control.rightPadding = 40
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 10)

        control.bottomPadding = 50
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 50)

        control.padding = 60
        compare(control.padding, 60)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 50)

        control.destroy()
    }

    function test_availableSize() {
        var control = component.createObject(testCase)
        verify(control)

        availableWidthSpy.target = control
        availableHeightSpy.target = control

        verify(availableWidthSpy.valid)
        verify(availableHeightSpy.valid)

        var availableWidthChanges = 0
        var availableHeightChanges = 0

        control.width = 100
        compare(control.availableWidth, 100)
        compare(availableWidthSpy.count, ++availableWidthChanges)
        compare(availableHeightSpy.count, availableHeightChanges)

        control.height = 100
        compare(control.availableHeight, 100)
        compare(availableWidthSpy.count, availableWidthChanges)
        compare(availableHeightSpy.count, ++availableHeightChanges)

        control.padding = 10
        compare(control.availableWidth, 80)
        compare(control.availableHeight, 80)
        compare(availableWidthSpy.count, ++availableWidthChanges)
        compare(availableHeightSpy.count, ++availableHeightChanges)

        control.topPadding = 20
        compare(control.availableWidth, 80)
        compare(control.availableHeight, 70)
        compare(availableWidthSpy.count, availableWidthChanges)
        compare(availableHeightSpy.count, ++availableHeightChanges)

        control.leftPadding = 30
        compare(control.availableWidth, 60)
        compare(control.availableHeight, 70)
        compare(availableWidthSpy.count, ++availableWidthChanges)
        compare(availableHeightSpy.count, availableHeightChanges)

        control.rightPadding = 40
        compare(control.availableWidth, 30)
        compare(control.availableHeight, 70)
        compare(availableWidthSpy.count, ++availableWidthChanges)
        compare(availableHeightSpy.count, availableHeightChanges)

        control.bottomPadding = 50
        compare(control.availableWidth, 30)
        compare(control.availableHeight, 30)
        compare(availableWidthSpy.count, availableWidthChanges)
        compare(availableHeightSpy.count, ++availableHeightChanges)

        control.padding = 60
        compare(control.availableWidth, 30)
        compare(control.availableHeight, 30)
        compare(availableWidthSpy.count, availableWidthChanges)
        compare(availableHeightSpy.count, availableHeightChanges)

        control.width = 0
        compare(control.availableWidth, 0)
        compare(availableWidthSpy.count, ++availableWidthChanges)
        compare(availableHeightSpy.count, availableHeightChanges)

        control.height = 0
        compare(control.availableHeight, 0)
        compare(availableWidthSpy.count, availableWidthChanges)
        compare(availableHeightSpy.count, ++availableHeightChanges)

        control.destroy()
    }

    function test_layoutDirection() {
        var control = component.createObject(testCase)
        verify(control)

        layoutDirectionSpy.target = control
        effectiveLayoutDirectionSpy.target = control
        mirroredSpy.target = control

        verify(layoutDirectionSpy.valid)
        verify(effectiveLayoutDirectionSpy.valid)
        verify(mirroredSpy.valid)

        verify(!control.LayoutMirroring.enabled)
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)
        compare(control.mirrored, false)

        control.layoutDirection = Qt.RightToLeft
        compare(control.layoutDirection, Qt.RightToLeft)
        compare(control.effectiveLayoutDirection, Qt.RightToLeft)
        compare(control.mirrored, true)
        compare(layoutDirectionSpy.count, 1)
        compare(effectiveLayoutDirectionSpy.count, 1)
        compare(mirroredSpy.count, 1)

        control.LayoutMirroring.enabled = true
        compare(control.layoutDirection, Qt.RightToLeft)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)
        compare(control.mirrored, false)
        compare(layoutDirectionSpy.count, 1)
        compare(effectiveLayoutDirectionSpy.count, 2)
        compare(mirroredSpy.count, 2)

        control.layoutDirection = Qt.LeftToRight
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.RightToLeft)
        compare(control.mirrored, true)
        compare(layoutDirectionSpy.count, 2)
        compare(effectiveLayoutDirectionSpy.count, 3)
        compare(mirroredSpy.count, 3)

        control.LayoutMirroring.enabled = false
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)
        compare(control.mirrored, false)
        compare(layoutDirectionSpy.count, 2)
        compare(effectiveLayoutDirectionSpy.count, 4)
        compare(mirroredSpy.count, 4)

        control.destroy()
    }

    function test_background() {
        var control = component.createObject(testCase)
        verify(control)

        control.background = component.createObject(control)

        // background has no x or width set, so its width follows control's width
        control.width = 320
        compare(control.background.width, control.width)

        // background has no y or height set, so its height follows control's height
        compare(control.background.height, control.height)
        control.height = 240

        // has width => width does not follow
        control.background.width /= 2
        control.width += 20
        verify(control.background.width !== control.width)

        // reset width => width follows again
        control.background.width = undefined
        control.width += 20
        compare(control.background.width, control.width)

        // has x => width does not follow
        control.background.x = 10
        control.width += 20
        verify(control.background.width !== control.width)

        // has height => height does not follow
        control.background.height /= 2
        control.height -= 20
        verify(control.background.height !== control.height)

        // reset height => height follows again
        control.background.height = undefined
        control.height -= 20
        compare(control.background.height, control.height)

        // has y => height does not follow
        control.background.y = 10
        control.height -= 20
        verify(control.background.height !== control.height)

        control.destroy()
    }

    Component {
        id: component2
        T.Control {
            id: item2
            objectName: "item2"
            property alias item2_2: _item2_2;
            property alias item2_3: _item2_3;
            property alias item2_4: _item2_4;
            property alias item2_5: _item2_5;
            property alias item2_6: _item2_6;
            font.family: "Arial"
            T.Control {
                id: _item2_2
                objectName: "_item2_2"
                T.Control {
                    id: _item2_3
                    objectName: "_item2_3"
                }
            }
            TextArea {
                id: _item2_4
                objectName: "_item2_4"
                text: "Text Area"
            }
            TextField {
                id: _item2_5
                objectName: "_item2_5"
                text: "Text Field"
            }
            Label {
                id: _item2_6
                objectName: "_item2_6"
                text: "Label"
            }
        }
    }

    function test_font() {
        var control2 = component2.createObject(testCase)
        verify(control2)
        verify(control2.item2_2)
        verify(control2.item2_3)
        verify(control2.item2_4)
        verify(control2.item2_5)
        verify(control2.item2_6)

        compare(control2.font.family, "Arial")
        compare(control2.item2_2.font.family, control2.font.family)
        compare(control2.item2_2.font.pointSize, control2.font.pointSize)
        compare(control2.item2_2.font.weight, control2.font.weight)
        compare(control2.item2_3.font.family, control2.font.family)
        compare(control2.item2_3.font.pointSize, control2.font.pointSize)
        compare(control2.item2_3.font.weight, control2.font.weight)
        compare(control2.item2_4.font.family, control2.font.family)
        compare(control2.item2_4.font.pointSize, control2.font.pointSize)
        compare(control2.item2_4.font.weight, control2.font.weight)
        compare(control2.item2_5.font.family, control2.font.family)
        compare(control2.item2_5.font.pointSize, control2.font.pointSize)
        compare(control2.item2_5.font.weight, control2.font.weight)
        compare(control2.item2_6.font.family, control2.font.family)
        compare(control2.item2_6.font.pointSize, control2.font.pointSize)
        compare(control2.item2_6.font.weight, control2.font.weight)

        control2.font.pointSize = 48
        compare(control2.item2_2.font.pointSize, 48)
        compare(control2.item2_3.font.pointSize, 48)
        compare(control2.item2_4.font.pointSize, 48)
        compare(control2.item2_5.font.pointSize, 48)

        control2.font.bold = true
        compare(control2.item2_2.font.weight, Font.Bold)
        compare(control2.item2_3.font.weight, Font.Bold)
        compare(control2.item2_4.font.weight, Font.Bold)
        compare(control2.item2_5.font.weight, Font.Bold)

        control2.item2_2.font.pointSize = 36
        compare(control2.item2_2.font.pointSize, 36)
        compare(control2.item2_3.font.pointSize, 36)

        control2.item2_2.font.weight = Font.Light
        compare(control2.item2_2.font.pointSize, 36)
        compare(control2.item2_3.font.pointSize, 36)

        compare(control2.item2_3.font.family, control2.item2_2.font.family)
        compare(control2.item2_3.font.pointSize, control2.item2_2.font.pointSize)
        compare(control2.item2_3.font.weight, control2.item2_2.font.weight)

        control2.font.pointSize = 50
        compare(control2.item2_2.font.pointSize, 36)
        compare(control2.item2_3.font.pointSize, 36)
        compare(control2.item2_4.font.pointSize, 50)
        compare(control2.item2_5.font.pointSize, 50)
        compare(control2.item2_6.font.pointSize, 50)

        control2.item2_3.font.pointSize = 60
        compare(control2.item2_3.font.pointSize, 60)

        control2.item2_3.font.weight = Font.Normal
        compare(control2.item2_3.font.weight, Font.Normal)

        control2.item2_4.font.pointSize = 16
        compare(control2.item2_4.font.pointSize, 16)

        control2.item2_4.font.weight = Font.Normal
        compare(control2.item2_4.font.weight, Font.Normal)

        control2.item2_5.font.pointSize = 32
        compare(control2.item2_5.font.pointSize, 32)

        control2.item2_5.font.weight = Font.DemiBold
        compare(control2.item2_5.font.weight, Font.DemiBold)

        control2.item2_6.font.pointSize = 36
        compare(control2.item2_6.font.pointSize, 36)

        control2.item2_6.font.weight = Font.Black
        compare(control2.item2_6.font.weight, Font.Black)

        compare(control2.font.family, "Arial")
        compare(control2.font.pointSize, 50)
        compare(control2.font.weight, Font.Bold)

        compare(control2.item2_2.font.family, "Arial")
        compare(control2.item2_2.font.pointSize, 36)
        compare(control2.item2_2.font.weight, Font.Light)

        compare(control2.item2_3.font.family, "Arial")
        compare(control2.item2_3.font.pointSize, 60)
        compare(control2.item2_3.font.weight, Font.Normal)

        compare(control2.item2_4.font.family, "Arial")
        compare(control2.item2_4.font.pointSize, 16)
        compare(control2.item2_4.font.weight, Font.Normal)

        compare(control2.item2_5.font.family, "Arial")
        compare(control2.item2_5.font.pointSize, 32)
        compare(control2.item2_5.font.weight, Font.DemiBold)

        compare(control2.item2_6.font.family, "Arial")
        compare(control2.item2_6.font.pointSize, 36)
        compare(control2.item2_6.font.weight, Font.Black)

        control2.destroy()
    }

    Component {
        id: component3
        T.Control {
            id: item3
            objectName: "item3"
            property alias item3_2: _item3_2;
            property alias item3_3: _item3_3;
            property alias item3_4: _item3_4;
            property alias item3_5: _item3_5;
            property alias item3_6: _item3_6;
            property alias item3_7: _item3_7;
            property alias item3_8: _item3_8;
            font.family: "Arial"
            Item {
                id: _item3_2
                objectName: "_item3_2"
                T.Control {
                    id: _item3_3
                    objectName: "_item3_3"
                    Item {
                        id: _item3_6
                        objectName: "_item3_6"
                        T.Control {
                            id: _item3_7
                            objectName: "_item3_7"
                        }
                    }
                }
                TextArea {
                    id: _item3_4
                    objectName: "_item3_4"
                    text: "Text Area"
                }
                TextField {
                    id: _item3_5
                    objectName: "_item3_5"
                    text: "Text Field"
                }
                Label {
                    id: _item3_8
                    objectName: "_item3_8"
                    text: "Label"
                }
            }
        }
    }

    function test_font_2() {
        var control3 = component3.createObject(testCase)
        verify(control3)
        verify(control3.item3_2)
        verify(control3.item3_3)
        verify(control3.item3_4)
        verify(control3.item3_5)
        verify(control3.item3_6)
        verify(control3.item3_7)
        verify(control3.item3_8)

        compare(control3.font.family, "Arial")
        compare(control3.item3_3.font.family, control3.font.family)
        compare(control3.item3_3.font.pointSize, control3.font.pointSize)
        compare(control3.item3_3.font.weight, control3.font.weight)
        compare(control3.item3_4.font.family, control3.font.family)
        compare(control3.item3_4.font.pointSize, control3.font.pointSize)
        compare(control3.item3_4.font.weight, control3.font.weight)
        compare(control3.item3_5.font.family, control3.font.family)
        compare(control3.item3_5.font.pointSize, control3.font.pointSize)
        compare(control3.item3_5.font.weight, control3.font.weight)
        compare(control3.item3_7.font.family, control3.font.family)
        compare(control3.item3_7.font.pointSize, control3.font.pointSize)
        compare(control3.item3_7.font.weight, control3.font.weight)
        compare(control3.item3_8.font.family, control3.font.family)
        compare(control3.item3_8.font.pointSize, control3.font.pointSize)
        compare(control3.item3_8.font.weight, control3.font.weight)

        control3.font.pointSize = 48
        compare(control3.item3_3.font.pointSize, 48)
        compare(control3.item3_4.font.pointSize, 48)
        compare(control3.item3_5.font.pointSize, 48)

        control3.font.bold = true
        compare(control3.item3_3.font.weight, Font.Bold)
        compare(control3.item3_4.font.weight, Font.Bold)
        compare(control3.item3_5.font.weight, Font.Bold)

        compare(control3.item3_3.font.family, control3.font.family)
        compare(control3.item3_3.font.pointSize, control3.font.pointSize)
        compare(control3.item3_3.font.weight, control3.font.weight)
        compare(control3.item3_7.font.family, control3.font.family)
        compare(control3.item3_7.font.pointSize, control3.font.pointSize)
        compare(control3.item3_7.font.weight, control3.font.weight)

        control3.item3_3.font.pointSize = 60
        compare(control3.item3_3.font.pointSize, 60)

        control3.item3_3.font.weight = Font.Normal
        compare(control3.item3_3.font.weight, Font.Normal)

        control3.item3_4.font.pointSize = 16
        compare(control3.item3_4.font.pointSize, 16)

        control3.item3_4.font.weight = Font.Normal
        compare(control3.item3_4.font.weight, Font.Normal)

        control3.item3_5.font.pointSize = 32
        compare(control3.item3_5.font.pointSize, 32)

        control3.item3_5.font.weight = Font.DemiBold
        compare(control3.item3_5.font.weight, Font.DemiBold)

        control3.item3_8.font.pointSize = 36
        compare(control3.item3_8.font.pointSize, 36)

        control3.item3_8.font.weight = Font.Black
        compare(control3.item3_8.font.weight, Font.Black)

        control3.font.pointSize = 100
        compare(control3.font.pointSize, 100)
        compare(control3.item3_3.font.pointSize, 60)
        compare(control3.item3_4.font.pointSize, 16)
        compare(control3.item3_5.font.pointSize, 32)
        compare(control3.item3_8.font.pointSize, 36)

        compare(control3.font.family, "Arial")
        compare(control3.font.pointSize, 100)
        compare(control3.font.weight, Font.Bold)

        compare(control3.item3_3.font.family, "Arial")
        compare(control3.item3_3.font.pointSize, 60)
        compare(control3.item3_3.font.weight, Font.Normal)
        compare(control3.item3_7.font.family, control3.item3_3.font.family)
        compare(control3.item3_7.font.pointSize, control3.item3_3.font.pointSize)
        compare(control3.item3_7.font.weight, control3.item3_3.font.weight)

        compare(control3.item3_4.font.family, "Arial")
        compare(control3.item3_4.font.pointSize, 16)
        compare(control3.item3_4.font.weight, Font.Normal)

        compare(control3.item3_5.font.family, "Arial")
        compare(control3.item3_5.font.pointSize, 32)
        compare(control3.item3_5.font.weight, Font.DemiBold)

        compare(control3.item3_8.font.family, "Arial")
        compare(control3.item3_8.font.pointSize, 36)
        compare(control3.item3_8.font.weight, Font.Black)

        control3.destroy()
    }
}
