import qqmllanguage.Test
import QtQml

ReadCounter {
    property int accessesCount: 4

    Component.onCompleted: {
        let reference;

        reference = dateTime, dateTimeChanged(), reference.getSeconds(),
                              dateTimeChanged(), reference.getMinutes(),
                              dateTimeChanged(), reference.getHours();
    }
}
