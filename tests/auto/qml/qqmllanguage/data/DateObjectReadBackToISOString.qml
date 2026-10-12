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

        date = dateProperty, dateProperty.setTime(0), datePropertyWasRead = (date.toISOString() === dateProperty.toISOString());
        time = timeProperty, timeProperty.setTime(0), timePropertyWasRead = (time.toISOString() === timeProperty.toISOString());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(0), dateTimePropertyWasRead = (dateTime.toISOString() === dateTimeProperty.toISOString());
    }
}
