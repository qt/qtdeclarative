// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickopenglutils.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <private/qopenglvertexarrayobject_p.h>

QT_BEGIN_NAMESPACE

/*!
    \namespace QQuickOpenGLUtils
    \inmodule QtQuick
    \since 6.0

    \brief The QQuickOpenGLUtils namespace contains utilities for Qt
    Quick when used with an OpenGL backend.
*/

/*!
    Call this function to reset the current OpenGL context its default state.

    The scene graph uses the OpenGL context and will both rely on and
    clobber its state. When mixing raw OpenGL commands with scene
    graph rendering, this function provides a convenient way of
    resetting the OpenGL context state back to its default values.

    This function does not touch state in the fixed-function pipeline.

    \warning This function will only reset the OpenGL context in
    relation to what may be changed internally as part of the OpenGL
    scene graph. It does not reset anything that has been changed
    externally such as direct OpenGL calls done inside the application
    code if those same calls are not used internally (for example,
    various OpenGL 3.x or 4.x specific state).

    \since 6.0
*/
void QQuickOpenGLUtils::resetOpenGLState()
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx)
        return;

    QOpenGLFunctions *gl = ctx->functions();

    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    QOpenGLVertexArrayObjectHelper *vaoHelper = QOpenGLVertexArrayObjectHelper::vertexArrayObjectHelperForContext(ctx);
    if (vaoHelper->isValid())
        vaoHelper->glBindVertexArray(0);

    if (ctx->isOpenGLES() || (gl->openGLFeatures() & QOpenGLFunctions::FixedFunctionPipeline)) {
        int maxAttribs;
        gl->glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
        for (int i=0; i<maxAttribs; ++i) {
            gl->glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
            gl->glDisableVertexAttribArray(i);
        }
    }

    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, 0);

    gl->glDisable(GL_DEPTH_TEST);
    gl->glDisable(GL_STENCIL_TEST);
    gl->glDisable(GL_SCISSOR_TEST);

    gl->glColorMask(true, true, true, true);
    gl->glClearColor(0, 0, 0, 0);

    gl->glDepthMask(true);
    gl->glDepthFunc(GL_LESS);
    gl->glClearDepthf(1);

    gl->glStencilMask(0xff);
    gl->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    gl->glStencilFunc(GL_ALWAYS, 0, 0xff);

    gl->glDisable(GL_BLEND);
    gl->glBlendFunc(GL_ONE, GL_ZERO);

    gl->glUseProgram(0);

    QOpenGLFramebufferObject::bindDefault();
}

QT_END_NAMESPACE
