/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKCANVAS_H
#define QQUICKCANVAS_H

#include <QtCore/qmetatype.h>
#include <QtGui/qopengl.h>
#include <QtGui/qwindow.h>
#include <QtGui/qevent.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGEngine;
class QQuickItem;
class QSGTexture;
class QInputMethodEvent;
class QQuickCanvasPrivate;
class QOpenGLFramebufferObject;
class QDeclarativeIncubationController;
class QInputMethodEvent;

class Q_DECLARATIVE_EXPORT QQuickCanvas : public QWindow
{
    Q_OBJECT
    Q_PRIVATE_PROPERTY(QQuickCanvas::d_func(), QDeclarativeListProperty<QObject> data READ data DESIGNABLE false)
    Q_PROPERTY(QColor color READ clearColor WRITE setClearColor NOTIFY clearColorChanged)
    Q_CLASSINFO("DefaultProperty", "data")
    Q_DECLARE_PRIVATE(QQuickCanvas)
public:
    enum CreateTextureOption {
        TextureHasAlphaChannel  = 0x0001,
        TextureHasMipmaps       = 0x0002,
        TextureOwnsGLTexture    = 0x0004
    };

    Q_DECLARE_FLAGS(CreateTextureOptions, CreateTextureOption)

    QQuickCanvas(QWindow *parent = 0);

    virtual ~QQuickCanvas();

    QQuickItem *rootItem() const;
    QQuickItem *activeFocusItem() const;

    QQuickItem *mouseGrabberItem() const;

    bool sendEvent(QQuickItem *, QEvent *);

    QSGEngine *sceneGraphEngine() const;

    void setVSyncAnimations(bool enabled);
    bool vsyncAnimations() const;

    QImage grabFrameBuffer();

    void setRenderTarget(QOpenGLFramebufferObject *fbo);
    QOpenGLFramebufferObject *renderTarget() const;

    QDeclarativeIncubationController *incubationController() const;

    // Scene graph specific functions
    QSGTexture *createTextureFromImage(const QImage &image) const;
    QSGTexture *createTextureFromId(uint id, const QSize &size, CreateTextureOptions options = CreateTextureOption(0)) const;

    void setClearBeforeRendering(bool enabled);
    bool clearBeforeRendering() const;

    void setClearColor(const QColor &color);
    QColor clearColor() const;

    QOpenGLContext *openglContext() const;

Q_SIGNALS:
    void frameSwapped();
    void sceneGraphInitialized();
    void beforeRendering();
    void afterRendering();
    void clearColorChanged(const QColor &);

protected:
    QQuickCanvas(QQuickCanvasPrivate &dd, QWindow *parent = 0);

    virtual void exposeEvent(QExposeEvent *);
    virtual void resizeEvent(QResizeEvent *);

    virtual void showEvent(QShowEvent *);
    virtual void hideEvent(QHideEvent *);

    virtual void focusInEvent(QFocusEvent *);
    virtual void focusOutEvent(QFocusEvent *);

    virtual bool event(QEvent *);
    virtual void keyPressEvent(QKeyEvent *);
    virtual void keyReleaseEvent(QKeyEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
#ifndef QT_NO_WHEELEVENT
    virtual void wheelEvent(QWheelEvent *);
#endif

private Q_SLOTS:
    void maybeUpdate();
    void animationStarted();
    void animationStopped();

private:
    friend class QQuickItem;
    friend class QQuickCanvasRenderLoop;
    Q_DISABLE_COPY(QQuickCanvas)
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQuickCanvas *)

QT_END_HEADER

#endif // QQUICKCANVAS_H

