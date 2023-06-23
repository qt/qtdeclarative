// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "visualtestutils_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/private/qvariantanimation_p.h>
#include <QtCore/QDebug>
#include <QtQuick/QQuickItem>
#if QT_CONFIG(quick_itemview)
#include <QtQuick/private/qquickitemview_p.h>
#endif
#include <QtQuickTest/QtQuickTest>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

QT_BEGIN_NAMESPACE

QQuickItem *QQuickVisualTestUtils::findVisibleChild(QQuickItem *parent, const QString &objectName)
{
    QQuickItem *item = nullptr;
    QList<QQuickItem*> items = parent->findChildren<QQuickItem*>(objectName);
    for (int i = 0; i < items.size(); ++i) {
        if (items.at(i)->isVisible() && !QQuickItemPrivate::get(items.at(i))->culled) {
            item = items.at(i);
            break;
        }
    }
    return item;
}

void QQuickVisualTestUtils::dumpTree(QQuickItem *parent, int depth)
{
    static QString padding = QStringLiteral("                       ");
    for (int i = 0; i < parent->childItems().size(); ++i) {
        QQuickItem *item = qobject_cast<QQuickItem*>(parent->childItems().at(i));
        if (!item)
            continue;
        qDebug() << padding.left(depth*2) << item;
        dumpTree(item, depth+1);
    }
}

void QQuickVisualTestUtils::moveMouseAway(QQuickWindow *window)
{
#if QT_CONFIG(cursor) // Get the cursor out of the way.
    // Using "bottomRight() + QPoint(100, 100)" was causing issues on Ubuntu,
    // where the window was positioned at the bottom right corner of the window
    // (even after centering the window on the screen), so we use another position.
    QCursor::setPos(window->frameGeometry().bottomLeft() + QPoint(-10, 10));
#endif

    // make sure hover events from QQuickDeliveryAgentPrivate::flushFrameSynchronousEvents()
    // do not interfere with the tests
    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(window, &leave);
}

void QQuickVisualTestUtils::centerOnScreen(QQuickWindow *window)
{
    const QRect screenGeometry = window->screen()->availableGeometry();
    const QPoint offset = QPoint(window->width() / 2, window->height() / 2);
    window->setFramePosition(screenGeometry.center() - offset);
}

QPoint QQuickVisualTestUtils::lerpPoints(const QPoint &point1, const QPoint &point2, qreal t)
{
    return QPoint(_q_interpolate(point1.x(), point2.x(), t), _q_interpolate(point1.y(), point2.y(), t));
};

/*!
    \internal

    Convenience class to linearly interpolate between two pointer move points.

    \code
    PointLerper pointLerper(window);
    // Lerps from {0, 0} to {15, 15}.
    pointLerper.move(15, 15);
    QVERIFY(parentButton->isHovered());

    // Lerps from {15, 15} to {25, 25}.
    pointLerper.move(25, 25);
    QVERIFY(childButton->isHovered());
    \endcode
*/
QQuickVisualTestUtils::PointLerper::PointLerper(QQuickWindow *window, const QPointingDevice *pointingDevice)
    : mWindow(window)
    , mPointingDevice(pointingDevice)
{
}

/*!
    \internal

    Moves from the last pos (or {0, 0} if there have been no calls
    to this function yet) to \a pos using linear interpolation
    over 10 (default value) steps with 1 ms (default value) delays
    between each step.
*/
void QQuickVisualTestUtils::PointLerper::move(const QPoint &pos, int steps, int delayInMilliseconds)
{
    forEachStep(steps, [&](qreal progress) {
        QQuickTest::pointerMove(mPointingDevice, mWindow, 0, lerpPoints(mFrom, pos, progress));
        QTest::qWait(delayInMilliseconds);
    });
    mFrom = pos;
};

void QQuickVisualTestUtils::PointLerper::move(int x, int y, int steps, int delayInMilliseconds)
{
    move(QPoint(x, y), steps, delayInMilliseconds);
};

bool QQuickVisualTestUtils::delegateVisible(QQuickItem *item)
{
    return item->isVisible() && !QQuickItemPrivate::get(item)->culled;
}

