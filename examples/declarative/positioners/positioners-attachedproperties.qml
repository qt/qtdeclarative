import QtQuick 2.0

Rectangle {
  width: 400
  height: 100

  // Create row with four rectangles, the fourth one is hidden
  Row {
    id: row

    Rectangle {
      id: red
      color: "red"
      width: 100
      height: 100

      // When mouse is clicked, display the values of the positioner
      MouseArea {
        anchors.fill: parent
        onClicked: row.showInfo(red.Positioner)
      }
    }

    Rectangle {
      id: green
      color: "green"
      width: 100
      height: 100

      // When mouse is clicked, display the values of the positioner
      MouseArea {
        anchors.fill: parent
        onClicked: row.showInfo(green.Positioner)
      }
    }

    Rectangle {
      id: blue
      color: "blue"
      width: 100
      height: 100

      // When mouse is clicked, display the values of the positioner
      MouseArea {
        anchors.fill: parent
        onClicked: row.showInfo(blue.Positioner)
      }
    }

    // This rectangle is not visible, so it doesn't have a positioner value
    Rectangle {
      color: "black"
      width: 100
      height: 100
      visible: false
    }

    // Print the index of the child item in the positioner and convenience
    // properties showing if it's the first or last item.
    function showInfo(positioner) {
      console.log("Item Index = " + positioner.index)
      console.log("  isFirstItem = " + positioner.isFirstItem)
      console.log("  isLastItem = " + positioner.isLastItem)
    }
  }
}
