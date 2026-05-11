// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

Style {
    control {
        // We start by styling a control in its 'normal' state
        leftPadding: 20
        rightPadding: 20

        background {
            border.color: palette.accent
            radius: 4
        }

        handle {
            color: palette.accent.lighter(1.2)
        }

        hovered {
            // Here we override some of the properties for the 'hovered' state. The ones
            // we don't set here will fall back to be read from the 'normal' state.
            background.color: palette.accent
            handle.color: palette.accent.darker(1.2)
        }

        hovered.pressed {
            // The states can also be nested. Since 'hovered.pressed' is more specific
            // than 'hovered', the former will be read first if the same property is set
            // in multiple states.
            background {
                color: palette.accent.darker(1.2)
                scale: 0.95
            }
        }

        checked {
            text.color: "white"
            background {
                color: palette.accent.darker(1.2)
                scale: 0.95
            }
        }

        checked.hovered {
            background.color: palette.accent
        }

        checked.hovered.pressed {
            background.color: palette.accent.darker(1.2)
        }

        transition: Transition {
           ColorAnimation {
                properties: "background.color"
                duration: 100
           }
        }
    }

    abstractButton {
        // After styling what is common to all the controls in the 'control' section
        // above, we now override and set properties that should be specific to only
        // some of the controls.
        // In this style, we want to show the background for all button types, such as
        // 'button', 'checkBox', 'radioButton', etc, so we set 'background.visible: true'.
        // By default, the background is normally hidden for most controls.
        background {
            visible: true
            shadow {
                color: "darkgray"
                horizontalOffset: 2
                verticalOffset: 2
            }
        }
    }

    itemDelegate {
        // We don't want the menu items in a ComboBox to fade, so we override and unset
        // the transition previously set for all controls in the 'control' section.
        transition: null
        background.color: "transparent"
        hovered {
            background.color: palette.accent
            text.color: "white"
        }
    }

    popup {
        // Remove padding so that item delegates span the full width
        padding: 0
    }

    scrollBar {
        // Hide the background, showing only the groove and handle
        background.visible: false
        padding: 0
    }

    pane {
        /* The controls change background color on states like hover, but panes
         * should not. Override the property here to disable that behavior for panes. */
        background.color: "white"
    }

    applicationWindow {
        background.color: "whitesmoke"
    }
}
