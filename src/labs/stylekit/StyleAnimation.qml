// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick

/*!
    \qmltype StyleAnimation
    \inqmlmodule Qt.labs.StyleKit
    \inherits ParallelAnimation
    \brief Animates style property changes during state transitions.

    \labs
*/

ParallelAnimation {
    id: root
    property alias easing: colorAnimation.easing
    property alias duration: colorAnimation.duration

    property bool animateColors: false
    property bool animateControlGeometry: false

    property bool animateBackgroundColors: false
    property bool animateBackgroundGeometry: false
    property bool animateBackgroundRadii: false
    property bool animateBackgroundBorder: false
    property bool animateBackgroundShadow: false
    property bool animateBackgroundScaleAndRotation: false

    property bool animateHandleColors: false
    property bool animateHandleGeometry: false
    property bool animateHandleRadii: false
    property bool animateHandleBorder: false
    property bool animateHandleShadow: false
    property bool animateHandleScaleAndRotation: false

    property bool animateIndicatorColors: false
    property bool animateIndicatorGeometry: false
    property bool animateIndicatorBorder: false
    property bool animateIndicatorRadii: false
    property bool animateIndicatorShadow: false
    property bool animateIndicatorScaleAndRotation: false

    readonly property string __geometryProps:
        "$.implicitWidth, $.implicitHeight, "
        + "$.margins, $.leftMargin, $.rightMargin, $.topMargin, $.bottomMargin, "
        + "$.leftPadding, $.rightPadding, $.topPadding, $.bottomPadding, "
        + "$.minimumWidth, "
    readonly property string __colorProps:
        "$.color, $.border.color, $.image.color, $.shadow.color, "
    readonly property string __radiiProps:
        "$.radius, $.topLeftRadius, $.topRightRadius, $.bottomLeftRadius, $.bottomRightRadius, "
    readonly property string __scaleAndRotationProps: "$.scale, $.rotation, "
    readonly property string __borderProps: "$.border.width, "
    readonly property string __shadowProps:
        "$.shadow.verticalOffset, $.shadow.horizontalOffset, "
        + "$.shadow.scale, $.shadow.blur, "


    function __animateIndicators(doAnimate, anim, props) {
        if (!doAnimate)
            return
        anim.properties += props.replace(/\$/g, "indicator")
        anim.properties += props.replace(/\$/g, "indicator.foreground")
        anim.properties += props.replace(/\$/g, "indicator.up")
        anim.properties += props.replace(/\$/g, "indicator.up.foreground")
        anim.properties += props.replace(/\$/g, "indicator.down")
        anim.properties += props.replace(/\$/g, "indicator.down.foreground")
    }

    function __animateHandles(doAnimate, anim, props) {
        if (!doAnimate)
            return
        anim.properties += props.replace(/\$/g, "handle")
        anim.properties += props.replace(/\$/g, "handle.first")
        anim.properties += props.replace(/\$/g, "handle.second")
    }

    Component.onCompleted: {
        if (animateControlGeometry)
            numberAnimation.properties += "spacing, padding, leftPadding, rightPadding, topPadding, bottomPadding"

        if (animateBackgroundGeometry)
            numberAnimation.properties += __geometryProps.replace(/\$/g, "background")
        if (animateBackgroundRadii)
            numberAnimation.properties += __radiiProps.replace(/\$/g, "background")
        if (animateBackgroundBorder)
            numberAnimation.properties += __borderProps.replace(/\$/g, "background")
        if (animateBackgroundShadow)
            numberAnimation.properties += __shadowProps.replace(/\$/g, "background")
        if (animateBackgroundScaleAndRotation)
            numberAnimation.properties += __scaleAndRotationProps.replace(/\$/g, "background")

        __animateIndicators(animateIndicatorGeometry, numberAnimation, __geometryProps)
        __animateIndicators(animateIndicatorRadii, numberAnimation, __radiiProps)
        __animateIndicators(animateIndicatorBorder, numberAnimation, __borderProps)
        __animateIndicators(animateIndicatorShadow, numberAnimation, __shadowProps)
        __animateIndicators(animateIndicatorScaleAndRotation, numberAnimation, __scaleAndRotationProps)

        __animateHandles(animateHandleGeometry, numberAnimation, __geometryProps)
        __animateHandles(animateHandleRadii, numberAnimation, __radiiProps)
        __animateHandles(animateHandleBorder, numberAnimation, __borderProps)
        __animateHandles(animateHandleShadow, numberAnimation, __shadowProps)
        __animateHandles(animateHandleScaleAndRotation, numberAnimation, __scaleAndRotationProps)

        if (!animateColors) {
            /* Note: a ColorAnimation will animate all colors by default if 'properties'
             * is empty. So we take advantage of that, and only add the per-delegate properties
             * when not all colors should animate. For the same reason, we need to set properties
             * to something else than "" if no colors should animate. */
            if (animateBackgroundColors)
                colorAnimation.properties += __colorProps.replace(/\$/g, "background")
            __animateIndicators(animateIndicatorColors, colorAnimation, __colorProps)
            __animateHandles(animateHandleColors, colorAnimation, __colorProps)

            if (colorAnimation.properties === "")
                colorAnimation.properties = "_none_"
        }
    }

    ColorAnimation {
        id: colorAnimation
    }

    NumberAnimation {
        id: numberAnimation
        easing: root.easing
        duration: root.duration
    }
}
