import QtQml
import LocaleTest

QtObject {
    property int monday: AppManager.primaryLocale.firstDayOfWeek
}
