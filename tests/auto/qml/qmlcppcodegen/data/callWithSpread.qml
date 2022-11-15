import QtQml

QtObject {
    Component.onCompleted: {
        let f = console.error;
        const data = [f, ["That is great!"]]
        data[0](...data[1]);
    }
}
