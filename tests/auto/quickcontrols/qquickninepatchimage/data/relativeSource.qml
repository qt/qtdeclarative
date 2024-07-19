import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Controls.Imagine
import QtQuick.Controls.Imagine.impl

NinePatchImage {
    width: 50
    height: 50
    source: "foo"
    NinePatchImageSelector on source {
        states: [
            { "disabled": false }
        ]
    }
}
