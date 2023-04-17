import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.Button {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    readonly property string backgroundName: "button-background"
    readonly property var config: ConfigReader.images[backgroundName]

    spacing: config?.spacing || 0

    topPadding: config?.topPadding || 0
    bottomPadding: config?.bottomPadding || 0
    leftPadding: config?.leftPadding || 0
    rightPadding: config?.rightPadding || 0

    topInset: -config?.topInset || 0
    bottomInset: -config?.bottomInset || 0
    leftInset: -config?.leftInset || 0
    rightInset: -config?.rightInset || 0

    icon.width: 17
    icon.height: 17
    icon.color: control.flat ? (control.enabled ? (control.down ? control.palette.highlight : control.palette.button)
                                                : control.palette.mid)
                             : control.palette.buttonText

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display

        icon: control.icon
        text: control.text
        font: control.font
        color: control.flat ? (control.enabled ? (control.down ? control.palette.highlight : control.palette.button)
                                               : control.palette.mid)
                            : control.palette.buttonText
    }

    background: BorderImage {
        source: Qt.resolvedUrl("images/" + control.backgroundName)

        border.top: control.config?.topOffset || 0
        border.bottom: control.config?.bottomOffset || 0
        border.left: control.config?.leftOffset || 0
        border.right: control.config?.rightOffset || 0

        ImageSelector on source {
            states: [
                {"disabled": !control.enabled},
                {"pressed": control.down},
                {"checked": control.checked},
                {"checkable": control.checkable},
                {"focused": control.visualFocus},
                {"highlighted": control.highlighted},
                {"mirrored": control.mirrored},
                {"flat": control.flat},
                {"hovered": control.enabled && control.hovered}
            ]
        }
    }
}
