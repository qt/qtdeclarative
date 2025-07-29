// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtTest
import QtQuick.Templates as T
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl as MaterialImpl

import Qt.test

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "Material"

    function init() {
        // This is particularly important for test_propertyBindingLoop,
        // which relies on binding loop warnings failing the test.
        failOnWarning(/.?/)
    }

    Component {
        id: signalSpyComponent
        SignalSpy {}
    }

    Component {
        id: buttonComponent
        Button { }
    }

    Component {
        id: styledButtonComponent
        Button {
            Material.theme: Material.Dark
            Material.primary: Material.DeepOrange
            Material.accent: Material.DeepPurple
            Material.background: Material.Green
            Material.foreground: Material.Blue
            Material.roundedScale: Material.SmallScale
        }
    }

    Component {
        id: windowComponent
        Window { }
    }

    Component {
        id: buttonLoaderComponent
        Loader {
            active: false
            sourceComponent: Button { }
        }
    }

    Component {
        id: swipeViewComponent
        SwipeView {
            Material.theme: Material.Dark
            Button { }
        }
    }

    Component {
        id: menuComponent
        ApplicationWindow {
            Material.primary: Material.Blue
            Material.accent: Material.Red
            property alias menu: popup
            Menu {
                id: popup
                Material.theme: Material.Dark
                MenuItem { }
            }
        }
    }

    Component {
        id: popupComponent
        ApplicationWindow {
            Material.primary: Material.Blue
            Material.accent: Material.Red
            visible: true
            property alias popup: popupInstance
            property alias label: labelInstance
            property alias label2: labelInstance2
            Popup {
                id: popupInstance
                objectName: "popupQObject"
                Label {
                    id: labelInstance
                    text: "test"
                    color: popupInstance.Material.textSelectionColor
                }
                Component.onCompleted: open()
            }
            T.Popup {
                contentItem: Label {
                    id: labelInstance2
                    text: "test"
                    color: Material.textSelectionColor
                }
                Component.onCompleted: open()
            }
        }
    }

    Component {
        id: comboBoxComponent
        ApplicationWindow {
            width: 200
            height: 200
            visible: true
            Material.primary: Material.Blue
            Material.accent: Material.Red
            property alias combo: box
            ComboBox {
                id: box
                Material.theme: Material.Dark
                model: 1
            }
        }
    }

    Component {
        id: windowPaneComponent
        ApplicationWindow {
            width: 200
            height: 200
            visible: true
            property alias pane: pane
            Pane { id: pane }
        }
    }

    // need to be synced with QQuickMaterialStyle::themeShade()
    function themeshade(theme) {
        if (theme === Material.Light)
            return Material.Shade500
        else
            return Material.Shade200
    }

    function test_defaults() {
        let control = createTemporaryObject(buttonComponent, testCase)
        verify(control)
        verify(control.Material)
        compare(control.Material.primary, Material.color(Material.Indigo))
        compare(control.Material.accent, Material.color(Material.Pink))
        compare(control.Material.foreground, "#dd000000")
        compare(control.Material.background, "#fffbfe")
        compare(control.Material.theme, Material.Light)
        // This doesn't propagate but we check its default anyway.
        compare(control.Material.roundedScale, Material.FullScale)
        compare(control.Material.containerStyle, Material.Filled)
    }

    function test_set() {
        let control = createTemporaryObject(buttonComponent, testCase)
        verify(control)
        control.Material.primary = Material.Green
        control.Material.accent = Material.Brown
        control.Material.background = Material.Red
        control.Material.foreground = Material.Blue
        control.Material.theme = Material.Dark
        control.Material.roundedScale = Material.SmallScale
        compare(control.Material.primary, Material.color(Material.Green))
        compare(control.Material.accent, Material.color(Material.Brown, themeshade(control.Material.theme)))
        compare(control.Material.background, Material.color(Material.Red, themeshade(control.Material.theme)))
        compare(control.Material.foreground, Material.color(Material.Blue))
        compare(control.Material.theme, Material.Dark)
        compare(control.Material.roundedScale, Material.SmallScale)
    }

    function test_reset() {
        let control = createTemporaryObject(styledButtonComponent, testCase)
        verify(control)
        compare(control.Material.primary, Material.color(Material.DeepOrange))
        compare(control.Material.accent, Material.color(Material.DeepPurple, themeshade(control.Material.theme)))
        compare(control.Material.background, Material.color(Material.Green, themeshade(control.Material.theme)))
        compare(control.Material.foreground, Material.color(Material.Blue))
        compare(control.Material.theme, Material.Dark)
        compare(control.Material.roundedScale, Material.SmallScale)
        control.Material.primary = undefined
        control.Material.accent = undefined
        control.Material.background = undefined
        control.Material.foreground = undefined
        control.Material.theme = undefined
        control.Material.roundedScale = undefined
        compare(control.Material.primary, testCase.Material.primary)
        compare(control.Material.accent, testCase.Material.accent)
        compare(control.Material.background, testCase.Material.background)
        compare(control.Material.foreground, testCase.Material.foreground)
        compare(control.Material.theme, testCase.Material.theme)
        // Button's default is FullyRounded, but it specifies that default in QML,
        // which means we have no way of knowing how to reset that in resetRoundedScale().
        compare(control.Material.roundedScale, Material.NotRounded)
    }

    function test_inheritance_data() {
        return [
            { tag: "primary", value1: Material.color(Material.Amber), value2: Material.color(Material.Indigo) },
            { tag: "accent", value1: Material.color(Material.Amber), value2: Material.color(Material.Indigo) },
            { tag: "background", value1: Material.color(Material.Amber), value2: Material.color(Material.Indigo) },
            { tag: "foreground", value1: Material.color(Material.Amber), value2: Material.color(Material.Indigo) },
            { tag: "theme", value1: Material.Dark, value2: Material.Light },
        ]
    }

    function test_inheritance(data) {
        let prop = data.tag
        let parent = createTemporaryObject(buttonComponent, testCase)
        parent.Material[prop] = data.value1
        compare(parent.Material[prop], data.value1)

        let child1 = buttonComponent.createObject(parent)
        compare(child1.Material[prop], data.value1)

        parent.Material[prop] = data.value2
        compare(parent.Material[prop], data.value2)
        compare(child1.Material[prop], data.value2)

        let child2 = buttonComponent.createObject(parent)
        compare(child2.Material[prop], data.value2)

        child2.Material[prop] = data.value1
        compare(child2.Material[prop], data.value1)
        compare(child1.Material[prop], data.value2)
        compare(parent.Material[prop], data.value2)

        parent.Material[prop] = undefined
        verify(parent.Material[prop] !== data.value1)
        verify(parent.Material[prop] !== undefined)
        compare(child1.Material[prop], parent.Material[prop])
        verify(child2.Material[prop] !== parent.Material[prop])

        let grandChild1 = buttonComponent.createObject(child1)
        let grandChild2 = buttonComponent.createObject(child2)
        compare(grandChild1.Material[prop], child1.Material[prop])
        compare(grandChild2.Material[prop], child2.Material[prop])

        let themelessGrandGrandChild = buttonComponent.createObject(grandChild1)
        let grandGrandGrandChild1 = buttonComponent.createObject(themelessGrandGrandChild)
        compare(grandGrandGrandChild1.Material[prop], parent.Material[prop])

        child1.Material[prop] = data.value2
        compare(child1.Material[prop], data.value2)
        compare(grandChild1.Material[prop], data.value2)
        compare(grandGrandGrandChild1.Material[prop], data.value2)
    }

    function test_inheritance_popup_data() {
        return [
            { tag: "primary", value1: Material.color(Material.Amber), value2: Material.color(Material.Indigo) },
            { tag: "accent", value1: Material.color(Material.Amber), value2: Material.color(Material.Indigo) },
            { tag: "theme", value1: Material.Dark, value2: Material.Light },
        ]
    }

    function test_inheritance_popup(data) {
        let prop = data.tag
        let popupObject = createTemporaryObject(popupComponent, testCase)

        if (popupObject.popup.popupType === Popup.Window)
            skip("QTBUG-126713: Palette propagation is currently not supported for QQuickPopupWindows.")

        compare(popupObject.popup.Material.textSelectionColor.toString(), popupObject.Material.textSelectionColor.toString())
        compare(popupObject.label.color.toString(), popupObject.Material.textSelectionColor.toString())
        compare(popupObject.label2.color.toString(), popupObject.Material.textSelectionColor.toString())

        popupObject.Material[prop] = data.value1
        compare(popupObject.Material[prop], data.value1)
        compare(popupObject.popup.Material.textSelectionColor.toString(), popupObject.Material.textSelectionColor.toString())
        compare(popupObject.label.color.toString(), popupObject.Material.textSelectionColor.toString())
        compare(popupObject.label2.color.toString(), popupObject.Material.textSelectionColor.toString())

        popupObject.Material[prop] = data.value2
        compare(popupObject.Material[prop], data.value2)
        compare(popupObject.popup.Material.textSelectionColor.toString(), popupObject.Material.textSelectionColor.toString())
        compare(popupObject.label.color.toString(), popupObject.Material.textSelectionColor.toString())
        compare(popupObject.label2.color.toString(), popupObject.Material.textSelectionColor.toString())
    }

    component StyledChildWindow: Window {
        objectName: "styledChildWindow"

        Material.objectName: objectName + "MaterialAttached"
        Material.theme: Material.Dark
        Material.primary: Material.Brown
        Material.accent: Material.Green
        Material.background: Material.Yellow
        Material.foreground: Material.Grey
    }

    component StyledChildAppWindow: ApplicationWindow {
        objectName: "styledChildAppWindow"

        Material.objectName: objectName + "MaterialAttached"
        Material.theme: Material.Dark
        Material.primary: Material.Brown
        Material.accent: Material.Green
        Material.background: Material.Yellow
        Material.foreground: Material.Grey
    }

    component UnstyledChildWindow: Window {
        objectName: "unstyledChildWindow"
        Material.objectName: objectName + "MaterialAttached"
    }

    component UnstyledChildAppWindow: ApplicationWindow {
        objectName: "unstyledChildAppWindow"
        Material.objectName: objectName + "MaterialAttached"
    }

    Component {
        id: parentWindowComponent

        Window {
            objectName: "rootWindow"

            Material.objectName: objectName + "MaterialAttached"

            property alias styledChildWindow: styledChildWindow
            property alias unstyledChildWindow: unstyledChildWindow

            StyledChildWindow {
                id: styledChildWindow
            }

            UnstyledChildWindow {
                id: unstyledChildWindow
            }
        }
    }

    Component {
        id: parentAppWindowComponent

        ApplicationWindow {
            objectName: "rootAppWindow"

            Material.objectName: objectName + "MaterialAttached"

            property alias styledChildWindow: styledChildWindow
            property alias unstyledChildWindow: unstyledChildWindow

            StyledChildAppWindow {
                id: styledChildWindow
            }

            UnstyledChildAppWindow {
                id: unstyledChildWindow
            }
        }
    }

    Component {
        id: parentMixed1WindowComponent

        ApplicationWindow {
            objectName: "rootAppWindow"

            Material.objectName: objectName + "MaterialAttached"

            property alias styledChildWindow: styledChildWindow
            property alias unstyledChildWindow: unstyledChildWindow

            StyledChildWindow {
                id: styledChildWindow
            }

            UnstyledChildWindow {
                id: unstyledChildWindow
            }
        }
    }

    Component {
        id: parentMixed2WindowComponent

        Window {
            id: rootWindow
            objectName: "rootWindow"

            Material.objectName: objectName + "MaterialAttached"

            property alias styledChildWindow: styledChildWindow
            property alias unstyledChildWindow: unstyledChildWindow

            StyledChildAppWindow {
                id: styledChildWindow
            }

            UnstyledChildAppWindow {
                id: unstyledChildWindow
            }
        }
    }

    function test_window_data() {
        return [
            { tag: "Window", component: parentWindowComponent },
            { tag: "ApplicationWindow", component: parentAppWindowComponent },
            // Test a combination of Window and ApplicationWindow.
            { tag: "mixed-1", component: parentMixed1WindowComponent },
            { tag: "mixed-2", component: parentMixed2WindowComponent },
        ]
    }

    function test_window(data) {
        let parentWindow = createTemporaryObject(data.component, null)
        verify(parentWindow)

        let control = buttonComponent.createObject(parentWindow.contentItem)
        verify(control)
        compare(control.Material.primary, parentWindow.Material.primary)
        compare(control.Material.accent, parentWindow.Material.accent)
        compare(control.Material.background, parentWindow.Material.background)
        compare(control.Material.foreground, parentWindow.Material.foreground)
        compare(control.Material.theme, parentWindow.Material.theme)

        let styledChildWindow = parentWindow.styledChildWindow
        verify(styledChildWindow)
        verify(styledChildWindow.Material.primary !== parentWindow.Material.primary)
        verify(styledChildWindow.Material.accent !== parentWindow.Material.accent)
        verify(styledChildWindow.Material.background !== parentWindow.Material.background)
        verify(styledChildWindow.Material.foreground !== parentWindow.Material.foreground)
        verify(styledChildWindow.Material.theme !== parentWindow.Material.theme)

        let unstyledChildWindow = parentWindow.unstyledChildWindow
        verify(unstyledChildWindow)
        compare(unstyledChildWindow.Material.primary, parentWindow.Material.primary)
        compare(unstyledChildWindow.Material.accent, parentWindow.Material.accent)
        compare(unstyledChildWindow.Material.background, parentWindow.Material.background)
        compare(unstyledChildWindow.Material.foreground, parentWindow.Material.foreground)
        compare(unstyledChildWindow.Material.theme, parentWindow.Material.theme)

        parentWindow.Material.primary = Material.Lime
        compare(control.Material.primary, Material.color(Material.Lime))
        verify(styledChildWindow.Material.primary !== Material.color(Material.Lime))
        compare(unstyledChildWindow.Material.primary, Material.color(Material.Lime))

        parentWindow.Material.accent = Material.Cyan
        compare(control.Material.accent, Material.color(Material.Cyan))
        verify(styledChildWindow.Material.accent !== Material.color(Material.Cyan))
        compare(unstyledChildWindow.Material.accent, Material.color(Material.Cyan))

        parentWindow.Material.background = Material.Indigo
        compare(control.Material.background, Material.color(Material.Indigo))
        verify(styledChildWindow.Material.background !== Material.color(Material.Indigo))
        compare(unstyledChildWindow.Material.background, Material.color(Material.Indigo))

        parentWindow.Material.foreground = Material.Pink
        compare(control.Material.foreground, Material.color(Material.Pink))
        verify(styledChildWindow.Material.foreground !== Material.color(Material.Pink))
        compare(unstyledChildWindow.Material.foreground, Material.color(Material.Pink))

        // Test that theme changes are propagated to child windows.
        // Make sure that this check actually does something (in case the default changes).
        compare(parentWindow.Material.theme, Material.Light)
        // styledChildWindow was already dark, so make it light to verify that it doesn't change.
        styledChildWindow.Material.theme = Material.Light
        // Reset background since Theme affects it and we use it to test that the colors change.
        parentWindow.Material.background = undefined
        // Setting theme to Dark should result in the background colors of control and unstyledChildWindow changing.
        // Note that since Window is unstyled, its actual (background) color won't change.
        parentWindow.Material.theme = Material.Dark
        compare(control.Material.theme, Material.Dark)
        compare(styledChildWindow.Material.theme, Material.Light)
        compare(unstyledChildWindow.Material.theme, Material.Dark)
        // Make sure that the colors actually changed.
        compare(parentWindow.Material.background, Qt.color("#1c1b1f"))
        compare(control.Material.background, parentWindow.Material.background)
        verify(styledChildWindow.Material.background !== parentWindow.Material.background)
        compare(unstyledChildWindow.Material.background, parentWindow.Material.background)
    }

    function test_loader() {
        let control = createTemporaryObject(buttonLoaderComponent, testCase)
        control.Material.primary = Material.Yellow
        control.Material.accent = Material.Lime
        control.Material.background = Material.LightGreen
        control.Material.foreground = Material.LightBlue
        control.active = true
        compare(control.item.Material.primary, Material.color(Material.Yellow))
        compare(control.item.Material.accent, Material.color(Material.Lime))
        compare(control.item.Material.background, Material.color(Material.LightGreen))
        compare(control.item.Material.foreground, Material.color(Material.LightBlue))
        control.Material.primary = Material.Red
        control.Material.accent = Material.Pink
        control.Material.background = Material.Blue
        control.Material.foreground = Material.Green
        compare(control.item.Material.primary, Material.color(Material.Red))
        compare(control.item.Material.accent, Material.color(Material.Pink))
        compare(control.item.Material.background, Material.color(Material.Blue))
        compare(control.item.Material.foreground, Material.color(Material.Green))
        control.active = false
        control.Material.primary = Material.Orange
        control.Material.accent = Material.Brown
        control.Material.background = Material.Red
        control.Material.foreground = Material.Pink
        control.active = true
        compare(control.item.Material.primary, Material.color(Material.Orange))
        compare(control.item.Material.accent, Material.color(Material.Brown))
        compare(control.item.Material.background, Material.color(Material.Red))
        compare(control.item.Material.foreground, Material.color(Material.Pink))
    }

    function test_swipeView() {
        let control = createTemporaryObject(swipeViewComponent, testCase)
        verify(control)
        let child = control.itemAt(0)
        verify(child)
        compare(control.Material.theme, Material.Dark)
        compare(child.Material.theme, Material.Dark)
    }

    function test_menu() {
        let container = createTemporaryObject(menuComponent, testCase)
        verify(container)
        verify(container.menu)

        if (container.menu.popupType === Popup.Window)
            skip("QTBUG-126713: Palette propagation is currently not supported for QQuickPopupWindows.")

        container.menu.open()
        verify(container.menu.visible)
        let child = container.menu.itemAt(0)
        verify(child)
        compare(container.Material.theme, Material.Light)
        compare(container.menu.Material.theme, Material.Dark)
        compare(child.Material.theme, Material.Dark)
        compare(container.Material.primary, Material.color(Material.Blue))
        compare(container.menu.Material.primary, Material.color(Material.Blue))
        compare(child.Material.primary, Material.color(Material.Blue))
        compare(container.Material.accent, Material.color(Material.Red))
        compare(container.menu.Material.accent, Material.color(Material.Red, themeshade(container.menu.Material.theme)))
        compare(child.Material.accent, Material.color(Material.Red, themeshade(child.Material.theme)))
    }

    function test_comboBox() {
        let window = createTemporaryObject(comboBoxComponent, testCase)
        verify(window)
        verify(window.combo)
        waitForRendering(window.combo)
        window.combo.forceActiveFocus()
        verify(window.combo.activeFocus)
        keyClick(Qt.Key_Space)
        let comboPopup = window.combo.popup
        verify(comboPopup.visible)
        let listView = comboPopup.contentItem
        verify(listView)
        let child = listView.contentItem.children[0]
        verify(child)
        compare(window.Material.theme, Material.Light)
        compare(window.combo.Material.theme, Material.Dark)
        compare(child.Material.theme, Material.Dark)
        compare(comboPopup.background.color, comboPopup.Material.dialogColor)
        compare(window.Material.primary, Material.color(Material.Blue))
        compare(window.combo.Material.primary, Material.color(Material.Blue))
        compare(child.Material.primary, Material.color(Material.Blue))
        compare(window.Material.accent, Material.color(Material.Red))
        compare(window.combo.Material.accent, Material.color(Material.Red, themeshade(window.combo.Material.theme)))
        compare(child.Material.accent, Material.color(Material.Red, themeshade(child.Material.theme)))
    }

    function test_windowChange() {
        let ldr = buttonLoaderComponent.createObject()
        verify(ldr)

        let wnd = createTemporaryObject(windowComponent)
        verify(wnd)

        wnd.Material.theme = Material.Dark
        compare(wnd.Material.theme, Material.Dark)

        ldr.active = true
        verify(ldr.item)
        compare(ldr.item.Material.theme, Material.Light)

        ldr.parent = wnd.contentItem
        compare(ldr.item.Material.theme, Material.Dark)
    }

    function test_colors_data() {
        return [
            { tag: "primary" }, { tag: "accent" }, { tag: "background" }, { tag: "foreground" }
        ]
    }

    function test_colors(data) {
        let control = createTemporaryObject(buttonComponent, testCase)
        verify(control)

        let prop = data.tag

        // Material.Color - enum
        control.Material[prop] = Material.Red
        compare(control.Material[prop], "#f44336")

        // Material.Color - string
        control.Material[prop] = "BlueGrey"
        compare(control.Material[prop], "#607d8b")

        // SVG named color
        control.Material[prop] = "tomato"
        compare(control.Material[prop], "#ff6347")

        // #rrggbb
        control.Material[prop] = "#123456"
        compare(control.Material[prop], "#123456")

        // #aarrggbb
        control.Material[prop] = "#12345678"
        compare(control.Material[prop], "#12345678")

        // Qt.rgba() - no alpha
        control.Material[prop] = Qt.rgba(0.5, 0.5, 0.5)
        compare(control.Material[prop], "#808080")

        // Qt.rgba() - with alpha
        control.Material[prop] = Qt.rgba(0.5, 0.5, 0.5, 0.5)
        compare(control.Material[prop], "#80808080")

        // unknown
        ignoreWarning(new RegExp("QML Button: unknown Material." + prop + " value: 123"))
        control.Material[prop] = 123
        ignoreWarning(new RegExp("QML Button: unknown Material." + prop + " value: foo"))
        control.Material[prop] = "foo"
        ignoreWarning(new RegExp("QML Button: unknown Material." + prop + " value: #1"))
        control.Material[prop] = "#1"
    }

    function test_font_data() {
        return [
            {tag: "Button:pixelSize", type: "Button", attribute: "pixelSize", value: 14, window: 20, pane: 10},
            {tag: "Button:weight", type: "Button", attribute: "weight", value: Font.Medium, window: Font.Black, pane: Font.Bold},
            {tag: "Button:capitalization", type: "Button", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "TabButton:pixelSize", type: "TabButton", attribute: "pixelSize", value: 14, window: 20, pane: 10},
            {tag: "TabButton:weight", type: "TabButton", attribute: "weight", value: Font.Medium, window: Font.Black, pane: Font.Bold},
            {tag: "TabButton:capitalization", type: "TabButton", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "ToolButton:pixelSize", type: "ToolButton", attribute: "pixelSize", value: 14, window: 20, pane: 10},
            {tag: "ToolButton:weight", type: "ToolButton", attribute: "weight", value: Font.Medium, window: Font.Black, pane: Font.Bold},
            {tag: "ToolButton:capitalization", type: "ToolButton", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "ItemDelegate:pixelSize", type: "ItemDelegate", attribute: "pixelSize", value: 14, window: 20, pane: 10},
            {tag: "ItemDelegate:weight", type: "ItemDelegate", attribute: "weight", value: Font.Medium, window: Font.Black, pane: Font.Bold},
            {tag: "ItemDelegate:capitalization", type: "ItemDelegate", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "CheckDelegate:pixelSize", type: "CheckDelegate", attribute: "pixelSize", value: 16, window: 20, pane: 10},
            {tag: "CheckDelegate:weight", type: "CheckDelegate", attribute: "weight", value: Font.Normal, window: Font.Black, pane: Font.Bold},
            {tag: "CheckDelegate:capitalization", type: "CheckDelegate", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "RadioDelegate:pixelSize", type: "RadioDelegate", attribute: "pixelSize", value: 16, window: 20, pane: 10},
            {tag: "RadioDelegate:weight", type: "RadioDelegate", attribute: "weight", value: Font.Normal, window: Font.Black, pane: Font.Bold},
            {tag: "RadioDelegate:capitalization", type: "RadioDelegate", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "SwitchDelegate:pixelSize", type: "SwitchDelegate", attribute: "pixelSize", value: 16, window: 20, pane: 10},
            {tag: "SwitchDelegate:weight", type: "SwitchDelegate", attribute: "weight", value: Font.Normal, window: Font.Black, pane: Font.Bold},
            {tag: "SwitchDelegate:capitalization", type: "SwitchDelegate", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "Label:pixelSize", type: "Label", attribute: "pixelSize", value: 14, window: 20, pane: 10},
            {tag: "Label:weight", type: "Label", attribute: "weight", value: Font.Normal, window: Font.Black, pane: Font.Bold},
            {tag: "Label:capitalization", type: "Label", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "CheckBox:pixelSize", type: "CheckBox", attribute: "pixelSize", value: 14, window: 20, pane: 10},
            {tag: "CheckBox:weight", type: "CheckBox", attribute: "weight", value: Font.Normal, window: Font.Black, pane: Font.Bold},
            {tag: "CheckBox:capitalization", type: "CheckBox", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "RadioButton:pixelSize", type: "RadioButton", attribute: "pixelSize", value: 14, window: 20, pane: 10},
            {tag: "RadioButton:weight", type: "RadioButton", attribute: "weight", value: Font.Normal, window: Font.Black, pane: Font.Bold},
            {tag: "RadioButton:capitalization", type: "RadioButton", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "Switch:pixelSize", type: "Switch", attribute: "pixelSize", value: 14, window: 20, pane: 10},
            {tag: "Switch:weight", type: "Switch", attribute: "weight", value: Font.Normal, window: Font.Black, pane: Font.Bold},
            {tag: "Switch:capitalization", type: "Switch", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "MenuItem:pixelSize", type: "MenuItem", attribute: "pixelSize", value: 16, window: 20, pane: 10},
            {tag: "MenuItem:weight", type: "MenuItem", attribute: "weight", value: Font.Normal, window: Font.Black, pane: Font.Bold},
            {tag: "MenuItem:capitalization", type: "MenuItem", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "ComboBox:pixelSize", type: "ComboBox", attribute: "pixelSize", value: 16, window: 20, pane: 10},
            {tag: "ComboBox:weight", type: "ComboBox", attribute: "weight", value: Font.Normal, window: Font.Black, pane: Font.Bold},
            {tag: "ComboBox:capitalization", type: "ComboBox", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "TextField:pixelSize", type: "TextField", attribute: "pixelSize", value: 16, window: 20, pane: 10},
            {tag: "TextField:weight", type: "TextField", attribute: "weight", value: Font.Normal, window: Font.Black, pane: Font.Bold},
            {tag: "TextField:capitalization", type: "TextField", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "TextArea:pixelSize", type: "TextArea", attribute: "pixelSize", value: 16, window: 20, pane: 10},
            {tag: "TextArea:weight", type: "TextArea", attribute: "weight", value: Font.Normal, window: Font.Black, pane: Font.Bold},
            {tag: "TextArea:capitalization", type: "TextArea", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase},

            {tag: "SpinBox:pixelSize", type: "SpinBox", attribute: "pixelSize", value: 16, window: 20, pane: 10},
            {tag: "SpinBox:weight", type: "SpinBox", attribute: "weight", value: Font.Normal, window: Font.Black, pane: Font.Bold},
            {tag: "SpinBox:capitalization", type: "SpinBox", attribute: "capitalization", value: Font.MixedCase, window: Font.Capitalize, pane: Font.AllLowercase}
        ]
    }

    function test_font(data) {
        let window = createTemporaryObject(windowPaneComponent, testCase)
        verify(window)
        verify(window.pane)

        let control = Qt.createQmlObject("import QtQuick.Controls; " + data.type + " { }", window.pane)
        verify(control)

        compare(control.font[data.attribute], data.value)

        window.font[data.attribute] = data.window
        compare(window.font[data.attribute], data.window)
        compare(window.pane.font[data.attribute], data.window)
        compare(control.font[data.attribute], data.window)

        window.pane.font[data.attribute] = data.pane
        compare(window.font[data.attribute], data.window)
        compare(window.pane.font[data.attribute], data.pane)
        compare(control.font[data.attribute], data.pane)

        window.pane.font = undefined
        compare(window.font[data.attribute], data.window)
        compare(window.pane.font[data.attribute], data.window)
        compare(control.font[data.attribute], data.window)
    }

    Component {
        id: backgroundControlsComponent
        ApplicationWindow {
            property Button button: Button { }
            property ComboBox combobox: ComboBox { }
            property Drawer drawer: Drawer { }
            property GroupBox groupbox: GroupBox { Material.elevation: 10 }
            property Frame frame: Frame { Material.elevation: 10 }
            property Menu menu: Menu { }
            property Page page: Page { }
            property Pane pane: Pane { }
            property Popup popup: Popup { }
            property Popup popup_window: Popup { popupType: Popup.Window }
            property TabBar tabbar: TabBar { }
            property ToolBar toolbar: ToolBar { }
            property ToolTip tooltip: ToolTip { }
        }
    }

    function test_background_data() {
        return [
            { tag: "button", inherit: false, wait: 400 },
            { tag: "drawer", inherit: true },
            { tag: "groupbox", inherit: true },
            { tag: "frame", inherit: true },
            { tag: "menu", inherit: true },
            { tag: "page", inherit: true },
            { tag: "pane", inherit: true },
            { tag: "popup", inherit: true },
            { tag: "popup_window", inherit: true },
            { tag: "tabbar", inherit: true },
            { tag: "toolbar", inherit: false },
            { tag: "tooltip", inherit: false }
        ]
    }

    function test_background(data) {
        let window = createTemporaryObject(backgroundControlsComponent, testCase)
        verify(window)

        let control = window[data.tag]
        verify(control)

        control.parent = window.contentItem
        control.visible = true

        let defaultBackground = control.background.color

        window.Material.background = "#ff0000"
        compare(window.color, "#ff0000")

        // For controls that have an animated background color, we wait the length
        // of the color animation to be sure that the color hasn't actually changed.
        if (data.wait)
            wait(data.wait)

        // We want the control's background color to be equal to the window's background
        // color, because we want the color to propagate to items that might actually use
        // it... Button, ComboBox, ToolBar and ToolTip have a special background color,
        // so they don't use the generic background color unless explicitly set, so we
        // compare the actual background rect color instead.
        if (data.inherit)
            compare(control.background.color, "#ff0000")
        else
            compare(control.background.color, defaultBackground)

        control.Material.background = "#0000ff"
        tryCompare(control.background, "color", "#0000ff")
    }

    Component {
        id: busyIndicatorComponent
        BusyIndicator { }
    }

    function test_shade() {
        let control = createTemporaryObject(busyIndicatorComponent, testCase)

        compare(control.contentItem.color.toString(), Material.color(Material.Pink, Material.Shade500))
        control.Material.theme = Material.Dark
        compare(control.contentItem.color.toString(), Material.color(Material.Pink, Material.Shade200))
    }

    // We can't declare components with JS syntax (when creating a data row),
    // so we use introspection to get the list of all components we should test.
    QtObject {
        id: bindingLoopComponents

        property Component row_foregroundToPrimaryTextColor: Item { Material.foreground: Material.primaryTextColor }
        // Not all properties can be bound without binding loops. For example, it's not possible to bind
        // foreground to primaryHighlightedTextColor, because primaryHighlightedTextColor() depends on
        // m_explicitForeground, which is modified when the foreground is set.
        // So, we use background instead.
        property Component row_backgroundToPrimaryHighlightedTextColor: Item { Material.background: Material.primaryHighlightedTextColor }
        property Component row_foregroundToSecondaryTextColor: Item { Material.foreground: Material.secondaryTextColor }
        property Component row_foregroundToSecondaryTextColorWithTheme: Item {
            Material.foreground: Material.theme === Material.Dark ? Material.secondaryTextColor : Material.Red
        }
        property Component row_foregroundToHintTextColor: Item { Material.foreground: Material.secondaryTextColor }
        property Component row_foregroundToTextSelectionColor: Item { Material.foreground: Material.textSelectionColor }
        property Component row_foregroundToDropShadowColor: Item { Material.foreground: Material.dropShadowColor }
        property Component row_foregroundToDividerColor: Item { Material.foreground: Material.dividerColor }
        property Component row_foregroundToIconColor: Item { Material.foreground: Material.iconColor }
        property Component row_foregroundToIconDisabledColor: Item { Material.foreground: Material.iconDisabledColor }
        property Component row_foregroundToFrameColor: Item { Material.foreground: Material.frameColor }
        property Component row_foregroundToRippleColor: Item { Material.foreground: Material.rippleColor }
        property Component row_foregroundToHighlightedRippleColor: Item { Material.foreground: Material.highlightedRippleColor }
        property Component row_foregroundToSwitchUncheckedTrackColor: Item { Material.foreground: Material.switchUncheckedTrackColor }
        property Component row_foregroundToSwitchCheckedTrackColor: Item { Material.foreground: Material.switchCheckedTrackColor }
        property Component row_foregroundToSwitchUncheckedHandleColor: Item { Material.foreground: Material.switchUncheckedHandleColor }
        property Component row_foregroundToSwitchCheckedHandleColor: Item { Material.foreground: Material.switchCheckedHandleColor }
        property Component row_foregroundToSwitchDisabledTrackColor: Item { Material.foreground: Material.switchDisabledTrackColor }
        property Component row_foregroundToSwitchDisabledHandleColor: Item { Material.foreground: Material.switchDisabledHandleColor }
        property Component row_foregroundToScrollBarColor: Item { Material.foreground: Material.scrollBarColor }
        property Component row_foregroundToScrollBarHoveredColor: Item { Material.foreground: Material.scrollBarHoveredColor }
        property Component row_foregroundToScrollBarPressedColor: Item { Material.foreground: Material.scrollBarPressedColor }
        property Component row_foregroundToDialogColor: Item { Material.foreground: Material.dialogColor }
        property Component row_foregroundToBackgroundDimColor: Item { Material.foreground: Material.backgroundDimColor }
        property Component row_foregroundToListHighlightColor: Item { Material.foreground: Material.listHighlightColor }
        property Component row_foregroundToTooltipColor: Item { Material.foreground: Material.tooltipColor }
        property Component row_foregroundToToolBarColor: Item { Material.foreground: Material.toolBarColor }
        property Component row_backgroundToToolTextColor: Item { Material.background: Material.toolTextColor }
        property Component row_foregroundToSpinBoxDisabledIconColor: Item { Material.foreground: Material.spinBoxDisabledIconColor }
        property Component row_foregroundToSliderDisableColor: Item { Material.foreground: Material.sliderDisableColor }
    }

    function test_propertyBindingLoop_data() {
        let data = []
        for (let propertyName in bindingLoopComponents) {
            if (!propertyName.startsWith("row_") || propertyName.endsWith("Changed"))
                continue

            let row = {}
            row.tag = propertyName.substr(4)
            row.component = bindingLoopComponents[propertyName]
            data.push(row)
        }
        return data
    }

    /*
        Test that binding attached Material properties to other (private, non-settable)
        Material properties does not result in a binding loop.
    */
    function test_propertyBindingLoop(data) {
        let item = createTemporaryObject(data.component, testCase)
        verify(item)
    }

    Component {
        id: sliderTickMarks
        Slider {
        }
    }

    function test_sliderTickMarks_data() {
        return [
            { // 1000 / 100 == 10 tick marks
                to: 1000,
                from: 0,
                stepSize: 100,
                snapMode: Slider.SnapAlways,
                expectTickmarks: true
            },
            { // 10 / 1 == 10 tick marks
                to: 10,
                from: 0,
                stepSize: 1,
                snapMode: Slider.SnapAlways,
                expectTickmarks: true
            },
            { // Should still be within the default backgrounds width/height ratio
                to: 50,
                from: 10,
                stepSize: 1,
                snapMode: Slider.SnapAlways,
                expectTickmarks: true
            },
            { // Math.abs(.0 - 10.0) % 3 != 0 (aka not divisible)
                to: 10,
                from: 0,
                stepSize: 3,
                snapMode: Slider.SnapAlways,
                expectTickmarks: false
            },
            { // Math.abs(0 - 810.0) % 100 != 0 (aka not divisible)
                to: 810,
                from: 0,
                stepSize: 100,
                snapMode: Slider.SnapAlways,
                expectTickmarks: false
            },
            { // stepSize isn't positive
                to: 10,
                from: 0,
                stepSize: -1,
                snapMode: Slider.SnapAlways,
                expectTickmarks: false
            },
            { // stepSize isn't positive
                to: 10,
                from: 0,
                stepSize: 0,
                snapMode: Slider.SnapAlways,
                expectTickmarks: false
            },
            { // Number of tick marks would exceed the default width/height ratio
                to: 100,
                from: 10,
                stepSize: 1,
                snapMode: Slider.SnapAlways,
                expectTickmarks: false
            },
            { // snapMode != Slider.SnapAlways
                to: 10,
                from: 0,
                stepSize: 1,
                snapMode: Slider.NoSnap,
                expectTickmarks: false
            }
        ]
    }

    function test_sliderTickMarks(data) {
        let params = {to: data.to, from: data.from, stepSize: data.stepSize, snapMode: data.snapMode}
        let item = createTemporaryObject(sliderTickMarks, testCase, params)
        verify(item)
        compare(item["__isDiscrete"], data.expectTickmarks)
    }

    Component {
        id: textFieldComponent
        TextField {}
    }

    Component {
        id: textAreaComponent
        TextArea {}
    }

    function test_textFieldPlaceholderTextHorizontalAlignment_data() {
        return [
            { tag: "AlignLeft", horizontalAlignment: TextField.AlignLeft },
            { tag: "AlignHCenter", horizontalAlignment: TextField.AlignHCenter },
            { tag: "AlignRight", horizontalAlignment: TextField.AlignRight }
        ]
    }

    function test_textFieldPlaceholderTextHorizontalAlignment(data) {
        // The placeholder text should always be near the left side of the TextField, regardless of its horizontalAlignment.
        let textField = createTemporaryObject(textFieldComponent, testCase, {
            placeholderText: "TextField",
            horizontalAlignment: data.horizontalAlignment
        })
        verify(textField)
        let placeholderTextItem = textField.children[0]
        verify(placeholderTextItem as MaterialImpl.FloatingPlaceholderText)
        compare(placeholderTextItem.horizontalAlignment, data.horizontalAlignment)

        textField.forceActiveFocus()
        compare(placeholderTextItem.horizontalAlignment, data.horizontalAlignment)
        textField.destroy()
    }

    function test_textAreaPlaceholderTextHorizontalAlignment_data() {
        return [
            { tag: "AlignLeft", horizontalAlignment: TextArea.AlignLeft },
            { tag: "AlignHCenter", horizontalAlignment: TextArea.AlignHCenter },
            { tag: "AlignRight", horizontalAlignment: TextArea.AlignRight },
            { tag: "AlignJustify", horizontalAlignment: TextArea.AlignJustify }
        ]
    }

    function test_textAreaPlaceholderTextHorizontalAlignment(data) {
        // The placeholder text should always be near the left side of the TextArea, regardless of its horizontalAlignment.
        let textArea = createTemporaryObject(textAreaComponent, testCase, {
            placeholderText: "TextArea",
            horizontalAlignment: data.horizontalAlignment
        })
        verify(textArea)
        let placeholderTextItem = textArea.children[0]
        verify(placeholderTextItem as MaterialImpl.FloatingPlaceholderText)
        compare(placeholderTextItem.horizontalAlignment, data.horizontalAlignment)

        textArea.forceActiveFocus()
        compare(placeholderTextItem.horizontalAlignment, data.horizontalAlignment)
    }

    function test_placeholderTextPos() {
        {
            // The non-floating placeholder text should be in the middle of TextField regardless of its height.
            let textField = createTemporaryObject(textFieldComponent, testCase, { placeholderText: "TextField" })
            verify(textField)
            let placeholderTextItem = textField.children[0]
            verify(placeholderTextItem as MaterialImpl.FloatingPlaceholderText)
            compare(placeholderTextItem.y, (textField.height - placeholderTextItem.height) / 2)
            textField.height = 10
            compare(placeholderTextItem.y, (textField.height - placeholderTextItem.height) / 2)
            textField.destroy()
        }

        {
            // The non-floating placeholder text should be near the top of TextArea while it has room...
            let textArea = createTemporaryObject(textAreaComponent, testCase, { placeholderText: "TextArea" })
            verify(textArea)
            let placeholderTextItem = textArea.children[0]
            verify(placeholderTextItem as MaterialImpl.FloatingPlaceholderText)
            compare(placeholderTextItem.y, (placeholderTextItem.controlImplicitBackgroundHeight - placeholderTextItem.largestHeight) / 2)

            // ... also when it has a lot of room...
            textArea.height = 200
            compare(placeholderTextItem.y, (placeholderTextItem.controlImplicitBackgroundHeight - placeholderTextItem.largestHeight) / 2)

            // ... but when it doesn't have room, it should start behaving like TextField's.
            textArea.height = 10
            compare(placeholderTextItem.y, (textArea.height - placeholderTextItem.height) / 2)
        }
    }

    function test_outlinedPlaceholderTextPosWithPadding_data() {
        return [
            { tag: "TextField, leftPadding=0", component: textFieldComponent, leftPadding: 0 },
            { tag: "TextField, rightPadding=0", component: textFieldComponent, rightPadding: 0 },
            { tag: "TextField, leftPadding=20", component: textFieldComponent, leftPadding: 20 },
            { tag: "TextField, rightPadding=20", component: textFieldComponent, rightPadding: 20 },
            { tag: "TextArea, leftPadding=0", component: textAreaComponent, leftPadding: 0 },
            { tag: "TextArea, rightPadding=0", component: textAreaComponent, rightPadding: 0 },
            { tag: "TextArea, leftPadding=20", component: textAreaComponent, leftPadding: 20 },
            { tag: "TextArea, rightPadding=20", component: textAreaComponent, rightPadding: 20 },
        ]
    }

    function test_outlinedPlaceholderTextPosWithPadding(data) {
        let control = createTemporaryObject(data.component, testCase, {
            text: "Text",
            placeholderText: "Enter text..."
        })
        verify(control)

        // Work around QTBUG-99231.
        if (data.leftPadding !== undefined)
            control.leftPadding = data.leftPadding
        if (data.rightPadding !== undefined)
            control.rightPadding = data.rightPadding

        let placeholderTextItem = control.children[0]
        verify(placeholderTextItem as MaterialImpl.FloatingPlaceholderText)
        // This is the default value returned by textFieldHorizontalPadding when using a non-dense variant.
        compare(placeholderTextItem.x, 16)

        // When the text is cleared, the placeholder text should appear in the previous text position.
        // If left padding is set, use its value; otherwise, use the default from textFieldHorizontalPadding.
        control.text = ""
        compare(placeholderTextItem.x, data.leftPadding != undefined ? data.leftPadding : 16)
    }

    Component {
        id: flickableTextAreaComponent

        Flickable {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 20
            width: 180
            height: 100

            TextArea.flickable: TextArea {
                placeholderText: "Type something..."
                text: "a\nb\nc\nd\ne\nf\ng\nh\ni\nj\nk\nl\nm\nn"
            }
        }
    }

    function test_placeholderTextInFlickable() {
        let flickable = createTemporaryObject(flickableTextAreaComponent, testCase)
        verify(flickable)

        let textArea = flickable.TextArea.flickable
        verify(textArea)
        let placeholderTextItem = flickable.children[2]
        verify(placeholderTextItem as MaterialImpl.FloatingPlaceholderText)

        // The placeholder text should always float at a fixed position at the top
        // when text has been set, even when it's in a Flickable.
        flickable.contentY = -50
        compare(placeholderTextItem.y, -placeholderTextItem.largestHeight / 2)
        flickable.contentY = 0

        // When the text is cleared, it shouldn't float.
        flickable.height = 160
        textArea.text = ""
        compare(placeholderTextItem.y, (placeholderTextItem.controlImplicitBackgroundHeight - placeholderTextItem.largestHeight) / 2)
        // The background outline gap should be closed.
        let textContainer = flickable.children[1]
        verify(textContainer as MaterialImpl.MaterialTextContainer)
        compare(textContainer.focusAnimationProgress, 0)
    }

    function test_outlinedTextAreaInFlickablePlaceholderTextClipping() {
        let flickable = createTemporaryObject(flickableTextAreaComponent, testCase)
        verify(flickable)

        let textArea = flickable.TextArea.flickable
        verify(textArea)
        let placeholderTextItem = flickable.children[2]
        verify(placeholderTextItem as MaterialImpl.FloatingPlaceholderText)
        compare(textArea.Material.containerStyle, Material.Outlined)
        // The Flickable doesn't clip at the moment so topInset should be 0.
        compare(textArea.topInset, 0)
        compare(textArea.topPadding, (textArea.implicitBackgroundHeight - placeholderTextItem.largestHeight) / 2)

        // topInset should now be half the placeholder text's height,
        // and topPadding adjusted accordingly.
        flickable.clip = true
        compare(textArea.topInset, placeholderTextItem.largestHeight / 2)
        compare(textArea.topPadding, ((textArea.implicitBackgroundHeight - placeholderTextItem.largestHeight) / 2) + textArea.topInset)

        // When the text is cleared, the placeholder text shouldn't float, but it should still be accounted for
        // to avoid it causing jumps in layout sizes, for example.
        const initialText = textArea.text
        textArea.text = ""
        compare(textArea.topPadding, ((textArea.implicitBackgroundHeight - placeholderTextItem.largestHeight) / 2) + textArea.topInset)

        flickable.clip = false
        compare(textArea.topInset, 0)
        compare(textArea.topPadding, (textArea.implicitBackgroundHeight - placeholderTextItem.largestHeight) / 2)
    }

    function test_outlinedTextAreaPlaceholderTextClipping() {
        let textArea = createTemporaryObject(textAreaComponent, testCase, {
            placeholderText: "Type something",
            text: "Text"
        })
        verify(textArea)
        let placeholderTextItem = textArea.children[0]
        verify(placeholderTextItem as MaterialImpl.FloatingPlaceholderText)
        compare(textArea.topInset, 0)
        compare(textArea.topPadding, (textArea.implicitBackgroundHeight - placeholderTextItem.largestHeight) / 2)

        // topInset should now be half the placeholder text's height, and topPadding adjusted accordingly.
        textArea.clip = true
        compare(textArea.topInset, placeholderTextItem.largestHeight / 2)
        compare(textArea.topPadding, ((textArea.implicitBackgroundHeight - placeholderTextItem.largestHeight) / 2) + textArea.topInset)
    }

    function test_outlinedTextFieldPlaceholderTextClipping() {
        let textField = createTemporaryObject(textFieldComponent, testCase, {
            placeholderText: "Type something",
            text: "Text"
        })
        verify(textField)
        let placeholderTextItem = textField.children[0]
        verify(placeholderTextItem as MaterialImpl.FloatingPlaceholderText)
        compare(textField.topInset, 0)
        compare(textField.topPadding, textField.Material.textFieldVerticalPadding)

        // topInset should now be half the placeholder text's height, and topPadding adjusted accordingly.
        textField.clip = true
        compare(textField.topInset, placeholderTextItem.largestHeight / 2)
        compare(textField.topPadding, textField.Material.textFieldVerticalPadding + textField.topInset)
    }

    function test_flatButton() {
        let button = createTemporaryObject(buttonComponent, testCase, { flat: true })
        verify(button)
        // A flat button should be transparent by default.
        compare(button.background.color, "#00000000")

        // However, if a background color is explicitly specified, it should be respected.
        button.Material.background = "#ff6347"
        compare(button.background.color, "#ff6347")

        // The text should be legible when it's highlighted.
        button.Material.background = undefined
        button.highlighted = true
        compare(button.background.color.a, 0.25)
        compare(button.contentItem.color.a, 1)
    }

    Component {
        id: itemPropagationToComboBoxPopup

        Rectangle {
            id: rectangle
            objectName: "rectangle"
            anchors.fill: parent
            color: "#444"

            property alias comboBox: comboBox

            Material.theme: Material.Dark

            ComboBox {
                id: comboBox
                objectName: "comboBox"
                model: 3
                popup.background.objectName: "comboBoxPopupBackground"
                popup.contentItem.objectName: "comboBoxPopupContentItem"
            }
        }
    }

    function test_itemPropagationToComboBoxPopup() {
        let rect = createTemporaryObject(itemPropagationToComboBoxPopup, testCase)
        verify(rect)

        let comboBox = rect.comboBox
        mouseClick(comboBox)
        let comboBoxPopup = comboBox.popup
        tryCompare(comboBoxPopup, "opened", true)
        compare(comboBoxPopup.background.color, comboBoxPopup.Material.dialogColor)
    }

    function test_nullTextAreaBackground() {
        let textArea = createTemporaryObject(textAreaComponent, testCase)
        verify(textArea)
        // Store the placeholder text item before we set the background to null,
        // because it will be unparented at that point.
        let placeholderTextItem = textArea.children[0]
        verify(placeholderTextItem as MaterialImpl.FloatingPlaceholderText)
        // Assigning null to the background shouldn't cause any warnings,
        // it should just hide the placeholder text item, since it has nothing to anchor to.
        // Note that we can't use the properties argument of createTemporaryObject due to QTBUG-117201.
        textArea.background = null
        verify(!placeholderTextItem.visible)
    }

    Component {
        id: textFieldAndButtonComponent

        FocusScope {
            focus: true
            anchors.fill: parent

            property alias textField: textField
            property alias button: button

            Keys.onEscapePressed: function (event) {
                event.accepted = true
                button.forceActiveFocus()
                textField.forceActiveFocus()
            }

            TextField {
                id: textField
                focus: true
                placeholderText: "placeholderText"
                anchors.fill: parent
            }

            Button {
                id: button
                anchors.right: parent.right
                text: focus ? "focus" : "no focus"
            }
        }
    }

    // QTBUG-118889
    function test_focusChanges() {
        let focusScope = createTemporaryObject(textFieldAndButtonComponent, testCase)
        verify(focusScope)
        testCase.Window.window.requestActivate()
        tryCompare(testCase.Window.window, "active", true)

        let textField = focusScope.textField
        verify(textField.activeFocus)
        let textFieldActiveFocusSpy = signalSpyComponent.createObject(textField,
            { target: textField, signalName: "activeFocusChanged" })
        verify(textFieldActiveFocusSpy.valid)

        let button = focusScope.button
        let buttonActiveFocusSpy = signalSpyComponent.createObject(button,
            { target: button, signalName: "activeFocusChanged" })
        verify(buttonActiveFocusSpy.valid)

        // Shouldn't assert after quickly switching focus.
        keyClick(Qt.Key_Escape)
        // true => false => true.
        compare(textFieldActiveFocusSpy.count, 2)
        // false => true => false.
        compare(buttonActiveFocusSpy.count, 2)
    }

    Component {
        id: childWindowComponent

        ApplicationWindow {
            objectName: "parentWindow"
            property alias childWindow: childWindow

            Material.theme: Material.Dark
            Material.primary: Material.Brown
            Material.accent: Material.Green
            Material.background: Material.Yellow
            Material.foreground: Material.Grey

            ApplicationWindow {
                id: childWindow
                objectName: "childWindow"
            }
        }
    }

    function test_windowBackgroundColorPropagation() {
        let parentWindow = createTemporaryObject(childWindowComponent, testCase)
        verify(parentWindow)

        let childWindow = parentWindow.childWindow
        compare(childWindow.Material.theme, Material.Dark)
    }

    Component {
        id: themePropagationWithBehaviorComponent

        ApplicationWindow {
            width: 200
            height: 200
            visible: true

            Material.theme: Material.Dark

            property alias listView: listView

            ListView {
                id: listView
                anchors.fill: parent
                header: Text {
                    text: `Material.theme for header is ${Material.theme} - should be 1`

                    Rectangle {
                        anchors.fill: parent
                        z: -1
                    }

                    Material.elevation: 6
                    // Having this would break the theme (QTBUG-122783)
                    Behavior on Material.elevation {}
                }
            }
        }
    }

    function test_themePropagationWithBehavior() {
        let window = createTemporaryObject(themePropagationWithBehaviorComponent, testCase)
        verify(window)

        let headerItem = window.listView.headerItem
        compare(headerItem.Material.theme, Material.Dark)
    }

    Component {
        id: systemThemeComponent

        ApplicationWindow {
            width: 200
            height: 200
            visible: true
            Material.theme: Material.System
        }
    }

    function test_systemTheme() {
        let window = createTemporaryObject(systemThemeComponent, testCase)
        verify(window)

        const toggleTheme = (theme) => (theme === Material.Dark) ? Material.Light : Material.Dark

        TestHelper.platformTheme = toggleTheme(TestHelper.platformTheme)
        tryCompare(window.Material, "theme", TestHelper.platformTheme)

        TestHelper.platformTheme = toggleTheme(TestHelper.platformTheme)
        tryCompare(window.Material, "theme", TestHelper.platformTheme)
    }

    // QTBUG-85860
    function test_busyIndicatorRunningChangedQuickly() {
        let busyIndicator = createTemporaryObject(busyIndicatorComponent, testCase)
        verify(busyIndicator)

        busyIndicator.running = false
        busyIndicator.running = true

        tryCompare(busyIndicator.contentItem, "visible", true)
    }

    Component {
        id: primaryColorAsBackgroundComponent

        ApplicationWindow {
            property alias container: container
            property alias label: label
            width: 200
            height: 200
            visible: true
            Material.theme: Material.Dark
            Material.background: Material.primary
            Rectangle {
                id: container
                anchors.fill: parent
                color: Material.background
                Text {
                    id: label
                    text: "TEXT"
                    color: Material.foreground
                }
            }
        }
    }

    function test_primaryColorAsBackground_data() {
        return [
            { primary: Material.Red,        expectedForeground: "#ffffff" },
            { primary: Material.Pink,       expectedForeground: "#ffffff" },
            { primary: Material.Purple,     expectedForeground: "#ffffff" },
            { primary: Material.DeepPurple, expectedForeground: "#ffffff" },
            { primary: Material.Indigo,     expectedForeground: "#ffffff" },
            { primary: Material.Blue,       expectedForeground: "#ffffff" },
            { primary: Material.Teal,       expectedForeground: "#ffffff" },
            { primary: Material.DeepOrange, expectedForeground: "#ffffff" },
            { primary: Material.Brown,      expectedForeground: "#ffffff" },
            { primary: Material.BlueGrey,   expectedForeground: "#ffffff" },
            { primary: Material.LightBlue,  expectedForeground: "#dd000000" },
            { primary: Material.Cyan,       expectedForeground: "#dd000000" },
            { primary: Material.Green,      expectedForeground: "#dd000000" },
            { primary: Material.LightGreen, expectedForeground: "#dd000000" },
            { primary: Material.Lime,       expectedForeground: "#dd000000" },
            { primary: Material.Yellow,     expectedForeground: "#dd000000" },
            { primary: Material.Amber,      expectedForeground: "#dd000000" },
            { primary: Material.Orange,     expectedForeground: "#dd000000" },
            { primary: Material.Grey,       expectedForeground: "#dd000000" },
            // The below color (#F44346) corresponds to Material.Red but still
            // it shall be considered as the custom primary color
            { primary: "#F44336",           expectedForegroundLight: "#dd000000",
                                            expectedForegroundDark: "#ffffff"},
            { primary: "#7938b6",           expectedForegroundLight: "#dd000000",
                                            expectedForegroundDark: "#ffffff"}
        ]
    }

    function test_primaryColorAsBackground(data) {
        let window = createTemporaryObject(primaryColorAsBackgroundComponent, testCase)
        verify(window)

        window.Material.primary = data.primary
        let customPrimary = !Number.isInteger(data.primary)
        let expectedBackground = customPrimary ?  window.Material.primary :
                            window.Material.color(data.primary, Material.Shade200)
        let expectedForeground = customPrimary ? data.expectedForegroundDark : data.expectedForeground
        compare(window.Material.background.toString(), expectedBackground)
        compare(window.Material.foreground.toString(), expectedForeground)
        compare(window.container.Material.background.toString(), expectedBackground)
        compare(window.container.Material.foreground.toString(), expectedForeground)
        compare(window.label.color.toString(), expectedForeground)

        window.Material.theme = Material.Light
        if (!customPrimary)
            expectedBackground = window.Material.color(data.primary, Material.Shade500)
        expectedForeground = customPrimary ? data.expectedForegroundLight : data.expectedForeground
        compare(window.Material.background.toString(), expectedBackground)
        compare(window.Material.foreground.toString(), expectedForeground)
        compare(window.container.Material.background.toString(), expectedBackground)
        compare(window.container.Material.foreground.toString(), expectedForeground)
        compare(window.label.color.toString(), expectedForeground)
    }

    Component {
        id: labelInPopupComponent

        Popup {
            id: popupInstance

            property alias label: label

            Material.theme: Material.Dark

            Label {
                id: label
                text: "Should have a dark theme"
            }
        }
    }

    function test_popupPropagatesToChildItem() {
        let popup = createTemporaryObject(labelInPopupComponent, testCase)
        verify(popup)

        compare(popup.Material.theme, Material.Dark)
        compare(popup.label.Material.theme, Material.Dark)

        popup.Material.theme = Material.Light
        compare(popup.Material.theme, Material.Light)
        compare(popup.label.Material.theme, Material.Light)
    }
}
