/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <qtest.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qscreen.h>
#include <private/qsgrendernode_p.h>

#include "../../shared/util.h"

class tst_rendernode: public QQmlDataTest
{
    Q_OBJECT
public:
    tst_rendernode();

    QImage runTest(const QString &fileName)
    {
        QQuickView view(&outerWindow);
        view.setResizeMode(QQuickView::SizeViewToRootObject);
        view.setSource(testFileUrl(fileName));
        view.setVisible(true);
        QTest::qWaitForWindowExposed(&view);
        return view.grabWindow();
    }

    //It is important for platforms that only are able to show fullscreen windows
    //to have a container for the window that is painted on.
    QQuickWindow outerWindow;

private slots:
    void renderOrder();
    void messUpState();
};

class ClearNode : public QSGRenderNode
{
public:
    virtual StateFlags changedStates()
    {
        return ColorState;
    }

    virtual void render(const RenderState &)
    {
        // If clip has been set, scissoring will make sure the right area is cleared.
        glClearColor(color.redF(), color.greenF(), color.blueF(), 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    QColor color;
};

class ClearItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
public:
    ClearItem() : m_color(Qt::black)
    {
        setFlag(ItemHasContents, true);
    }

    QColor color() const { return m_color; }
    void setColor(const QColor &color)
    {
        if (color == m_color)
            return;
        m_color = color;
        emit colorChanged();
    }

protected:
    virtual QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
    {
        ClearNode *node = static_cast<ClearNode *>(oldNode);
        if (!node)
            node = new ClearNode;
        node->color = m_color;
        return node;
    }

Q_SIGNALS:
    void colorChanged();

private:
    QColor m_color;
};

class MessUpNode : public QSGRenderNode
{
public:
    virtual StateFlags changedStates()
    {
        return StateFlags(DepthState) | StencilState | ScissorState | ColorState | BlendState
                | CullState | ViewportState;
    }

    virtual void render(const RenderState &)
    {
        // Don't draw anything, just mess up the state
        glViewport(10, 10, 10, 10);
        glDisable(GL_SCISSOR_TEST);
        glDepthMask(true);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_EQUAL);
#if defined(QT_OPENGL_ES)
        glClearDepthf(1);
#else
        glClearDepth(1);
#endif
        glClearStencil(42);
        glClearColor(1.0f, 0.5f, 1.0f, 0.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_SCISSOR_TEST);
        glScissor(190, 190, 10, 10);
        glStencilFunc(GL_EQUAL, 28, 0xff);
        glBlendFunc(GL_ZERO, GL_ZERO);
        GLint frontFace;
        glGetIntegerv(GL_FRONT_FACE, &frontFace);
        glFrontFace(frontFace == GL_CW ? GL_CCW : GL_CW);
        glEnable(GL_CULL_FACE);
    }
};

class MessUpItem : public QQuickItem
{
    Q_OBJECT
public:
    MessUpItem()
    {
        setFlag(ItemHasContents, true);
    }

protected:
    virtual QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
    {
        MessUpNode *node = static_cast<MessUpNode *>(oldNode);
        if (!node)
            node = new MessUpNode;
        return node;
    }
};

tst_rendernode::tst_rendernode()
{
    qmlRegisterType<ClearItem>("Test", 1, 0, "ClearItem");
    qmlRegisterType<MessUpItem>("Test", 1, 0, "MessUpItem");
    outerWindow.showNormal();
    outerWindow.setGeometry(0,0,400,400);
}

static bool fuzzyCompareColor(QRgb x, QRgb y, QByteArray *errorMessage)
{
    enum { fuzz = 4 };
    if (qAbs(qRed(x) - qRed(y)) >= fuzz || qAbs(qGreen(x) - qGreen(y)) >= fuzz || qAbs(qBlue(x) - qBlue(y)) >= fuzz) {
        QString s;
        QDebug(&s).nospace() << hex << "Color mismatch 0x" << x << " 0x" << y << dec << " (fuzz=" << fuzz << ").";
        *errorMessage = s.toLocal8Bit();
        return false;
    }
    return true;
}

static inline QByteArray msgColorMismatchAt(const QByteArray &colorMsg, int x, int y)
{
    return colorMsg + QByteArrayLiteral(" at ") + QByteArray::number(x) +',' + QByteArray::number(y);
}

/* The test draws four rects, each 100x100 and verifies
 * that a rendernode which calls glClear() is stacked
 * correctly. The red rectangles come under the white
 * and are obscured.
 */
void tst_rendernode::renderOrder()
{
    if (QGuiApplication::primaryScreen()->depth() < 24)
        QSKIP("This test does not work at display depths < 24");
    QImage fb = runTest("RenderOrder.qml");

    QCOMPARE(fb.width(), 200);
    QCOMPARE(fb.height(), 200);

    QCOMPARE(fb.pixel(50, 50), qRgb(0xff, 0xff, 0xff));
    QCOMPARE(fb.pixel(50, 150), qRgb(0xff, 0xff, 0xff));
    QCOMPARE(fb.pixel(150, 50), qRgb(0x00, 0x00, 0xff));

    QByteArray errorMessage;
    QVERIFY2(fuzzyCompareColor(fb.pixel(150, 150), qRgb(0x7f, 0x7f, 0xff), &errorMessage),
             msgColorMismatchAt(errorMessage, 150, 150).constData());
}

/* The test uses a number of nested rectangles with clipping
 * and rotation to verify that using a render node which messes
 * with the state does not break rendering that comes after it.
 */
void tst_rendernode::messUpState()
{
    if (QGuiApplication::primaryScreen()->depth() < 24)
        QSKIP("This test does not work at display depths < 24");
    QImage fb = runTest("MessUpState.qml");
    int x1 = 0;
    int x2 = fb.width() / 2;
    int x3 = fb.width() - 1;
    int y1 = 0;
    int y2 = fb.height() * 3 / 16;
    int y3 = fb.height() / 2;
    int y4 = fb.height() * 13 / 16;
    int y5 = fb.height() - 1;

    QCOMPARE(fb.pixel(x1, y3), qRgb(0xff, 0xff, 0xff));
    QCOMPARE(fb.pixel(x3, y3), qRgb(0xff, 0xff, 0xff));

    QCOMPARE(fb.pixel(x2, y1), qRgb(0x00, 0x00, 0x00));
    QCOMPARE(fb.pixel(x2, y2), qRgb(0x00, 0x00, 0x00));
    QByteArray errorMessage;
    QVERIFY2(fuzzyCompareColor(fb.pixel(x2, y3), qRgb(0x7f, 0x00, 0x7f), &errorMessage),
             msgColorMismatchAt(errorMessage, x2, y3).constData());
    QCOMPARE(fb.pixel(x2, y4), qRgb(0x00, 0x00, 0x00));
    QCOMPARE(fb.pixel(x2, y5), qRgb(0x00, 0x00, 0x00));
}


QTEST_MAIN(tst_rendernode)

#include "tst_rendernode.moc"
