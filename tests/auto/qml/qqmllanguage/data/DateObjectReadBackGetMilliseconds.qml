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

        date = dateProperty, dateProperty.setTime(3829292), dateProperty.setMilliseconds(311), datePropertyWasRead = (date.getMilliseconds() === dateProperty.getMilliseconds());
        time = timeProperty, timeProperty.setTime(3489245), timeProperty.setMilliseconds(311), timePropertyWasRead = (time.getMilliseconds() === timeProperty.getMilliseconds());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(849229), dateTimeProperty.setMilliseconds(311), dateTimePropertyWasRead = (dateTime.getMilliseconds() === dateTimeProperty.getMilliseconds());
    }
}
