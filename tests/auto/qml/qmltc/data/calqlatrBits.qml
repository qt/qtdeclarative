import QtQml
import QtQuick
Item {
   id: root

    Text {
        id: textItem
        wrapMode: Text.WrapAnywhere
        font.pixelSize: 12
    }

    AnimationController {
        id: controller
        animation: NumberAnimation {
            target: textItem
            property: "scale"
            duration: 50
            from: 1
            to: 0.5
            easing.type: Easing.InOutQuad
        }
    }
}
