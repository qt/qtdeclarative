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

        date = dateProperty, dateProperty.setTime(3829292), dateProperty.setSeconds(32), datePropertyWasRead = (date.getUTCSeconds() === dateProperty.getUTCSeconds());
        time = timeProperty, timeProperty.setTime(3489245), timeProperty.setSeconds(32), timePropertyWasRead = (time.getUTCSeconds() === timeProperty.getUTCSeconds());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(849229), dateTimeProperty.setSeconds(32), dateTimePropertyWasRead = (dateTime.getUTCSeconds() === dateTimeProperty.getUTCSeconds());
    }
}
