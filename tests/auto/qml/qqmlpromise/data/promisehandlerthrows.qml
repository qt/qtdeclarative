import QtQuick 2.12

Item {
    id: root
    property string errorMessage
    Component.onCompleted: () => {
    let prom = Promise.reject("Some error")
        .then(
                o => {console.log("Never reached");},
                err => {
                    console.log("Rethrowing err");
                    throw err;
                }
             )
        .catch(err => root.errorMessage = err)
    }
}
