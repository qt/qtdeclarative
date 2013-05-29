/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQUICKWINDOW_H
#define QQUICKWINDOW_H

#include <QtQuick/qtquickglobal.h>
#include <QtCore/qmetatype.h>
#include <QtGui/qopengl.h>
#include <QtGui/qwindow.h>
#include <QtGui/qevent.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QSGTexture;
class QInputMethodEvent;
class QQuickWindowPrivate;
class QOpenGLFramebufferObject;
class QQmlIncubationController;
class QInputMethodEvent;
class QQuickCloseEvent;

class Q_QUICK_EXPORT QQuickWindow : public QWindow
{
    Q_OBJECT
    Q_PRIVATE_PROPERTY(QQuickWindow::d_func(), QQmlListProperty<QObject> data READ data DESIGNABLE false)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QQuickItem* contentItem READ contentItem CONSTANT FINAL)
    Q_PROPERTY(QQuickItem* activeFocusItem READ activeFocusItem NOTIFY activeFocusItemChanged REVISION 1)
    Q_CLASSINFO("DefaultProperty", "data")
    Q_DECLARE_PRIVATE(QQuickWindow)
public:
    enum CreateTextureOption {
        TextureHasAlphaChannel  = 0x0001,
        TextureHasMipmaps       = 0x0002,
        TextureOwnsGLTexture    = 0x0004
    };

    Q_DECLARE_FLAGS(CreateTextureOptions, CreateTextureOption)

    QQuickWindow(QWindow *parent = 0);

    virtual ~QQuickWindow();

    QQuickItem *contentItem() const;

    QQuickItem *activeFocusItem() const;
    QObject *focusObject() const;

    QQuickItem *mouseGrabberItem() const;

    bool sendEvent(QQuickItem *, QEvent *);

    QImage grabWindow();

    void setRenderTarget(QOpenGLFramebufferObject *fbo);
    QOpenGLFramebufferObject *renderTarget() const;

    void setRenderTarget(uint fboId, const QSize &size);
    uint renderTargetId() const;
    QSize renderTargetSize() const;

    QQmlIncubationController *incubationController() const;

#ifndef QT_NO_ACCESSIBILITY
    virtual QAccessibleInterface *accessibleRoot() const;
#endif

    // Scene graph specific functions
    QSGTexture *createTextureFromImage(const QImage &image) const;
    QSGTexture *createTextureFromId(uint id, const QSize &size, CreateTextureOptions options = CreateTextureOption(0)) const;

    void setClearBeforeRendering(bool enabled);
    bool clearBeforeRendering() const;

    void setColor(const QColor &color);
    QColor color() const;

    static bool hasDefaultAlphaBuffer();
    static void setDefaultAlphaBuffer(bool useAlpha);

    void setPersistentOpenGLContext(bool persistent);
    bool isPersistentOpenGLContext() const;

    void setPersistentSceneGraph(bool persistent);
    bool isPersistentSceneGraph() const;

    QOpenGLContext *openglContext() const;

Q_SIGNALS:
    void frameSwapped();
    void sceneGraphInitialized();
    void sceneGraphInvalidated();
    void beforeSynchronizing();
    void beforeRendering();
    void afterRendering();
    Q_REVISION(1) void closing(QQuickCloseEvent *close);
    void colorChanged(const QColor &);
    Q_REVISION(1) void activeFocusItemChanged();

public Q_SLOTS:
    void update();
    void releaseResources();

protected:
    QQuickWindow(QQuickWindowPrivate &dd, QWindow *parent = 0);

    virtual void exposeEvent(QExposeEvent *);
    virtual void resizeEvent(QResizeEvent *);

    virtual void showEvent(QShowEvent *);
    virtual void hideEvent(QHideEvent *);
    // TODO Qt 6: reimplement QWindow::closeEvent to emit closing

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
    void cleanupSceneGraph();
    void setTransientParent_helper(QQuickWindow *window);

private:
    friend class QQuickItem;
    friend class QQuickWindowRenderLoop;
    Q_DISABLE_COPY(QQuickWindow)
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQuickWindow *)

#endif // QQUICKWINDOW_H

