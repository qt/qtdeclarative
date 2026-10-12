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

        date = dateProperty, dateProperty.setTime(0), dateProperty.setMonth(4), datePropertyWasRead = (date.toDateString() === dateProperty.toDateString());
        time = timeProperty, timeProperty.setTime(0), timeProperty.setMonth(4), timePropertyWasRead = (time.toDateString() === timeProperty.toDateString());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(0), dateTimeProperty.setMonth(4), dateTimePropertyWasRead = (dateTime.toDateString() === dateTimeProperty.toDateString());
    }
}
