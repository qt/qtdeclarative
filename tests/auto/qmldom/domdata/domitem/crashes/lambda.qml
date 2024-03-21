import QtQuick.Controls

Action {
    onTriggered: foo(Bla.Bar, function() {
        console.log("Hello")
    })
}
