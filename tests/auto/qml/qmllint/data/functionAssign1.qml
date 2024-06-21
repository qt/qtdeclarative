import QtQml
QtObject {
    onObjectNameChanged: () => {
        console.log("Entered")
    }
}
