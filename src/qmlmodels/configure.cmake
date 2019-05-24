

#### Inputs



#### Libraries



#### Tests



#### Features

qt_feature("qml_object_model" PRIVATE
    SECTION "QML"
    LABEL "QML list model"
    PURPOSE "Provides the ObjectModel and Instantiator QML types."
)
qt_feature("qml_list_model" PRIVATE
    SECTION "QML"
    LABEL "QML list model"
    PURPOSE "Provides the ListModel QML type."
)
qt_feature("qml_delegate_model" PRIVATE
    SECTION "QML"
    LABEL "QML delegate model"
    PURPOSE "Provides the DelegateModel QML type."
    CONDITION QT_FEATURE_qml_object_model
)
qt_feature("qml_table_model" PRIVATE
    SECTION "QML"
    LABEL "QML table model"
    PURPOSE "Provides the TableModel QML type."
)
