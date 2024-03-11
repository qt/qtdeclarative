// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "cuberenderer.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOffscreenSurface>
#include <QWindow>

CubeRenderer::CubeRenderer(QOffscreenSurface *offscreenSurface)
    : m_offscreenSurface(offscreenSurface),
      m_context(nullptr),
      m_program(nullptr),
      m_vbo(nullptr),
      m_vao(nullptr),
      m_matrixLoc(0)
{
}

CubeRenderer::~CubeRenderer()
{
    // Use a temporary offscreen surface to do the cleanup.
    // There may not be a native window surface available anymore at this stage.
    m_context->makeCurrent(m_offscreenSurface);

    delete m_program;
    delete m_vbo;
    delete m_vao;

    m_context->doneCurrent();
    delete m_context;
}

void CubeRenderer::init(QWindow *w, QOpenGLContext *share)
{
    m_context = new QOpenGLContext;
    m_context->setShareContext(share);
    m_context->setFormat(w->requestedFormat());
    m_context->create();
    if (!m_context->makeCurrent(w))
        return;

    QOpenGLFunctions *f = m_context->functions();
    f->glViewport(0, 0, w->width() * w->devicePixelRatio(), w->height() * w->devicePixelRatio());

    static const char *vertexShaderSource =
        "attribute highp vec4 vertex;\n"
        "attribute lowp vec2 coord;\n"
        "varying lowp vec2 v_coord;\n"
        "uniform highp mat4 matrix;\n"
        "void main() {\n"
        "   v_coord = coord;\n"
        "   gl_Position = matrix * vertex;\n"
        "}\n";
    static const char *fragmentShaderSource =
        "varying lowp vec2 v_coord;\n"
        "uniform sampler2D sampler;\n"
        "void main() {\n"
        "   gl_FragColor = vec4(texture2D(sampler, v_coord).rgb, 1.0);\n"
        "}\n";
    m_program = new QOpenGLShaderProgram;
    m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->bindAttributeLocation("vertex", 0);
    m_program->bindAttributeLocation("coord", 1);
    m_program->link();
    m_matrixLoc = m_program->uniformLocation("matrix");

    m_vao = new QOpenGLVertexArrayObject;
    m_vao->create();
    QOpenGLVertexArrayObject::Binder vaoBinder(m_vao);

    m_vbo = new QOpenGLBuffer;
    m_vbo->create();
    m_vbo->bind();

    GLfloat v[] = {
        -0.5, 0.5, 0.5, 0.5,-0.5,0.5,-0.5,-0.5,0.5,
        0.5, -0.5, 0.5, -0.5,0.5,0.5,0.5,0.5,0.5,
        -0.5, -0.5, -0.5, 0.5,-0.5,-0.5,-0.5,0.5,-0.5,
        0.5, 0.5, -0.5, -0.5,0.5,-0.5,0.5,-0.5,-0.5,

        0.5, -0.5, -0.5, 0.5,-0.5,0.5,0.5,0.5,-0.5,
        0.5, 0.5, 0.5, 0.5,0.5,-0.5,0.5,-0.5,0.5,
        -0.5, 0.5, -0.5, -0.5,-0.5,0.5,-0.5,-0.5,-0.5,
        -0.5, -0.5, 0.5, -0.5,0.5,-0.5,-0.5,0.5,0.5,

        0.5, 0.5,  -0.5, -0.5, 0.5,  0.5,  -0.5,  0.5,  -0.5,
        -0.5,  0.5,  0.5,  0.5,  0.5,  -0.5, 0.5, 0.5,  0.5,
        -0.5,  -0.5, -0.5, -0.5, -0.5, 0.5,  0.5, -0.5, -0.5,
        0.5, -0.5, 0.5,  0.5,  -0.5, -0.5, -0.5,  -0.5, 0.5
    };
    GLfloat texCoords[] = {
        0.0f,0.0f, 1.0f,1.0f, 1.0f,0.0f,
        1.0f,1.0f, 0.0f,0.0f, 0.0f,1.0f,
        1.0f,1.0f, 1.0f,0.0f, 0.0f,1.0f,
        0.0f,0.0f, 0.0f,1.0f, 1.0f,0.0f,

        1.0f,1.0f, 1.0f,0.0f, 0.0f,1.0f,
        0.0f,0.0f, 0.0f,1.0f, 1.0f,0.0f,
        0.0f,0.0f, 1.0f,1.0f, 1.0f,0.0f,
        1.0f,1.0f, 0.0f,0.0f, 0.0f,1.0f,

        0.0f,1.0f, 1.0f,0.0f, 1.0f,1.0f,
        1.0f,0.0f, 0.0f,1.0f, 0.0f,0.0f,
        1.0f,0.0f, 1.0f,1.0f, 0.0f,0.0f,
        0.0f,1.0f, 0.0f,0.0f, 1.0f,1.0f
    };

    const int vertexCount = 36;
    m_vbo->allocate(sizeof(GLfloat) * vertexCount * 5);
    m_vbo->write(0, v, sizeof(GLfloat) * vertexCount * 3);
    m_vbo->write(sizeof(GLfloat) * vertexCount * 3, texCoords, sizeof(GLfloat) * vertexCount * 2);
    m_vbo->release();

    if (m_vao->isCreated())
        setupVertexAttribs();
}

void CubeRenderer::resize(int w, int h)
{
    m_proj.setToIdentity();
    m_proj.perspective(45, w / float(h), 0.01f, 100.0f);
}

void CubeRenderer::setupVertexAttribs()
{
    m_vbo->bind();
    m_program->enableAttributeArray(0);
    m_program->enableAttributeArray(1);
    m_context->functions()->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    m_context->functions()->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0,
                                                  (const void *)(36 * 3 * sizeof(GLfloat)));
    m_vbo->release();
}

void CubeRenderer::render(QWindow *w, QOpenGLContext *share, uint texture)
{
    if (!m_context)
        init(w, share);

    if (!m_context->makeCurrent(w))
        return;

    QOpenGLFunctions *f = m_context->functions();
    f->glClearColor(0.0f, 0.1f, 0.25f, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (texture) {
        f->glBindTexture(GL_TEXTURE_2D, texture);
        f->glFrontFace(GL_CW); // because our cube's vertex data is such
        f->glEnable(GL_CULL_FACE);
        f->glEnable(GL_DEPTH_TEST);

        m_program->bind();
        QOpenGLVertexArrayObject::Binder vaoBinder(m_vao);
        // If VAOs are not supported, set the vertex attributes every time.
        if (!m_vao->isCreated())
            setupVertexAttribs();

        static GLfloat angle = 0;
        QMatrix4x4 m;
        m.translate(0, 0, -2);
        m.rotate(90, 0, 0, 1);
        m.rotate(angle, 0.5, 1, 0);
        angle += 0.5f;

        m_program->setUniformValue(m_matrixLoc, m_proj * m);

        // Draw the cube.
        f->glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    m_context->swapBuffers(w);
}
