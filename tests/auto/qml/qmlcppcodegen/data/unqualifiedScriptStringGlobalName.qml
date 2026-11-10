import QtQml
import TestTypes

// The script strings must not consist of nothing but a call to a translation function. Otherwise
// they are not compiled to a function with lookups, but re-parsed on every evaluation.
QtObject {
    readonly property ScriptStringHolder qsTrHolder: ScriptStringHolder {
        ss: "" + qsTr("text")
    }
    readonly property ScriptStringHolder qsTranslateHolder: ScriptStringHolder {
        ss: "" + qsTranslate("context", "text")
    }
    readonly property ScriptStringHolder qsTrIdHolder: ScriptStringHolder {
        ss: "" + qsTrId("text")
    }
    readonly property ScriptStringHolder trNoopHolder: ScriptStringHolder {
        ss: "" + QT_TR_NOOP("text")
    }
    readonly property ScriptStringHolder translateNoopHolder: ScriptStringHolder {
        ss: "" + QT_TRANSLATE_NOOP("context", "text")
    }
    readonly property ScriptStringHolder tridNoopHolder: ScriptStringHolder {
        ss: "" + QT_TRID_NOOP("text")
    }
    readonly property ScriptStringHolder infinityHolder: ScriptStringHolder {
        ss: "" + Infinity
    }
    readonly property ScriptStringHolder nanHolder: ScriptStringHolder {
        ss: "" + NaN
    }
    readonly property ScriptStringHolder undefinedHolder: ScriptStringHolder {
        ss: "" + undefined
    }
    readonly property ScriptStringHolder consoleHolder: ScriptStringHolder {
        ss: "" + console.debug("text")
    }
    readonly property ScriptStringHolder mathHolder: ScriptStringHolder {
        ss: "" + Math.max(1, 2)
    }
}
