# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs



#### Libraries


# special case begin
qt_find_package(LTTngUST PROVIDED_TARGETS LTTng::UST MODULE_NAME quick QMAKE_LIB lttng-ust)
# special case end

#### Tests



#### Features

qt_feature("quick-animatedimage" PRIVATE
    SECTION "Qt Quick"
    LABEL "AnimatedImage item"
    PURPOSE "Provides the AnimatedImage item."
    CONDITION TARGET Qt::Gui AND QT_FEATURE_movie
)
qt_feature("quick-canvas" PRIVATE
    SECTION "Qt Quick"
    LABEL "Canvas item"
    PURPOSE "Provides the Canvas item."
    CONDITION QT_FEATURE_quick_path
)
qt_feature("quick-designer" PRIVATE
    SECTION "Qt Quick"
    LABEL "Support for Qt Quick Designer"
    PURPOSE "Provides support for the Qt Quick Designer in Qt Creator."
)
qt_feature("quick-flipable" PRIVATE
    SECTION "Qt Quick"
    LABEL "Flipable item"
    PURPOSE "Provides the Flipable item."
)
qt_feature("quick-gridview" PRIVATE
    SECTION "Qt Quick"
    LABEL "GridView item"
    PURPOSE "Provides the GridView item."
    CONDITION QT_FEATURE_qml_delegate_model
)
qt_feature("quick-itemview" PRIVATE
    LABEL "ItemView item"
    CONDITION QT_FEATURE_quick_gridview OR QT_FEATURE_quick_listview OR QT_FEATURE_quick_tableview OR QT_FEATURE_quick_treeview
)
qt_feature("quick-viewtransitions" PRIVATE
    LABEL "Transitions required for ItemViews and Positioners"
    CONDITION QT_FEATURE_quick_itemview OR QT_FEATURE_quick_positioners
)
qt_feature("quick-listview" PRIVATE
    SECTION "Qt Quick"
    LABEL "ListView item"
    PURPOSE "Provides the ListView item."
    CONDITION QT_FEATURE_qml_delegate_model
)
qt_feature("quick-tableview" PRIVATE
    SECTION "Qt Quick"
    LABEL "TableView item"
    PURPOSE "Provides the TableView item."
    CONDITION QT_FEATURE_qml_table_model
)
qt_feature("quick-treeview" PRIVATE
    SECTION "Qt Quick"
    LABEL "TreeView item"
    PURPOSE "Provides the TreeView item."
    CONDITION QT_FEATURE_quick_tableview
)
qt_feature("quick-particles" PRIVATE
    SECTION "Qt Quick"
    LABEL "Particle support"
    PURPOSE "Provides a particle system."
    CONDITION TARGET Qt::Gui AND QT_FEATURE_quick_shadereffect AND QT_FEATURE_quick_sprite
)
qt_feature("quick-path" PRIVATE
    SECTION "Qt Quick"
    LABEL "Path support"
    PURPOSE "Provides Path elements."
    CONDITION TARGET Qt::Gui
)
qt_feature("quick-pathview" PRIVATE
    SECTION "Qt Quick"
    LABEL "PathView item"
    PURPOSE "Provides the PathView item."
    CONDITION ( QT_FEATURE_qml_delegate_model ) AND ( QT_FEATURE_quick_path )
)
qt_feature("quick-positioners" PRIVATE
    SECTION "Qt Quick"
    LABEL "Positioner items"
    PURPOSE "Provides Positioner items."
)
qt_feature("quick-repeater" PRIVATE
    SECTION "Qt Quick"
    LABEL "Repeater item"
    PURPOSE "Provides the Repeater item."
    CONDITION QT_FEATURE_qml_delegate_model
)
qt_feature("quick-shadereffect" PUBLIC
    SECTION "Qt Quick"
    LABEL "ShaderEffect item"
    PURPOSE "Provides Shader effects."
)
qt_feature("quick-sprite" PRIVATE
    SECTION "Qt Quick"
    LABEL "Sprite item"
    PURPOSE "Provides the Sprite item."
)
qt_feature("quick-draganddrop" PUBLIC
    SECTION "Qt Quick"
    LABEL "Drag & Drop"
    PURPOSE "Drag and drop support for Qt Quick"
    CONDITION ( QT_FEATURE_draganddrop ) AND ( QT_FEATURE_regularexpression )
)

qt_feature("quick-pixmap-cache-threaded-download" PUBLIC
    SECTION "Qt Quick"
    LABEL "Threaded download in pixmap cache"
    PURPOSE "Pixmap cache pixmap downloads on separate threads"
    CONDITION ( QT_FEATURE_thread ) AND ( NOT WASM )
)
qt_configure_add_summary_section(NAME "Qt Quick")
qt_configure_add_summary_entry(ARGS "quick-animatedimage")
qt_configure_add_summary_entry(ARGS "quick-canvas")
qt_configure_add_summary_entry(ARGS "quick-designer")
qt_configure_add_summary_entry(ARGS "quick-flipable")
qt_configure_add_summary_entry(ARGS "quick-gridview")
qt_configure_add_summary_entry(ARGS "quick-listview")
qt_configure_add_summary_entry(ARGS "quick-tableview")
qt_configure_add_summary_entry(ARGS "quick-treeview")
qt_configure_add_summary_entry(ARGS "quick-path")
qt_configure_add_summary_entry(ARGS "quick-pathview")
qt_configure_add_summary_entry(ARGS "quick-positioners")
qt_configure_add_summary_entry(ARGS "quick-repeater")
qt_configure_add_summary_entry(ARGS "quick-shadereffect")
qt_configure_add_summary_entry(ARGS "quick-sprite")
qt_configure_end_summary_section() # end of "Qt Quick" section
