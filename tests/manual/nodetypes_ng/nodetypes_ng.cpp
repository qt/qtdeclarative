/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
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

#include <QGuiApplication>
#include <QThread>
#include <QQuickView>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickPaintedItem>
#include <QPainter>
#include <QTimer>
#include <QQuickItemGrabResult>
#include <QSGRectangleNode>

class Helper : public QObject
{
    Q_OBJECT

public:
    Helper(QQuickWindow *w) : m_window(w) { }

    Q_INVOKABLE void sleep(int ms) {
        QThread::msleep(ms);
    }

    Q_INVOKABLE void testGrabWindow() {
        QImage img = m_window->grabWindow();
        qDebug() << "Saving image to grab_window_result.png" << img;
        img.save("grab_window_result.png");
    }

    Q_INVOKABLE void testGrabItem(QQuickItem *item) {
        qDebug() << item;
        if (!item)
            return;

        QSharedPointer<QQuickItemGrabResult> result = item->grabToImage();
        if (!result)
            return;

        auto f = [](const QImage &image) {
            qDebug() << "Saving image to grab_item_result.png" << image;
            image.save("grab_item_result.png");
        };
        if (result->image().isNull()) {
            connect(result.data(), &QQuickItemGrabResult::ready, [f, result] {
                f(result->image());
            });
        } else {
            f(result->image());
        }
    }

    QQuickWindow *m_window;
};

class TextBalloon : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(bool rightAligned READ isRightAligned WRITE setRightAligned NOTIFY rightAlignedChanged)
    Q_PROPERTY(bool innerAnim READ innerAnimEnabled WRITE setInnerAnimEnabled NOTIFY innerAnimChanged)

public:
    TextBalloon(QQuickItem *parent = nullptr) : QQuickPaintedItem(parent) {
        connect(&m_timer, &QTimer::timeout, this, &TextBalloon::onAnim);
        m_timer.setInterval(500);
    }
    void paint(QPainter *painter);

    bool isRightAligned() { return m_rightAligned; }
    void setRightAligned(bool rightAligned);

    bool innerAnimEnabled() const { return m_innerAnim; }
    void setInnerAnimEnabled(bool b);

signals:
    void rightAlignedChanged();
    void innerAnimChanged();

private slots:
    void onAnim();

private:
    bool m_rightAligned = false;
    bool m_innerAnim = false;
    QTimer m_timer;
    QRect m_animRect = QRect(10, 10, 50, 20);
    int m_anim = 0;
};

void TextBalloon::paint(QPainter *painter)
{
    QBrush brush(QColor("#007430"));

    painter->setBrush(brush);
    painter->setPen(Qt::NoPen);
    painter->setRenderHint(QPainter::Antialiasing);

    painter->drawRoundedRect(0, 0, boundingRect().width(), boundingRect().height() - 10, 10, 10);

    if (m_rightAligned) {
        const QPointF points[3] = {
            QPointF(boundingRect().width() - 10.0, boundingRect().height() - 10.0),
            QPointF(boundingRect().width() - 20.0, boundingRect().height()),
            QPointF(boundingRect().width() - 30.0, boundingRect().height() - 10.0),
        };
        painter->drawConvexPolygon(points, 3);
    } else {
        const QPointF points[3] = {
            QPointF(10.0, boundingRect().height() - 10.0),
            QPointF(20.0, boundingRect().height()),
            QPointF(30.0, boundingRect().height() - 10.0),
        };
        painter->drawConvexPolygon(points, 3);
    }

    if (m_innerAnim) {
        painter->fillRect(m_animRect, Qt::lightGray);
        const int x = m_animRect.x() + m_anim;
        const int y = m_animRect.y() + m_animRect.height() / 2;
        painter->setPen(QPen(QBrush(Qt::SolidLine), 4));
        painter->drawLine(x + 4, y, x + 10, y);
        m_anim += 10;
        if (m_anim > m_animRect.width())
            m_anim = 0;
    }
}

void TextBalloon::setRightAligned(bool rightAligned)
{
    if (m_rightAligned == rightAligned)
        return;

    m_rightAligned = rightAligned;
    emit rightAlignedChanged();
}

