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

        date = dateProperty, dateProperty.setTime(3829292), dateProperty.setMinutes(47), datePropertyWasRead = (date.getUTCMinutes() === dateProperty.getUTCMinutes());
        time = timeProperty, timeProperty.setTime(3489245), timeProperty.setMinutes(47), timePropertyWasRead = (time.getUTCMinutes() === timeProperty.getUTCMinutes());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(849229), dateTimeProperty.setMinutes(47), dateTimePropertyWasRead = (dateTime.getUTCMinutes() === dateTimeProperty.getUTCMinutes());
    }
}
