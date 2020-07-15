import QtQuick 2.15

Item {
  component Test: Item {
    id: test
    property int t: 42
    Component.onCompleted: console.info(test.t)
  }
  Test {}
}
