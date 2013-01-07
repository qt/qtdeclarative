/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qguiapplication.h>

#include <qsgmaterial.h>
#include <qsgnode.h>

#include <qquickitem.h>
#include <qquickview.h>

#include <qsgsimplerectnode.h>

#include <qsgsimplematerial.h>

//! [1]
struct Color
{
    QColor color;

    int compare(const Color *other) const {
        uint rgb = color.rgba();
        uint otherRgb = other->color.rgba();

        if (rgb == otherRgb) {
            return 0;
        } else if (rgb < otherRgb) {
            return -1;
        } else {
            return 1;
        }
    }
};
//! [1]

//! [2]
class Shader : public QSGSimpleMaterialShader<Color>
{
    QSG_DECLARE_SIMPLE_COMPARABLE_SHADER(Shader, Color);
//! [2] //! [3]
public:

    const char *vertexShader() const {
        return
                "attribute highp vec4 aVertex;                              \n"
                "attribute highp vec2 aTexCoord;                            \n"
                "uniform highp mat4 qt_Matrix;                              \n"
                "varying highp vec2 texCoord;                               \n"
                "void main() {                                              \n"
                "    gl_Position = qt_Matrix * aVertex;                     \n"
                "    texCoord = aTexCoord;                                  \n"
                "}";
    }

    const char *fragmentShader() const {
        return
                "uniform lowp float qt_Opacity;                             \n"
                "uniform lowp vec4 color;                                   \n"
                "varying highp vec2 texCoord;                               \n"
                "void main ()                                               \n"
                "{                                                          \n"
                "    highp vec2 z = texCoord;                               \n"
                "    gl_FragColor = vec4(0);                                \n"
                "    const highp float maxIterations = 100.;                \n"
                "    for (float i = 0.; i < maxIterations; i += 1.0) {      \n"
                "        z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + texCoord; \n"
                "        if (dot(z, z) > 4.0) {                             \n"
                "            float col = pow(1. - i / maxIterations, sqrt(maxIterations / 10.)); \n"
                "            gl_FragColor = color * col * qt_Opacity;       \n"
                "            break;                                         \n"
                "        }                                                  \n"
                "    }                                                      \n"
                "}                                                          \n";
    }
//! [3] //! [4]
    QList<QByteArray> attributes() const
    {
        return QList<QByteArray>() << "aVertex" << "aTexCoord";
    }
//! [4] //! [5]
    void updateState(const Color *color, const Color *)
    {
        program()->setUniformValue(id_color, color->color);
    }
//! [5] //! [6]
    void resolveUniforms()
    {
        id_color = program()->uniformLocation("color");
    }

private:
    int id_color;
};
//! [6]


//! [7]
class TestNode : public QSGGeometryNode
{
public:
    TestNode(const QRectF &bounds)
        : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
    {
        QSGGeometry::updateTexturedRectGeometry(&m_geometry, bounds, QRectF(-0.60, -0.66, 0.08, 0.04));
        setGeometry(&m_geometry);

//! [7] //! [8]
        QSGSimpleMaterial<Color> *material = Shader::createMaterial();
        material->state()->color = Qt::blue;
        material->setFlag(QSGMaterial::Blending);

        setMaterial(material);
        setFlag(OwnsMaterial);
    }
//! [8] //! [9]
    QSGGeometry m_geometry;
};
//! [9]


//! [10]
class Item : public QQuickItem
{
    Q_OBJECT
public:

    Item()
    {
        setFlag(ItemHasContents, true);
    }

    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
    {
        delete node;
        return new TestNode(boundingRect());
    }
};
//! [10]



//! [11]
int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<Item>("SimpleMaterial", 1, 0, "SimpleMaterialItem");

    QQuickView view;
    view.setSource(QUrl("qrc:///scenegraph/simplematerial/main.qml"));
    view.show();

    return app.exec();
}
//! [11]

#include "simplematerial.moc"
