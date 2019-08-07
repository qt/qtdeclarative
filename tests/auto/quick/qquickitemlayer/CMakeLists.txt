# Generated from qquickitemlayer.pro.

#####################################################################
## tst_qquickitemlayer Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qquickitemlayer
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickitemlayer.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickitemlayer.pro:<TRUE>:
# CONFIG = "testcase"
# OTHER_FILES = "data/Smooth.qml" "data/Enabled.qml" "data/Mipmap.qml" "data/Effect.qml" "data/SourceRect.qml" "data/TextureProvider.qml" "data/Visible.qml" "data/ZOrder.qml" "data/ZOrderChange.qml" "data/ToggleLayerAndEffect.qml" "data/DisableLayer.qml" "data/SamplerNameChange.qml" "data/ItemEffect.qml" "data/RectangleEffect.qml" "data/TextureMirroring.qml"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qquickitemlayer.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qquickitemlayer CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qquickitemlayer CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
