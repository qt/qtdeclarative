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

        date = dateProperty, dateProperty.setTime(3829292), dateProperty.setDate(28), datePropertyWasRead = (date.getUTCDate() === dateProperty.getUTCDate());
        time = timeProperty, timeProperty.setTime(3489245), timeProperty.setDate(28), timePropertyWasRead = (time.getUTCDate() === timeProperty.getUTCDate());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(849229), dateTimeProperty.setDate(28), dateTimePropertyWasRead = (dateTime.getUTCDate() === dateTimeProperty.getUTCDate());
    }
}
