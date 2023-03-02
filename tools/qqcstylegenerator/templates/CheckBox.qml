import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.CheckBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: 6

    topPadding: background ? background.topPadding : 0
    leftPadding: background ? background.leftPadding : 0
    rightPadding: background ? background.rightPadding : 0
    bottomPadding: background ? background.bottomPadding : 0

    topInset: background ? -background.topInset || 0 : 0
    leftInset: background ? -background.leftInset || 0 : 0
    rightInset: background ? -background.rightInset || 0 : 0
    bottomInset: background ? -background.bottomInset || 0 : 0

    indicator: Image {
        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2

        source: Qt.resolvedUrl("images/checkbox-indicator-background")
        ImageSelector on source {
            states: [
                {"tristate": control.checkState === Qt.PartiallyChecked},
                {"icon": control.checkState !== Qt.PartiallyChecked},
                {"disabled": !control.enabled},
                {"checked": control.checkState === Qt.Checked},
                {"pressed": control.down},
                {"focused": control.visualFocus},
                {"mirrored": control.mirrored},
                {"hovered": control.enabled && control.hovered}
            ]
        }

        Image {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            source: Qt.resolvedUrl("images/checkbox-indicator")
            ImageSelector on source {
                states: [
                    {"icon": control.checkState !== Qt.PartiallyChecked},
                    {"partialicon": control.checkState === Qt.PartiallyChecked},
                    {"tristate": control.checkState === Qt.PartiallyChecked},
                    {"disabled": !control.enabled},
                    {"checked": control.checkState === Qt.Checked},
                    {"pressed": control.down},
                    {"focused": control.visualFocus},
                    {"mirrored": control.mirrored},
                    {"hovered": control.enabled && control.hovered}
                ]
            }
        }
    }

    contentItem: Text {
        leftPadding: control.indicator && !control.mirrored ? control.indicator.width + control.spacing : 0
        rightPadding: control.indicator && control.mirrored ? control.indicator.width + control.spacing : 0

        text: control.text
        font: control.font
        color: control.palette.windowText
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    background: NinePatchImage {
        source: Qt.resolvedUrl("images/checkbox-background")
        NinePatchImageSelector on source {
            states: [
                {"tristate": control.checkState === Qt.PartiallyChecked},
                {"disabled": !control.enabled},
                {"checked": control.checkState === Qt.Checked},
                {"pressed": control.down},
                {"focused": control.visualFocus},
                {"mirrored": control.mirrored},
                {"hovered": control.enabled && control.hovered}
            ]
        }
    }
}