/*!
    \internal

    Compares \a ia with \a ib, returning \c true if the images are equal.
    If they are not equal, \c false is returned and \a errorMessage is set.

    A custom compare function to avoid issues such as:
    When running on native Nvidia graphics cards on linux, the
    distance field glyph pixels have a measurable, but not visible
    pixel error. This was GT-216 with the ubuntu "nvidia-319" driver package.
    llvmpipe does not show the same issue.
*/
bool QQuickVisualTestUtils::compareImages(const QImage &ia, const QImage &ib, QString *errorMessage)
{
    if (ia.size() != ib.size()) {
        QDebug(errorMessage) << "Images are of different size:" << ia.size() << ib.size()
            << "DPR:" << ia.devicePixelRatio() << ib.devicePixelRatio();
        return false;
    }
    if (ia.format() != ib.format()) {
        QDebug(errorMessage) << "Images are of different formats:" << ia.format() << ib.format();
        return false;
    }

    int w = ia.width();
    int h = ia.height();
    const int tolerance = 5;
    for (int y=0; y<h; ++y) {
        const uint *as= (const uint *) ia.constScanLine(y);
        const uint *bs= (const uint *) ib.constScanLine(y);
        for (int x=0; x<w; ++x) {
            uint a = as[x];
            uint b = bs[x];

            // No tolerance for error in the alpha.
            if ((a & 0xff000000) != (b & 0xff000000)
                || qAbs(qRed(a) - qRed(b)) > tolerance
                || qAbs(qRed(a) - qRed(b)) > tolerance
                || qAbs(qRed(a) - qRed(b)) > tolerance) {
                QDebug(errorMessage) << "Mismatch at:" << x << y << ':'
                    << Qt::hex << Qt::showbase << a << b;
                return false;
            }
        }
    }
    return true;
}

#if QT_CONFIG(quick_itemview)
/*!
    \internal

    Finds the delegate at \c index belonging to \c itemView, using the given \c flags.

    If the view needs to be polished, the function will wait for it to be done before continuing,
    and returns \c nullptr if the polish didn't happen.
*/
QQuickItem *QQuickVisualTestUtils::findViewDelegateItem(QQuickItemView *itemView, int index, FindViewDelegateItemFlags flags)
{
    if (QQuickTest::qIsPolishScheduled(itemView)) {
        if (!QQuickTest::qWaitForPolish(itemView)) {
            qWarning() << "failed to polish" << itemView;
            return nullptr;
        }
    }

    // Do this after the polish, just in case the count changes after a polish...
    if (index <= -1 || index >= itemView->count()) {
        qWarning() << "index" << index << "is out of bounds for" << itemView;
        return nullptr;
    }

    if (flags.testFlag(FindViewDelegateItemFlag::PositionViewAtIndex))
        itemView->positionViewAtIndex(index, QQuickItemView::Center);

    return itemView->itemAtIndex(index);
}
#endif

QQuickVisualTestUtils::QQuickApplicationHelper::QQuickApplicationHelper(QQmlDataTest *testCase,
    const QString &testFilePath, const QVariantMap &initialProperties, const QStringList &qmlImportPaths)
{
    for (const auto &path : qmlImportPaths)
        engine.addImportPath(path);

    QQmlComponent component(&engine);

    component.loadUrl(testCase->testFileUrl(testFilePath));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QObject *rootObject = component.createWithInitialProperties(initialProperties);
    cleanup.reset(rootObject);
    if (component.isError() || !rootObject) {
        errorMessage = QString::fromUtf8("Failed to create window: %1").arg(component.errorString()).toUtf8();
        return;
    }

    window = qobject_cast<QQuickWindow*>(rootObject);
    if (!window) {
        errorMessage = QString::fromUtf8("Root object %1 must be a QQuickWindow subclass").arg(QDebug::toString(window)).toUtf8();
        return;
    }

    if (window->isVisible()) {
        errorMessage = QString::fromUtf8("Expected window not to be visible, but it is").toUtf8();
        return;
    }

    ready = true;
}

QQuickVisualTestUtils::MnemonicKeySimulator::MnemonicKeySimulator(QWindow *window)
    : m_window(window), m_modifiers(Qt::NoModifier)
{
}

void QQuickVisualTestUtils::MnemonicKeySimulator::press(Qt::Key key)
{
    // QTest::keyPress() but not generating the press event for the modifier key.
    if (key == Qt::Key_Alt)
        m_modifiers |= Qt::AltModifier;
    QTest::simulateEvent(m_window, true, key, m_modifiers, QString(), false);
}

void QQuickVisualTestUtils::MnemonicKeySimulator::release(Qt::Key key)
{
    // QTest::keyRelease() but not generating the release event for the modifier key.
    if (key == Qt::Key_Alt)
        m_modifiers &= ~Qt::AltModifier;
    QTest::simulateEvent(m_window, false, key, m_modifiers, QString(), false);
}

void QQuickVisualTestUtils::MnemonicKeySimulator::click(Qt::Key key)
{
    press(key);
    release(key);
}

QPoint QQuickVisualTestUtils::mapCenterToWindow(const QQuickItem *item)
{
    return item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
}

QPoint QQuickVisualTestUtils::mapToWindow(const QQuickItem *item, qreal relativeX, qreal relativeY)
{
    return item->mapToScene(QPointF(relativeX, relativeY)).toPoint();
}

QPoint QQuickVisualTestUtils::mapToWindow(const QQuickItem *item, const QPointF &relativePos)
{
    return mapToWindow(item, relativePos.x(), relativePos.y());
}

QT_END_NAMESPACE

#include "moc_visualtestutils_p.cpp"
