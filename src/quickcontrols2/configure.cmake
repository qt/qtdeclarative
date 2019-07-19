

#### Inputs



#### Libraries



#### Tests



#### Features

qt_feature("quickcontrols2_default" PRIVATE
    LABEL "Default"
)
qt_feature("quickcontrols2_fusion" PRIVATE
    SECTION "Quick Controls 2"
    LABEL "Fusion"
    PURPOSE "Provides the platform agnostic desktop-oriented Fusion style."
    CONDITION QT_FEATURE_quickcontrols2_default
)
qt_feature("quickcontrols2_imagine" PRIVATE
    SECTION "Quick Controls 2"
    LABEL "Imagine"
    PURPOSE "Provides a style based on configurable image assets."
    CONDITION QT_FEATURE_quickcontrols2_default
)
qt_feature("quickcontrols2_material" PRIVATE
    SECTION "Quick Controls 2"
    LABEL "Material"
    PURPOSE "Provides a style based on the Material Design guidelines."
    CONDITION QT_FEATURE_quickcontrols2_default
)
qt_feature("quickcontrols2_universal" PRIVATE
    SECTION "Quick Controls 2"
    LABEL "Universal"
    PURPOSE "Provides a style based on the Universal Design guidelines."
    CONDITION QT_FEATURE_quickcontrols2_default
)
