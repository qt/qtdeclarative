# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs



#### Libraries



#### Tests



#### Features

qt_feature("qml-object-model" PRIVATE
    SECTION "QML"
    LABEL "QML object model"
    PURPOSE "Provides the ObjectModel and Instantiator QML types."
)
qt_feature("qml-list-model" PRIVATE
    SECTION "QML"
    LABEL "QML list model"
    PURPOSE "Provides the ListModel QML type."
    CONDITION QT_FEATURE_qml_itemmodel
)
qt_feature("qml-delegate-model" PRIVATE
    SECTION "QML"
    LABEL "QML delegate model"
    PURPOSE "Provides the DelegateModel QML type."
    CONDITION QT_FEATURE_qml_object_model AND QT_FEATURE_qml_itemmodel
)
qt_feature("qml-table-model" PRIVATE
    SECTION "QML"
    LABEL "QML table model"
    PURPOSE "Provides the TableModel QML type."
    CONDITION QT_FEATURE_qml_itemmodel AND QT_FEATURE_qml_delegate_model
)
qt_configure_add_summary_section(NAME "Qt QML Models")
qt_configure_add_summary_entry(ARGS "qml-list-model")
qt_configure_add_summary_entry(ARGS "qml-delegate-model")
qt_configure_end_summary_section() # end of "Qt QML Models" section
