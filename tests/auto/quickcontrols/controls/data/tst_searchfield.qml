// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtTest
import QtQuick.Controls
import Qt.test.controls
import Qt.labs.qmlmodels

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "SearchField"

    Component {
        id: signalSpy
        SignalSpy { }
    }

    Component {
        id: searchField
        SearchField { }
    }

    Component {
        id: searchText
        SearchField {
            TextField{ }
        }
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(searchField, testCase)
        verify(control)

        compare(control.suggestionModel, undefined)
        compare(control.suggestionCount, 0)
        compare(control.currentIndex, -1)
        compare(control.highlightedIndex, -1)
        compare(control.text, "")
        compare(control.placeholderText, "")
        compare(control.textRole, "")
        compare(control.live, true)
        compare(control.selectTextByMouse, true)
        compare(control.selectedText, "")
        compare(control.selectionStart, 0)
        compare(control.selectionEnd, 0)
        compare(control.cursorPosition, 0)
        verify(control.delegate)
        if (StyleInfo.styleName !== "iOS")
            verify(control.popup)
        else
            compare(StyleInfo.styleName, "iOS")
    }

    Component {
        id: sfpmComponent

        Item {
            width: 400
            height: 400

            property alias sfpmSearchField: sfpmSearchField

            ListModel {
                id: countryModels
                ListElement { text: "Norway" }
                ListElement { text: "Turkey" }
                ListElement { text: "Albania" }
                ListElement { text: "India" }
                ListElement { text: "Iran" }
                ListElement { text: "Australia" }
                ListElement { text: "United States" }
            }

            SortFilterProxyModel {
                id: countryFilter
                sourceModel: countryModels
                sorters: RoleSorter { roleName: "text" }

                filters: FunctionFilter {
                    property var regExp: sfpmSearchField.text.length > 0
                                         ? new RegExp(sfpmSearchField.text, "i")
                                         : null
                    onRegExpChanged: invalidate()

                    function filter(text: string): bool {
                        return regExp ? regExp.test(text) : false
                    }
                }
            }

            SearchField {
                id: sfpmSearchField
                suggestionModel: countryFilter
                textRole: "text"
            }
        }
    }

    function test_sortFilterProxyModel() {
        if (StyleInfo.styleName === "iOS")
            skip("iOS style does not provide a popup for SearchField.")

        let component = createTemporaryObject(sfpmComponent, testCase)
        verify(component)

        let control = component.sfpmSearchField
        verify(control)

        control.forceActiveFocus()
        verify(control.activeFocus)

        let textItem = control.contentItem

        textItem.text = "e"
        compare(control.text, "e")
        compare(control.suggestionCount, 2)
        compare(control.currentIndex, -1)
        compare(control.popup.visible, true)

        textItem.text = "i"
        compare(control.text, "i")
        compare(control.suggestionCount, 5)
        compare(control.currentIndex, -1)
        compare(control.popup.visible, true)

        textItem.text = ""
        compare(control.text, "")
        compare(control.suggestionCount, 0)
        compare(control.currentIndex, -1)
        compare(control.popup.visible, false)
    }

    ListModel {
        id : fruitModel
        ListElement { name: "Apple"; color: "green" }
        ListElement { name: "Cherry"; color: "red" }
        ListElement { name: "Banana"; color: "yellow" }
        ListElement { name: "Orange"; color: "orange" }
        ListElement { name: "WaterMelon"; color: "pink" }
    }

    function test_textRole() {
        if (StyleInfo.styleName !== "iOS")
            ignoreWarning(/Unable to assign QQmlDMAbstractItemModelData to QString/)

        let control = createTemporaryObject(searchField, testCase)
        verify(control)

        control.suggestionModel = fruitModel
        control.textRole = "name"

        let textItem = control.contentItem
        textItem.text = "a"

        compare(control.text, "a")
        compare(control.suggestionCount, 5)
        if (StyleInfo.styleName !== "iOS")
            compare(control.popup.visible,true)
        else
            compare(StyleInfo.styleName, "iOS")

        control.textRole = "color"

        textItem.text = "r"

        compare(control.text, "r")
        compare(control.suggestionCount, 5)
        if (StyleInfo.styleName !== "iOS")
            compare(control.popup.visible,true)
        else
            compare(StyleInfo.styleName, "iOS")
    }

    Component {
        id: suggestion
        SearchField {
            onTextEdited: {
                if (text === "a") {
                    suggestionModel = ["Apple", "Apricot"]
                } else if (text === "c") {
                    suggestionModel = ["Cherry", "Coconut", "Cranberry"]
                }
            }
        }
    }

    function test_suggestionPopup() {
        if (StyleInfo.styleName === "iOS")
            skip("iOS style does not provide a popup for SearchField.")

        let control = createTemporaryObject(suggestion, testCase)
        verify(control)

        compare(control.popup.visible, false)

        let textItem = control.contentItem

        textItem.text = "a"
        compare(control.suggestionCount, 2)
        compare(control.currentIndex, -1)
        compare(control.highlightedIndex, 0)
        compare(control.popup.visible, true)

        textItem.text = "c"
        compare(control.suggestionCount, 3)
        compare(control.currentIndex, -1)
        compare(control.highlightedIndex, 0)
        compare(control.popup.visible, true)
    }

    function test_textEdited() {
        let control = createTemporaryObject(searchField, testCase)
        verify(control)

        let textEditedSpy = signalSpy.createObject(control, {target: control, signalName: "textEdited"})
        verify(textEditedSpy.valid)

        let searchTriggeredSpy = signalSpy.createObject(control, {target: control, signalName: "searchTriggered"})
        verify(searchTriggeredSpy.valid)

        control.live = true
        let textItem = control.contentItem
        textItem.text = "a"

        compare(control.text, "a")
        compare(control.currentIndex, -1)
        compare(textEditedSpy.count, 1)

        compare(searchTriggeredSpy.count, 1)
    }

    function test_currentIndexResetsOnEdit() {
        if (StyleInfo.styleName === "iOS")
            skip("iOS style does not provide a popup for SearchField.")

        ignoreWarning(/Unable to assign QQmlDMAbstractItemModelData to QString/)

        let control = createTemporaryObject(searchField, testCase)
        verify(control)

        control.forceActiveFocus()
        verify(control.activeFocus)

        control.suggestionModel = fruitModel
        control.textRole = "name"

        let textItem = control.contentItem
        textItem.text = "a"

        compare(control.popup.visible, true)

        compare(control.currentIndex, -1)
        compare(control.highlightedIndex, 0)

        keyClick(Qt.Key_Down)
        compare(control.currentIndex, -1)
        compare(control.highlightedIndex, 1)

        keyClick(Qt.Key_Enter)
        compare(control.currentIndex, 1)
        compare(control.popup.visible, false)

        textItem.text = control.text + "x"
        compare(control.currentIndex, -1)
    }

    function test_arrowKeys() {
        if (StyleInfo.styleName === "iOS")
            skip("iOS style does not provide a popup for SearchField.")

        ignoreWarning(/Unable to assign QQmlDMAbstractItemModelData to QString/)

        let control = createTemporaryObject(searchField, testCase)
        verify(control)

        let activatedSpy = signalSpy.createObject(control, {target: control, signalName: "activated"})
        verify(activatedSpy.valid)

        let highlightedSpy = signalSpy.createObject(control, {target: control, signalName: "highlighted"})
        verify(highlightedSpy.valid)

        let openedSpy = signalSpy.createObject(control, {target: control.popup, signalName: "opened"})
        verify(openedSpy.valid)

        let closedSpy = signalSpy.createObject(control, {target: control.popup, signalName: "closed"})
        verify(closedSpy.valid)

        let acceptedSpy = signalSpy.createObject(control, {target: control, signalName: "accepted"})
        verify(closedSpy.valid)

        let searchTriggeredSpy = signalSpy.createObject(control, {target: control, signalName: "searchTriggered"})
        verify(searchTriggeredSpy.valid)

        control.forceActiveFocus()
        verify(control.activeFocus)

        compare(control.currentIndex, -1)
        compare(control.highlightedIndex, -1)

        control.suggestionModel = fruitModel
        control.textRole = "name"

        let textItem = control.contentItem
        textItem.text = "a"

        compare(control.popup.visible, true)

        keyClick(Qt.Key_Down)
        compare(control.currentIndex, -1)
        compare(control.highlightedIndex, 1)
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 1)
        compare(highlightedSpy.signalArguments[0][0], 1)
        highlightedSpy.clear()

        keyClick(Qt.Key_Down)
        compare(control.currentIndex, -1)
        compare(control.highlightedIndex, 2)
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 1)
        compare(highlightedSpy.signalArguments[0][0], 2)
        highlightedSpy.clear()

        keyClick(Qt.Key_Up)
        compare(control.currentIndex, -1)
        compare(control.highlightedIndex, 1)
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 1)
        compare(highlightedSpy.signalArguments[0][0], 1)
        highlightedSpy.clear()

        keyClick(Qt.Key_Enter)
        compare(control.text, "Cherry")
        compare(control.currentIndex, 1)
        compare(acceptedSpy.count, 1)
        compare(searchTriggeredSpy.count, 2)
        compare(control.popup.visible, false)

        keyClick(Qt.Key_Back)
        compare(control.text, "")
        compare(control.popup.visible, false)

        keyClick(Qt.Key_Escape)
        compare(control.text, "")
        compare(control.popup.visible, false)

        control.currentIndex = -1
        activatedSpy.clear()
        acceptedSpy.clear()
        searchTriggeredSpy.clear()

        // Can navigate between the suggestions even if no input text was given by the user.

        control.popup.open()
        compare(control.popup.visible, true)

        keyClick(Qt.Key_Down)
        compare(control.currentIndex, -1)
        compare(control.highlightedIndex, 1)
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 1)
        compare(highlightedSpy.signalArguments[0][0], 1)
        highlightedSpy.clear()

        keyClick(Qt.Key_Down)
        compare(control.currentIndex, -1)
        compare(control.highlightedIndex, 2)
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 1)
        compare(highlightedSpy.signalArguments[0][0], 2)
        highlightedSpy.clear()

        keyClick(Qt.Key_Up)
        compare(control.currentIndex, -1)
        compare(control.highlightedIndex, 1)
        compare(activatedSpy.count, 0)
        compare(highlightedSpy.count, 1)
        compare(highlightedSpy.signalArguments[0][0], 1)
        highlightedSpy.clear()

        keyClick(Qt.Key_Enter)
        compare(control.text, "Cherry")
        compare(control.currentIndex, 1)
        compare(acceptedSpy.count, 1)
        compare(searchTriggeredSpy.count, 1)
    }

    Component {
        id: delegateComponent1

        ItemDelegate {}
    }

    Component {
        id: delegateComponent2

        ItemDelegate {}
    }

    function test_dontDeleteDelegates() {
        let control = createTemporaryObject(searchField, testCase, { delegate: delegateComponent1 })
        verify(control)

        // When setting a new delegate, the old one shouldn't be destroyed.
        control.delegate = delegateComponent2
        verify(delegateComponent1)

        // The same goes for the new delegate: it shouldn't be destroyed when setting the old one.
        control.delegate = delegateComponent1
        verify(delegateComponent2)
    }

    Component {
        id: findShortcutComponent
        ApplicationWindow {
            id: window
            width: 400
            height: 400
            visible: true

            property alias searchFieldShortcut: searchFieldShortcut
            property alias shortcut: shortcut

            SearchField {
                id: searchFieldShortcut

                Shortcut {
                    id: shortcut
                    sequences: [ StandardKey.Find ]
                    onActivated: {
                        searchFieldShortcut.forceActiveFocus()
                        searchFieldShortcut.selectAll()
                    }
                }
            }
        }
    }

    function test_findShortcut_givesFocus() {
        let window = createTemporaryObject(findShortcutComponent, testCase)
        let control = window.searchFieldShortcut
        verify(control)

        window.requestActivate()
        tryCompare(window, "active", true)

        let shortcutActivatedSpy = createTemporaryObject(signalSpy, testCase,
            { target: window.shortcut, signalName: "activated"} )
        verify(shortcutActivatedSpy.valid)

        waitForRendering(window.contentItem)
        verify(!control.contentItem.activeFocus)
        compare(shortcutActivatedSpy.count, 0)

        keySequence(StandardKey.Find)
        compare(shortcutActivatedSpy.count, 1)

        var isWayland = Qt.platform.pluginName
                        && Qt.platform.pluginName.indexOf("wayland") !== -1

        if (isWayland) {
            expectFail("", "Fails on Wayland - QTBUG-144177")
        }

        verify(control.contentItem.activeFocus)
    }

    function test_findShortcut_selectAll() {
        let window = createTemporaryObject(findShortcutComponent, testCase)
        let control = window.searchFieldShortcut
        verify(control)

        window.requestActivate()
        tryCompare(window, "active", true)

        let shortcutActivatedSpy = createTemporaryObject(signalSpy, testCase,
            { target: window.shortcut, signalName: "activated"} )
        verify(shortcutActivatedSpy.valid)

        waitForRendering(window.contentItem)

        control.contentItem.text = "hello"

        verify(!control.contentItem.activeFocus)
        compare(control.contentItem.selectedText, "")
        compare(shortcutActivatedSpy.count, 0)

        keySequence(StandardKey.Find)
        compare(shortcutActivatedSpy.count, 1)

        var isWayland = Qt.platform.pluginName
                        && Qt.platform.pluginName.indexOf("wayland") !== -1

        if (isWayland) {
            expectFail("", "Fails on Wayland - QTBUG-144177")
        }

        verify(control.contentItem.activeFocus)
        compare(control.contentItem.selectedText, "hello")
    }

    function test_selectTextByMouse() {
        let control = createTemporaryObject(searchField, testCase,
                { text: "Some text", selectTextByMouse: true, width: parent.width })
        verify(control)
        waitForRendering(control)
        control.forceActiveFocus()

        mouseClick(control, control.leftPadding, control.height / 2)
        mousePress(control, control.leftPadding, control.height / 2)
        mouseMove(control, control.leftPadding + control.contentItem.width, control.height / 2)
        mouseRelease(control, control.leftPadding + control.contentItem.width, control.height / 2)

        compare(control.selectedText, "Some text")
        verify(control.selectionStart >= 0)
        verify(control.selectionEnd >= 0)
        verify(control.selectionStart < control.selectionEnd)
    }

    function test_select_and_deselect() {
        let control = createTemporaryObject(searchField, testCase,
                { text: "Some text", selectTextByMouse: true, width: parent.width })
        verify(control)
        waitForRendering(control)
        control.forceActiveFocus()

        control.select(0, 4)

        compare(control.selectionStart, 0)
        compare(control.selectionEnd, 4)
        compare(control.selectedText, "Some")

        control.deselect()

        compare(control.selectionStart, 4)
        compare(control.selectionEnd, 4)
        compare(control.selectedText, "")
    }

    function test_selectWord_withCursorPosition() {
        let control = createTemporaryObject(searchField, testCase,
                { text: "Some text", selectTextByMouse: true, width: parent.width })
        verify(control)
        waitForRendering(control)
        control.forceActiveFocus()

        control.cursorPosition = 6
        control.selectWord()
        compare(control.selectedText, "text")
        compare(control.cursorPosition, 9)

        control.cursorPosition = 1
        control.selectWord()
        compare(control.selectedText, "Some")
        compare(control.cursorPosition, 4)
    }
}
