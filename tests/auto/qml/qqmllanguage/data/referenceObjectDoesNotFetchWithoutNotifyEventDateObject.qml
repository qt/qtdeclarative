import qqmllanguage.Test
import QtQml

ReadCounter {
    Component.onCompleted: {
        let reference = dateTime
        reference.getSeconds()
        reference.getMinutes()
        reference.getHours()
    }
}
