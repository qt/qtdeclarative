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

        date = dateProperty, dateProperty.setTime(0), dateProperty.setMonth(4), datePropertyWasRead = (date.toString() === dateProperty.toString());
        time = timeProperty, timeProperty.setTime(0), timeProperty.setMonth(4), timePropertyWasRead = (time.toString() === timeProperty.toString());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(0), dateTimeProperty.setMonth(4), dateTimePropertyWasRead = (dateTime.toString() === dateTimeProperty.toString());
    }
}
