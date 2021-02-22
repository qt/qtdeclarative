import QtQml 2.0

QtObject {
    onSomePropertyChanged: { // Note: there's no property someProperty, so this fails
        console.log("42");
    }
}
