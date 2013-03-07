/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "threadrenderer.h"
#include "logorenderer.h"

#include <QtCore/QMutex>
#include <QtCore/QThread>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/QGuiApplication>
#include <QtGui/QOffscreenSurface>

#include <QtQuick/QQuickWindow>
#include <qsgsimpletexturenode.h>

QList<QThread *> ThreadRenderer::threads;

/*
 * The render thread shares a context with the scene graph and will
 * render into two separate FBOs, one to use for display and one
 * to use for rendering
 */
class RenderThread : public QThread
{
    Q_OBJECT
public:
    RenderThread(const QSize &size, QOpenGLContext *context)
        : m_renderFbo(0)
        , m_displayFbo(0)
        , m_logoRenderer(0)
        , m_fakeSurface(0)
        , m_size(size)
    {
        ThreadRenderer::threads << this;

        // Set up the QOpenGLContext to use for rendering in this thread. It is sharing
        // memory space with the GL context of the scene graph. This constructor is called
        // during updatePaintNode, so we are currently on the scene graph thread with the
        // scene graph's OpenGL context current.
        m_context = new QOpenGLContext();
        m_context->setShareContext(context);
        m_context->setFormat(context->format());
        m_context->moveToThread(this);

        // We need a non-visible surface to make current in the other thread
        // and QWindows must be created and managed on the GUI thread.
        m_fakeSurface = new QOffscreenSurface();
        m_fakeSurface->setFormat(context->format());
        m_fakeSurface->create();
    }

    void setSurface(QOffscreenSurface *surface) { m_fakeSurface = surface; }

public slots:
    void renderNext()
    {
        if (!m_context->isValid())
            m_context->create();

        m_context->makeCurrent(m_fakeSurface);

        if (!m_renderFbo) {
            // Initialize the buffers and renderer
            QOpenGLFramebufferObjectFormat format;
            format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
            m_renderFbo = new QOpenGLFramebufferObject(m_size, format);
            m_displayFbo = new QOpenGLFramebufferObject(m_size, format);
            m_logoRenderer = new LogoRenderer();
            m_logoRenderer->initialize();
        }

        m_renderFbo->bind();
        glViewport(0, 0, m_size.width(), m_size.height());

        m_logoRenderer->render();

        // We need to flush the contents to the FBO before posting
        // the texture to the other thread, otherwise, we might
        // get unexpected results.
        glFlush();

        m_renderFbo->bindDefault();
        qSwap(m_renderFbo, m_displayFbo);

        emit textureReady(m_displayFbo->texture(), m_size);
    }

    void shutDown()
    {
        m_context->makeCurrent(m_fakeSurface);
        delete m_renderFbo;
        delete m_displayFbo;
        delete m_logoRenderer;
        m_context->doneCurrent();
        delete m_context;

        // schedule this to be deleted only after we're done cleaning up
        m_fakeSurface->deleteLater();

        // Stop event processing, move the thread to GUI and make sure it is deleted.
        exit();
        moveToThread(QGuiApplication::instance()->thread());
    }

signals:
    void textureReady(int id, const QSize &size);

private:
    QOpenGLFramebufferObject *m_renderFbo;
    QOpenGLFramebufferObject *m_displayFbo;

    LogoRenderer *m_logoRenderer;

    QOffscreenSurface *m_fakeSurface;
    QOpenGLContext *m_context;
    QSize m_size;
};



class TextureNode : public QObject, public QSGSimpleTextureNode
{
    Q_OBJECT

public:
    TextureNode(QQuickWindow *window)
        : m_id(0)
        , m_size(0, 0)
        , m_texture(0)
        , m_window(window)
    {
        // Our texture node must have a texture, so use the default 0 texture.
        m_texture = m_window->createTextureFromId(0, QSize(1, 1));
        setTexture(m_texture);
        setFiltering(QSGTexture::Linear);
    }

    ~TextureNode()
    {
        delete m_texture;
    }

signals:
    void textureInUse();
    void pendingNewTexture();

public slots:

    // This function gets called on the FBO rendering thread and will store the
    // texture id and size and schedule an update on the window.
    void newTexture(int id, const QSize &size) {
        m_mutex.lock();
        m_id = id;
        m_size = size;
        m_mutex.unlock();

        // We cannot call QQuickWindow::update directly here, as this is only allowed
        // from the rendering thread or GUI thread.
        emit pendingNewTexture();
    }


    // Before the scene graph starts to render, we update to the pending texture
    void prepareNode() {
        m_mutex.lock();
        int newId = m_id;
        QSize size = m_size;
        m_id = 0;
        m_mutex.unlock();
        if (newId) {
            delete m_texture;
            m_texture = m_window->createTextureFromId(newId, size);
            setTexture(m_texture);

            // This will notify the rendering thread that the texture is now being rendered
            // and it can start rendering to the other one.
            emit textureInUse();
        }
    }

private:

    int m_id;
    QSize m_size;

    QMutex m_mutex;

    QSGTexture *m_texture;
    QQuickWindow *m_window;
};



ThreadRenderer::ThreadRenderer()
    : m_renderThread(0)
{
    setFlag(ItemHasContents, true);
    polish();
}

void ThreadRenderer::updatePolish()
{
    if (!window() || !window()->openglContext())
        return;

    m_renderThread = new RenderThread(QSize(512, 512), window()->openglContext());
    m_renderThread->moveToThread(m_renderThread);
    m_renderThread->start();
    connect(window(), SIGNAL(sceneGraphInvalidated()), m_renderThread, SLOT(shutDown()), Qt::QueuedConnection);
}

QSGNode *ThreadRenderer::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (!m_renderThread) {
        polish();
        update();
        return 0;
    }

    TextureNode *node = static_cast<TextureNode *>(oldNode);

    if (!node) {
        node = new TextureNode(window());

        /* Set up connections to get the production of FBO textures in sync with vsync on the
         * rendering thread.
         *
         * When a new texture is ready on the rendering thread, we use a direct connection to
         * the texture node to let it know a new texture can be used. The node will then
         * emit pendingNewTexture which we bind to QQuickWindow::update to schedule a redraw.
         *
         * When the scene graph starts rendering the next frame, the prepareNode() function
         * is used to update the node with the new texture. Once it completes, it emits
         * textureInUse() which we connect to the FBO rendering thread's renderNext() to have
         * it start producing content into its current "back buffer".
         *
         * This FBO rendering pipeline is throttled by vsync on the scene graph rendering thread.
         */
        connect(m_renderThread, SIGNAL(textureReady(int,QSize)), node, SLOT(newTexture(int,QSize)), Qt::DirectConnection);
        connect(node, SIGNAL(pendingNewTexture()), window(), SLOT(update()), Qt::QueuedConnection);
        connect(window(), SIGNAL(beforeRendering()), node, SLOT(prepareNode()), Qt::DirectConnection);
        connect(node, SIGNAL(textureInUse()), m_renderThread, SLOT(renderNext()), Qt::QueuedConnection);

        // Get the production of FBO textures started..
        QMetaObject::invokeMethod(m_renderThread, "renderNext", Qt::QueuedConnection);
    }

    node->setRect(boundingRect());

    return node;
}

#include "threadrenderer.moc"
