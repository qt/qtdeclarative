import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.CheckBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    readonly property string backgroundName: "checkbox-background"
    readonly property bool mirroredIndicator: control.mirrored !== ConfigReader.images[backgroundName].flipped

    spacing: ConfigReader.images[backgroundName].spacing || 0

    topPadding: ConfigReader.images[backgroundName].topPadding || 0
    bottomPadding: ConfigReader.images[backgroundName].bottomPadding || 0
    leftPadding: ConfigReader.images[backgroundName].leftPadding || 0
    rightPadding: ConfigReader.images[backgroundName].rightPadding || 0

    topInset: -ConfigReader.images[backgroundName].topInset || 0
    bottomInset: -ConfigReader.images[backgroundName].bottomInset || 0
    leftInset: -ConfigReader.images[backgroundName].leftInset || 0
    rightInset: -ConfigReader.images[backgroundName].rightInset || 0

    indicator: Image {
        x: control.text ? (control.mirroredIndicator ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
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
        leftPadding: control.indicator && !control.mirroredIndicator ? control.indicator.width + control.spacing : 0
        rightPadding: control.indicator && control.mirroredIndicator ? control.indicator.width + control.spacing : 0

        text: control.text
        font: control.font
        color: control.palette.windowText
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    background: BorderImage {
        source: Qt.resolvedUrl("images/" + control.backgroundName)

        border.top: ConfigReader.images[control.backgroundName].topOffset || 0
        border.bottom: ConfigReader.images[control.backgroundName].bottomOffset || 0
        border.left: ConfigReader.images[control.backgroundName].leftOffset || 0
        border.right: ConfigReader.images[control.backgroundName].rightOffset || 0

        ImageSelector on source {
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
