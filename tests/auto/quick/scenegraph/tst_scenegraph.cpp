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

#include <QtQuick>

#include <private/qsgcontext_p.h>


#include <QtQml>

class tst_SceneGraph : public QObject
{
    Q_OBJECT

private slots:
    void manyWindows_data();
    void manyWindows();

};

template <typename T> class ScopedList : public QList<T> {
public:
    ~ScopedList() { qDeleteAll(*this); }
};

QQuickView *createView(const QString &file, QWindow *parent = 0, int x = -1, int y = -1, int w = -1, int h = -1)
{
    QQuickView *view = new QQuickView(parent);
    view->setSource(QUrl::fromLocalFile("data/" + file));
    if (x >= 0 && y >= 0) view->setPosition(x, y);
    if (w >= 0 && h >= 0) view->resize(w, h);
    view->show();
    return view;
}

QImage showAndGrab(const QString &file, int w, int h)
{
    QQuickView view;
    view.setSource(QUrl::fromLocalFile(QStringLiteral("data/") + file));
    if (w >= 0 && h >= 0)
        view.resize(w, h);
    view.create();
    return view.grabWindow();
}

// Assumes the images are opaque white...
bool containsSomethingOtherThanWhite(const QImage &image)
{
    Q_ASSERT(image.format() == QImage::Format_ARGB32_Premultiplied
             || image.format() == QImage::Format_RGB32);
    int w = image.width();
    int h = image.height();
    for (int y=0; y<h; ++y) {
        const uint *pixels = (const uint *) image.constScanLine(y);
        for (int x=0; x<w; ++x)
            if (pixels[x] != 0xffffffff)
                return true;
    }
    return false;
}

// When running on native Nvidia graphics cards on linux, the
// distance field glyph pixels have a measurable, but not visible
// pixel error. Use a custom compare function to avoid
//
// This was GT-216 with the ubuntu "nvidia-319" driver package.
// llvmpipe does not show the same issue.
//
bool compareImages(const QImage &ia, const QImage &ib)
{
    if (ia.size() != ib.size())
        qDebug() << "images are of different size" << ia.size() << ib.size();
    Q_ASSERT(ia.size() == ib.size());
    Q_ASSERT(ia.format() == ib.format());

    int w = ia.width();
    int h = ia.height();
    const int tolerance = 1;
    for (int y=0; y<h; ++y) {
        const uint *as= (const uint *) ia.constScanLine(y);
        const uint *bs= (const uint *) ib.constScanLine(y);
        for (int x=0; x<w; ++x) {
            uint a = as[x];
            uint b = bs[x];

            // No tolerance for error in the alpha.
            if ((a & 0xff000000) != (b & 0xff000000))
                return false;
            if (qAbs(qRed(a) - qRed(b)) > tolerance)
                return false;
            if (qAbs(qRed(a) - qRed(b)) > tolerance)
                return false;
            if (qAbs(qRed(a) - qRed(b)) > tolerance)
                return false;
        }
    }
    return true;
}

void tst_SceneGraph::manyWindows_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<bool>("toplevel");
    QTest::addColumn<bool>("shared");

    QTest::newRow("image,toplevel") << QStringLiteral("manyWindows_image.qml") << true << false;
    QTest::newRow("image,subwindow") << QStringLiteral("manyWindows_image.qml") << false << false;
    QTest::newRow("dftext,toplevel") << QStringLiteral("manyWindows_dftext.qml") << true << false;
    QTest::newRow("dftext,subwindow") << QStringLiteral("manyWindows_dftext.qml") << false << false;
    QTest::newRow("ntext,toplevel") << QStringLiteral("manyWindows_ntext.qml") << true << false;
    QTest::newRow("ntext,subwindow") << QStringLiteral("manyWindows_ntext.qml") << false << false;
    QTest::newRow("rects,toplevel") << QStringLiteral("manyWindows_rects.qml") << true << false;
    QTest::newRow("rects,subwindow") << QStringLiteral("manyWindows_rects.qml") << false << false;

    QTest::newRow("image,toplevel,sharing") << QStringLiteral("manyWindows_image.qml") << true << true;
    QTest::newRow("image,subwindow,sharing") << QStringLiteral("manyWindows_image.qml") << false << true;
    QTest::newRow("dftext,toplevel,sharing") << QStringLiteral("manyWindows_dftext.qml") << true << true;
    QTest::newRow("dftext,subwindow,sharing") << QStringLiteral("manyWindows_dftext.qml") << false << true;
    QTest::newRow("ntext,toplevel,sharing") << QStringLiteral("manyWindows_ntext.qml") << true << true;
    QTest::newRow("ntext,subwindow,sharing") << QStringLiteral("manyWindows_ntext.qml") << false << true;
    QTest::newRow("rects,toplevel,sharing") << QStringLiteral("manyWindows_rects.qml") << true << true;
    QTest::newRow("rects,subwindow,sharing") << QStringLiteral("manyWindows_rects.qml") << false << true;
}

struct ShareContextResetter {
public:
    ~ShareContextResetter() { QSGContext::setSharedOpenGLContext(0); }
};

void tst_SceneGraph::manyWindows()
{
    QFETCH(QString, file);
    QFETCH(bool, toplevel);
    QFETCH(bool, shared);

    QOpenGLContext sharedGLContext;
    ShareContextResetter cleanup; // To avoid dangling pointer in case of test-failure.
    if (shared) {
        sharedGLContext.create();
        QSGContext::setSharedOpenGLContext(&sharedGLContext);
    }

    QScopedPointer<QWindow> parent;
    if (!toplevel) {
        parent.reset(new QWindow());
        parent->resize(200, 200);
        parent->show();
    }

    ScopedList <QQuickView *> views;

    const int COUNT = 4;

    QImage baseLine;
    for (int i=0; i<COUNT; ++i) {
        views << createView(file, parent.data(), (i % 2) * 100, (i / 2) * 100, 100, 100);
    }
    for (int i=0; i<COUNT; ++i) {
        QQuickView *view = views.at(i);
        QTest::qWaitForWindowExposed(view);
        QImage content = view->grabWindow();
        if (i == 0) {
            baseLine = content;
            QVERIFY(containsSomethingOtherThanWhite(baseLine));
        } else {
            QVERIFY(compareImages(content, baseLine));
        }
    }

    // Wipe and recreate one (scope pointer delets it...)
    delete views.takeLast();
    QQuickView *last = createView(file, parent.data(), 100, 100, 100, 100);
    QTest::qWaitForWindowExposed(last);
    views << last;
    QVERIFY(compareImages(baseLine, last->grabWindow()));

    // Wipe and recreate all
    qDeleteAll(views);
    views.clear();

    for (int i=0; i<COUNT; ++i) {
        views << createView(file, parent.data(), (i % 2) * 100, (i / 2) * 100, 100, 100);
    }
    for (int i=0; i<COUNT; ++i) {
        QQuickView *view = views.at(i);
        QTest::qWaitForWindowExposed(view);
        QImage content = view->grabWindow();
        QVERIFY(compareImages(content, baseLine));
    }
}


#include "tst_scenegraph.moc"

QTEST_MAIN(tst_SceneGraph)

