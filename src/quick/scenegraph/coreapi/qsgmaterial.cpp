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

    QSGMaterial and QSGMaterialShader subclasses form a tight relationship. For
    one scene graph (including nested graphs), there is one unique
    QSGMaterialShader instance which encapsulates the shaders the scene graph
    uses to render that material, such as a shader to flat coloring of
    geometry. Each QSGGeometryNode can have a unique QSGMaterial containing the
    how the shader should be configured when drawing that node, such as the
    actual color to used to render the geometry.

    QSGMaterial has two virtual functions that both need to be implemented. The
    function type() should return a unique instance for all instances of a
    specific subclass. The createShader() function should return a new instance
    of QSGMaterialShader, specific to that subclass of QSGMaterial.

    A minimal QSGMaterial implementation could look like this:
    \code
        class Material : public QSGMaterial
        {
        public:
            QSGMaterialType *type() const override { static QSGMaterialType type; return &type; }
            QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override { return new Shader; }
        };
    \endcode

    See the \l{Scene Graph - Custom Material}{Custom Material example} for an introduction
    on implementing a QQuickItem subclass backed by a QSGGeometryNode and a custom
    material.

    \note createShader() is called only once per QSGMaterialType, to reduce
    redundant work with shader preparation. If a QSGMaterial is backed by
    multiple sets of vertex and fragment shader combinations, the implementation
    of type() must return a different, unique QSGMaterialType pointer for each
    combination of shaders.

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.

    \sa QSGMaterialShader, {Scene Graph - Custom Material}, {Scene Graph - Two Texture Providers}, {Scene Graph - Graph}
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
    const qintptr diff = qintptr(this) - qintptr(other);
    return diff < 0 ? -1 : (diff > 0 ? 1 : 0);
}



/*!
    \fn QSGMaterialType *QSGMaterial::type() const

    This function is called by the scene graph to query an identifier that is
    unique to the QSGMaterialShader instantiated by createShader().

    For many materials, the typical approach will be to return a pointer to a
    static, and so globally available, QSGMaterialType instance. The
    QSGMaterialType is an opaque object. Its purpose is only to serve as a
    type-safe, simple way to generate unique material identifiers.
    \code
        QSGMaterialType *type() const override
        {
            static QSGMaterialType type;
            return &type;
        }
    \endcode
 */



/*!
    \fn QSGMaterialShader *QSGMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const

    This function returns a new instance of a the QSGMaterialShader
    implementation used to render geometry for a specific implementation
    of QSGMaterial.

    The function will be called only once for each combination of material type and \a renderMode
    and will be cached internally.

    For most materials, the \a renderMode can be ignored. A few materials may need
    custom handling for specific render modes. For instance if the material implements
    antialiasing in a way that needs to account for perspective transformations when
    RenderMode3D is in use.
*/

QT_END_NAMESPACE
