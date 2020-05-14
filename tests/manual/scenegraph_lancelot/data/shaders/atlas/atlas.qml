import QtQuick 2.15

Item {
    width: 320
    height: 480

    Column {
        spacing: 10
        padding: 5

        Repeater {
            model: ["sand_16x16.png", "circle_16x16.png", "star_16x16.png", "square_16x16.png"]
            Row {
                spacing: 20
                Image {
                    id: img
                    source: "../../shared/" + modelData
                }

                // Non-atlased: copies subrect of atlas to a new texture
                ShaderEffect {
                    width: img.width
                    height: img.height
                    property variant source: img
                    fragmentShader: "qrc:shaders/gradientgrid.frag"
                    supportsAtlasTextures: false
                }

                // Atlased: qt_MultiTexCoord0 points to the atlas subrect
                ShaderEffect {
                    width: img.width
                    height: img.height
                    property variant source: img
                    fragmentShader: "qrc:shaders/gradientgrid.frag"
                    supportsAtlasTextures: true
                }

                // Subrect mode: qt_MultiTexCoord0 goes from 0.0 to 1.0,
                // and qt_Subrect_source specifies the atlas subrect
                ShaderEffect {
                    width: img.width
                    height: img.height
                    property variant source: img
                    fragmentShader: "qrc:shaders/gradientgrid.frag"
                    vertexShader: "qrc:shaders/subrect.vert"
                    supportsAtlasTextures: false
                }
            }
        }
    }
}
