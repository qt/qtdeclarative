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

    spacing: ConfigReader.images[backgroundName].spacing || 0

    topPadding: ConfigReader.images[backgroundName].topPadding || 0
    bottomPadding: ConfigReader.images[backgroundName].bottomPadding || 0
    leftPadding: ConfigReader.images[backgroundName].leftPadding || 0
    rightPadding: ConfigReader.images[backgroundName].rightPadding || 0

    topInset: -ConfigReader.images[backgroundName].topInset || 0
    bottomInset: -ConfigReader.images[backgroundName].bottomInset || 0
    leftInset: -ConfigReader.images[backgroundName].leftInset || 0
    rightInset: -ConfigReader.images[backgroundName].rightInset || 0

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

        border.top: ConfigReader.images[control.backgroundName].topOffset || 0
        border.bottom: ConfigReader.images[control.backgroundName].bottomOffset || 0
        border.left: ConfigReader.images[control.backgroundName].leftOffset || 0
        border.right: ConfigReader.images[control.backgroundName].rightOffset || 0

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
