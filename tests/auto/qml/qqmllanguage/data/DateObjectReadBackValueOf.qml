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

        date = dateProperty, dateProperty.setTime(3829292), datePropertyWasRead = (date.valueOf() === dateProperty.valueOf());
        time = timeProperty, timeProperty.setTime(3489245), timePropertyWasRead = (time.valueOf() === timeProperty.valueOf());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(849229), dateTimePropertyWasRead = (dateTime.valueOf() === dateTimeProperty.valueOf());
    }
}
