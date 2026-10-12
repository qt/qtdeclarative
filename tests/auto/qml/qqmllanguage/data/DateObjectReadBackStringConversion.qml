import qqmllanguage.Test
import QtQml

MyTypeObject {
    property bool datePropertyWasRead: false
    property bool timePropertyWasRead: false
    property bool dateTimePropertyWasRead: false

    Component.onCompleted: {
        let date;
        let time;
        let dateTime;

        date = dateProperty, dateProperty.setTime(0), stringProperty = date, datePropertyWasRead = (stringProperty === dateProperty.toString());
        time = timeProperty, timeProperty.setTime(0), stringProperty = time, timePropertyWasRead = (stringProperty === timeProperty.toString());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(0), stringProperty = dateTime, dateTimePropertyWasRead = (stringProperty === dateTimeProperty.toString());
    }
}
