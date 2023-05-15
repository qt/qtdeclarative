import QtQml
import i18ntest

QtObject {
   property I18nAware aware: I18nAware {}
   property string result: aware.text
}
