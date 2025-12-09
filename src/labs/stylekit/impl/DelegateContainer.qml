// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Effects
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

// This container takes care of loading, positioning, and resizing the delegate.
Item {
    id: root

    /* If an implicit size has been set the style, it wins.
     * Otherwise we use the implicit size of the instantiated delegate. */
    implicitWidth: delegateProperties.implicitWidth > 0
                      ? delegateProperties.implicitWidth
                      : delegateInstance
                        ? delegateInstance.implicitWidth
                        : 0

    implicitHeight: delegateProperties.implicitHeight > 0
                       ? delegateProperties.implicitHeight
                       : delegateInstance
                         ? delegateInstance.implicitHeight
                         : 0

    scale: delegateProperties.scale
    rotation: delegateProperties.rotation
    visible: delegateProperties.visible

    required property QtObject parentControl
    required property StyleKitDelegateProperties delegateProperties

    property real leftMargin: delegateProperties.leftMargin
    property real rightMargin: delegateProperties.rightMargin
    property real topMargin: delegateProperties.topMargin
    property real bottomMargin: delegateProperties.bottomMargin

    readonly property real marginWidth: leftMargin + rightMargin
    readonly property real marginHeight: topMargin + bottomMargin
    readonly property real subDelegateWidth: width
    readonly property real subDelegateHeight: height

    onDelegatePropertiesChanged: {
        instantiateDelegate()
        instantiateShadowDelegate();
    }

    onVisibleChanged: {
        instantiateDelegate()
        instantiateShadowDelegate();
     }

    Connections {
        target: delegateProperties
        function onDelegateChanged() { instantiateDelegate() }
    }

    Connections {
        target: delegateProperties.shadow
        function onDelegateChanged() { instantiateShadowDelegate() }
        function onColorChanged() { instantiateShadowDelegate() }
        function onOpacityChanged() { instantiateShadowDelegate() }
        function onVisibleChanged() { instantiateShadowDelegate() }
    }

    Item {
        id: centerContainer
        width: root.subDelegateWidth
        height: root.subDelegateHeight
    }

    // Uncomment to draw overlay:
    // Rectangle {
    //     id: debugRectForMargins
    //     x: centerContainer.x
    //     y: centerContainer.y
    //     z: 5
    //     width: root.subDelegateWidth
    //     height: root.subDelegateHeight
    //     color: "transparent"
    //     border.color: "limegreen"
    //     border.width: 1
    // }

    // Rectangle {
    //     id: debugRectForDelegate
    //     z: 5
    //     width: root.width
    //     height: root.height
    //     color: "transparent"
    //     border.color: "magenta"
    //     border.width: 1
    // }

    // ------------------------------------------

    property Item delegateInstance: null
    property Component effectiveDelegate: null

    // onImplicitWidthChanged: {
    //     if (parentControl && parentControl instanceof T.CheckBox) {
    //         print("DelegateContainer delegateInstance changed to " + delegateInstance)
    //         print("delegate instance implicitWidth: " + (delegateInstance ? delegateInstance.implicitWidth : "N/A"))
    //     }
    // }
    // onWidthChanged: {
    //     if (parentControl && parentControl instanceof T.CheckBox) {
    //         print("DelegateContainer width changed to " + width)
    //         print("delegate instance width: " + (delegateInstance ? delegateInstance.width : "N/A"))
    //     }
    // }

    function instantiateDelegate() {
        if (!visible || !StyleKit.styleLoaded())
            return

        let delegate = root.delegateProperties.delegate
        if (!delegate)
            delegate = DelegateSingleton.defaultDelegate
        if (delegate === effectiveDelegate)
            return

        effectiveDelegate = delegate
        let prevInstance = delegateInstance
        if (delegate)
            delegateInstance = delegate
                    ? delegate.createObject(
                          centerContainer,
                          { delegateProperties: root.delegateProperties, control: root.parentControl })
                    : null

        if (prevInstance)
            prevInstance.destroy()
    }

    // ------------------------------------------

    property Item shadowInstance: null
    property Component effectiveShadowDelegate: null

    function instantiateShadowDelegate() {
        if (!visible || !StyleKit.styleLoaded())
            return

        const shadowProps = root.delegateProperties.shadow
        const shadowVisible = shadowProps.visible && shadowProps.color.a !== 0 && shadowProps.opacity !== 0
        if (shadowInstance)
            shadowInstance.visible = shadowVisible

        if (!shadowVisible)
            return

        const delegate = shadowProps.delegate
                       ? shadowProps.delegate : DelegateSingleton.defaultShadowDelegate
        if (delegate === effectiveShadowDelegate)
            return

        let prevDelegateInstance = shadowInstance
        effectiveShadowDelegate = delegate
        shadowInstance = delegate
                ? delegate.createObject(
                      root,
                      { z: -1, delegateProperties: root.delegateProperties, control: root.parentControl })
                : null

        if (prevDelegateInstance)
            prevDelegateInstance.destroy()
    }

}
