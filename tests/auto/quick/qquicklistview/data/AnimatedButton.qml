import QtQuick 2.12

/*
  \qmltype AnimatedButton
*/
Rectangle {
    width: 100
    height: 20
    Behavior on color { ColorAnimation { duration : 300 } }
}
