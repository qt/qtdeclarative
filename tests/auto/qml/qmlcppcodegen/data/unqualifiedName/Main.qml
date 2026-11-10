import QtQml
import TestTypes


// For tst_QmlCppCodegen::unqualifiedName. Each component places a name as id in the
// component context, and on the scope object or the context object, or not at all. In the
// files with "Root" in their names, the root object is the enclosing context object, and it
// has all the names. In the files with "Swapped" in their names, the objects holding the
// names have the other kind of member for each name.
QtObject {
    readonly property Component consoleComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + console.debug("text") }
                property string probeBinding: "" + console.debug("text")
                property string probeScript: { return "" + console.debug("text") }
            }
        }
    }

    readonly property Component consoleContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + console.debug("text") }
                property string probeBinding: "" + console.debug("text")
                property string probeScript: { return "" + console.debug("text") }
            }
        }
    }

    readonly property Component consoleDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + console.debug("text") }
            }
        }
    }

    readonly property Component consoleDynamicScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + console.debug("text") }
            }
        }
    }

    readonly property Component consoleScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + console.debug("text") }
                property string probeBinding: "" + console.debug("text")
                property string probeScript: { return "" + console.debug("text") }
            }
        }
    }

    readonly property Component consoleScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + console.debug("text") }
                property string probeBinding: "" + console.debug("text")
                property string probeScript: { return "" + console.debug("text") }
            }
        }
    }

    readonly property Component fooIdComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject idHolder: QtObject { id: foo; objectName: "id" }
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + foo.objectName }
                property string probeBinding: "" + foo.objectName
                property string probeScript: { return "" + foo.objectName }
            }
        }
    }

    readonly property Component fooIdContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject idHolder: QtObject { id: foo; objectName: "id" }
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + foo.objectName }
                property string probeBinding: "" + foo.objectName
                property string probeScript: { return "" + foo.objectName }
            }
        }
    }

    readonly property Component fooIdDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject idHolder: QtObject { id: foo; objectName: "id" }
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + foo.objectName }
            }
        }
    }

    readonly property Component fooIdScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject idHolder: QtObject { id: foo; objectName: "id" }
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + foo.objectName }
                property string probeBinding: "" + foo.objectName
                property string probeScript: { return "" + foo.objectName }
            }
        }
    }

    readonly property Component fooScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + foo.objectName }
                property string probeBinding: "" + foo.objectName
                property string probeScript: { return "" + foo.objectName }
            }
        }
    }

    readonly property Component fooScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + foo.objectName }
                property string probeBinding: "" + foo.objectName
                property string probeScript: { return "" + foo.objectName }
            }
        }
    }

    readonly property Component infinityComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + Infinity }
                property string probeBinding: "" + Infinity
                property string probeScript: { return "" + Infinity }
            }
        }
    }

    readonly property Component infinityContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + Infinity }
                property string probeBinding: "" + Infinity
                property string probeScript: { return "" + Infinity }
            }
        }
    }

    readonly property Component infinityDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + Infinity }
            }
        }
    }

    readonly property Component infinityDynamicScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + Infinity }
            }
        }
    }

    readonly property Component infinityScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + Infinity }
                property string probeBinding: "" + Infinity
                property string probeScript: { return "" + Infinity }
            }
        }
    }

    readonly property Component infinityScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + Infinity }
                property string probeBinding: "" + Infinity
                property string probeScript: { return "" + Infinity }
            }
        }
    }

    readonly property Component mathComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + Math.max(1, 2) }
                property string probeBinding: "" + Math.max(1, 2)
                property string probeScript: { return "" + Math.max(1, 2) }
            }
        }
    }

    readonly property Component mathContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + Math.max(1, 2) }
                property string probeBinding: "" + Math.max(1, 2)
                property string probeScript: { return "" + Math.max(1, 2) }
            }
        }
    }

    readonly property Component mathDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + Math.max(1, 2) }
            }
        }
    }

    readonly property Component mathDynamicScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + Math.max(1, 2) }
            }
        }
    }

    readonly property Component mathScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + Math.max(1, 2) }
                property string probeBinding: "" + Math.max(1, 2)
                property string probeScript: { return "" + Math.max(1, 2) }
            }
        }
    }

    readonly property Component mathScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + Math.max(1, 2) }
                property string probeBinding: "" + Math.max(1, 2)
                property string probeScript: { return "" + Math.max(1, 2) }
            }
        }
    }

    readonly property Component nanComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + NaN }
                property string probeBinding: "" + NaN
                property string probeScript: { return "" + NaN }
            }
        }
    }

    readonly property Component nanContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + NaN }
                property string probeBinding: "" + NaN
                property string probeScript: { return "" + NaN }
            }
        }
    }

    readonly property Component nanDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + NaN }
            }
        }
    }

    readonly property Component nanDynamicScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + NaN }
            }
        }
    }

    readonly property Component nanScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + NaN }
                property string probeBinding: "" + NaN
                property string probeScript: { return "" + NaN }
            }
        }
    }

    readonly property Component nanScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + NaN }
                property string probeBinding: "" + NaN
                property string probeScript: { return "" + NaN }
            }
        }
    }

    readonly property Component plughScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + Plugh.objectName }
                property string probeBinding: "" + Plugh.objectName
                property string probeScript: { return "" + Plugh.objectName }
            }
        }
    }

    readonly property Component plughScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + Plugh.objectName }
                property string probeBinding: "" + Plugh.objectName
                property string probeScript: { return "" + Plugh.objectName }
            }
        }
    }

    readonly property Component qsTrComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return qsTr("text") }
                property string probeBinding: qsTr("text")
                property string probeScript: { return qsTr("text") }
            }
        }
    }

    readonly property Component qsTrContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return qsTr("text") }
                property string probeBinding: qsTr("text")
                property string probeScript: { return qsTr("text") }
            }
        }
    }

    readonly property Component qsTrDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return qsTr("text") }
            }
        }
    }

    readonly property Component qsTrDynamicScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return qsTr("text") }
            }
        }
    }

    readonly property Component qsTrIdComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return qsTrId("text") }
                property string probeBinding: qsTrId("text")
                property string probeScript: { return qsTrId("text") }
            }
        }
    }

    readonly property Component qsTrIdContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return qsTrId("text") }
                property string probeBinding: qsTrId("text")
                property string probeScript: { return qsTrId("text") }
            }
        }
    }

    readonly property Component qsTrIdDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return qsTrId("text") }
            }
        }
    }

    readonly property Component qsTrIdDynamicScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return qsTrId("text") }
            }
        }
    }

    readonly property Component qsTrIdScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return qsTrId("text") }
                property string probeBinding: qsTrId("text")
                property string probeScript: { return qsTrId("text") }
            }
        }
    }

    readonly property Component qsTrIdScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return qsTrId("text") }
                property string probeBinding: qsTrId("text")
                property string probeScript: { return qsTrId("text") }
            }
        }
    }

    readonly property Component qsTrScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return qsTr("text") }
                property string probeBinding: qsTr("text")
                property string probeScript: { return qsTr("text") }
            }
        }
    }

    readonly property Component qsTrScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return qsTr("text") }
                property string probeBinding: qsTr("text")
                property string probeScript: { return qsTr("text") }
            }
        }
    }

    readonly property Component qsTranslateComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return qsTranslate("context", "text") }
                property string probeBinding: qsTranslate("context", "text")
                property string probeScript: { return qsTranslate("context", "text") }
            }
        }
    }

    readonly property Component qsTranslateContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return qsTranslate("context", "text") }
                property string probeBinding: qsTranslate("context", "text")
                property string probeScript: { return qsTranslate("context", "text") }
            }
        }
    }

    readonly property Component qsTranslateDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return qsTranslate("context", "text") }
            }
        }
    }

    readonly property Component qsTranslateDynamicScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return qsTranslate("context", "text") }
            }
        }
    }

    readonly property Component qsTranslateScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return qsTranslate("context", "text") }
                property string probeBinding: qsTranslate("context", "text")
                property string probeScript: { return qsTranslate("context", "text") }
            }
        }
    }

    readonly property Component qsTranslateScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return qsTranslate("context", "text") }
                property string probeBinding: qsTranslate("context", "text")
                property string probeScript: { return qsTranslate("context", "text") }
            }
        }
    }

    readonly property Component rectIdComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject idHolder: QtObject { id: rect; objectName: "id" }
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + rect.objectName }
                property string probeBinding: "" + rect.objectName
                property string probeScript: { return "" + rect.objectName }
            }
        }
    }

    readonly property Component rectIdContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject idHolder: QtObject { id: rect; objectName: "id" }
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + rect.objectName }
                property string probeBinding: "" + rect.objectName
                property string probeScript: { return "" + rect.objectName }
            }
        }
    }

    readonly property Component rectIdDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject idHolder: QtObject { id: rect; objectName: "id" }
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + rect.objectName }
            }
        }
    }

    readonly property Component rectIdScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject idHolder: QtObject { id: rect; objectName: "id" }
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + rect.objectName }
                property string probeBinding: "" + rect.objectName
                property string probeScript: { return "" + rect.objectName }
            }
        }
    }

    readonly property Component rectScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + rect.objectName }
                property string probeBinding: "" + rect.objectName
                property string probeScript: { return "" + rect.objectName }
            }
        }
    }

    readonly property Component rectScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + rect.objectName }
                property string probeBinding: "" + rect.objectName
                property string probeScript: { return "" + rect.objectName }
            }
        }
    }

    readonly property Component trNoopComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return QT_TR_NOOP("text") }
                property string probeBinding: QT_TR_NOOP("text")
                property string probeScript: { return QT_TR_NOOP("text") }
            }
        }
    }

    readonly property Component trNoopContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return QT_TR_NOOP("text") }
                property string probeBinding: QT_TR_NOOP("text")
                property string probeScript: { return QT_TR_NOOP("text") }
            }
        }
    }

    readonly property Component trNoopDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return QT_TR_NOOP("text") }
            }
        }
    }

    readonly property Component trNoopDynamicScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return QT_TR_NOOP("text") }
            }
        }
    }

    readonly property Component trNoopScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return QT_TR_NOOP("text") }
                property string probeBinding: QT_TR_NOOP("text")
                property string probeScript: { return QT_TR_NOOP("text") }
            }
        }
    }

    readonly property Component trNoopScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return QT_TR_NOOP("text") }
                property string probeBinding: QT_TR_NOOP("text")
                property string probeScript: { return QT_TR_NOOP("text") }
            }
        }
    }

    readonly property Component translateNoopComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return QT_TRANSLATE_NOOP("context", "text") }
                property string probeBinding: QT_TRANSLATE_NOOP("context", "text")
                property string probeScript: { return QT_TRANSLATE_NOOP("context", "text") }
            }
        }
    }

    readonly property Component translateNoopContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return QT_TRANSLATE_NOOP("context", "text") }
                property string probeBinding: QT_TRANSLATE_NOOP("context", "text")
                property string probeScript: { return QT_TRANSLATE_NOOP("context", "text") }
            }
        }
    }

    readonly property Component translateNoopDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return QT_TRANSLATE_NOOP("context", "text") }
            }
        }
    }

    readonly property Component translateNoopDynamicScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return QT_TRANSLATE_NOOP("context", "text") }
            }
        }
    }

    readonly property Component translateNoopScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return QT_TRANSLATE_NOOP("context", "text") }
                property string probeBinding: QT_TRANSLATE_NOOP("context", "text")
                property string probeScript: { return QT_TRANSLATE_NOOP("context", "text") }
            }
        }
    }

    readonly property Component translateNoopScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return QT_TRANSLATE_NOOP("context", "text") }
                property string probeBinding: QT_TRANSLATE_NOOP("context", "text")
                property string probeScript: { return QT_TRANSLATE_NOOP("context", "text") }
            }
        }
    }

    readonly property Component tridNoopComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return QT_TRID_NOOP("text") }
                property string probeBinding: QT_TRID_NOOP("text")
                property string probeScript: { return QT_TRID_NOOP("text") }
            }
        }
    }

    readonly property Component tridNoopContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return QT_TRID_NOOP("text") }
                property string probeBinding: QT_TRID_NOOP("text")
                property string probeScript: { return QT_TRID_NOOP("text") }
            }
        }
    }

    readonly property Component tridNoopDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return QT_TRID_NOOP("text") }
            }
        }
    }

    readonly property Component tridNoopDynamicScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return QT_TRID_NOOP("text") }
            }
        }
    }

    readonly property Component tridNoopScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return QT_TRID_NOOP("text") }
                property string probeBinding: QT_TRID_NOOP("text")
                property string probeScript: { return QT_TRID_NOOP("text") }
            }
        }
    }

    readonly property Component tridNoopScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return QT_TRID_NOOP("text") }
                property string probeBinding: QT_TRID_NOOP("text")
                property string probeScript: { return QT_TRID_NOOP("text") }
            }
        }
    }

    readonly property Component undefinedComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + undefined }
                property string probeBinding: "" + undefined
                property string probeScript: { return "" + undefined }
            }
        }
    }

    readonly property Component undefinedContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + undefined }
                property string probeBinding: "" + undefined
                property string probeScript: { return "" + undefined }
            }
        }
    }

    readonly property Component undefinedDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + undefined }
            }
        }
    }

    readonly property Component undefinedDynamicScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + undefined }
            }
        }
    }

    readonly property Component undefinedScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + undefined }
                property string probeBinding: "" + undefined
                property string probeScript: { return "" + undefined }
            }
        }
    }

    readonly property Component undefinedScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: NameHolder {
                objectName: "scope object"
                function probeFunction(): string { return "" + undefined }
                property string probeBinding: "" + undefined
                property string probeScript: { return "" + undefined }
            }
        }
    }

    readonly property Component fooComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + foo.objectName }
                property string probeBinding: "" + foo.objectName
                property string probeScript: { return "" + foo.objectName }
            }
        }
    }

    readonly property Component fooContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + foo.objectName }
                property string probeBinding: "" + foo.objectName
                property string probeScript: { return "" + foo.objectName }
            }
        }
    }

    readonly property Component fooDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + foo.objectName }
            }
        }
    }

    readonly property Component fooDynamicScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + foo.objectName }
            }
        }
    }

    readonly property Component plughComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + Plugh.objectName }
                property string probeBinding: "" + Plugh.objectName
                property string probeScript: { return "" + Plugh.objectName }
            }
        }
    }

    readonly property Component plughContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + Plugh.objectName }
                property string probeBinding: "" + Plugh.objectName
                property string probeScript: { return "" + Plugh.objectName }
            }
        }
    }

    readonly property Component plughDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + Plugh.objectName }
            }
        }
    }

    readonly property Component plughDynamicScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + Plugh.objectName }
            }
        }
    }

    readonly property Component rectComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + rect.objectName }
                property string probeBinding: "" + rect.objectName
                property string probeScript: { return "" + rect.objectName }
            }
        }
    }

    readonly property Component rectContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: QtObject {
                objectName: "scope object"
                function probeFunction(): string { return "" + rect.objectName }
                property string probeBinding: "" + rect.objectName
                property string probeScript: { return "" + rect.objectName }
            }
        }
    }

    readonly property Component rectDynamicScopeComponent: Component {
        QtObject {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + rect.objectName }
            }
        }
    }

    readonly property Component rectDynamicScopeContextComponent: Component {
        NameHolder {
            objectName: "context object"
            readonly property QtObject child: DynamicNameHolder {
                objectName: "scope object"
                probe: { trigger; return "" + rect.objectName }
            }
        }
    }
}
