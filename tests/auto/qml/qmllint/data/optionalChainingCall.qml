import QtQuick 2.0

MouseArea {
     id: ma
     function f() {}
     onClicked: {
         f?.()
     }
}
