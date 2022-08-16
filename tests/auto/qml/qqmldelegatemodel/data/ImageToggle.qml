import QtQuick 2.0

Item {
    property var isSelected: null
    property string source
    implicitWidth: 16
    implicitHeight: 16

    onSourceChanged: {
      updateImageSource()
    }

    onIsSelectedChanged: {
      updateImageSource()
    }

    function updateImageSource() {
     let result = isSelected ? source + "_selected_dark.png" : source + "_active_dark.png"
    }

}
