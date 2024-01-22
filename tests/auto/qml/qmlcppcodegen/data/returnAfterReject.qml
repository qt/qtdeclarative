import QtQml

QtObject {
    id: remaining

    property int bar: 0

    Component.onCompleted: {
        let remainingTime = 123
        if (remainingTime < 0) {
            remainingTime += 24 * 60 * 60
        }
        remaining.bar = isNaN(remainingTime) ? 0 : remainingTime
    }
}
