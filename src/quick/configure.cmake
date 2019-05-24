

#### Inputs



#### Libraries



#### Tests



#### Features

qt_feature("d3d12" PUBLIC
    SECTION "Qt Quick"
    LABEL "Direct3D 12"
    PURPOSE "Provides a Direct3D 12 backend for the scenegraph."
    CONDITION TEST_d3d12
)
qt_feature("quick_animatedimage" PRIVATE
    SECTION "Qt Quick"
    LABEL "AnimatedImage item"
    PURPOSE "Provides the AnimatedImage item."
    CONDITION module.gui AND QT_FEATURE_movie OR FIXME
)
qt_feature("quick_canvas" PRIVATE
    SECTION "Qt Quick"
    LABEL "Canvas item"
    PURPOSE "Provides the Canvas item."
    CONDITION QT_FEATURE_quick_path
)
qt_feature("quick_designer" PRIVATE
    SECTION "Qt Quick"
    LABEL "Support for Qt Quick Designer"
    PURPOSE "Provides support for the Qt Quick Designer in Qt Creator."
)
qt_feature("quick_flipable" PRIVATE
    SECTION "Qt Quick"
    LABEL "Flipable item"
    PURPOSE "Provides the Flipable item."
)
qt_feature("quick_gridview" PRIVATE
    SECTION "Qt Quick"
    LABEL "GridView item"
    PURPOSE "Provides the GridView item."
    CONDITION QT_FEATURE_qml_delegate_model
)
qt_feature("quick_itemview" PRIVATE
    LABEL "ItemView item"
    CONDITION QT_FEATURE_quick_gridview OR QT_FEATURE_quick_listview OR QT_FEATURE_quick_tableview
)
qt_feature("quick_viewtransitions" PRIVATE
    LABEL "Transitions required for ItemViews and Positioners"
    CONDITION QT_FEATURE_quick_itemview OR QT_FEATURE_quick_positioners
)
qt_feature("quick_listview" PRIVATE
    SECTION "Qt Quick"
    LABEL "ListView item"
    PURPOSE "Provides the ListView item."
    CONDITION QT_FEATURE_qml_delegate_model
)
qt_feature("quick_tableview" PRIVATE
    SECTION "Qt Quick"
    LABEL "TableView item"
    PURPOSE "Provides the TableView item."
    CONDITION QT_FEATURE_qml_table_model
)
qt_feature("quick_particles" PRIVATE
    SECTION "Qt Quick"
    LABEL "Particle support"
    PURPOSE "Provides a particle system."
    CONDITION module.gui AND QT_FEATURE_quick_shadereffect AND QT_FEATURE_quick_sprite AND QT_FEATURE_opengl OR FIXME
)
qt_feature("quick_path" PRIVATE
    SECTION "Qt Quick"
    LABEL "Path support"
    PURPOSE "Provides Path elements."
    CONDITION QT_FEATURE_quick_shadereffect
)
qt_feature("quick_pathview" PRIVATE
    SECTION "Qt Quick"
    LABEL "PathView item"
    PURPOSE "Provides the PathView item."
    CONDITION ( QT_FEATURE_qml_delegate_model ) AND ( QT_FEATURE_quick_path )
)
qt_feature("quick_positioners" PRIVATE
    SECTION "Qt Quick"
    LABEL "Positioner items"
    PURPOSE "Provides Positioner items."
)
qt_feature("quick_repeater" PRIVATE
    SECTION "Qt Quick"
    LABEL "Repeater item"
    PURPOSE "Provides the Repeater item."
    CONDITION QT_FEATURE_qml_delegate_model
)
qt_feature("quick_shadereffect" PRIVATE
    SECTION "Qt Quick"
    LABEL "ShaderEffect item"
    PURPOSE "Provides Shader effects."
)
qt_feature("quick_sprite" PRIVATE
    SECTION "Qt Quick"
    LABEL "Sprite item"
    PURPOSE "Provides the Sprite item."
)
qt_feature("quick_draganddrop" PUBLIC
    SECTION "Qt Quick"
    LABEL "Drag & Drop"
    PURPOSE "Drag and drop support for Qt Quick"
    CONDITION ( QT_FEATURE_draganddrop ) AND ( QT_FEATURE_regularexpression )
)
