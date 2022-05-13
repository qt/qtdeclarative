// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CUBERENDERER_H
#define CUBERENDERER_H

#include <QMatrix4x4>

QT_FORWARD_DECLARE_CLASS(QOpenGLContext)
QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)
QT_FORWARD_DECLARE_CLASS(QOpenGLBuffer)
QT_FORWARD_DECLARE_CLASS(QOpenGLVertexArrayObject)
QT_FORWARD_DECLARE_CLASS(QWindow)
QT_FORWARD_DECLARE_CLASS(QOffscreenSurface)

class CubeRenderer
{
public:
    CubeRenderer(QOffscreenSurface *offscreenSurface);
    ~CubeRenderer();

    void resize(int w, int h);
    void render(QWindow *w, QOpenGLContext *share, uint texture);

private:
    void init(QWindow *w, QOpenGLContext *share);
    void setupVertexAttribs();

    QOffscreenSurface *m_offscreenSurface;
    QOpenGLContext *m_context;
    QOpenGLShaderProgram *m_program;
    QOpenGLBuffer *m_vbo;
    QOpenGLVertexArrayObject *m_vao;
    int m_matrixLoc;
    QMatrix4x4 m_proj;
};

#endif
