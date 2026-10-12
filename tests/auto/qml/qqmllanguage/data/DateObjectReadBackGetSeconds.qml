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

        date = dateProperty, dateProperty.setTime(3829292), dateProperty.setSeconds(32), datePropertyWasRead = (date.getSeconds() === dateProperty.getSeconds());
        time = timeProperty, timeProperty.setTime(3489245), timeProperty.setSeconds(32), timePropertyWasRead = (time.getSeconds() === timeProperty.getSeconds());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(849229), dateTimeProperty.setSeconds(32), dateTimePropertyWasRead = (dateTime.getSeconds() === dateTimeProperty.getSeconds());
    }
}
