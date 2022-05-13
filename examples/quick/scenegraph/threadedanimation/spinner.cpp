// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "spinner.h"

#include <QtQuick/QQuickWindow>
#include <QtGui/QScreen>
#include <QtQuick/QSGSimpleTextureNode>

#include <QtGui/QConicalGradient>
#include <QtGui/QPainter>

class SpinnerNode : public QObject, public QSGTransformNode
{
    Q_OBJECT
public:
    SpinnerNode(QQuickWindow *window)
        : m_rotation(0)
        , m_spinning(false)
        , m_window(window)
    {
        connect(window, &QQuickWindow::beforeRendering, this, &SpinnerNode::maybeRotate, Qt::DirectConnection);
        connect(window, &QQuickWindow::frameSwapped, this, &SpinnerNode::maybeUpdate, Qt::DirectConnection);

        QImage image(":/scenegraph/threadedanimation/spinner.png");
        m_texture = window->createTextureFromImage(image);
        QSGSimpleTextureNode *textureNode = new QSGSimpleTextureNode();
        textureNode->setTexture(m_texture);
        textureNode->setRect(0, 0, image.width(), image.height());
        textureNode->setFiltering(QSGTexture::Linear);
        appendChildNode(textureNode);
    }

    ~SpinnerNode() override {
        delete m_texture;
    }

    void setSpinning(bool spinning)
    {
        m_spinning = spinning;
    }

public slots:
    void maybeRotate() {
        if (m_spinning) {
            m_rotation += (360 / m_window->screen()->refreshRate());
            QMatrix4x4 matrix;
            matrix.translate(32, 32);
            matrix.rotate(m_rotation, 0, 0, 1);
            matrix.translate(-32, -32);
            setMatrix(matrix);

            // If we're inside a QQuickWidget, this call is necessary to ensure the widget gets updated.
            m_window->update();
        }
    }

    void maybeUpdate() {
        if (m_spinning) {
            m_window->update();
        }
    }

private:
    qreal m_rotation;
    bool m_spinning;
    QSGTexture *m_texture;
    QQuickWindow *m_window;
};

Spinner::Spinner()
    : m_spinning(false)
{
    setSize(QSize(64, 64));
    setFlag(ItemHasContents);
}

void Spinner::setSpinning(bool spinning)
{
    if (spinning == m_spinning)
        return;
    m_spinning = spinning;
    emit spinningChanged();
    update();
}

QSGNode *Spinner::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
{
    SpinnerNode *n = static_cast<SpinnerNode *>(old);
    if (!n)
        n = new SpinnerNode(window());

    n->setSpinning(m_spinning);

    return n;
}

#include "spinner.moc"
