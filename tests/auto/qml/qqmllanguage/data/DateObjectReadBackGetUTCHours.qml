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

        date = dateProperty, dateProperty.setTime(3829292), dateProperty.setHours(7), datePropertyWasRead = (date.getUTCHours() === dateProperty.getUTCHours());
        time = timeProperty, timeProperty.setTime(3489245), timeProperty.setHours(7), timePropertyWasRead = (time.getUTCHours() === timeProperty.getUTCHours());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(849229), dateTimeProperty.setHours(7), dateTimePropertyWasRead = (dateTime.getUTCHours() === dateTimeProperty.getUTCHours());
    }
}
