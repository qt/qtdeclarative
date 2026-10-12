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

        date = dateProperty, dateProperty.setTime(3829292), dateProperty.setYear(1995), datePropertyWasRead = (date.getFullYear() === dateProperty.getFullYear());
        time = timeProperty, timeProperty.setTime(3489245), timeProperty.setYear(1995), timePropertyWasRead = (time.getFullYear() === timeProperty.getFullYear());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(849229), dateTimeProperty.setYear(1995), dateTimePropertyWasRead = (dateTime.getFullYear() === dateTimeProperty.getFullYear());
    }
}
