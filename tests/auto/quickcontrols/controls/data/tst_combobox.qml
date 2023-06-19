// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtTest
import QtQuick.Controls
import QtQuick.NativeStyle as NativeStyle

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "ComboBox"

    Component {
        id: signalSpy
        SignalSpy { }
    }

    Component {
        id: comboBox
        ComboBox { }
    }

    Component {
        id: emptyBox
        ComboBox {
            delegate: ItemDelegate {
                width: parent.width
            }
        }
    }

    Component {
        id: mouseArea
        MouseArea { }
    }

    Component {
        id: customPopup
        Popup {
            width: 100
            implicitHeight: contentItem.implicitHeight
            contentItem: TextInput {
                anchors.fill: parent
            }
        }
    }

    Component {
        id: comboBoxWithShaderEffect
        ComboBox {
            delegate: Rectangle {
                Text {
                    id: txt
                    anchors.centerIn: parent
                    text: "item" + index
                    font.pixelSize: 20
                    color: "red"
                }
                id: rect
                objectName: "rect"
                width: parent.width
                height: txt.implicitHeight
                gradient: Gradient {
                    GradientStop { color: "lightsteelblue"; position: 0.0 }
                    GradientStop { color: "blue"; position: 1.0 }
                }
                layer.enabled: true
                layer.effect: ShaderEffect {
                    objectName: "ShaderFX"
                    width: rect.width
                    height: rect.height
                    fragmentShader: "
                            uniform lowp sampler2D source; // this item
                            uniform lowp float qt_Opacity; // inherited opacity of this item
                            varying highp vec2 qt_TexCoord0;
                            void main() {
                                lowp vec4 p = texture2D(source, qt_TexCoord0);
                                lowp float g = dot(p.xyz, vec3(0.344, 0.5, 0.156));
                                gl_FragColor = vec4(g, g, g, p.a) * qt_Opacity;
                            }"

                }
            }
        }
    }

    function init() {
        // QTBUG-61225: Move the mouse away to avoid QQuickDeliveryAgentPrivate::flushFrameSynchronousEvents()
        // delivering interfering hover events based on the last mouse position from earlier tests. For
        // example, ComboBox::test_activation() kept receiving hover events for the last mouse position
        // from CheckDelegate::test_checked().
        mouseMove(testCase, testCase.width - 1, testCase.height - 1)
    }

    function test_defaults() {
        failOnWarning(/.?/)

        var control = createTemporaryObject(comboBox, testCase)
        verify(control)

        compare(control.count, 0)
        compare(control.model, undefined)
        compare(control.flat, false)
        compare(control.pressed, false)
        compare(control.currentIndex, -1)
        compare(control.highlightedIndex, -1)
        compare(control.currentText, "")
        verify(control.delegate)
        if (Qt.platform.pluginName !== "cocoa" && Qt.platform.pluginName !== "windows") {
            // Only the non-native styles sets an indicator delegate. The native
            // styles will instead draw the indicator as a part of the background.
            verify(control.indicator)
        }
        verify(control.popup)
        verify(control.acceptableInput)
        compare(control.inputMethodHints, Qt.ImhNoPredictiveText)
    }

    function test_array() {
        var control = createTemporaryObject(comboBox, testCase)
        verify(control)

        var items = [ "Banana", "Apple", "Coconut" ]

        control.model = items
        compare(control.model, items)

        compare(control.count, 3)
        compare(control.currentIndex, 0)
        compare(control.currentText, "Banana")

        control.currentIndex = 2
        compare(control.currentIndex, 2)
        compare(control.currentText, "Coconut")

        control.model = null
        compare(control.model, null)
        compare(control.count, 0)
        compare(control.currentIndex, -1)
        compare(control.currentText, "")
    }

    function test_objects() {
        var control = createTemporaryObject(emptyBox, testCase)
        verify(control)

        var items = [
            { text: "Apple" },
            { text: "Orange" },
            { text: "Banana" }
        ]

        control.model = items
        compare(control.model, items)

        compare(control.count, 3)
        compare(control.currentIndex, 0)
        compare(control.currentText, "Apple")

        control.currentIndex = 2
        compare(control.currentIndex, 2)
        compare(control.currentText, "Banana")

        control.model = null
        compare(control.model, null)
        compare(control.count, 0)
        compare(control.currentIndex, -1)
        compare(control.currentText, "")
    }

    function test_qobjects() {
        var control = createTemporaryObject(emptyBox, testCase, {textRole: "text"})
        verify(control)

        var obj1 = Qt.createQmlObject("import QtQml; QtObject { property string text: 'one' }", control)
        var obj2 = Qt.createQmlObject("import QtQml; QtObject { property string text: 'two' }", control)
        var obj3 = Qt.createQmlObject("import QtQml; QtObject { property string text: 'three' }", control)

        control.model = [obj1, obj2, obj3]

        compare(control.count, 3)
        compare(control.currentIndex, 0)
        compare(control.currentText, "one")

        control.currentIndex = 2
        compare(control.currentIndex, 2)
        compare(control.currentText, "three")

        control.model = null
        compare(control.model, null)
        compare(control.count, 0)
        compare(control.currentIndex, -1)
        compare(control.currentText, "")
    }

    function test_number() {
        var control = createTemporaryObject(comboBox, testCase)
        verify(control)

        control.model = 10
        compare(control.model, 10)

        compare(control.count, 10)
        compare(control.currentIndex, 0)
        compare(control.currentText, "0")

        control.currentIndex = 9
        compare(control.currentIndex, 9)
        compare(control.currentText, "9")

        control.model = 0
        compare(control.model, 0)
        compare(control.count, 0)
        compare(control.currentIndex, -1)
        compare(control.currentText, "")
    }

    ListModel {
        id: listmodel
        ListElement { text: "First" }
        ListElement { text: "Second" }
        ListElement { text: "Third" }
        ListElement { text: "Fourth" }
        ListElement { text: "Fifth" }
    }

    function test_listModel() {
        var control = createTemporaryObject(comboBox, testCase)
        verify(control)

        control.model = listmodel
        compare(control.model, listmodel)

        compare(control.count, 5)
        compare(control.currentIndex, 0)
        compare(control.currentText, "First")

        control.currentIndex = 2
        compare(control.currentIndex, 2)
        compare(control.currentText, "Third")

        control.model = undefined
        compare(control.model, undefined)
        compare(control.count, 0)
        compare(control.currentIndex, -1)
        compare(control.currentText, "")
    }

    ListModel {
        id: fruitmodel
        ListElement { name: "Apple"; color: "red" }
        ListElement { name: "Orange"; color: "orange" }
        ListElement { name: "Banana"; color: "yellow" }
    }

    Component {
        id: fruitModelComponent
        ListModel {
            ListElement { name: "Apple"; color: "red" }
            ListElement { name: "Orange"; color: "orange" }
            ListElement { name: "Banana"; color: "yellow" }
        }
    }

    property var fruitarray: [
        { name: "Apple", color: "red" },
        { name: "Orange", color: "orange" },
        { name: "Banana", color: "yellow" }
    ]

    Component {
        id: birdModelComponent
        ListModel {
            ListElement { name: "Galah"; color: "pink" }
            ListElement { name: "Kookaburra"; color: "brown" }
            ListElement { name: "Magpie"; color: "black" }
        }
    }

    function test_textRole_data() {
        return [
            { tag: "ListModel", model: fruitmodel },
            { tag: "ObjectArray", model: fruitarray }
        ]
    }

    function test_textRole(data) {
        var control = createTemporaryObject(emptyBox, testCase)
        verify(control)

        control.model = data.model
        compare(control.count, 3)
        compare(control.currentIndex, 0)
        compare(control.currentText, "")

        control.textRole = "name"
        compare(control.currentText, "Apple")

        control.textRole = "color"
        compare(control.currentText, "red")

        control.currentIndex = 1
        compare(control.currentIndex, 1)
        compare(control.currentText, "orange")

        control.textRole = "name"
        compare(control.currentText, "Orange")

        control.textRole = ""
        compare(control.currentText, "")
    }

    function test_textAt() {
        var control = createTemporaryObject(comboBox, testCase)
        verify(control)

        control.model = ["Apple", "Orange", "Banana"]
        compare(control.textAt(0), "Apple")
        compare(control.textAt(1), "Orange")
        compare(control.textAt(2), "Banana")
        compare(control.textAt(-1), "") // TODO: null?
        compare(control.textAt(5), "") // TODO: null?
    }

    function test_find_data() {
        return [
            { tag: "Banana (MatchExactly)", term: "Banana", flags: Qt.MatchExactly, index: 0 },
            { tag: "banana (MatchExactly)", term: "banana", flags: Qt.MatchExactly, index: 1 },
            { tag: "bananas (MatchExactly)", term: "bananas", flags: Qt.MatchExactly, index: -1 },
            { tag: "Cocomuffin (MatchExactly)", term: "Cocomuffin", flags: Qt.MatchExactly, index: 4 },

            { tag: "b(an)+a (MatchRegularExpression)", term: "B(an)+a", flags: Qt.MatchRegularExpression, index: 0 },
            { tag: "b(an)+a (MatchRegularExpression|MatchCaseSensitive)", term: "b(an)+a", flags: Qt.MatchRegularExpression | Qt.MatchCaseSensitive, index: 1 },
            { tag: "[coc]+\\w+ (MatchRegularExpression)", term: "[coc]+\\w+", flags: Qt.MatchRegularExpression, index: 2 },

            { tag: "?pp* (MatchWildcard)", term: "?pp*", flags: Qt.MatchWildcard, index: 3 },
            { tag: "app* (MatchWildcard|MatchCaseSensitive)", term: "app*", flags: Qt.MatchWildcard | Qt.MatchCaseSensitive, index: -1 },

            { tag: "Banana (MatchFixedString)", term: "Banana", flags: Qt.MatchFixedString, index: 0 },
            { tag: "banana (MatchFixedString|MatchCaseSensitive)", term: "banana", flags: Qt.MatchFixedString | Qt.MatchCaseSensitive, index: 1 },

            { tag: "coco (MatchStartsWith)", term: "coco", flags: Qt.MatchStartsWith, index: 2 },
            { tag: "coco (MatchStartsWith|MatchCaseSensitive)", term: "coco", flags: Qt.StartsWith | Qt.MatchCaseSensitive, index: -1 },

            { tag: "MUFFIN (MatchEndsWith)", term: "MUFFIN", flags: Qt.MatchEndsWith, index: 4 },
            { tag: "MUFFIN (MatchEndsWith|MatchCaseSensitive)", term: "MUFFIN", flags: Qt.MatchEndsWith | Qt.MatchCaseSensitive, index: -1 },

            { tag: "Con (MatchContains)", term: "Con", flags: Qt.MatchContains, index: 2 },
            { tag: "Con (MatchContains|MatchCaseSensitive)", term: "Con", flags: Qt.MatchContains | Qt.MatchCaseSensitive, index: -1 },
        ]
    }

    function test_find(data) {
        var control = createTemporaryObject(comboBox, testCase)
        verify(control)

        control.model = ["Banana", "banana", "Coconut", "Apple", "Cocomuffin"]

        compare(control.find(data.term, data.flags), data.index)
    }

    function test_valueRole_data() {
        return [
            { tag: "ListModel", model: fruitmodel },
            { tag: "ObjectArray", model: fruitarray }
        ]
    }

    function test_valueRole(data) {
        var control = createTemporaryObject(emptyBox, testCase,
            { model: data.model, valueRole: "color" })
        verify(control)
        compare(control.count, 3)
        compare(control.currentIndex, 0)
        compare(control.currentValue, "red")

        control.valueRole = "name"
        compare(control.currentValue, "Apple")

        control.currentIndex = 1
        compare(control.currentIndex, 1)
        compare(control.currentValue, "Orange")

        control.valueRole = "color"
        compare(control.currentValue, "orange")

        control.model = null
        compare(control.currentIndex, -1)
        // An invalid QVariant is represented as undefined.
        compare(control.currentValue, undefined)

        control.valueRole = ""
        compare(control.currentValue, undefined)
    }

    function test_valueAt() {
        var control = createTemporaryObject(comboBox, testCase,
            { model: fruitmodel, textRole: "name", valueRole: "color" })
        verify(control)

        compare(control.valueAt(0), "red")
        compare(control.valueAt(1), "orange")
        compare(control.valueAt(2), "yellow")
        compare(control.valueAt(-1), undefined)
        compare(control.valueAt(5), undefined)
    }

    function test_indexOfValue_data() {
        return [
            { tag: "red", expectedIndex: 0 },
            { tag: "orange", expectedIndex: 1 },
            { tag: "yellow", expectedIndex: 2 },
            { tag: "brown", expectedIndex: -1 },
        ]
    }

    function test_indexOfValue(data) {
        var control = createTemporaryObject(comboBox, testCase,
            { model: fruitmodel, textRole: "name", valueRole: "color" })
        verify(control)

        compare(control.indexOfValue(data.tag), data.expectedIndex)
    }

    function test_currentValueAfterModelChanged() {
        let fruitModel = createTemporaryObject(fruitModelComponent, testCase)
        verify(fruitModel)

        let control = createTemporaryObject(comboBox, testCase,
            { model: fruitModel, textRole: "name", valueRole: "color", currentIndex: 1 })
        verify(control)
        compare(control.currentText, "Orange")
        compare(control.currentValue, "orange")

        // Remove "Apple"; the current item should now be "Banana", so currentValue should be "yellow".
        fruitModel.remove(0)
        compare(control.currentText, "Banana")
        compare(control.currentValue, "yellow")
    }

    function test_currentValueAfterNewModelSet() {
        let control = createTemporaryObject(comboBox, testCase,
            { model: fruitmodel, textRole: "name", valueRole: "color", currentIndex: 0 })
        verify(control)
        compare(control.currentText, "Apple")
        compare(control.currentValue, "red")

        // Swap the model out entirely. Since the currentIndex was 0 and
        // is reset to 0 when a new model is set, it remains 0.
        let birdModel = createTemporaryObject(birdModelComponent, testCase)
        verify(birdModel)
        control.model = birdModel
        compare(control.currentText, "Galah")
        compare(control.currentValue, "pink")
    }

    function test_arrowKeys() {
        var control = createTemporaryObject(comboBox, testCase,
            { model: fruitmodel, textRole: "name", valueRole: "color" })
        verify(control)

        var activatedSpy = signalSpy.createObject(control, {target: control, signalName: "activated"})
        verify(activatedSpy.valid)

        var highlightedSpy = signalSpy.createObject(control, {target: control, signalName: "highlighted"})
        verify(highlightedSpy.valid)

        var openedSpy = signalSpy.createObject(control, {target: control.popup, signalName: "opened"})
        verify(openedSpy.valid)

        var closedSpy = signalSpy.createObject(control, {target: control.popup, signalName: "closed"})
        verify(closedSpy.valid)

        control.forceActiveFocus()
        verify(control.activeFocus)

        compare(control.currentIndex, 0)
        compare(control.highlightedIndex, -1)

        keyClick(Qt.Key_Down)
        compare(control.currentIndex, 1)
        compare(control.highlightedIndex, -1)
        compare(highlightedSpy.count, 0)
        compare(activatedSpy.count, 1)
        compare(activatedSpy.signalArguments[0][0], 1)
        activatedSpy.clear()

        keyClick(Qt.Key_Down)
        compare(control.currentIndex, 2)
        compare(control.highlightedIndex, -1)
        compare(highlightedSpy.count, 0)
        compare(activatedSpy.count, 1)
        compare(activatedSpy.signalArguments[0][0], 2)
        activatedSpy.clear()

        keyClick(Qt.Key_Down)
        compare(control.currentIndex, 2)
        compare(control.highlightedIndex, -1)
        compare(highlightedSpy.count, 0)
        compare(activatedSpy.count, 0)

        keyClick(Qt.Key_Up)
        compare(control.currentIndex, 1)
        compare(control.highlightedIndex, -1)
        compare(highlightedSpy.count, 0)
        compare(activatedSpy.count, 1)
        compare(activatedSpy.signalArguments[0][0], 1)
        activatedSpy.clear()

        keyClick(Qt.Key_Up)
        compare(control.currentIndex, 0)
        compare(control.highlightedIndex, -1)
        compare(highlightedSpy.count, 0)
        compare(activatedSpy.count, 1)
        compare(activatedSpy.signalArguments[0][0], 0)
        activatedSpy.clear()

        keyClick(Qt.Key_Up)
        compare(control.currentIndex, 0)
        compare(control.highlightedIndex, -1)
        compare(highlightedSpy.count, 0)
        compare(activatedSpy.count, 0)

        // show popup
        keyClick(Qt.Key_Space)
        openedSpy.wait()
        compare(openedSpy.count, 1)

        compare(control.currentIndex, 0)
        compare(control.highlightedIndex, 0)

        keyClick(Qt.Key_Down)
        compare(control.currentIndex, 0)
        compare(control.highlightedIndex, 1)
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 1)
        compare(highlightedSpy.signalArguments[0][0], 1)
        highlightedSpy.clear()

        keyClick(Qt.Key_Down)
        compare(control.currentIndex, 0)
        compare(control.highlightedIndex, 2)
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 1)
        compare(highlightedSpy.signalArguments[0][0], 2)
        highlightedSpy.clear()

        keyClick(Qt.Key_Down)
        compare(control.currentIndex, 0)
        compare(control.highlightedIndex, 2)
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 0)

        keyClick(Qt.Key_Up)
        compare(control.currentIndex, 0)
        compare(control.highlightedIndex, 1)
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 1)
        compare(highlightedSpy.signalArguments[0][0], 1)
        highlightedSpy.clear()

        keyClick(Qt.Key_Up)
        compare(control.currentIndex, 0)
        compare(control.highlightedIndex, 0)
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 1)
        compare(highlightedSpy.signalArguments[0][0], 0)
        highlightedSpy.clear()

        keyClick(Qt.Key_Up)
        compare(control.currentIndex, 0)
        compare(control.highlightedIndex, 0)
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 0)

        keyClick(Qt.Key_Down)
        compare(control.currentIndex, 0)
        compare(control.highlightedIndex, 1)
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 1)
        compare(highlightedSpy.signalArguments[0][0], 1)
        highlightedSpy.clear()

        // hide popup
        keyClick(Qt.Key_Space)
        closedSpy.wait()
        compare(closedSpy.count, 1)

        compare(control.currentIndex, 1)
        compare(control.highlightedIndex, -1)
    }

    function test_keys_space_enter_escape_data() {
        // Not testing Key_Enter + Key_Enter and Key_Return + Key_Return because
        // QGnomeTheme uses Key_Enter and Key_Return for pressing buttons/comboboxes
        // and the CI uses the QGnomeTheme platform theme.
        return [
            { tag: "space-space", key1: Qt.Key_Space, key2: Qt.Key_Space, showPopup: true, showPress: true, hidePopup: true, hidePress: true },
            { tag: "space-enter", key1: Qt.Key_Space, key2: Qt.Key_Enter, showPopup: true, showPress: true, hidePopup: true, hidePress: true },
            { tag: "space-return", key1: Qt.Key_Space, key2: Qt.Key_Return, showPopup: true, showPress: true, hidePopup: true, hidePress: true },
            { tag: "space-escape", key1: Qt.Key_Space, key2: Qt.Key_Escape, showPopup: true, showPress: true, hidePopup: true, hidePress: false },
            { tag: "space-0", key1: Qt.Key_Space, key2: Qt.Key_0, showPopup: true, showPress: true, hidePopup: false, hidePress: false },
            { tag: "escape-escape", key1: Qt.Key_Escape, key2: Qt.Key_Escape, showPopup: false, showPress: false, hidePopup: true, hidePress: false }
        ]
    }

    function test_keys_space_enter_escape(data) {
        var control = createTemporaryObject(comboBox, testCase, {model: 3})
        verify(control)

        var openedSpy = signalSpy.createObject(control, {target: control.popup, signalName: "opened"})
        verify(openedSpy.valid)

        control.forceActiveFocus()
        verify(control.activeFocus)

        compare(control.pressed, false)
        compare(control.popup.visible, false)

        // show popup
        keyPress(data.key1)
        compare(control.pressed, data.showPress)
        compare(control.popup.visible, false)
        keyRelease(data.key1)
        compare(control.pressed, false)
        compare(control.popup.visible, data.showPopup)
        if (data.showPopup)
            openedSpy.wait()

        // hide popup
        keyPress(data.key2)
        compare(control.pressed, data.hidePress)
        keyRelease(data.key2)
        compare(control.pressed, false)
        tryCompare(control.popup, "visible", !data.hidePopup)
    }

    function test_keys_home_end() {
        var control = createTemporaryObject(comboBox, testCase, {model: 5})
        verify(control)

        control.forceActiveFocus()
        verify(control.activeFocus)
        compare(control.currentIndex, 0)
        compare(control.highlightedIndex, -1)

        var activatedCount = 0
        var activatedSpy = signalSpy.createObject(control, {target: control, signalName: "activated"})
        verify(activatedSpy.valid)

        var highlightedCount = 0
        var highlightedSpy = signalSpy.createObject(control, {target: control, signalName: "highlighted"})
        verify(highlightedSpy.valid)

        var currentIndexCount = 0
        var currentIndexSpy = signalSpy.createObject(control, {target: control, signalName: "currentIndexChanged"})
        verify(currentIndexSpy.valid)

        var highlightedIndexCount = 0
        var highlightedIndexSpy = signalSpy.createObject(control, {target: control, signalName: "highlightedIndexChanged"})
        verify(highlightedIndexSpy.valid)

        // end (popup closed)
        keyClick(Qt.Key_End)
        compare(control.currentIndex, 4)
        compare(currentIndexSpy.count, ++currentIndexCount)

        compare(control.highlightedIndex, -1)
        compare(highlightedIndexSpy.count, highlightedIndexCount)

        compare(activatedSpy.count, ++activatedCount)
        compare(activatedSpy.signalArguments[activatedCount-1][0], 4)

        compare(highlightedSpy.count, highlightedCount)

        // repeat (no changes/signals)
        keyClick(Qt.Key_End)
        compare(currentIndexSpy.count, currentIndexCount)
        compare(highlightedIndexSpy.count, highlightedIndexCount)
        compare(activatedSpy.count, activatedCount)
        compare(highlightedSpy.count, highlightedCount)

        // home (popup closed)
        keyClick(Qt.Key_Home)
        compare(control.currentIndex, 0)
        compare(currentIndexSpy.count, ++currentIndexCount)

        compare(control.highlightedIndex, -1)
        compare(highlightedIndexSpy.count, highlightedIndexCount)

        compare(activatedSpy.count, ++activatedCount)
        compare(activatedSpy.signalArguments[activatedCount-1][0], 0)

        compare(highlightedSpy.count, highlightedCount)

        // repeat (no changes/signals)
        keyClick(Qt.Key_Home)
        compare(currentIndexSpy.count, currentIndexCount)
        compare(highlightedIndexSpy.count, highlightedIndexCount)
        compare(activatedSpy.count, activatedCount)
        compare(highlightedSpy.count, highlightedCount)

        control.popup.open()
        compare(control.highlightedIndex, 0)
        compare(highlightedIndexSpy.count, ++highlightedIndexCount)
        compare(highlightedSpy.count, highlightedCount)

        // end (popup open)
        keyClick(Qt.Key_End)
        compare(control.currentIndex, 0)
        compare(currentIndexSpy.count, currentIndexCount)

        compare(control.highlightedIndex, 4)
        compare(highlightedIndexSpy.count, ++highlightedIndexCount)

        compare(activatedSpy.count, activatedCount)

        compare(highlightedSpy.count, ++highlightedCount)
        compare(highlightedSpy.signalArguments[highlightedCount-1][0], 4)

        // repeat (no changes/signals)
        keyClick(Qt.Key_End)
        compare(currentIndexSpy.count, currentIndexCount)
        compare(highlightedIndexSpy.count, highlightedIndexCount)
        compare(activatedSpy.count, activatedCount)
        compare(highlightedSpy.count, highlightedCount)

        // home (popup open)
        keyClick(Qt.Key_Home)
        compare(control.currentIndex, 0)
        compare(currentIndexSpy.count, currentIndexCount)

        compare(control.highlightedIndex, 0)
        compare(highlightedIndexSpy.count, ++highlightedIndexCount)

        compare(activatedSpy.count, activatedCount)

        compare(highlightedSpy.count, ++highlightedCount)
        compare(highlightedSpy.signalArguments[highlightedCount-1][0], 0)

        // repeat (no changes/signals)
        keyClick(Qt.Key_Home)
        compare(currentIndexSpy.count, currentIndexCount)
        compare(highlightedIndexSpy.count, highlightedIndexCount)
        compare(activatedSpy.count, activatedCount)
        compare(highlightedSpy.count, highlightedCount)
    }

    function test_keySearch() {
        var control = createTemporaryObject(comboBox, testCase, {model: ["Banana", "Coco", "Coconut", "Apple", "Cocomuffin"]})
        verify(control)

        control.forceActiveFocus()
        verify(control.activeFocus)

        compare(control.currentIndex, 0)
        compare(control.currentText, "Banana")
        compare(control.highlightedIndex, -1)

        keyPress(Qt.Key_C)
        compare(control.currentIndex, 1)
        compare(control.currentText, "Coco")
        compare(control.highlightedIndex, -1)

        // no match
        keyPress(Qt.Key_N)
        compare(control.currentIndex, 1)
        compare(control.currentText, "Coco")
        compare(control.highlightedIndex, -1)

        keyPress(Qt.Key_C)
        compare(control.currentIndex, 2)
        compare(control.currentText, "Coconut")
        compare(control.highlightedIndex, -1)

        keyPress(Qt.Key_C)
        compare(control.currentIndex, 4)
        compare(control.currentText, "Cocomuffin")
        compare(control.highlightedIndex, -1)

        // wrap
        keyPress(Qt.Key_C)
        compare(control.currentIndex, 1)
        compare(control.currentText, "Coco")
        compare(control.highlightedIndex, -1)

        keyPress(Qt.Key_A)
        compare(control.currentIndex, 3)
        compare(control.currentText, "Apple")
        compare(control.highlightedIndex, -1)

        keyPress(Qt.Key_B)
        compare(control.currentIndex, 0)
        compare(control.currentText, "Banana")
        compare(control.highlightedIndex, -1)

        // popup
        control.popup.open()
        tryCompare(control.popup, "opened", true)

        compare(control.currentIndex, 0)
        compare(control.highlightedIndex, 0)

        keyClick(Qt.Key_C)
        compare(control.highlightedIndex, 1) // "Coco"
        compare(control.currentIndex, 0)

        // no match
        keyClick(Qt.Key_N)
        compare(control.highlightedIndex, 1)
        compare(control.currentIndex, 0)

        keyClick(Qt.Key_C)
        compare(control.highlightedIndex, 2) // "Coconut"
        compare(control.currentIndex, 0)

        keyClick(Qt.Key_C)
        compare(control.highlightedIndex, 4) // "Cocomuffin"
        compare(control.currentIndex, 0)

        // wrap
        keyClick(Qt.Key_C)
        compare(control.highlightedIndex, 1) // "Coco"
        compare(control.currentIndex, 0)

        keyClick(Qt.Key_B)
        compare(control.highlightedIndex, 0) // "Banana"
        compare(control.currentIndex, 0)

        keyClick(Qt.Key_A)
        compare(control.highlightedIndex, 3) // "Apple"
        compare(control.currentIndex, 0)

        verify(control.popup.visible)

        // accept
        keyClick(Qt.Key_Return)
        tryCompare(control.popup, "visible", false)
        compare(control.currentIndex, 3)
        compare(control.currentText, "Apple")
        compare(control.highlightedIndex, -1)
    }

    function test_popup() {
        var control = createTemporaryObject(comboBox, testCase, {model: 3})
        verify(control)

        // show below
        mousePress(control)
        compare(control.pressed, true)
        compare(control.popup.visible, false)
        mouseRelease(control)
        compare(control.pressed, false)
        compare(control.popup.visible, true)
        verify(control.popup.contentItem.y >= control.y)

        // hide
        mouseClick(control)
        compare(control.pressed, false)
        tryCompare(control.popup, "visible", false)

        // show above
        control.y = control.Window.height - control.height
        mousePress(control)
        compare(control.pressed, true)
        compare(control.popup.visible, false)
        mouseRelease(control)
        compare(control.pressed, false)
        compare(control.popup.visible, true)
        verify(control.popup.contentItem.y < control.y)


        // Account for when a transition of a scale from 0.9-1.0 that it is placed above right away and not below
        // first just because there is room at the 0.9 scale
        if (control.popup.enter !== null) {
            // hide
            mouseClick(control)
            compare(control.pressed, false)
            tryCompare(control.popup, "visible", false)
            control.y = control.Window.height - (control.popup.contentItem.height * 0.99)
            var popupYSpy = createTemporaryObject(signalSpy, testCase, {target: control.popup, signalName: "yChanged"})
            verify(popupYSpy.valid)
            mousePress(control)
            compare(control.pressed, true)
            compare(control.popup.visible, false)
            mouseRelease(control)
            compare(control.pressed, false)
            compare(control.popup.visible, true)
            tryCompare(control.popup.enter, "running", false)
            verify(control.popup.contentItem.y < control.y)
            verify(popupYSpy.count === 1)
        }

        var leftLayoutMargin = control.background.layoutMargins === undefined ? 0 : control.popup.layoutMargins.left
        // follow the control outside the horizontal window bounds
        control.x = -control.width / 2
        compare(control.x, -control.width / 2)
        compare(control.popup.contentItem.parent.x, -control.width / 2 + leftLayoutMargin)
        control.x = testCase.width - control.width / 2
        compare(control.x, testCase.width - control.width / 2)
        compare(control.popup.contentItem.parent.x, testCase.width - control.width / 2 + leftLayoutMargin)

        // close the popup when hidden (QTBUG-67684)
        control.popup.open()
        tryCompare(control.popup, "opened", true)
        control.visible = false
        tryCompare(control.popup, "visible", false)
    }

    Component {
        id: reopenCombo
        Window {
            property alias innerCombo: innerCombo
            visible: true
            width: 300
            height: 300
            ComboBox {
                id: innerCombo
                model: 10
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    // This test checks that when reopening the combobox that it is still appears at the same y position as
    // previously
    function test_reopen_popup() {
        var control = createTemporaryObject(reopenCombo, testCase)
        verify(control)
        var y = 0;
        for (var i = 0; i < 2; ++i) {
            tryCompare(control.innerCombo.popup, "visible", false)
            control.innerCombo.y = control.height - (control.innerCombo.popup.contentItem.height * 0.99)
            var popupYSpy = createTemporaryObject(signalSpy, testCase, {target: control.innerCombo.popup, signalName: "yChanged"})
            verify(popupYSpy.valid)
            mousePress(control.innerCombo)
            compare(control.innerCombo.pressed, true)
            compare(control.innerCombo.popup.visible, false)
            mouseRelease(control.innerCombo)
            compare(control.innerCombo.pressed, false)
            compare(control.innerCombo.popup.visible, true)
            if (control.innerCombo.popup.enter)
                tryCompare(control.innerCombo.popup.enter, "running", false)
            // Check on the second opening that it has the same y position as before
            if (i !== 0) {
                // y should not have changed again
                verify(popupYSpy.count === 0)
                verify(y === control.innerCombo.popup.y)
            } else {
                // In some cases on the initial show, y changes more than once
                verify(popupYSpy.count >= 1)
                y = control.innerCombo.popup.y
                mouseClick(control.innerCombo)
                compare(control.innerCombo.pressed, false)
                tryCompare(control.innerCombo.popup, "visible", false)
            }
        }
    }

    function test_mouse() {
        var control = createTemporaryObject(comboBox, testCase, {model: 3, hoverEnabled: false})
        verify(control)

        var activatedSpy = signalSpy.createObject(control, {target: control, signalName: "activated"})
        verify(activatedSpy.valid)

        mouseClick(control)
        compare(control.popup.visible, true)

        var content = control.popup.contentItem
        waitForRendering(content)

        // press - move - release outside - not activated - not closed
        mousePress(content)
        compare(activatedSpy.count, 0)
        mouseMove(content, content.width * 2)
        compare(activatedSpy.count, 0)
        mouseRelease(content, content.width * 2)
        compare(activatedSpy.count, 0)
        compare(control.popup.visible, true)

        // press - move - release inside - activated - closed
        mousePress(content)
        compare(activatedSpy.count, 0)
        mouseMove(content, content.width / 2 + 1, content.height / 2 + 1)
        compare(activatedSpy.count, 0)
        mouseRelease(content)
        compare(activatedSpy.count, 1)
        tryCompare(control.popup, "visible", false)
    }

    function test_touch() {
        var control = createTemporaryObject(comboBox, testCase, {model: 3})
        verify(control)

        var touch = touchEvent(control)

        var activatedSpy = signalSpy.createObject(control, {target: control, signalName: "activated"})
        verify(activatedSpy.valid)

        var highlightedSpy = signalSpy.createObject(control, {target: control, signalName: "highlighted"})
        verify(highlightedSpy.valid)

        touch.press(0, control).commit()
        touch.release(0, control).commit()
        compare(control.popup.visible, true)

        var content = control.popup.contentItem
        waitForRendering(content)

        // press - move - release outside - not activated - not closed
        touch.press(0, control).commit()
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 0)
        touch.move(0, control, control.width * 2, control.height / 2).commit()
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 0)
        touch.release(0, control, control.width * 2, control.height / 2).commit()
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 0)
        compare(control.popup.visible, true)

        // press - move - release inside - activated - closed
        touch.press(0, content).commit()
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 0)
        touch.move(0, content, content.width / 2 + 1, content.height / 2 + 1).commit()
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 0)
        touch.release(0, content).commit()
        compare(activatedSpy.count, 1)
        compare(highlightedSpy.count, 1)
        tryCompare(control.popup, "visible", false)
    }

    function test_down() {
        var control = createTemporaryObject(comboBox, testCase, {model: 3})
        verify(control)

        // some styles position the popup over the combo button. move it out
        // of the way to avoid stealing mouse presses. we want to test the
        // combinations of the button being pressed and the popup being visible.
        control.popup.y = control.height

        var downSpy = signalSpy.createObject(control, {target: control, signalName: "downChanged"})
        verify(downSpy.valid)

        var pressedSpy = signalSpy.createObject(control, {target: control, signalName: "pressedChanged"})
        verify(pressedSpy.valid)

        mousePress(control)
        compare(control.popup.visible, false)
        compare(control.pressed, true)
        compare(control.down, true)
        compare(downSpy.count, 1)
        compare(pressedSpy.count, 1)

        mouseRelease(control)
        compare(control.popup.visible, true)
        compare(control.pressed, false)
        compare(control.down, true)
        compare(downSpy.count, 3)
        compare(pressedSpy.count, 2)

        compare(control.popup.y, control.height)

        control.down = false
        compare(control.down, false)
        compare(downSpy.count, 4)

        mousePress(control)
        compare(control.popup.visible, true)
        compare(control.pressed, true)
        compare(control.down, false) // explicit false
        compare(downSpy.count, 4)
        compare(pressedSpy.count, 3)

        control.down = undefined
        compare(control.down, true)
        compare(downSpy.count, 5)

        mouseRelease(control)
        tryCompare(control.popup, "visible", false)
        compare(control.pressed, false)
        compare(control.down, false)
        compare(downSpy.count, 6)
        compare(pressedSpy.count, 4)

        control.popup.open()
        compare(control.popup.visible, true)
        compare(control.pressed, false)
        compare(control.down, true)
        compare(downSpy.count, 7)
        compare(pressedSpy.count, 4)

        control.popup.close()
        tryCompare(control.popup, "visible", false)
        compare(control.pressed, false)
        compare(control.down, false)
        compare(downSpy.count, 8)
        compare(pressedSpy.count, 4)
    }

    function test_focus() {
        var control = createTemporaryObject(comboBox, testCase, {model: 3})
        verify(control)

        var openedSpy = signalSpy.createObject(control, {target: control.popup, signalName: "opened"})
        verify(openedSpy.valid)

        var closedSpy = signalSpy.createObject(control, {target: control.popup, signalName: "closed"})
        verify(openedSpy.valid)

        // click - gain focus - show popup
        mouseClick(control)
        verify(control.activeFocus)
        openedSpy.wait()
        compare(openedSpy.count, 1)
        compare(control.popup.visible, true)

        // lose focus - hide popup
        control.focus = false
        verify(!control.activeFocus)
        closedSpy.wait()
        compare(closedSpy.count, 1)
        compare(control.popup.visible, false)
    }

    function test_baseline() {
        var control = createTemporaryObject(comboBox, testCase)
        verify(control)
        compare(control.baselineOffset, control.contentItem.y + control.contentItem.baselineOffset)
    }

    Component {
        id: displayBox
        ComboBox {
            textRole: "key"
            model: ListModel {
                ListElement { key: "First"; value: 123 }
                ListElement { key: "Second"; value: 456 }
                ListElement { key: "Third"; value: 789 }
            }
        }
    }

    function test_displayText() {
        var control = createTemporaryObject(displayBox, testCase)
        verify(control)

        compare(control.displayText, "First")
        control.currentIndex = 1
        compare(control.displayText, "Second")
        control.textRole = "value"
        compare(control.displayText, "456")
        control.displayText = "Display"
        compare(control.displayText, "Display")
        control.currentIndex = 2
        compare(control.displayText, "Display")
        control.displayText = undefined
        compare(control.displayText, "789")
    }

    Component {
        id: component
        Pane {
            id: panel
            property alias button: _button;
            property alias combobox: _combobox;
            font.pixelSize: 30
            Column {
                Button {
                    id: _button
                    text: "Button"
                    font.pixelSize: 20
                }
                ComboBox {
                    id: _combobox
                    model: ["ComboBox", "With"]
                    delegate: ItemDelegate {
                        width: _combobox.width
                        text: _combobox.textRole ? (Array.isArray(_combobox.model) ? modelData[_combobox.textRole] : model[_combobox.textRole]) : modelData
                        objectName: "delegate"
                        autoExclusive: true
                        checked: _combobox.currentIndex === index
                        highlighted: _combobox.highlightedIndex === index
                    }
                }
            }
        }
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

    function test_font() { // QTBUG_50984, QTBUG-51696
        var control = createTemporaryObject(component, testCase)
        verify(control)
        verify(control.button)
        verify(control.combobox)

        var expectedComboBoxFontPixelSize = 30
        compare(control.font.pixelSize, 30)
        compare(control.button.font.pixelSize, 20)
        compare(control.combobox.font.pixelSize, expectedComboBoxFontPixelSize)

//        verify(control.combobox.popup)
//        var popup = control.combobox.popup
//        popup.open()

//        verify(popup.contentItem)

//        var listview = popup.contentItem
//        verify(listview.contentItem)
//        waitForRendering(listview)

//        var idx1 = getChild(listview.contentItem, "delegate", -1)
//        compare(listview.contentItem.children[idx1].font.pixelSize, 25)
//        var idx2 = getChild(listview.contentItem, "delegate", idx1)
//        compare(listview.contentItem.children[idx2].font.pixelSize, 25)

//        compare(listview.contentItem.children[idx1].font.pixelSize, 25)
//        compare(listview.contentItem.children[idx2].font.pixelSize, 25)

        control.font.pixelSize = control.font.pixelSize + 10
        expectedComboBoxFontPixelSize += 10
        compare(control.combobox.font.pixelSize, expectedComboBoxFontPixelSize)
//        waitForRendering(listview)
//        compare(listview.contentItem.children[idx1].font.pixelSize, 25)
//        compare(listview.contentItem.children[idx2].font.pixelSize, 25)

        control.combobox.font.pixelSize = control.combobox.font.pixelSize + 5
        compare(control.combobox.font.pixelSize, 45)
//        waitForRendering(listview)

//        idx1 = getChild(listview.contentItem, "delegate", -1)
//        compare(listview.contentItem.children[idx1].font.pixelSize, 25)
//        idx2 = getChild(listview.contentItem, "delegate", idx1)
//        compare(listview.contentItem.children[idx2].font.pixelSize, 25)
    }

    function test_wheel() {
        var ma = createTemporaryObject(mouseArea, testCase, {width: 100, height: 100})
        verify(ma)

        var control = comboBox.createObject(ma, {model: 2, wheelEnabled: true})
        verify(control)

        var delta = 120

        var spy = signalSpy.createObject(ma, {target: ma, signalName: "wheel"})
        verify(spy.valid)

        mouseWheel(control, control.width / 2, control.height / 2, -delta, -delta)
        compare(control.currentIndex, 1)
        compare(spy.count, 0) // no propagation

        // reached bounds -> no change
        mouseWheel(control, control.width / 2, control.height / 2, -delta, -delta)
        compare(control.currentIndex, 1)
        compare(spy.count, 0) // no propagation

        mouseWheel(control, control.width / 2, control.height / 2, delta, delta)
        compare(control.currentIndex, 0)
        compare(spy.count, 0) // no propagation

        // reached bounds -> no change
        mouseWheel(control, control.width / 2, control.height / 2, delta, delta)
        compare(control.currentIndex, 0)
        compare(spy.count, 0) // no propagation
    }

    function test_activation_data() {
        return [
            { tag: "open:enter", key: Qt.Key_Enter, open: true },
            { tag: "open:return", key: Qt.Key_Return, open: true },
            { tag: "closed:enter", key: Qt.Key_Enter, open: false },
            { tag: "closed:return", key: Qt.Key_Return, open: false }
        ]
    }

    // QTBUG-51645
    function test_activation(data) {
        var control = createTemporaryObject(comboBox, testCase, {currentIndex: 1, model: ["Apple", "Orange", "Banana"]})
        verify(control)

        control.forceActiveFocus()
        verify(control.activeFocus)

        if (data.open) {
            var openedSpy = signalSpy.createObject(control, {target: control.popup, signalName: "opened"})
            verify(openedSpy.valid)

            keyClick(Qt.Key_Space)
            openedSpy.wait()
            compare(openedSpy.count, 1)
        }
        compare(control.popup.visible, data.open)

        compare(control.currentIndex, 1)
        compare(control.currentText, "Orange")
        compare(control.displayText, "Orange")

        keyClick(data.key)

        compare(control.currentIndex, 1)
        compare(control.currentText, "Orange")
        compare(control.displayText, "Orange")
    }

    Component {
        id: asyncLoader
        Loader {
            active: false
            asynchronous: true
            sourceComponent: ComboBox {
                model: ["First", "Second", "Third"]
            }
        }
    }

    // QTBUG-51972
    function test_async() {
        var loader = createTemporaryObject(asyncLoader, testCase)
        verify(loader)

        loader.active = true
        tryCompare(loader, "status", Loader.Ready)
        verify(loader.item)
        compare(loader.item.currentText, "First")
        compare(loader.item.displayText, "First")
    }

    // QTBUG-52615
    function test_currentIndex() {
        var control = createTemporaryObject(comboBox, testCase, {currentIndex: -1, model: 3})
        verify(control)

        compare(control.currentIndex, -1)
    }

    ListModel {
        id: resetmodel
        ListElement { text: "First" }
        ListElement { text: "Second" }
        ListElement { text: "Third" }
    }

    // QTBUG-54573
    function test_modelReset() {
        var control = createTemporaryObject(comboBox, testCase, {model: resetmodel})
        verify(control)
        control.popup.open()

        var listview = control.popup.contentItem
        verify(listview)

        tryCompare(listview.contentItem.children, "length", resetmodel.count + 1) // + highlight item

        resetmodel.clear()
        resetmodel.append({text: "Fourth"})
        resetmodel.append({text: "Fifth"})

        tryCompare(listview.contentItem.children, "length", resetmodel.count + 1) // + highlight item
    }

    // QTBUG-55118
    function test_currentText() {
        var control = createTemporaryObject(comboBox, testCase, {model: listmodel})
        verify(control)

        compare(control.currentIndex, 0)
        compare(control.currentText, "First")

        listmodel.setProperty(0, "text", "1st")
        compare(control.currentText, "1st")

        control.currentIndex = 1
        compare(control.currentText, "Second")

        listmodel.setProperty(0, "text", "First")
        compare(control.currentText, "Second")
    }

    // QTBUG-55030
    function test_highlightRange() {
        var control = createTemporaryObject(comboBox, testCase, {model: 100})
        verify(control)

        control.currentIndex = 50
        compare(control.currentIndex, 50)
        compare(control.highlightedIndex, -1)

        var openedSpy = signalSpy.createObject(control, {target: control.popup, signalName: "opened"})
        verify(openedSpy.valid)

        control.popup.open()
        compare(control.highlightedIndex, 50)
        tryCompare(openedSpy, "count", 1)

        var listview = control.popup.contentItem
        verify(listview)

        var first = listview.itemAt(0, listview.contentY)
        verify(first)
        compare(first.text, "50")

        var closedSpy = signalSpy.createObject(control, {target: control.popup, signalName: "closed"})
        verify(closedSpy.valid)

        control.popup.close()
        tryCompare(closedSpy, "count", 1)
        compare(control.highlightedIndex, -1)

        control.currentIndex = 99
        compare(control.currentIndex, 99)
        compare(control.highlightedIndex, -1)

        control.popup.open()
        compare(control.highlightedIndex, 99)
        tryCompare(openedSpy, "count", 2)
        tryVerify(function() { return listview.height > 0 })

        var last = listview.itemAt(0, listview.contentY + listview.height - 1)
        verify(last)
        compare(last.text, "99")

        openedSpy.target = null
        closedSpy.target = null
    }

    function test_mouseHighlight() {
        if ((Qt.platform.pluginName === "offscreen")
            || (Qt.platform.pluginName === "minimal"))
            skip("Mouse highlight not functional on offscreen/minimal platforms")
        var control = createTemporaryObject(comboBox, testCase, {model: 20})
        verify(control)

        compare(control.highlightedIndex, -1)

        var openedSpy = signalSpy.createObject(control, {target: control.popup, signalName: "opened"})
        verify(openedSpy.valid)

        control.popup.open()
        compare(control.highlightedIndex, 0)
        tryCompare(openedSpy, "count", 1)

        var listview = control.popup.contentItem
        verify(listview)
        waitForRendering(listview)

        // hover-highlight through all visible list items one by one
        var hoverIndex = -1
        var prevHoverItem = null
        for (var y = 0; y < listview.height; ++y) {
            var hoverItem = listview.itemAt(0, listview.contentY + y)
            if (!hoverItem || !hoverItem.visible || hoverItem === prevHoverItem)
                continue
            mouseMove(hoverItem, 0, 0)
            tryCompare(control, "highlightedIndex", ++hoverIndex)
            prevHoverItem = hoverItem
        }

        mouseMove(listview, listview.width / 2, listview.height / 2)

        // wheel-highlight the rest of the items
        var delta = 120
        var prevWheelItem = null
        while (!listview.atYEnd) {
            var prevContentY = listview.contentY
            mouseWheel(listview, listview.width / 2, listview.height / 2, -delta, -delta)
            tryCompare(listview, "moving", false)
            verify(listview.contentY > prevContentY)

            var wheelItem = listview.itemAt(listview.width / 2, listview.contentY + listview.height / 2)
            if (!wheelItem || !wheelItem.visible || wheelItem === prevWheelItem)
                continue

            tryCompare(control, "highlightedIndex", parseInt(wheelItem.text))
            prevWheelItem = wheelItem
        }
    }

    RegularExpressionValidator {
        id: regExpValidator
        regularExpression: /(red|blue|green)?/
    }

    function test_validator() {
        var control = createTemporaryObject(comboBox, testCase, {editable: true, validator: regExpValidator})

        control.editText = "blu"
        compare(control.acceptableInput, false)
        control.editText = "blue"
        compare(control.acceptableInput, true)
        control.editText = "bluee"
        compare(control.acceptableInput, false)
        control.editText = ""
        compare(control.acceptableInput, true)
        control.editText = ""
        control.contentItem.forceActiveFocus()
        keyPress(Qt.Key_A)
        compare(control.editText, "")
        keyPress(Qt.Key_A)
        compare(control.editText, "")
        keyPress(Qt.Key_R)
        compare(control.editText, "r")
        keyPress(Qt.Key_A)
        compare(control.editText, "r")
        compare(control.acceptableInput, false)
        keyPress(Qt.Key_E)
        compare(control.editText, "re")
        compare(control.acceptableInput, false)
        keyPress(Qt.Key_D)
        compare(control.editText, "red")
        compare(control.acceptableInput, true)
    }

    Component {
        id: appendFindBox
        ComboBox {
            editable: true
            model: ListModel {
                ListElement { text:"first" }
            }
            onAccepted: {
                if (find(editText) === -1)
                    model.append({text: editText})
            }
        }
    }

    function test_append_find() {
        var control = createTemporaryObject(appendFindBox, testCase)

        compare(control.currentIndex, 0)
        compare(control.currentText, "first")
        control.contentItem.forceActiveFocus()
        compare(control.activeFocus, true)

        control.selectAll()
        keyPress(Qt.Key_T)
        keyPress(Qt.Key_H)
        keyPress(Qt.Key_I)
        keyPress(Qt.Key_R)
        keyPress(Qt.Key_D)
        compare(control.count, 1)
        compare(control.currentText, "first")
        compare(control.editText, "third")

        keyPress(Qt.Key_Enter)
        compare(control.count, 2)
        compare(control.currentIndex, 1)
        compare(control.currentText, "third")
    }

    function test_editable() {
        var control = createTemporaryObject(comboBox, testCase, {editable: true, model: ["Banana", "Coco", "Coconut", "Apple", "Cocomuffin"]})
        verify(control)

        control.contentItem.forceActiveFocus()
        verify(control.activeFocus)

        var acceptCount = 0

        var acceptSpy = signalSpy.createObject(control, {target: control, signalName: "accepted"})
        verify(acceptSpy.valid)

        compare(control.editText, "Banana")
        compare(control.currentText, "Banana")
        compare(control.currentIndex, 0)
        compare(acceptSpy.count, 0)
        control.editText = ""

        keyPress(Qt.Key_C)
        compare(control.editText, "coco")
        compare(control.currentText, "Banana")
        compare(control.currentIndex, 0)

        keyPress(Qt.Key_Right)
        keyPress(Qt.Key_N)
        compare(control.editText, "coconut")
        compare(control.currentText, "Banana")
        compare(control.currentIndex, 0)

        keyPress(Qt.Key_Enter) // Accept
        compare(control.editText, "Coconut")
        compare(control.currentText, "Coconut")
        compare(control.currentIndex, 2)
        compare(acceptSpy.count, ++acceptCount)

        keyPress(Qt.Key_Backspace)
        keyPress(Qt.Key_Backspace)
        keyPress(Qt.Key_Backspace)
        keyPress(Qt.Key_M)
        compare(control.editText, "Cocomuffin")
        compare(control.currentText, "Coconut")
        compare(control.currentIndex, 2)

        keyPress(Qt.Key_Enter) // Accept
        compare(control.editText, "Cocomuffin")
        compare(control.currentText, "Cocomuffin")
        compare(control.currentIndex, 4)
        compare(acceptSpy.count, ++acceptCount)

        keyPress(Qt.Key_Return) // Accept
        compare(control.editText, "Cocomuffin")
        compare(control.currentText, "Cocomuffin")
        compare(control.currentIndex, 4)
        compare(acceptSpy.count, ++acceptCount)

        control.editText = ""
        compare(control.editText, "")
        compare(control.currentText, "Cocomuffin")
        compare(control.currentIndex, 4)

        keyPress(Qt.Key_A)
        compare(control.editText, "apple")
        compare(control.currentText, "Cocomuffin")
        compare(control.currentIndex, 4)

        keyPress(Qt.Key_Return) // Accept
        compare(control.editText, "Apple")
        compare(control.currentText, "Apple")
        compare(control.currentIndex, 3)
        compare(acceptSpy.count, ++acceptCount)

        control.editText = ""
        keyPress(Qt.Key_A)
        keyPress(Qt.Key_B)
        compare(control.editText, "ab")
        compare(control.currentText, "Apple")
        compare(control.currentIndex, 3)

        keyPress(Qt.Key_Return) // Accept
        compare(control.editText, "ab")
        compare(control.currentText, "")
        compare(control.currentIndex, -1)
        compare(acceptSpy.count, ++acceptCount)

        control.editText = ""
        compare(control.editText, "")
        compare(control.currentText, "")
        compare(control.currentIndex, -1)

        keyPress(Qt.Key_C)
        keyPress(Qt.Key_Return) // Accept
        compare(control.editText, "Coco")
        compare(control.currentText, "Coco")
        compare(control.currentIndex, 1)
        compare(acceptSpy.count, ++acceptCount)

        keyPress(Qt.Key_Down)
        compare(control.editText, "Coconut")
        compare(control.currentText, "Coconut")
        compare(control.currentIndex, 2)

        keyPress(Qt.Key_Up)
        compare(control.editText, "Coco")
        compare(control.currentText, "Coco")
        compare(control.currentIndex, 1)

        control.editText = ""
        compare(control.editText, "")
        compare(control.currentText, "Coco")
        compare(control.currentIndex, 1)

        keyPress(Qt.Key_C)
        keyPress(Qt.Key_O)
        keyPress(Qt.Key_C) // autocompletes "coco"
        keyPress(Qt.Key_Backspace)
        keyPress(Qt.Key_Return) // Accept "coc"
        compare(control.editText, "coc")
        compare(control.currentText, "")
        compare(control.currentIndex, -1)
        compare(acceptSpy.count, ++acceptCount)

        control.editText = ""
        compare(control.editText, "")
        compare(control.currentText, "")
        compare(control.currentIndex, -1)

        keyPress(Qt.Key_C)
        keyPress(Qt.Key_O)
        keyPress(Qt.Key_C) // autocompletes "coc"
        keyPress(Qt.Key_Space)
        keyPress(Qt.Key_Return) // Accept "coc "
        compare(control.editText, "coc ")
        compare(control.currentText, "")
        compare(control.currentIndex, -1)
        compare(acceptSpy.count, ++acceptCount)
    }

    Component {
        id: keysAttachedBox
        ComboBox {
            editable: true
            property bool gotit: false
            Keys.onPressed: function (event) {
                if (!gotit && event.key === Qt.Key_B) {
                    gotit = true
                    event.accepted = true
                }
            }
        }
    }

    function test_keys_attached() {
        var control = createTemporaryObject(keysAttachedBox, testCase)
        verify(control)

        control.contentItem.forceActiveFocus()
        verify(control.activeFocus)

        verify(!control.gotit)
        compare(control.editText, "")

        keyPress(Qt.Key_A)
        verify(control.activeFocus)
        verify(!control.gotit)
        compare(control.editText, "a")

        keyPress(Qt.Key_B)
        verify(control.activeFocus)
        verify(control.gotit)
        compare(control.editText, "a")

        keyPress(Qt.Key_B)
        verify(control.activeFocus)
        verify(control.gotit)
        compare(control.editText, "ab")
    }

    function test_minusOneIndexResetsSelection_QTBUG_35794_data() {
        return [
            { tag: "editable", editable: true },
            { tag: "non-editable", editable: false }
        ]
    }

    function test_minusOneIndexResetsSelection_QTBUG_35794(data) {
        var control = createTemporaryObject(comboBox, testCase, {editable: data.editable, model: ["A", "B", "C"]})
        verify(control)

        compare(control.currentIndex, 0)
        compare(control.currentText, "A")
        control.currentIndex = -1
        compare(control.currentIndex, -1)
        compare(control.currentText, "")
        control.currentIndex = 1
        compare(control.currentIndex, 1)
        compare(control.currentText, "B")
    }

    function test_minusOneToZeroSelection_QTBUG_38036() {
        var control = createTemporaryObject(comboBox, testCase, {model: ["A", "B", "C"]})
        verify(control)

        compare(control.currentIndex, 0)
        compare(control.currentText, "A")
        control.currentIndex = -1
        compare(control.currentIndex, -1)
        compare(control.currentText, "")
        control.currentIndex = 0
        compare(control.currentIndex, 0)
        compare(control.currentText, "A")
    }

    function test_emptyPopupAfterModelCleared() {
        var control = createTemporaryObject(comboBox, testCase, { model: 1 })
        verify(control)
        compare(control.popup.implicitHeight, 0)

        // Ensure that it's open so that the popup's implicitHeight changes when we increase the model count.
        control.popup.open()
        tryCompare(control.popup, "visible", true)

        // Add lots of items to the model. The popup should take up the entire height of the window.
        control.model = 100
        compare(control.popup.height, control.Window.height - control.popup.topMargin - control.popup.bottomMargin)

        control.popup.close()

        // Clearing the model should result in a zero height.
        control.model = 0
        control.popup.open()
        tryCompare(control.popup, "visible", true)
        compare(control.popup.height, control.popup.topPadding + control.popup.bottomPadding)
    }

    Component {
        id: keysMonitor
        Item {
            property int pressedKeys: 0
            property int releasedKeys: 0
            property int lastPressedKey: 0
            property int lastReleasedKey: 0
            property alias comboBox: comboBox

            width: 200
            height: 200

            Keys.onPressed: (event) => { ++pressedKeys; lastPressedKey = event.key }
            Keys.onReleased: (event) => { ++releasedKeys; lastReleasedKey = event.key }

            ComboBox {
                id: comboBox
            }
        }
    }

    function test_keyClose_data() {
        return [
            { tag: "Escape", key: Qt.Key_Escape },
            { tag: "Back", key: Qt.Key_Back }
        ]
    }

    function test_keyClose(data) {
        var container = createTemporaryObject(keysMonitor, testCase)
        verify(container)

        var control = comboBox.createObject(container)
        verify(control)

        control.forceActiveFocus()
        verify(control.activeFocus)

        var pressedKeys = 0
        var releasedKeys = 0

        // popup not visible -> propagates
        keyPress(data.key)
        compare(container.pressedKeys, ++pressedKeys)
        compare(container.lastPressedKey, data.key)

        keyRelease(data.key)
        compare(container.releasedKeys, ++releasedKeys)
        compare(container.lastReleasedKey, data.key)

        verify(control.activeFocus)

        // popup visible -> handled -> does not propagate
        control.popup.open()
        tryCompare(control.popup, "opened", true)

        keyPress(data.key)
        compare(container.pressedKeys, pressedKeys)

        keyRelease(data.key)
        compare(container.releasedKeys, releasedKeys)

        tryCompare(control.popup, "visible", false)
        verify(control.activeFocus)

        // popup not visible -> propagates
        keyPress(data.key)
        compare(container.pressedKeys, ++pressedKeys)
        compare(container.lastPressedKey, data.key)

        keyRelease(data.key)
        compare(container.releasedKeys, ++releasedKeys)
        compare(container.lastReleasedKey, data.key)
    }

    function test_popupFocus_QTBUG_74661() {
        var control = createTemporaryObject(comboBox, testCase)
        verify(control)

        var popup = createTemporaryObject(customPopup, testCase)
        verify(popup)

        control.popup = popup

        var openedSpy = signalSpy.createObject(control, {target: popup, signalName: "opened"})
        verify(openedSpy.valid)

        var closedSpy = signalSpy.createObject(control, {target: popup, signalName: "closed"})
        verify(closedSpy.valid)

        control.forceActiveFocus()
        verify(control.activeFocus)

        // show popup
        keyClick(Qt.Key_Space)
        openedSpy.wait()
        compare(openedSpy.count, 1)

        popup.contentItem.forceActiveFocus()
        verify(popup.contentItem.activeFocus)

        // type something in the text field
        keyClick(Qt.Key_Space)
        keyClick(Qt.Key_H)
        keyClick(Qt.Key_I)
        compare(popup.contentItem.text, " hi")

        compare(closedSpy.count, 0)

        // hide popup
        keyClick(Qt.Key_Escape)
        closedSpy.wait()
        compare(closedSpy.count, 1)
    }

    function test_comboBoxWithShaderEffect() {
        var control = createTemporaryObject(comboBoxWithShaderEffect, testCase, {model: 9})
        verify(control)
        waitForRendering(control)
        control.forceActiveFocus()
        var openedSpy = signalSpy.createObject(control, {target: control.popup, signalName: "opened"})
        verify(openedSpy.valid)

        var closedSpy = signalSpy.createObject(control, {target: control.popup, signalName: "closed"})
        verify(closedSpy.valid)

        control.popup.open()
        openedSpy.wait()
        compare(openedSpy.count, 1)
        control.popup.close()
        closedSpy.wait()
        compare(closedSpy.count, 1)
    }

    function test_comboBoxSelectTextByMouse() {
        let control = createTemporaryObject(comboBox, testCase,
            { editable: true, selectTextByMouse: true, model: [ "Some text" ], width: parent.width })
        verify(control)
        waitForRendering(control)
        control.forceActiveFocus()

        // Position the text cursor at the beginning of the text.
        mouseClick(control, control.leftPadding, control.height / 2)
        // Select all of the text.
        mousePress(control, control.leftPadding, control.height / 2)
        mouseMove(control, control.leftPadding + control.contentItem.width, control.height / 2)
        mouseRelease(control, control.leftPadding + control.contentItem.width, control.height / 2)
        compare(control.contentItem.selectedText, "Some text")
    }

    // QTBUG-78885: When the edit text is changed on an editable ComboBox,
    // and then that ComboBox loses focus, its currentIndex should change
    // to the index of the edit text (assuming a match is found).
    function test_currentIndexChangeOnLostFocus() {
        if (Qt.styleHints.tabFocusBehavior !== Qt.TabFocusAllControls)
            skip("This platform only allows tab focus for text controls")

        let theModel = []
        for (let i = 0; i < 10; ++i)
            theModel.push("Item " + (i + 1))

        let comboBox1 = createTemporaryObject(comboBox, testCase,
            { objectName: "comboBox1", editable: true, model: theModel })
        verify(comboBox1)
        compare(comboBox1.currentIndex, 0)

        let comboBox2 = createTemporaryObject(comboBox, testCase, { objectName: "comboBox2" })
        verify(comboBox2)

        // Give the first ComboBox focus and type in 0 to select "Item 10" (default is "Item 1").
        waitForRendering(comboBox1)
        comboBox1.contentItem.forceActiveFocus()
        verify(comboBox1.activeFocus)
        keyClick(Qt.Key_0)
        compare(comboBox1.editText, "Item 10")

        let currentIndexSpy = signalSpy.createObject(comboBox1,
            { target: comboBox1, signalName: "currentIndexChanged" })
        verify(currentIndexSpy.valid)

        // Give focus to the other ComboBox so that the first one loses it.
        // The first ComboBox's currentIndex should change to that of "Item 10".
        keyClick(Qt.Key_Tab)
        verify(comboBox2.activeFocus)
        compare(comboBox1.currentIndex, 9)
        compare(currentIndexSpy.count, 1)

        // Give focus back to the first ComboBox, and try the same thing except
        // with non-existing text; the currentIndex should not change.
        comboBox1.contentItem.forceActiveFocus()
        verify(comboBox1.activeFocus)
        keySequence(StandardKey.SelectAll)
        compare(comboBox1.contentItem.selectedText, "Item 10")
        keyClick(Qt.Key_N)
        keyClick(Qt.Key_O)
        keyClick(Qt.Key_P)
        keyClick(Qt.Key_E)
        compare(comboBox1.editText, "nope")
        compare(comboBox1.currentIndex, 9)
        compare(currentIndexSpy.count, 1)
    }

    Component {
        id: appFontTextFieldComponent
        TextField {
            objectName: "appFontTextField"
            font: Qt.application.font
            // We don't want the background's implicit width to interfere with our tests,
            // which are about implicit width of the contentItem of ComboBox, which is by default TextField.
            background: null
        }
    }

    Component {
        id: appFontContentItemComboBoxComponent
        ComboBox {
            // Override the contentItem so that the font doesn't vary between styles.
            contentItem: TextField {
                objectName: "appFontContentItemTextField"
                // We do this just to be extra sure that the font never comes from the control,
                // as we want it to match that of the TextField in the appFontTextFieldComponent.
                font: Qt.application.font
                background: null
            }
        }
    }

    Component {
        id: twoItemListModelComponent

        ListModel {
            ListElement { display: "Short" }
            ListElement { display: "Kinda long" }
        }
    }

    function appendedToModel(model, item) {
        if (Array.isArray(model)) {
            let newModel = model
            newModel.push(item)
            return newModel
        }

        if (model.hasOwnProperty("append")) {
            model.append({ display: item })
            // To account for the fact that changes to a JS array are not seen by the QML engine,
            // we need to reassign the entire model and hence return it. For simplicity in the
            // calling code, we do it for the ListModel code path too. It should be a no-op.
            return model
        }

        console.warn("appendedToModel: unrecognised model")
        return undefined
    }

    function removedFromModel(model, index, count) {
        if (Array.isArray(model)) {
            let newModel = model
            newModel.splice(index, count)
            return newModel
        }

        if (model.hasOwnProperty("remove")) {
            model.remove(index, count)
            return model
        }

        console.warn("removedFromModel: unrecognised model")
        return undefined
    }

    // We don't use a data-driven test for the policy  because the checks vary a lot based on which enum we're testing.
    function test_implicitContentWidthPolicy_ContentItemImplicitWidth() {
        // Set ContentItemImplicitWidth and ensure that implicitContentWidth is as wide as the current item
        // by comparing it against the implicitWidth of an identical TextField
        let control = createTemporaryObject(appFontContentItemComboBoxComponent, testCase, {
            model: ["Short", "Kinda long"],
            implicitContentWidthPolicy: ComboBox.ContentItemImplicitWidth
        })
        verify(control)
        compare(control.implicitContentWidthPolicy, ComboBox.ContentItemImplicitWidth)

        let textField = createTemporaryObject(appFontTextFieldComponent, testCase)
        verify(textField)
        // Don't set any text on textField because we're not accounting for the widest
        // text here, so we want to compare it against an empty TextField.
        compare(control.implicitContentWidth, textField.implicitWidth)

        textField.font.pixelSize *= 2
        control.font.pixelSize *= 2
        compare(control.implicitContentWidth, textField.implicitWidth)
    }

    function test_implicitContentWidthPolicy_WidestText_data() {
        return [
            { tag: "Array", model: ["Short", "Kinda long"] },
            { tag: "ListModel", model: twoItemListModelComponent.createObject(testCase) },
        ]
    }

    function test_implicitContentWidthPolicy_WidestText(data) {
        let control = createTemporaryObject(appFontContentItemComboBoxComponent, testCase, {
            model: data.model,
            implicitContentWidthPolicy: ComboBox.WidestText
        })
        verify(control)
        compare(control.implicitContentWidthPolicy, ComboBox.WidestText)

        let textField = createTemporaryObject(appFontTextFieldComponent, testCase)
        verify(textField)
        textField.text = "Kinda long"
        // Note that we don't need to change the current index here, as the implicitContentWidth
        // is set to the implicitWidth of the TextField within the ComboBox as if it had the largest
        // text from the model set on it.
        // We use Math.ceil because TextInput uses qCeil internally, whereas the implicitWidth
        // binding for TextField does not.
        compare(Math.ceil(control.implicitContentWidth), Math.ceil(textField.implicitWidth))

        // Add a longer item; it should affect the implicit content width.
        let modifiedModel = appendedToModel(data.model, "Moderately long")
        control.model = modifiedModel
        textField.text = "Moderately long"
        compare(Math.ceil(control.implicitContentWidth), Math.ceil(textField.implicitWidth))

        // Remove the last two items; it should use the only remaining item's width.
        modifiedModel = removedFromModel(data.model, 1, 2)
        control.model = modifiedModel
        compare(control.count, 1)
        compare(control.currentText, "Short")
        textField.text = "Short"
        compare(Math.ceil(control.implicitContentWidth), Math.ceil(textField.implicitWidth))

        // Changes in font should result in the implicitContentWidth being updated.
        textField.font.pixelSize *= 2
        // We have to change the contentItem's font size manually since we break the
        // style's binding to the control's font when we set Qt.application.font to it.
        control.contentItem.font.pixelSize *= 2
        control.font.pixelSize *= 2
        compare(Math.ceil(control.implicitContentWidth), Math.ceil(textField.implicitWidth))
    }

    function test_implicitContentWidthPolicy_WidestTextWhenCompleted_data() {
        return test_implicitContentWidthPolicy_WidestText_data()
    }

    function test_implicitContentWidthPolicy_WidestTextWhenCompleted(data) {
        let control = createTemporaryObject(appFontContentItemComboBoxComponent, testCase, {
            model: data.model,
            implicitContentWidthPolicy: ComboBox.WidestTextWhenCompleted
        })
        verify(control)
        compare(control.implicitContentWidthPolicy, ComboBox.WidestTextWhenCompleted)

        let textField = createTemporaryObject(appFontTextFieldComponent, testCase)
        verify(textField)
        textField.text = "Kinda long"
        compare(Math.ceil(control.implicitContentWidth), Math.ceil(textField.implicitWidth))

        // Add a longer item; it should not affect the implicit content width
        // since we've already accounted for it once.
        let modifiedModel = appendedToModel(data.model, "Moderately long")
        control.model = modifiedModel
        compare(Math.ceil(control.implicitContentWidth), Math.ceil(textField.implicitWidth))

        // Remove the last two items; it should still not affect the implicit content width.
        modifiedModel = removedFromModel(data.model, 1, 2)
        control.model = modifiedModel
        compare(control.count, 1)
        compare(control.currentText, "Short")
        compare(Math.ceil(control.implicitContentWidth), Math.ceil(textField.implicitWidth))

        // Changes in font should not result in the implicitContentWidth being updated.
        let oldTextFieldImplicitWidth = textField.implicitWidth
        // Changes in font should result in the implicitContentWidth being updated.
        textField.font.pixelSize *= 2
        control.contentItem.font.pixelSize *= 2
        control.font.pixelSize *= 2
        compare(Math.ceil(control.implicitContentWidth), Math.ceil(oldTextFieldImplicitWidth))
    }

    // QTBUG-61021: text line should not be focused by default
    // It causes (e.g. on Android) showing virtual keyboard when it is not needed
    function test_doNotFocusTextLineByDefault() {
        var control = createTemporaryObject(comboBox, testCase)
        // Focus not set after creating combobox
        verify(!control.activeFocus)
        verify(!control.contentItem.focus)

        // After setting focus on combobox, text line should not be focused
        control.forceActiveFocus()
        verify(control.activeFocus)
        verify(!control.contentItem.focus)

        // Text line is focused after intentional setting focus on it
        control.contentItem.forceActiveFocus()
        verify(control.activeFocus)
        verify(control.contentItem.focus)
    }

    Component {
        id: intValidatorComponent
        IntValidator {
            bottom: 0
            top: 255
        }
    }

    function test_acceptableInput_QTBUG_94307() {
        let items = [
            { text: "A" },
            { text: "2" },
            { text: "3" }
        ]
        let control = createTemporaryObject(comboBox, testCase, {model: items, editable: true})
        verify(control)

        verify(control.acceptableInput)
        compare(control.displayText, "A")

        let acceptableInputSpy = signalSpy.createObject(control, {target: control, signalName: "acceptableInputChanged"})
        verify(acceptableInputSpy.valid)

        let intValidator = intValidatorComponent.createObject(testCase)
        verify(intValidator)

        control.validator = intValidator

        compare(acceptableInputSpy.count, 1)
        compare(control.displayText, "A")
        compare(control.acceptableInput, false)

        control.currentIndex = 1

        compare(acceptableInputSpy.count, 2)
        compare(control.displayText, "2")
        compare(control.acceptableInput, true)
    }

    function test_selectionCleared() {
        const model = [
            { text: "Apple" },
            { text: "Banana" },
            { text: "Coconut" }
        ]
        let control = createTemporaryObject(comboBox, testCase, { model: model, editable: true })
        verify(control)

        compare(control.displayText, "Apple")
        compare(control.editText, "Apple")
        compare(control.currentIndex, 0)

        // Give the TextField focus and select the text.
        let textField = control.contentItem
        textField.forceActiveFocus()
        textField.selectAll()
        compare(textField.selectedText, "Apple")

        // Type "B" so that Banana is selected.
        keyPress(Qt.Key_Shift)
        keyClick(Qt.Key_B)
        keyRelease(Qt.Key_Shift)
        compare(control.displayText, "Apple")
        expectFail("", "QTBUG-102950")
        compare(control.editText, "Banana")
        compare(textField.selectedText, "anana")
        compare(control.currentIndex, 0)

        // Select Banana by pressing enter.
        keyClick(Qt.Key_Return)
        compare(control.displayText, "Banana")
        compare(control.editText, "Banana")
        compare(textField.selectedText, "")
        compare(control.currentIndex, 1)
    }

    // QTBUG-109721 - verify that an eaten press event for the space key
    // doesn't open the popup when the key is released.
    Component {
        id: comboboxEatsSpace
        ComboBox {
            id: nonEditableComboBox
            editable: false
            model: ["NonEditable", "Delta", "Echo", "Foxtrot"]
            Keys.onSpacePressed: (event) => event.accept
        }
    }

    function test_spacePressEaten() {
        let control = createTemporaryObject(comboboxEatsSpace, testCase)
        verify(control)
        control.forceActiveFocus()

        var visibleChangedSpy = signalSpy.createObject(control, {target: control.popup, signalName: "visibleChanged"})
        verify(visibleChangedSpy.valid)

        // press doesn't open
        keyPress(Qt.Key_Space)
        verify(!control.pressed)
        compare(visibleChangedSpy.count, 0)
        // neither does release
        keyRelease(Qt.Key_Space)
        compare(visibleChangedSpy.count, 0)
    }

    Component {
        id: listOfGadgets
        QtObject {
            property list<rect> rects: [Qt.rect(1, 2, 3, 4), Qt.rect(5, 6, 7, 8)]
        }
    }

    function test_listOfGadgetsWithRole() {
        let model = listOfGadgets.createObject(testCase);
        let control = createTemporaryObject(
                comboBox, testCase, {model: model.rects, textRole: "width"});
        verify(control);
        compare(control.currentIndex, 0);
        compare(control.displayText, "3");

        control.currentIndex = 1;
        compare(control.displayText, "7");
    }

    function test_contextObject() {
        // We use the default delegate with required properties and pass
        // an array of objects as model. This should work despite
        // ComboBox setting itself as model object for the delegate.

        let control = createTemporaryObject(
                comboBox, testCase, {model: fruitarray, textRole: "color"});
        verify(control);
        compare(control.popup.contentItem.itemAtIndex(0).text, "red");

        // Now we pass an AbstractItemModel with 2 roles. Since we use required properties
        // the model object should still have the anonymous property, and it should be a
        // QQmlDMAbstractItemModelData.

        control = createTemporaryObject(comboBox, testCase, { model: fruitmodel });
        verify(control);
        for (var i = 0; i < 3; ++i)
            ignoreWarning(/ComboBox\.qml\:[0-9]+\:[0-9]+\: Unable to assign QQmlDMAbstractItemModelData to QString/);
        compare(control.popup.contentItem.itemAtIndex(0).text, "");
    }
}
