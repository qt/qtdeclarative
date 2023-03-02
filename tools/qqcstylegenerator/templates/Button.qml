import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.Button {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    bottomPadding: 4
    topPadding: 4
    rightPadding: 4
    leftPadding: 4

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

    background: NinePatchImage {
        source: Qt.resolvedUrl("images/button-background")
        NinePatchImageSelector on source {
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
