/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQUICKWIDGET_P_H
#define QQUICKWIDGET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qquickwidget.h"
#include <private/qwidget_p.h>

#include <QtCore/qurl.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qtimer.h>
#include <QtCore/qpointer.h>
#include <QtCore/QWeakPointer>

#include <QtQml/qqmlengine.h>

#include "private/qquickitemchangelistener_p.h"

QT_BEGIN_NAMESPACE

class QQmlContext;
class QQmlError;
class QQuickItem;
class QQmlComponent;
class QQuickRenderControl;
class QOpenGLContext;
class QOffscreenSurface;

class QQuickWidgetPrivate
        : public QWidgetPrivate,
          public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickWidget)
public:
    static QQuickWidgetPrivate* get(QQuickWidget *view) { return view->d_func(); }
    static const QQuickWidgetPrivate* get(const QQuickWidget *view) { return view->d_func(); }

    QQuickWidgetPrivate();
    ~QQuickWidgetPrivate();

    void execute();
    void itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &oldGeometry) override;
    void initResize();
    void updateSize();
    void updatePosition();
    void updateFrambufferObjectSize();
    void setRootObject(QObject *);
    void render(bool needsSync);
    void renderSceneGraph();
    void createContext();
    void destroyContext();
    void handleContextCreationFailure(const QSurfaceFormat &format);

#if QT_CONFIG(opengl)
    GLuint textureId() const override;
    QPlatformTextureList::Flags textureListFlags() override;
    QImage grabFramebuffer() override;
#else
    QImage grabFramebuffer();
#endif

    void init(QQmlEngine* e = 0);
    void ensureEngine() const;
    void handleWindowChange();
    void invalidateRenderControl();

    QSize rootObjectSize() const;

    QPointer<QQuickItem> root;

    QUrl source;

    mutable QPointer<QQmlEngine> engine;
    QQmlComponent *component;
    QBasicTimer resizetimer;
    QQuickWindow *offscreenWindow;
    QOffscreenSurface *offscreenSurface;
    QQuickRenderControl *renderControl;

#if QT_CONFIG(opengl)
    QOpenGLFramebufferObject *fbo;
    QOpenGLFramebufferObject *resolvedFbo;
    QOpenGLContext *context;
#endif

    QQuickWidget::ResizeMode resizeMode;
    QSize initialSize;
    QElapsedTimer frameTimer;

    QBasicTimer updateTimer;
    bool eventPending;
    bool updatePending;
    bool fakeHidden;

    int requestedSamples;

    bool useSoftwareRenderer;
    QImage softwareImage;
    QRegion updateRegion;
    bool forceFullUpdate;
};

QT_END_NAMESPACE

#endif // QQuickWidget_P_H
