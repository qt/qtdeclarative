// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtQuick.Controls
import Qt.test.controls

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "TextArea"

    Component {
        id: textArea
        TextArea { background: Item { } }
    }

    Component {
        id: flickable
        Flickable {
            width: 200
            height: 200
            TextArea.flickable: TextArea { }
        }
    }

    Component {
        id: flickableCustomBackground
        Flickable {
            width: 200
            height: 200
            TextArea.flickable: TextArea {
                background: Rectangle {
                    color: "green"
                }
            }
        }
    }

    Component {
        id: flickableWithScrollBar
        Flickable {
            width: 200
            height: 200
            TextArea.flickable: TextArea { }
            ScrollBar.vertical: ScrollBar { }
        }
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    Component {
        id: rectangle
        Rectangle { }
    }

    FontMetrics {
        id: defaultFontMetrics
    }

    TestUtil {
        id: util
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_creation() {
        let control = createTemporaryObject(textArea, testCase)
        verify(control)
    }

    function test_implicitSize() {
        let control = createTemporaryObject(textArea, testCase)
        verify(control)

        let implicitWidthSpy = signalSpy.createObject(control, { target: control, signalName: "implicitWidthChanged"} )
        verify(implicitWidthSpy.valid)

        let implicitHeightSpy = signalSpy.createObject(control, { target: control, signalName: "implicitHeightChanged"} )
        verify(implicitHeightSpy.valid)

        let implicitBackgroundWidthSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "implicitBackgroundWidthChanged"})
        verify(implicitBackgroundWidthSpy.valid)

        let implicitBackgroundHeightSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "implicitBackgroundHeightChanged"})
        verify(implicitBackgroundHeightSpy.valid)

        let implicitWidthChanges = 0
        let implicitHeightChanges = 0
        let implicitBackgroundWidthChanges = 0
        let implicitBackgroundHeightChanges = 0

        verify(control.implicitWidth >= control.leftPadding + control.rightPadding)
        verify(control.implicitHeight >= control.contentHeight + control.topPadding + control.bottomPadding)
        compare(control.implicitBackgroundWidth, 0)
        compare(control.implicitBackgroundHeight, 0)

        control.background = rectangle.createObject(control, {implicitWidth: 400, implicitHeight: 200})
        compare(control.implicitWidth, 400)
        compare(control.implicitHeight, 200)
        compare(control.implicitBackgroundWidth, 400)
        compare(control.implicitBackgroundHeight, 200)
        compare(implicitWidthSpy.count, ++implicitWidthChanges)
        compare(implicitHeightSpy.count, ++implicitHeightChanges)
        compare(implicitBackgroundWidthSpy.count, ++implicitBackgroundWidthChanges)
        compare(implicitBackgroundHeightSpy.count, ++implicitBackgroundHeightChanges)

        control.background = null
        compare(control.implicitWidth, control.leftPadding + control.rightPadding)
        verify(control.implicitHeight >= control.contentHeight + control.topPadding + control.bottomPadding)
        compare(control.implicitBackgroundWidth, 0)
        compare(control.implicitBackgroundHeight, 0)
        compare(implicitWidthSpy.count, ++implicitWidthChanges)
        compare(implicitHeightSpy.count, ++implicitHeightChanges)
        compare(implicitBackgroundWidthSpy.count, ++implicitBackgroundWidthChanges)
        compare(implicitBackgroundHeightSpy.count, ++implicitBackgroundHeightChanges)

        control.text = "TextArea"
        compare(control.implicitWidth, control.contentWidth + control.leftPadding + control.rightPadding)
        verify(control.implicitHeight >= control.contentHeight + control.topPadding + control.bottomPadding)
        compare(control.implicitBackgroundWidth, 0)
        compare(control.implicitBackgroundHeight, 0)
        compare(implicitWidthSpy.count, ++implicitWidthChanges)

        defaultFontMetrics.font = control.font
        let leading = defaultFontMetrics.leading
        let ascent = defaultFontMetrics.ascent
        let descent = defaultFontMetrics.descent

        let leadingOverflow = Math.ceil(ascent + descent) < Math.ceil(ascent + descent + leading)

        // If the font in use triggers QTBUG-83894, it is possible that this will cause
        // the following compare to fail if the implicitHeight from the TextEdit is ued.
        // Unfortunately, since some styles override implicitHeight, we cannot guarantee
        // that it will fail, so we need to simply skip the test for these cases.
        if (!leadingOverflow)
            compare(implicitHeightSpy.count, implicitHeightChanges)
        compare(implicitBackgroundWidthSpy.count, implicitBackgroundWidthChanges)
        compare(implicitBackgroundHeightSpy.count, implicitBackgroundHeightChanges)

        control.placeholderText = "..."
        compare(control.implicitWidth, control.contentWidth + control.leftPadding + control.rightPadding)
        verify(control.implicitHeight >= control.contentHeight + control.topPadding + control.bottomPadding)
        compare(control.implicitBackgroundWidth, 0)
        compare(control.implicitBackgroundHeight, 0)
        compare(implicitWidthSpy.count, implicitWidthChanges)
        if (!leadingOverflow)
            compare(implicitHeightSpy.count, implicitHeightChanges)
        compare(implicitBackgroundWidthSpy.count, implicitBackgroundWidthChanges)
        compare(implicitBackgroundHeightSpy.count, implicitBackgroundHeightChanges)
    }

    function test_alignment_data() {
        return [
            { tag: "empty", text: "", placeholderText: "", textAlignment: undefined, placeholderAlignment: Qt.AlignLeft },
            { tag: "empty,left", text: "", placeholderText: "", textAlignment: Qt.AlignLeft, placeholderAlignment: Qt.AlignLeft },
            { tag: "empty,center", text: "", placeholderText: "", textAlignment: Qt.AlignHCenter, placeholderAlignment: Qt.AlignHCenter },
            { tag: "empty,right", text: "", placeholderText: "", textAlignment: Qt.AlignRight, placeholderAlignment: Qt.AlignRight },

            { tag: "empty,ltr", text: "", placeholderText: "Search", textAlignment: undefined, placeholderAlignment: Qt.AlignLeft },
            { tag: "empty,ltr,left", text: "", placeholderText: "Search", textAlignment: Qt.AlignLeft, placeholderAlignment: Qt.AlignLeft },
            { tag: "empty,ltr,center", text: "", placeholderText: "Search", textAlignment: Qt.AlignHCenter, placeholderAlignment: Qt.AlignHCenter },
            { tag: "empty,ltr,right", text: "", placeholderText: "Search", textAlignment: Qt.AlignRight, placeholderAlignment: Qt.AlignRight },

            { tag: "empty,rtl", text: "", placeholderText: "بحث", textAlignment: undefined, placeholderAlignment: Qt.AlignRight },
            { tag: "empty,rtl,left", text: "", placeholderText: "بحث", textAlignment: Qt.AlignLeft, placeholderAlignment: Qt.AlignLeft },
            { tag: "empty,rtl,center", text: "", placeholderText: "بحث", textAlignment: Qt.AlignHCenter, placeholderAlignment: Qt.AlignHCenter },
            { tag: "empty,rtl,right", text: "", placeholderText: "بحث", textAlignment: Qt.AlignRight, placeholderAlignment: Qt.AlignRight },

            { tag: "ltr,empty", text: "Text", placeholderText: "", textAlignment: undefined, placeholderAlignment: Qt.AlignLeft },
            { tag: "ltr,empty,left", text: "Text", placeholderText: "", textAlignment: Qt.AlignLeft, placeholderAlignment: Qt.AlignLeft },
            { tag: "ltr,empty,center", text: "Text", placeholderText: "", textAlignment: Qt.AlignHCenter, placeholderAlignment: Qt.AlignHCenter },
            { tag: "ltr,empty,right", text: "Text", placeholderText: "", textAlignment: Qt.AlignRight, placeholderAlignment: Qt.AlignRight },

            { tag: "ltr,ltr", text: "Text", placeholderText: "Search", textAlignment: undefined, placeholderAlignment: Qt.AlignLeft },
            { tag: "ltr,ltr,left", text: "Text", placeholderText: "Search", textAlignment: Qt.AlignLeft, placeholderAlignment: Qt.AlignLeft },
            { tag: "ltr,ltr,center", text: "Text", placeholderText: "Search", textAlignment: Qt.AlignHCenter, placeholderAlignment: Qt.AlignHCenter },
            { tag: "ltr,ltr,right", text: "Text", placeholderText: "Search", textAlignment: Qt.AlignRight, placeholderAlignment: Qt.AlignRight },

            { tag: "ltr,rtl", text: "Text", placeholderText: "بحث", textAlignment: undefined, placeholderAlignment: Qt.AlignRight },
            { tag: "ltr,rtl,left", text: "Text", placeholderText: "بحث", textAlignment: Qt.AlignLeft, placeholderAlignment: Qt.AlignLeft },
            { tag: "ltr,rtl,center", text: "Text", placeholderText: "بحث", textAlignment: Qt.AlignHCenter, placeholderAlignment: Qt.AlignHCenter },
            { tag: "ltr,rtl,right", text: "Text", placeholderText: "بحث", textAlignment: Qt.AlignRight, placeholderAlignment: Qt.AlignRight },

            { tag: "rtl,empty", text: "نص", placeholderText: "", textAlignment: undefined, placeholderAlignment: Qt.AlignLeft },
            { tag: "rtl,empty,left", text: "نص", placeholderText: "", textAlignment: Qt.AlignLeft, placeholderAlignment: Qt.AlignLeft },
            { tag: "rtl,empty,center", text: "نص", placeholderText: "", textAlignment: Qt.AlignHCenter, placeholderAlignment: Qt.AlignHCenter },
            { tag: "rtl,empty,right", text: "نص", placeholderText: "", textAlignment: Qt.AlignRight, placeholderAlignment: Qt.AlignRight },

            { tag: "rtl,ltr", text: "نص", placeholderText: "Search", textAlignment: undefined, placeholderAlignment: Qt.AlignLeft },
            { tag: "rtl,ltr,left", text: "نص", placeholderText: "Search", textAlignment: Qt.AlignLeft, placeholderAlignment: Qt.AlignLeft },
            { tag: "rtl,ltr,center", text: "نص", placeholderText: "Search", textAlignment: Qt.AlignHCenter, placeholderAlignment: Qt.AlignHCenter },
            { tag: "rtl,ltr,right", text: "نص", placeholderText: "Search", textAlignment: Qt.AlignRight, placeholderAlignment: Qt.AlignRight },

            { tag: "rtl,rtl", text: "نص", placeholderText: "بحث", textAlignment: undefined, placeholderAlignment: Qt.AlignRight },
            { tag: "rtl,rtl,left", text: "نص", placeholderText: "بحث", textAlignment: Qt.AlignLeft, placeholderAlignment: Qt.AlignLeft },
            { tag: "rtl,rtl,center", text: "نص", placeholderText: "بحث", textAlignment: Qt.AlignHCenter, placeholderAlignment: Qt.AlignHCenter },
            { tag: "rtl,rtl,right", text: "نص", placeholderText: "بحث", textAlignment: Qt.AlignRight, placeholderAlignment: Qt.AlignRight },
        ]
    }

    function test_alignment(data) {
        let control = createTemporaryObject(textArea, testCase, {text: data.text, placeholderText: data.placeholderText})

        if (data.textAlignment !== undefined) {
            control.horizontalAlignment = data.textAlignment
            compare(control.horizontalAlignment, data.textAlignment)
        }

        // The placeholder text of the Material style doesn't currently respect the alignment of the control.
        if (StyleInfo.styleName !== "Material") {
            for (let i = 0; i < control.children.length; ++i) {
                if (control.children[i].hasOwnProperty("horizontalAlignment"))
                    compare(control.children[i].effectiveHorizontalAlignment, data.placeholderAlignment) // placeholder
            }
        }

        control.verticalAlignment = TextArea.AlignBottom
        compare(control.verticalAlignment, TextArea.AlignBottom)
        if (StyleInfo.styleName !== "Material") {
            for (let j = 0; j < control.children.length; ++j) {
                if (control.children[j].hasOwnProperty("verticalAlignment"))
                    compare(control.children[j].verticalAlignment, Text.AlignBottom) // placeholder
            }
        }
    }

    function test_font_explicit_attributes_data() {
        return [
            {tag: "bold", value: true},
            {tag: "capitalization", value: Font.Capitalize},
            {tag: "family", value: "Tahoma"},
            {tag: "italic", value: true},
            {tag: "strikeout", value: true},
            {tag: "underline", value: true},
            {tag: "weight", value: Font.Black},
            {tag: "wordSpacing", value: 55}
        ]
    }

    function test_font_explicit_attributes(data) {
        let control = createTemporaryObject(textArea, testCase)
        verify(control)

        let child = textArea.createObject(control)
        verify(child)

        let controlSpy = signalSpy.createObject(control, {target: control, signalName: "fontChanged"})
        verify(controlSpy.valid)

        let childSpy = signalSpy.createObject(child, {target: child, signalName: "fontChanged"})
        verify(childSpy.valid)

        let defaultValue = control.font[data.tag]
        child.font[data.tag] = defaultValue

        compare(child.font[data.tag], defaultValue)
        compare(childSpy.count, 0)

        control.font[data.tag] = data.value

        compare(control.font[data.tag], data.value)
        compare(controlSpy.count, 1)

        compare(child.font[data.tag], defaultValue)
        compare(childSpy.count, 0)
    }

    function test_flickable() {
        let control = createTemporaryObject(flickable, testCase)
        verify(control)

        let textArea = control.TextArea.flickable
        verify(textArea)

        if (textArea.background)
            compare(textArea.background.parent, control)

        for (let i = 1; i <= 100; ++i)
            textArea.text += "line\n" + i

        verify(textArea.contentWidth > 0)
        verify(textArea.contentHeight > 200)

        compare(control.contentWidth, textArea.implicitWidth)
        compare(control.contentHeight, textArea.implicitHeight)

        compare(textArea.cursorPosition, 0)

        let center = textArea.positionAt(control.width / 2, control.height / 2)
        verify(center > 0)
        mouseClick(textArea, control.width / 2, control.height / 2)
        compare(textArea.cursorPosition, center)

        // click inside text area, but below flickable
        let below = textArea.positionAt(control.width / 2, control.height + 1)
        verify(below > center)
        mouseClick(textArea, control.width / 2, control.height + 1)
        compare(textArea.cursorPosition, center) // no change

        // scroll down
        control.contentY = -(control.contentHeight - control.height) / 2

        // click inside textarea, but above flickable
        let above = textArea.positionAt(control.width / 2, textArea.topPadding)
        verify(above > 0 && above < center)
        mouseClick(textArea, control.width / 2, 0)
        compare(textArea.cursorPosition, center) // no change
    }

    function test_flickableCustomBackground() {
        // Test that the TextArea background item is parented out of the
        // TextArea and into the Flicable, and that it has the same size
        // as the flickable.
        let flickable = createTemporaryObject(flickableCustomBackground, testCase)
        verify(flickable)

        let textArea = flickable.TextArea.flickable
        verify(textArea)
        verify(textArea.background)
        compare(textArea.background.width, flickable.width)
        compare(textArea.background.height, flickable.height)
    }

    function test_scrollable_paste_large() {
        let control = createTemporaryObject(flickableWithScrollBar, testCase)
        verify(control)

        let textArea = control.TextArea.flickable
        verify(textArea)

        if (typeof(textArea.paste) !== "function")
            skip("Clipboard is not supported on this platform.")

        util.populateClipboardText(100)
        waitForRendering(control)
        // don't crash (QTBUG-99582)
        textArea.paste()
        // verify that the cursor moved to the bottom after pasting, and we scrolled down to show it
        tryVerify(function() { return textArea.cursorRectangle.y > 1000 }); // maybe > 2000, depending on font size
        tryVerify(function() { return control.contentY > textArea.cursorRectangle.y - control.height })
    }

    function test_warning() {
        ignoreWarning(/QML TestCase: TextArea attached property must be attached to an object deriving from Flickable/)
        testCase.TextArea.flickable = null
    }

    function test_hover_data() {
        return [
            { tag: "enabled", hoverEnabled: true },
            { tag: "disabled", hoverEnabled: false },
        ]
    }

    function test_hover(data) {
        let control = createTemporaryObject(textArea, testCase, {text: "TextArea", hoverEnabled: data.hoverEnabled})
        verify(control)

        compare(control.hovered, false)

        mouseMove(control, control.width / 2, control.height / 2)
        compare(control.hovered, data.hoverEnabled)

        mouseMove(control, -1, -1)
        compare(control.hovered, false)
    }

    function test_pressedReleased_data() {
        return [
            {
                tag: "pressed outside", x: -1, y: -1, button: Qt.LeftButton,
                controlPressEvent: null,
                controlReleaseEvent: null,
                parentPressEvent: {
                    x: 0, y: 0, button: Qt.LeftButton, buttons: Qt.LeftButton, modifiers: Qt.NoModifier, wasHeld: false, isClick: false
                },
                parentReleaseEvent: {
                    x: 0, y: 0, button: Qt.LeftButton, buttons: Qt.NoButton, modifiers: Qt.NoModifier, wasHeld: false, isClick: false
                },
            },
            {
                tag: "left click", x: 0, y: 0, button: Qt.LeftButton,
                controlPressEvent: {
                    x: 0, y: 0, button: Qt.LeftButton, buttons: Qt.LeftButton, modifiers: Qt.NoModifier, wasHeld: false, isClick: false
                },
                controlReleaseEvent: {
                    x: 0, y: 0, button: Qt.LeftButton, buttons: Qt.NoButton, modifiers: Qt.NoModifier, wasHeld: false, isClick: false
                },
                parentPressEvent: null,
                parentReleaseEvent: null,
            },
            {
                tag: "right click", x: 0, y: 0, button: Qt.RightButton,
                controlPressEvent: {
                    x: 0, y: 0, button: Qt.RightButton, buttons: Qt.RightButton, modifiers: Qt.NoModifier, wasHeld: false, isClick: false
                },
                controlReleaseEvent: {
                    x: 0, y: 0, button: Qt.RightButton, buttons: Qt.NoButton, modifiers: Qt.NoModifier, wasHeld: false, isClick: false
                },
                parentPressEvent: null,
                parentReleaseEvent: null,
            },
        ];
    }

    Component {
        id: mouseAreaComponent
        MouseArea {
            anchors.fill: parent
        }
    }

    function checkMouseEvent(event, expectedEvent) {
        compare(event.x, expectedEvent.x)
        compare(event.y, expectedEvent.y)
        compare(event.button, expectedEvent.button)
        compare(event.buttons, expectedEvent.buttons)
    }

    function test_pressedReleased(data) {
        let mouseArea = createTemporaryObject(mouseAreaComponent, testCase)
        verify(mouseArea)
        let control = textArea.createObject(mouseArea, {text: "TextArea"})
        verify(control)

        // Give enough room to check presses outside of the control and on the parent.
        control.x = 1;
        control.y = 1;

        function checkControlPressEvent(event) {
            checkMouseEvent(event, data.controlPressEvent)
        }
        function checkControlReleaseEvent(event) {
            checkMouseEvent(event, data.controlReleaseEvent)
        }
        function checkParentPressEvent(event) {
            checkMouseEvent(event, data.parentPressEvent)
        }
        function checkParentReleaseEvent(event) {
            checkMouseEvent(event, data.parentReleaseEvent)
        }

        // Can't use signalArguments, because the event won't live that long.
        if (data.controlPressEvent)
            control.onPressed.connect(checkControlPressEvent)
        if (data.controlReleaseEvent)
            control.onReleased.connect(checkControlReleaseEvent)
        if (data.parentPressEvent)
            control.onPressed.connect(checkParentPressEvent)
        if (data.parentReleaseEvent)
            control.onReleased.connect(checkParentReleaseEvent)

        let controlPressedSpy = signalSpy.createObject(control, { target: control, signalName: "pressed" })
        verify(controlPressedSpy.valid)
        let controlReleasedSpy = signalSpy.createObject(control, { target: control, signalName: "released" })
        verify(controlReleasedSpy.valid)
        let parentPressedSpy = signalSpy.createObject(mouseArea, { target: mouseArea, signalName: "pressed" })
        verify(parentPressedSpy.valid)
        let parentReleasedSpy = signalSpy.createObject(mouseArea, { target: mouseArea, signalName: "released" })
        verify(parentReleasedSpy.valid)

        mousePress(control, data.x, data.y, data.button)
        compare(controlPressedSpy.count, data.controlPressEvent ? 1 : 0)
        compare(parentPressedSpy.count, data.parentPressEvent ? 1 : 0)
        mouseRelease(control, data.x, data.y, data.button)
        compare(controlReleasedSpy.count, data.controlReleaseEvent ? 1 : 0)
        compare(parentReleasedSpy.count, data.parentReleaseEvent ? 1 : 0)
    }

    Component {
        id: ignoreTextArea

        TextArea {
            property bool ignorePress: false
            property bool ignoreRelease: false

            onPressed: function (event) { if (ignorePress) event.accepted = false }
            onReleased: function (event) { if (ignoreRelease) event.accepted = false }
        }
    }

    function checkEventAccepted(event) {
        compare(event.accepted, true)
    }

    function checkEventIgnored(event) {
        compare(event.accepted, false)
    }

    function test_ignorePressRelease() {
        let mouseArea = createTemporaryObject(mouseAreaComponent, testCase)
        verify(mouseArea)
        let control = ignoreTextArea.createObject(mouseArea)
        verify(control)

        let controlPressedSpy = signalSpy.createObject(control, { target: control, signalName: "pressed" })
        verify(controlPressedSpy.valid)
        let controlReleasedSpy = signalSpy.createObject(control, { target: control, signalName: "released" })
        verify(controlReleasedSpy.valid)
        let parentPressedSpy = signalSpy.createObject(mouseArea, { target: mouseArea, signalName: "pressed" })
        verify(parentPressedSpy.valid)
        let parentReleasedSpy = signalSpy.createObject(mouseArea, { target: mouseArea, signalName: "released" })
        verify(parentReleasedSpy.valid)

        // Ignore only press events.
        control.onPressed.connect(checkEventIgnored)
        control.ignorePress = true
        mousePress(control, 0, 0, data.button)
        // The control will still get the signal, it just won't accept the event.
        compare(controlPressedSpy.count, 1)
        compare(parentPressedSpy.count, 1)
        mouseRelease(control, 0, 0, data.button)
        compare(controlReleasedSpy.count, 0)
        compare(parentReleasedSpy.count, 1)
        control.onPressed.disconnect(checkEventIgnored)

        // Ignore only release events.
        control.onPressed.connect(checkEventAccepted)
        control.onReleased.connect(checkEventIgnored)
        control.ignorePress = false
        control.ignoreRelease = true
        mousePress(control, 0, 0, data.button)
        compare(controlPressedSpy.count, 2)
        compare(parentPressedSpy.count, 1)
        mouseRelease(control, 0, 0, data.button)
        compare(controlReleasedSpy.count, 1)
        compare(parentReleasedSpy.count, 1)
        control.onPressed.disconnect(checkEventAccepted)
        control.onReleased.disconnect(checkEventIgnored)
    }

    function test_multiClick() {
        let control = createTemporaryObject(textArea, testCase, {text: "Qt Quick Controls 2 TextArea", selectByMouse: true})
        verify(control)

        waitForRendering(control)
        control.width = control.contentWidth
        let rect = control.positionToRectangle(12)

        // double click -> select word
        mouseDoubleClickSequence(control, rect.x + rect.width / 2, rect.y + rect.height / 2)
        compare(control.selectedText, "Controls")

        // tripple click -> select whole line
        mouseClick(control, rect.x + rect.width / 2, rect.y + rect.height / 2)
        compare(control.selectedText, "Qt Quick Controls 2 TextArea")
    }

    Component {
        id: scrollView
        ScrollView {
            TextArea { }
        }
    }

    function test_scrollView() {
        let control = createTemporaryObject(scrollView, testCase)
        verify(control)

        // don't crash (QTBUG-62292)
        control.destroy()
        wait(0)
    }

    function test_placeholderTextColor() {
        let control = createTemporaryObject(textArea, testCase)
        verify(control)

        // usually default value should not be pure opacue black
        verify(control.placeholderTextColor !== "#ff000000")
        control.placeholderTextColor = "#12345678"
        compare(control.placeholderTextColor, "#12345678")

        for (let i = 0; i < control.children.length; ++i) {
            if (control.children[i].hasOwnProperty("text"))
                compare(control.children[i].color, control.placeholderTextColor) // placeholder.color
        }
    }

    function test_inset() {
        let control = createTemporaryObject(textArea, testCase, {background: rectangle.createObject(testCase)})
        verify(control)

        let topInsetSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "topInsetChanged"})
        verify(topInsetSpy.valid)

        let leftInsetSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "leftInsetChanged"})
        verify(leftInsetSpy.valid)

        let rightInsetSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "rightInsetChanged"})
        verify(rightInsetSpy.valid)

        let bottomInsetSpy = createTemporaryObject(signalSpy, testCase, {target: control, signalName: "bottomInsetChanged"})
        verify(bottomInsetSpy.valid)

        let topInsetChanges = 0
        let leftInsetChanges = 0
        let rightInsetChanges = 0
        let bottomInsetChanges = 0

        compare(control.topInset, 0)
        compare(control.leftInset, 0)
        compare(control.rightInset, 0)
        compare(control.bottomInset, 0)

        control.width = 100
        control.height = 100
        compare(control.background.x, 0)
        compare(control.background.y, 0)
        compare(control.background.width, 100)
        compare(control.background.height, 100)

        control.topInset = 10
        compare(control.topInset, 10)
        compare(control.leftInset, 0)
        compare(control.rightInset, 0)
        compare(control.bottomInset, 0)
        compare(topInsetSpy.count, ++topInsetChanges)
        compare(leftInsetSpy.count, leftInsetChanges)
        compare(rightInsetSpy.count, rightInsetChanges)
        compare(bottomInsetSpy.count, bottomInsetChanges)
        compare(control.background.x, 0)
        compare(control.background.y, 10)
        compare(control.background.width, 100)
        compare(control.background.height, 90)

        control.leftInset = 20
        compare(control.topInset, 10)
        compare(control.leftInset, 20)
        compare(control.rightInset, 0)
        compare(control.bottomInset, 0)
        compare(topInsetSpy.count, topInsetChanges)
        compare(leftInsetSpy.count, ++leftInsetChanges)
        compare(rightInsetSpy.count, rightInsetChanges)
        compare(bottomInsetSpy.count, bottomInsetChanges)
        compare(control.background.x, 20)
        compare(control.background.y, 10)
        compare(control.background.width, 80)
        compare(control.background.height, 90)

        control.rightInset = 30
        compare(control.topInset, 10)
        compare(control.leftInset, 20)
        compare(control.rightInset, 30)
        compare(control.bottomInset, 0)
        compare(topInsetSpy.count, topInsetChanges)
        compare(leftInsetSpy.count, leftInsetChanges)
        compare(rightInsetSpy.count, ++rightInsetChanges)
        compare(bottomInsetSpy.count, bottomInsetChanges)
        compare(control.background.x, 20)
        compare(control.background.y, 10)
        compare(control.background.width, 50)
        compare(control.background.height, 90)

        control.bottomInset = 40
        compare(control.topInset, 10)
        compare(control.leftInset, 20)
        compare(control.rightInset, 30)
        compare(control.bottomInset, 40)
        compare(topInsetSpy.count, topInsetChanges)
        compare(leftInsetSpy.count, leftInsetChanges)
        compare(rightInsetSpy.count, rightInsetChanges)
        compare(bottomInsetSpy.count, ++bottomInsetChanges)
        compare(control.background.x, 20)
        compare(control.background.y, 10)
        compare(control.background.width, 50)
        compare(control.background.height, 50)

        control.topInset = undefined
        compare(control.topInset, 0)
        compare(control.leftInset, 20)
        compare(control.rightInset, 30)
        compare(control.bottomInset, 40)
        compare(topInsetSpy.count, ++topInsetChanges)
        compare(leftInsetSpy.count, leftInsetChanges)
        compare(rightInsetSpy.count, rightInsetChanges)
        compare(bottomInsetSpy.count, bottomInsetChanges)
        compare(control.background.x, 20)
        compare(control.background.y, 0)
        compare(control.background.width, 50)
        compare(control.background.height, 60)

        control.leftInset = undefined
        compare(control.topInset, 0)
        compare(control.leftInset, 0)
        compare(control.rightInset, 30)
        compare(control.bottomInset, 40)
        compare(topInsetSpy.count, topInsetChanges)
        compare(leftInsetSpy.count, ++leftInsetChanges)
        compare(rightInsetSpy.count, rightInsetChanges)
        compare(bottomInsetSpy.count, bottomInsetChanges)
        compare(control.background.x, 0)
        compare(control.background.y, 0)
        compare(control.background.width, 70)
        compare(control.background.height, 60)

        control.rightInset = undefined
        compare(control.topInset, 0)
        compare(control.leftInset, 0)
        compare(control.rightInset, 0)
        compare(control.bottomInset, 40)
        compare(topInsetSpy.count, topInsetChanges)
        compare(leftInsetSpy.count, leftInsetChanges)
        compare(rightInsetSpy.count, ++rightInsetChanges)
        compare(bottomInsetSpy.count, bottomInsetChanges)
        compare(control.background.x, 0)
        compare(control.background.y, 0)
        compare(control.background.width, 100)
        compare(control.background.height, 60)

        control.bottomInset = undefined
        compare(control.topInset, 0)
        compare(control.leftInset, 0)
        compare(control.rightInset, 0)
        compare(control.bottomInset, 0)
        compare(topInsetSpy.count, topInsetChanges)
        compare(leftInsetSpy.count, leftInsetChanges)
        compare(rightInsetSpy.count, rightInsetChanges)
        compare(bottomInsetSpy.count, ++bottomInsetChanges)
        compare(control.background.x, 0)
        compare(control.background.y, 0)
        compare(control.background.width, 100)
        compare(control.background.height, 100)
    }

    // QTBUG-76369
    Component {
        id: testResizeBackground
        Item {
            width: 200
            height: 200
            property alias textArea: textArea
            ScrollView {
                anchors.fill: parent
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                TextArea {
                    id: textArea
                    // workaround test failing due to default insets on Imagine
                    topInset: undefined
                    leftInset: undefined
                    rightInset: undefined
                    bottomInset: undefined
                    wrapMode : TextEdit.WordWrap
                    readOnly: false
                    selectByMouse: true
                    focus: true
                    text: "test message"

                    background: Rectangle {
                        y: parent.height - height - textArea.bottomPadding / 2
                        implicitWidth: 120
                        height: textArea.activeFocus ? 2 : 1
                    }
                }
            }
        }
    }

    function test_resize_background() {
        let control = createTemporaryObject(testResizeBackground, testCase)

        compare(control.textArea.background.width, control.width)
        compare(control.textArea.background.height, 1)
        control.width = 400
        control.height = 400
        compare(control.textArea.background.width, control.width)
        compare(control.textArea.background.height, 1)
        control.width = 200
        control.height = 200
        compare(control.textArea.background.width, control.width)
        compare(control.textArea.background.height, 1)

        // hasBackgroundWidth=true
        control.textArea.background.width = 1
        compare(control.textArea.background.width, 1)
        compare(control.textArea.background.height, 1)
        control.width = 400
        control.height = 400
        compare(control.textArea.background.width, 1)
        compare(control.textArea.background.height, 1)
        // hasBackgroundHeight=false
        control.textArea.background.height = undefined
        compare(control.textArea.background.width, 1)
        compare(control.textArea.background.height, 0)
        control.textArea.background.y = 0
        compare(control.textArea.background.width, 1)
        compare(control.textArea.background.height, control.height)
        control.width = 200
        control.height = 200
        compare(control.textArea.background.width, 1)
        compare(control.textArea.background.height, control.height)
    }
}
