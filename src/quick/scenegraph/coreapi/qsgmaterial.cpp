/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgmaterial.h"
#include "qsgrenderer_p.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DEBUG
bool qsg_material_failure = false;
bool qsg_test_and_clear_material_failure()
{
    bool fail = qsg_material_failure;
    qsg_material_failure = false;
    return fail;
}

void qsg_set_material_failure()
{
    qsg_material_failure = true;
}
#endif

#ifndef QT_NO_DEBUG
static const bool qsg_leak_check = !qEnvironmentVariableIsEmpty("QML_LEAK_CHECK");
#endif

/*!
    \group qtquick-scenegraph-materials
    \title Qt Quick Scene Graph Material Classes
    \brief classes used to define materials in the Qt Quick Scene Graph.

    This page lists the material classes in \l {Qt Quick}'s
    \l {scene graph}{Qt Quick Scene Graph}.
 */

#ifndef QT_NO_DEBUG
static int qt_material_count = 0;

static void qt_print_material_count()
{
    qDebug("Number of leaked materials: %i", qt_material_count);
    qt_material_count = -1;
}
#endif

/*!
    \class QSGMaterialType
    \brief The QSGMaterialType class is used as a unique type token in combination with QSGMaterial.
    \inmodule QtQuick
    \ingroup qtquick-scenegraph-materials

    It serves no purpose outside the QSGMaterial::type() function.

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.
 */

/*!
    \class QSGMaterial
    \brief The QSGMaterial class encapsulates rendering state for a shader program.
    \inmodule QtQuick
    \ingroup qtquick-scenegraph-materials

    The QSGMaterial, QSGMaterialShader and QSGMaterialRhiShader subclasses
    form a tight relationship. For one scene graph (including nested graphs),
    there is one unique QSGMaterialShader or QSGMaterialRhiShader instance
    which encapsulates the shaders the scene graph uses to render that
    material, such as a shader to flat coloring of geometry. Each
    QSGGeometryNode can have a unique QSGMaterial containing the how the shader
    should be configured when drawing that node, such as the actual color to
    used to render the geometry.

    QSGMaterial has two virtual functions that both need to be implemented. The
    function type() should return a unique instance for all instances of a
    specific subclass. The createShader() function should return a new instance
    of QSGMaterialShader or QSGMaterialRhiShader, specific to that subclass of
    QSGMaterial.

    A minimal QSGMaterial implementation could look like this:
    \code
        class Material : public QSGMaterial
        {
        public:
            QSGMaterialType *type() const { static QSGMaterialType type; return &type; }
            QSGMaterialShader *createShader() const { return new Shader; }
        };
    \endcode

    This is suitable only for the OpenGL-based, traditional renderer of the
    scene graph. When using the new, graphics API abstracted renderer,
    materials must create QSGMaterialRhiShader instances instead, or in
    addition:
    \code
        class Material : public QSGMaterial
        {
        public:
            Material() { setFlag(SupportsRhiShader, true); }
            QSGMaterialType *type() const { static QSGMaterialType type; return &type; }
            QSGMaterialShader *createShader() {
                if (flags().testFlag(RhiShaderWanted)) {
                    return new RhiShader;
                } else {
                    // this is optional, relevant for materials that intend to be usable with the legacy OpenGL renderer as well
                    return new Shader;
                }
            }
        };
    \endcode

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.
 */

/*!
    \internal
 */

QSGMaterial::QSGMaterial()
{
    Q_UNUSED(m_reserved);
#ifndef QT_NO_DEBUG
    if (qsg_leak_check) {
        ++qt_material_count;
        static bool atexit_registered = false;
        if (!atexit_registered) {
            atexit(qt_print_material_count);
            atexit_registered = true;
        }
    }
#endif
}


/*!
    \internal
 */

QSGMaterial::~QSGMaterial()
{
#ifndef QT_NO_DEBUG
    if (qsg_leak_check) {
        --qt_material_count;
        if (qt_material_count < 0)
            qDebug("Material destroyed after qt_print_material_count() was called.");
    }
#endif
}



/*!
    \enum QSGMaterial::Flag

    \value Blending Set this flag to true if the material requires GL_BLEND to be
    enabled during rendering.

    \value RequiresDeterminant Set this flag to true if the material relies on
    the determinant of the matrix of the geometry nodes for rendering.

    \value RequiresFullMatrixExceptTranslate Set this flag to true if the material
    relies on the full matrix of the geometry nodes for rendering, except the translation part.

    \value RequiresFullMatrix Set this flag to true if the material relies on
    the full matrix of the geometry nodes for rendering.

    \value CustomCompileStep Starting with Qt 5.2, the scene graph will not always call
    QSGMaterialShader::compile() when its shader program is compiled and linked.
    Set this flag to enforce that the function is called.

    \value SupportsRhiShader Starting with Qt 5.14, the scene graph supports
    QSGMaterialRhiShader as an alternative to the OpenGL-specific
    QSGMaterialShader. Set this flag to indicate createShader() is capable of
    returning QSGMaterialRhiShader instances when the RhiShaderWanted flag is
    set.

    \value RhiShaderWanted This flag is set by the scene graph, not by the
    QSGMaterial. When set, and that can only happen when SupportsRhiShader was
    set by the material, it indicates that createShader() must return a
    QSGMaterialRhiShader instance instead of QSGMaterialShader.
 */

/*!
    \fn QSGMaterial::Flags QSGMaterial::flags() const

    Returns the material's flags.
 */



/*!
    Sets the flags \a flags on this material if \a on is true;
    otherwise clears the attribute.
*/

void QSGMaterial::setFlag(Flags flags, bool on)
{
    if (on)
        m_flags |= flags;
    else
        m_flags &= ~flags;
}



/*!
    Compares this material to \a other and returns 0 if they are equal; -1 if
    this material should sort before \a other and 1 if \a other should sort
    before.

    The scene graph can reorder geometry nodes to minimize state changes.
    The compare function is called during the sorting process so that
    the materials can be sorted to minimize state changes in each
    call to QSGMaterialShader::updateState().

    The this pointer and \a other is guaranteed to have the same type().
 */

int QSGMaterial::compare(const QSGMaterial *other) const
{
    Q_ASSERT(other && type() == other->type());
    return qint64(this) - qint64(other);
}



/*!
    \fn QSGMaterialType QSGMaterial::type() const

    This function is called by the scene graph to return a unique instance
    per subclass.
 */



/*!
    \fn QSGMaterialShader *QSGMaterial::createShader() const

    This function returns a new instance of a the QSGMaterialShader
    implementatation used to render geometry for a specific implementation
    of QSGMaterial.

    The function will be called only once for each material type that
    exists in the scene graph and will be cached internally.

    When the QSGMaterial reports SupportsRhiShader in flags(), the scene graph
    may request a QSGMaterialRhiShader instead of QSGMaterialShader. This is
    indicated by having the RhiShaderWanted flag set. In this case the return
    value must be a QSGRhiMaterialShader subclass.
*/

QT_END_NAMESPACE
