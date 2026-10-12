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

        date = dateProperty, dateProperty.setTime(49024939), datePropertyWasRead = (date.toJSON() === dateProperty.toJSON());
        time = timeProperty, timeProperty.setTime(49024939), timePropertyWasRead = (time.toJSON() === timeProperty.toJSON());
        dateTime = dateTimeProperty, dateTimeProperty.setTime(49024939), dateTimePropertyWasRead = (dateTime.toJSON() === dateTimeProperty.toJSON());
    }
}
