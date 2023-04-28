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
    readonly property var config: ConfigReader.images[backgroundName]
    readonly property bool mirroredIndicator: control.mirrored !== (config?.mirrored || false)

    spacing: config?.spacing || 0

    topPadding: config?.topPadding || 0
    bottomPadding: config?.bottomPadding || 0
    leftPadding: config?.leftPadding || 0
    rightPadding: config?.rightPadding || 0

    topInset: -config?.topInset || 0
    bottomInset: -config?.bottomInset || 0
    leftInset: -config?.leftInset || 0
    rightInset: -config?.rightInset || 0

    indicator: Image {
        x: control.text ? (control.mirroredIndicator ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2

        source: Qt.resolvedUrl("images/checkbox-indicator-background")
        ImageSelector on source {
            states: [
                {"partialicon": control.checkState === Qt.PartiallyChecked},
                {"disabled": !control.enabled},
                {"checked": control.checkState === Qt.Checked},
                {"partially-checked": control.checkState === Qt.PartiallyChecked},
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
                    {"partialicon": control.checkState === Qt.PartiallyChecked},
                    {"disabled": !control.enabled},
                    {"checked": control.checkState === Qt.Checked},
                    {"partially-checked": control.checkState === Qt.PartiallyChecked},
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

        border.top: control.config?.topOffset || 0
        border.bottom: control.config?.bottomOffset || 0
        border.left: control.config?.leftOffset || 0
        border.right: control.config?.rightOffset || 0

        ImageSelector on source {
            states: [
                {"checked": control.checkState === Qt.Checked},
                {"partially-checked": control.checkState === Qt.PartiallyChecked},
                {"disabled": !control.enabled},
                {"pressed": control.down},
                {"focused": control.visualFocus},
                {"mirrored": control.mirrored},
                {"hovered": control.enabled && control.hovered}
            ]
        }
    }
}