void TextBalloon::setInnerAnimEnabled(bool b)
{
    if (m_innerAnim == b)
        return;

    m_innerAnim = b;
    if (!b)
        m_timer.stop();
    else
        m_timer.start();
    emit innerAnimChanged();
}

void TextBalloon::onAnim()
{
    update(m_animRect);
}

class PerPixelRect : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    PerPixelRect();
    void setColor(const QColor &c);
    QColor color() const { return m_color; }
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *);

signals:
    void colorChanged(const QColor &c);

private:
    QColor m_color;
};

PerPixelRect::PerPixelRect()
{
    setFlag(ItemHasContents);
}

void PerPixelRect::setColor(const QColor &c)
{
    if (c == m_color)
        return;
    m_color = c;
    emit colorChanged(c);
}

QSGNode *PerPixelRect::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    delete node;
    node = new QSGNode;

    const int w = width();
    const int h = height();
    QQuickWindow *win = window();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            QSGRectangleNode *rn = win->createRectangleNode();
            rn->setRect(x, y, 1, 1);
            rn->setColor(m_color);
            node->appendChildNode(rn);
        }
    }

    return node;
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    qDebug("Available tests:");
    qDebug("  [R] - Simple rectangle (vertexcolor material)");
    qDebug("  [3] - Rectangles (incl. smoothcolor material, scissor)");
    qDebug("  [4] - A lot of rectangles (incl. stencil and scissor)");
    qDebug("  [5] - Rectangles with multiple clip list entries");
    qDebug("  [I] - Images");
    qDebug("  [A] - Atlased images");
    qDebug("  [P] - QQuickPaintedItem");
    qDebug("  [C] - Compressed textures");
    qDebug("  [T] - Text (native)");
    qDebug("  [D] - Text (distance field)");
    qDebug("  [L] - Layers");
    qDebug("  [6] - ShaderEffectSource without ShaderEffect");
    qDebug("  [E] - ShaderEffect (and GraphicsInfo)");
    qDebug("  [Z] - ShaderEffect without animated properties");
    qDebug("  [G] - Grab current window");
    qDebug("  [F] - Grab item");
    qDebug("  [W] - Multiple windows");
    qDebug("  [N] - Lots of rectangle nodes");
    qDebug("\nPress S to stop the currently running test\n");

    QQuickView view;
    Helper helper(&view);

    const bool usingRhi = qEnvironmentVariableIntValue("QSG_RHI") != 0;
    const QString rhiBackend = QString::fromLatin1(qgetenv("QSG_RHI_BACKEND"));
    if (usingRhi)
        view.setTitle(QLatin1String("RHI: ") + (rhiBackend.isEmpty() ? QLatin1String("default") : rhiBackend));
    else
        view.setTitle(QLatin1String("legacy OpenGL"));

    if (app.arguments().contains(QLatin1String("--multisample"))) {
        qDebug("Requesting sample count 4");
        QSurfaceFormat fmt = view.format();
        fmt.setSamples(4);
        fmt.setDepthBufferSize(24);
        fmt.setStencilBufferSize(8);
        view.setFormat(fmt);
    }
    if (app.arguments().contains(QLatin1String("--coreprofile"))) {
        qDebug("Requesting core profile (applicable only with OpenGL)");
        QSurfaceFormat fmt = view.format();
        fmt.setVersion(3, 2);
        fmt.setProfile(QSurfaceFormat::CoreProfile);
        view.setFormat(fmt);
    }
    if (app.arguments().contains(QLatin1String("--transparent"))) {
        qDebug("Requesting alpha channel for the window and using Qt::transparent as background");
        QSurfaceFormat fmt = view.format();
        fmt.setAlphaBufferSize(8);
        view.setFormat(fmt);
        view.setColor(Qt::transparent);
    }

    view.engine()->rootContext()->setContextProperty(QLatin1String("helper"), &helper);

    qmlRegisterType<TextBalloon>("Stuff", 1, 0, "TextBalloon");
    qmlRegisterType<PerPixelRect>("Stuff", 1, 0, "PerPixelRect");

    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(1024, 768);
    view.setSource(QUrl("qrc:/main.qml"));
    view.show();

    return app.exec();
}

#include "nodetypes_ng.moc"
