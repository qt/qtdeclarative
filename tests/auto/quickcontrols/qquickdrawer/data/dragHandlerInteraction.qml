import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    property int changedCounter: 0

    width: 300
    height: 300

    Drawer {
      width: 30
      height: 30
    }

    DragHandler {
      target: null
      onActiveChanged: {
        console.log("active changed")
        root.changedCounter++
      }
    }
}
