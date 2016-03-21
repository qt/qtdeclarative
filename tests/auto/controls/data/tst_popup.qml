/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.4
import QtTest 1.0
import Qt.labs.controls 1.0
import Qt.labs.templates 1.0 as T

TestCase {
    id: testCase
    width: 480
    height: 360
    visible: true
    when: windowShown
    name: "Popup"

    ApplicationWindow {
        id: applicationWindow
        width: 480
        height: 360
    }

    Component {
        id: popupTemplate
        T.Popup { }
    }

    Component {
        id: popupControl
        Popup { }
    }

    Component {
        id: rect
        Rectangle { }
    }

    SignalSpy {
        id: availableWidthSpy
        signalName: "availableWidthChanged"
    }

    SignalSpy {
        id: availableHeightSpy
        signalName: "availableHeightChanged"
    }

    SignalSpy {
        id: paddingSpy
        signalName: "paddingChanged"
    }

    SignalSpy {
        id: topPaddingSpy
        signalName: "topPaddingChanged"
    }

    SignalSpy {
        id: leftPaddingSpy
        signalName: "leftPaddingChanged"
    }

    SignalSpy {
        id: rightPaddingSpy
        signalName: "rightPaddingChanged"
    }

    SignalSpy {
        id: bottomPaddingSpy
        signalName: "bottomPaddingChanged"
    }

    function test_padding() {
        var control = popupTemplate.createObject(testCase)
        verify(control)

        paddingSpy.target = control
        topPaddingSpy.target = control
        leftPaddingSpy.target = control
        rightPaddingSpy.target = control
        bottomPaddingSpy.target = control

        verify(paddingSpy.valid)
        verify(topPaddingSpy.valid)
        verify(leftPaddingSpy.valid)
        verify(rightPaddingSpy.valid)
        verify(bottomPaddingSpy.valid)

        var paddingChanges = 0
        var topPaddingChanges = 0
        var leftPaddingChanges = 0
        var rightPaddingChanges = 0
        var bottomPaddingChanges = 0

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
        compare(paddingSpy.count, ++paddingChanges)
        compare(topPaddingSpy.count, ++topPaddingChanges)
        compare(leftPaddingSpy.count, ++leftPaddingChanges)
        compare(rightPaddingSpy.count, ++rightPaddingChanges)
        compare(bottomPaddingSpy.count, ++bottomPaddingChanges)

        control.topPadding = 20
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 10)
        compare(control.rightPadding, 10)
        compare(control.bottomPadding, 10)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, ++topPaddingChanges)
        compare(leftPaddingSpy.count, leftPaddingChanges)
        compare(rightPaddingSpy.count, rightPaddingChanges)
        compare(bottomPaddingSpy.count, bottomPaddingChanges)

        control.leftPadding = 30
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 10)
        compare(control.bottomPadding, 10)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, topPaddingChanges)
        compare(leftPaddingSpy.count, ++leftPaddingChanges)
        compare(rightPaddingSpy.count, rightPaddingChanges)
        compare(bottomPaddingSpy.count, bottomPaddingChanges)

        control.rightPadding = 40
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 10)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, topPaddingChanges)
        compare(leftPaddingSpy.count, leftPaddingChanges)
        compare(rightPaddingSpy.count, ++rightPaddingChanges)
        compare(bottomPaddingSpy.count, bottomPaddingChanges)

        control.bottomPadding = 50
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 50)
        compare(paddingSpy.count, paddingChanges)
        compare(topPaddingSpy.count, topPaddingChanges)
        compare(leftPaddingSpy.count, leftPaddingChanges)
        compare(rightPaddingSpy.count, rightPaddingChanges)
        compare(bottomPaddingSpy.count, ++bottomPaddingChanges)

        control.padding = 60
        compare(control.padding, 60)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 50)
        compare(paddingSpy.count, ++paddingChanges)
        compare(topPaddingSpy.count, topPaddingChanges)
        compare(leftPaddingSpy.count, leftPaddingChanges)
        compare(rightPaddingSpy.count, rightPaddingChanges)
        compare(bottomPaddingSpy.count, bottomPaddingChanges)

        control.destroy()
    }

    function test_availableSize() {
        var control = popupTemplate.createObject(testCase)
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

    function test_margins() {
        var control = popupControl.createObject(testCase, {width: 100, height: 100})
        verify(control)

        control.open()
        verify(control.visible)

        control.margins = 10
        compare(control.margins, 10)
        compare(control.topMargin, 10)
        compare(control.leftMargin, 10)
        compare(control.rightMargin, 10)
        compare(control.bottomMargin, 10)
        compare(control.contentItem.parent.x, 10)
        compare(control.contentItem.parent.y, 10)

        control.topMargin = 20
        compare(control.margins, 10)
        compare(control.topMargin, 20)
        compare(control.leftMargin, 10)
        compare(control.rightMargin, 10)
        compare(control.bottomMargin, 10)
        compare(control.contentItem.parent.x, 10)
        compare(control.contentItem.parent.y, 20)

        control.leftMargin = 20
        compare(control.margins, 10)
        compare(control.topMargin, 20)
        compare(control.leftMargin, 20)
        compare(control.rightMargin, 10)
        compare(control.bottomMargin, 10)
        compare(control.contentItem.parent.x, 20)
        compare(control.contentItem.parent.y, 20)

        control.x = testCase.width
        control.y = testCase.height
        compare(control.contentItem.parent.x, testCase.width - control.width - 10)
        compare(control.contentItem.parent.y, testCase.height - control.height - 10)

        control.rightMargin = 20
        compare(control.margins, 10)
        compare(control.topMargin, 20)
        compare(control.leftMargin, 20)
        compare(control.rightMargin, 20)
        compare(control.bottomMargin, 10)
        compare(control.contentItem.parent.x, testCase.width - control.width - 20)
        compare(control.contentItem.parent.y, testCase.height - control.height - 10)

        control.bottomMargin = 20
        compare(control.margins, 10)
        compare(control.topMargin, 20)
        compare(control.leftMargin, 20)
        compare(control.rightMargin, 20)
        compare(control.bottomMargin, 20)
        compare(control.contentItem.parent.x, testCase.width - control.width - 20)
        compare(control.contentItem.parent.y, testCase.height - control.height - 20)

        control.destroy()
    }

    function test_background() {
        var control = popupTemplate.createObject(testCase)
        verify(control)

        control.background = rect.createObject(testCase)

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

    function getChild(control, objname, idx) {
        var index = idx
        for (var i = index+1; i < control.children.length; i++)
        {
            if (control.children[i].objectName === objname) {
                index = i
                break
            }
        }
        return index
    }

    Component {
        id: component
        ApplicationWindow {
            width: 400
            height: 400
            visible: true
            font.pixelSize: 40
            property alias pane: _pane
            Pane {
                id: _pane
                property alias button: _button;
                property alias popup: _popup;
                property alias listview: _listview
                font.pixelSize: 30
                Column {
                    Button {
                        id: _button
                        text: "Button"
                        font.pixelSize: 20

                        Popup {
                            id: _popup
                            y: _button.height
                            implicitHeight: Math.min(396, _listview.contentHeight)
                            contentItem: ListView {
                                id: _listview
                                height: _button.height * 20
                                model: 2
                                delegate: Button {
                                    objectName: "delegate"
                                    width: _button.width
                                    height: _button.height
                                    text: "N: " + index
                                    checkable: true
                                    autoExclusive: true
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    function test_font() { // QTBUG_50984, QTBUG-51696
        var window = component.createObject(testCase)
        verify(window)

        window.requestActivate()
        tryCompare(window, "active", true)

        var control = window.pane
        waitForRendering(control)

        control.forceActiveFocus()
        verify(control.activeFocus)

        compare(window.font.pixelSize, 40)
        compare(control.font.pixelSize, 30)
        compare(control.button.font.pixelSize, 20)

        var popup = control.popup
        popup.open()

        verify(popup.contentItem)

        var listview = popup.contentItem
        verify(listview.contentItem)
        waitForRendering(listview)

        var idx1 = getChild(listview.contentItem, "delegate", -1)
        compare(listview.contentItem.children[idx1].font.pixelSize, 40)
        var idx2 = getChild(listview.contentItem, "delegate", idx1)
        compare(listview.contentItem.children[idx2].font.pixelSize, 40)

        control.button.font.pixelSize = 30
        compare(control.button.font.pixelSize, 30)
        waitForRendering(listview)
        compare(listview.contentItem.children[idx1].font.pixelSize, 40)
        compare(listview.contentItem.children[idx2].font.pixelSize, 40)

        window.font.pixelSize = 50
        waitForRendering(listview)
        compare(listview.contentItem.children[idx1].font.pixelSize, 50)
        compare(listview.contentItem.children[idx2].font.pixelSize, 50)

        window.destroy()
    }

    Component {
        id: localeComponent
        Pane {
            property alias button: _button
            property alias popup: _popup
            locale: Qt.locale("en_US")
            Column {
                Button {
                    id: _button
                    text: "Button"
                    locale: Qt.locale("nb_NO")
                    Popup {
                        id: _popup
                        property alias button1: _button1
                        property alias button2: _button2
                        y: _button.height
                        implicitHeight: Math.min(396, _column.contentHeight)
                        contentItem: Column {
                            id: _column
                            Button {
                                id: _button1
                                text: "Button 1"
                            }
                            Button {
                                id: _button2
                                text: "Button 2"
                            }
                        }
                    }
                }
            }
        }
    }

    function test_locale() { // QTBUG_50984
        // test looking up natural locale from ancestors
        var control = localeComponent.createObject(applicationWindow.contentItem)
        verify(control)
        verify(control.button)
        verify(control.popup)
        verify(control.popup.button1)
        verify(control.popup.button2)

        applicationWindow.visible = true
        waitForRendering(control)

        control.popup.open()
        verify(control.popup.visible)

        control.ApplicationWindow.window.locale = Qt.locale("fi_FI")
        compare(control.ApplicationWindow.window.locale.name, "fi_FI")
        compare(control.locale.name, "en_US")
        compare(control.button.locale.name, "nb_NO")
        compare(control.popup.button1.locale.name, "fi_FI")
        compare(control.popup.button2.locale.name, "fi_FI")

        control.ApplicationWindow.window.locale = undefined
        applicationWindow.visible = false
        control.destroy()
    }

    Component {
        id: localeChangeComponent
        Pane {
            id: _pane
            property alias button: _button
            property alias popup: _popup
            property SignalSpy localespy: SignalSpy {
                target: _pane
                signalName: "localeChanged"
            }
            property SignalSpy mirrorspy: SignalSpy {
                target: _pane
                signalName: "mirroredChanged"
            }
            Column {
                Button {
                    id: _button
                    text: "Button"
                    property SignalSpy localespy: SignalSpy {
                        target: _button
                        signalName: "localeChanged"
                    }
                    property SignalSpy mirrorspy: SignalSpy {
                        target: _button
                        signalName: "mirroredChanged"
                    }
                    Popup {
                        id: _popup
                        property alias button1: _button1
                        property alias button2: _button2
                        y: _button.height
                        implicitHeight: Math.min(396, _column.contentHeight)
                        contentItem: Column {
                            id: _column
                            Button {
                                id: _button1
                                text: "Button 1"
                                property SignalSpy localespy: SignalSpy {
                                    target: _button1
                                    signalName: "localeChanged"
                                }
                                property SignalSpy mirrorspy: SignalSpy {
                                    target: _button1
                                    signalName: "mirroredChanged"
                                }
                            }
                            Button {
                                id: _button2
                                text: "Button 2"
                                property SignalSpy localespy: SignalSpy {
                                    target: _button2
                                    signalName: "localeChanged"
                                }
                                property SignalSpy mirrorspy: SignalSpy {
                                    target: _button2
                                    signalName: "mirroredChanged"
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    function test_locale_changes() { // QTBUG_50984
        // test default locale and locale inheritance
        var control = localeChangeComponent.createObject(applicationWindow.contentItem)
        verify(control)
        verify(control.button)
        verify(control.popup)
        verify(control.popup.button1)
        verify(control.popup.button2)

        applicationWindow.visible = true
        waitForRendering(control)

        control.popup.open()
        verify(control.popup.visible)

        var defaultLocale = Qt.locale()
        compare(control.ApplicationWindow.window.locale.name, defaultLocale.name)
        compare(control.locale.name, defaultLocale.name)
        compare(control.button.locale.name, defaultLocale.name)
        compare(control.popup.button1.locale.name, defaultLocale.name)
        compare(control.popup.button2.locale.name, defaultLocale.name)

        control.ApplicationWindow.window.locale = Qt.locale("nb_NO")
        compare(control.ApplicationWindow.window.locale.name, "nb_NO")
        compare(control.locale.name, "nb_NO")
        compare(control.button.locale.name, "nb_NO")
        compare(control.popup.button1.locale.name, "nb_NO")
        compare(control.popup.button2.locale.name, "nb_NO")
        compare(control.localespy.count, 1)
        compare(control.button.localespy.count, 1)
        compare(control.popup.button1.localespy.count, 1)
        compare(control.popup.button2.localespy.count, 1)

        control.ApplicationWindow.window.locale = undefined
        compare(control.ApplicationWindow.window.locale.name, defaultLocale.name)
        compare(control.locale.name, defaultLocale.name)
        compare(control.button.locale.name, defaultLocale.name)
        compare(control.popup.button1.locale.name, defaultLocale.name)
        compare(control.popup.button2.locale.name, defaultLocale.name)
        compare(control.localespy.count, 2)
        compare(control.button.localespy.count, 2)
        compare(control.popup.button1.localespy.count, 2)
        compare(control.popup.button2.localespy.count, 2)

        control.locale = Qt.locale("ar_EG")
        compare(control.ApplicationWindow.window.locale.name, defaultLocale.name)
        compare(control.locale.name, "ar_EG")
        compare(control.button.locale.name, "ar_EG")
        compare(control.popup.button1.locale.name, defaultLocale.name)
        compare(control.popup.button2.locale.name, defaultLocale.name)
        compare(control.localespy.count, 3)
        compare(control.mirrorspy.count, 1)
        compare(control.button.localespy.count, 3)
        compare(control.button.mirrorspy.count, 1)
        compare(control.popup.button1.localespy.count, 2)
        compare(control.popup.button2.localespy.count, 2)

        control.ApplicationWindow.window.locale = Qt.locale("ar_EG")
        compare(control.ApplicationWindow.window.locale.name, "ar_EG")
        compare(control.locale.name, "ar_EG")
        compare(control.button.locale.name, "ar_EG")
        compare(control.popup.button1.locale.name, "ar_EG")
        compare(control.popup.button2.locale.name, "ar_EG")
        compare(control.localespy.count, 3)
        compare(control.mirrorspy.count, 1)
        compare(control.button.localespy.count, 3)
        compare(control.button.mirrorspy.count, 1)
        compare(control.popup.button1.localespy.count, 3)
        compare(control.popup.button1.mirrorspy.count, 1)
        compare(control.popup.button2.localespy.count, 3)
        compare(control.popup.button2.mirrorspy.count, 1)

        control.button.locale = Qt.locale("nb_NO")
        compare(control.ApplicationWindow.window.locale.name, "ar_EG")
        compare(control.locale.name, "ar_EG")
        compare(control.button.locale.name, "nb_NO")
        compare(control.popup.button1.locale.name, "ar_EG")
        compare(control.popup.button2.locale.name, "ar_EG")
        compare(control.localespy.count, 3)
        compare(control.mirrorspy.count, 1)
        compare(control.button.localespy.count, 4)
        compare(control.button.mirrorspy.count, 2)
        compare(control.popup.button1.localespy.count, 3)
        compare(control.popup.button2.localespy.count, 3)

        control.locale = undefined
        compare(control.ApplicationWindow.window.locale.name, "ar_EG")
        compare(control.locale.name, "ar_EG")
        compare(control.button.locale.name, "nb_NO")
        compare(control.popup.button1.locale.name, "ar_EG")
        compare(control.popup.button2.locale.name, "ar_EG")
        compare(control.localespy.count, 3)
        compare(control.mirrorspy.count, 1)
        compare(control.button.localespy.count, 4)
        compare(control.button.mirrorspy.count, 2)
        compare(control.popup.button1.localespy.count, 3)
        compare(control.popup.button2.localespy.count, 3)

        control.popup.button1.locale = Qt.locale("nb_NO")
        compare(control.ApplicationWindow.window.locale.name, "ar_EG")
        compare(control.locale.name, "ar_EG")
        compare(control.button.locale.name, "nb_NO")
        compare(control.popup.button1.locale.name, "nb_NO")
        compare(control.popup.button2.locale.name, "ar_EG")
        compare(control.localespy.count, 3)
        compare(control.mirrorspy.count, 1)
        compare(control.button.localespy.count, 4)
        compare(control.button.mirrorspy.count, 2)
        compare(control.popup.button1.localespy.count, 4)
        compare(control.popup.button1.mirrorspy.count, 2)
        compare(control.popup.button2.localespy.count, 3)
        compare(control.popup.button2.mirrorspy.count, 1)

        control.ApplicationWindow.window.locale = undefined
        compare(control.ApplicationWindow.window.locale.name, defaultLocale.name)
        compare(control.locale.name, defaultLocale.name)
        compare(control.button.locale.name, "nb_NO")
        compare(control.popup.button1.locale.name, "nb_NO")
        compare(control.popup.button2.locale.name, defaultLocale.name)
        compare(control.localespy.count, 4)
        compare(control.mirrorspy.count, 2)
        compare(control.button.localespy.count, 4)
        compare(control.button.mirrorspy.count, 2)
        compare(control.popup.button1.localespy.count, 4)
        compare(control.popup.button1.mirrorspy.count, 2)
        compare(control.popup.button2.localespy.count, 4)
        compare(control.popup.button2.mirrorspy.count, 2)

        applicationWindow.visible = false
        control.destroy()
    }

    function test_size() {
        var control = popupControl.createObject(testCase)
        verify(control)

        control.width = 200
        control.height = 200

        control.open()
        waitForRendering(control.contentItem)

        compare(control.width, 200)
        compare(control.height, 200)

        control.destroy()
    }

    Component {
        id: overlayTest
        ApplicationWindow {
            property alias popup1: popup1
            property alias popup2: popup2
            visible: true
            Popup {
                id: popup1
                modal: true
                exit: Transition { PauseAnimation { duration: 200 } }
            }
            Popup {
                id: popup2
                modal: true
            }
        }
    }

    function test_overlay() {
        var window = overlayTest.createObject(testCase)
        verify(window)

        window.requestActivate()
        tryCompare(window, "active", true)
        compare(window.overlay.background.opacity, 0.0)

        window.popup1.open()
        compare(window.popup1.visible, true)
        compare(window.popup2.visible, false)
        tryCompare(window.overlay.background, "opacity", 1.0)

        window.popup1.close()
        window.popup2.open()
        compare(window.popup2.visible, true)
        tryCompare(window.popup1, "visible", false)
        compare(window.overlay.background.opacity, 1.0)

        window.destroy()
    }
}
