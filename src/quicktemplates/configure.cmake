# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs



#### Libraries



#### Tests



#### Features

qt_feature("quicktemplates2-hover" PRIVATE
    SECTION "Quick Templates 2"
    LABEL "Hover support"
    PURPOSE "Provides support for hover effects."
)
qt_feature("quicktemplates2-multitouch" PRIVATE
    SECTION "Quick Templates 2"
    LABEL "Multi-touch support"
    PURPOSE "Provides support for multi-touch."
)
qt_feature("quicktemplates2-calendar" PRIVATE
    SECTION "Quick Templates 2"
    LABEL "Calendar support"
    PURPOSE "Provides calendar types."
    CONDITION QT_FEATURE_itemmodel
)
qt_feature("quicktemplates2-container" PRIVATE
    SECTION "Quick Templates 2"
    LABEL "Container controls support"
    PURPOSE "Provides support for Container and its sub-classes."
    CONDITION QT_FEATURE_qml_object_model
)
qt_configure_add_summary_section(NAME "Qt Quick Templates 2")
qt_configure_add_summary_entry(ARGS "quicktemplates2-hover")
qt_configure_add_summary_entry(ARGS "quicktemplates2-multitouch")
qt_configure_add_summary_entry(ARGS "quicktemplates2-calendar")
qt_configure_end_summary_section() # end of "Qt Quick Templates 2" section
