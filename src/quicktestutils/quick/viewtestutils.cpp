// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "viewtestutils_p.h"

#include <QtCore/QRandomGenerator>
#include <QtCore/QTimer>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickView>
#include <QtGui/QScreen>
#include <QtGui/qpa/qwindowsysteminterface.h>

#include <QtTest/QTest>

#include <QtQuick/private/qquickdeliveryagent_p_p.h>
#if QT_CONFIG(quick_itemview)
#include <QtQuick/private/qquickitemview_p_p.h>
#endif
#include <QtQuick/private/qquickwindow_p.h>

#include <QtQuickTestUtils/private/visualtestutils_p.h>

QT_BEGIN_NAMESPACE

QQuickView *QQuickViewTestUtils::createView()
{
    QQuickView *window = new QQuickView(0);
    const QSize size(240, 320);
    window->resize(size);
    QQuickViewTestUtils::centerOnScreen(window, size);
    return window;
}

void QQuickViewTestUtils::centerOnScreen(QQuickView *window, const QSize &size)
{
    const QRect screenGeometry = window->screen()->availableGeometry();
    const QPoint offset = QPoint(size.width() / 2, size.height() / 2);
    window->setFramePosition(screenGeometry.center() - offset);
}

void QQuickViewTestUtils::centerOnScreen(QQuickView *window)
{
    QQuickViewTestUtils::centerOnScreen(window, window->size());
}

void QQuickViewTestUtils::moveMouseAway(QQuickView *window)
{
#if QT_CONFIG(cursor) // Get the cursor out of the way.
    QCursor::setPos(window->geometry().topRight() + QPoint(100, 100));
#else
    Q_UNUSED(window);
#endif
}

void QQuickViewTestUtils::moveAndRelease(QQuickView *window, const QPoint &position)
{
    QTest::mouseMove(window, position);
    QTest::mouseRelease(window, Qt::LeftButton, {}, position);
}

void QQuickViewTestUtils::moveAndPress(QQuickView *window, const QPoint &position)
{
    QTest::mouseMove(window, position);
    QTest::mousePress(window, Qt::LeftButton, {}, position);
}

void QQuickViewTestUtils::flick(QQuickView *window, const QPoint &from, const QPoint &to, int duration)
{
    const int pointCount = 5;
    QPoint diff = to - from;

    // send press, five equally spaced moves, and release.
    moveAndPress(window, from);

    for (int i = 0; i < pointCount; ++i)
        QTest::mouseMove(window, from + (i+1)*diff/pointCount, duration / pointCount);

    moveAndRelease(window, to);
    QTest::qWait(50);
}

QList<int> QQuickViewTestUtils::adjustIndexesForAddDisplaced(const QList<int> &indexes, int index, int count)
{
    QList<int> result;
    for (int i=0; i<indexes.size(); i++) {
        int num = indexes[i];
        if (num >= index) {
            num += count;
        }
        result << num;
    }
    return result;
}

QList<int> QQuickViewTestUtils::adjustIndexesForMove(const QList<int> &indexes, int from, int to, int count)
{
    QList<int> result;
    for (int i=0; i<indexes.size(); i++) {
        int num = indexes[i];
        if (from < to) {
            if (num >= from && num < from + count)
                num += (to - from); // target
            else if (num >= from && num < to + count)
                num -= count;   // displaced
        } else if (from > to) {
            if (num >= from && num < from + count)
                num -= (from - to);  // target
            else if (num >= to && num < from + count)
                num += count;   // displaced
        }
        result << num;
    }
    return result;
}

QList<int> QQuickViewTestUtils::adjustIndexesForRemoveDisplaced(const QList<int> &indexes, int index, int count)
{
    QList<int> result;
    for (int i=0; i<indexes.size(); i++) {
        int num = indexes[i];
        if (num >= index)
            num -= count;
        result << num;
    }
    return result;
}

QQuickViewTestUtils::QaimModel::QaimModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int QQuickViewTestUtils::QaimModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return list.size();
}

int QQuickViewTestUtils::QaimModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns;
}

QHash<int,QByteArray> QQuickViewTestUtils::QaimModel::roleNames() const
{
    QHash<int,QByteArray> roles = QAbstractListModel::roleNames();
    roles.insert(Name, "name");
    roles.insert(Number, "number");
    return roles;
}

QVariant QQuickViewTestUtils::QaimModel::data(const QModelIndex &index, int role) const
{
    QVariant rv;
    if (role == Name)
        rv = list.at(index.row()).first;
    else if (role == Number)
        rv = list.at(index.row()).second;

    return rv;
}

int QQuickViewTestUtils::QaimModel::count() const
{
    return rowCount() * columnCount();
}

QString QQuickViewTestUtils::QaimModel::name(int index) const
{
    return list.at(index).first;
}

QString QQuickViewTestUtils::QaimModel::number(int index) const
{
    return list.at(index).second;
}

void QQuickViewTestUtils::QaimModel::addItem(const QString &name, const QString &number)
{
    emit beginInsertRows(QModelIndex(), list.size(), list.size());
    list.append(QPair<QString,QString>(name, number));
    emit endInsertRows();
}

void QQuickViewTestUtils::QaimModel::addItems(const QList<QPair<QString, QString> > &items)
{
    emit beginInsertRows(QModelIndex(), list.size(), list.size()+items.size()-1);
    for (int i=0; i<items.size(); i++)
        list.append(QPair<QString,QString>(items[i].first, items[i].second));
    emit endInsertRows();
}

void QQuickViewTestUtils::QaimModel::insertItem(int index, const QString &name, const QString &number)
{
    emit beginInsertRows(QModelIndex(), index, index);
    list.insert(index, QPair<QString,QString>(name, number));
    emit endInsertRows();
}

void QQuickViewTestUtils::QaimModel::insertItems(int index, const QList<QPair<QString, QString> > &items)
{
    emit beginInsertRows(QModelIndex(), index, index+items.size()-1);
    for (int i=0; i<items.size(); i++)
        list.insert(index + i, QPair<QString,QString>(items[i].first, items[i].second));
    emit endInsertRows();
}

void QQuickViewTestUtils::QaimModel::removeItem(int index)
{
    emit beginRemoveRows(QModelIndex(), index, index);
    list.removeAt(index);
    emit endRemoveRows();
}

void QQuickViewTestUtils::QaimModel::removeItems(int index, int count)
{
    emit beginRemoveRows(QModelIndex(), index, index+count-1);
    while (count--)
        list.removeAt(index);
    emit endRemoveRows();
}

void QQuickViewTestUtils::QaimModel::moveItem(int from, int to)
{
    emit beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
    list.move(from, to);
    emit endMoveRows();
}

void QQuickViewTestUtils::QaimModel::moveItems(int from, int to, int count)
{
    emit beginMoveRows(QModelIndex(), from, from+count-1, QModelIndex(), to > from ? to+count : to);
    qquickmodelviewstestutil_move(from, to, count, &list);
    emit endMoveRows();
}

void QQuickViewTestUtils::QaimModel::modifyItem(int idx, const QString &name, const QString &number)
{
    list[idx] = QPair<QString,QString>(name, number);
    emit dataChanged(index(idx,0), index(idx,0));
}

void QQuickViewTestUtils::QaimModel::clear()
{
    int count = list.size();
    if (count > 0) {
        beginRemoveRows(QModelIndex(), 0, count-1);
        list.clear();
        endRemoveRows();
    }
}

void QQuickViewTestUtils::QaimModel::reset()
{
    emit beginResetModel();
    emit endResetModel();
}

void QQuickViewTestUtils::QaimModel::resetItems(const QList<QPair<QString, QString> > &items)
{
    beginResetModel();
    list = items;
    endResetModel();
}

class ScopedPrintable
{
    Q_DISABLE_COPY_MOVE(ScopedPrintable)

public:
    ScopedPrintable(const QString &string) : data(QTest::toString(string)) {}
    ~ScopedPrintable() { delete[] data; }

    operator const char*() const { return data; }

private:
    const char *data;
};

void QQuickViewTestUtils::QaimModel::matchAgainst(const QList<QPair<QString, QString> > &other, const QString &error1, const QString &error2) {
    for (int i=0; i<other.size(); i++) {
        QVERIFY2(list.contains(other[i]),
                 ScopedPrintable(other[i].first + QLatin1Char(' ') + other[i].second + QLatin1Char(' ') + error1));
    }
    for (int i=0; i<list.size(); i++) {
        QVERIFY2(other.contains(list[i]),
                 ScopedPrintable(list[i].first + QLatin1Char(' ') + list[i].second + QLatin1Char(' ') + error2));
    }
}



QQuickViewTestUtils::ListRange::ListRange()
    : valid(false)
{
}

QQuickViewTestUtils::ListRange::ListRange(const ListRange &other)
    : valid(other.valid)
{
    indexes = other.indexes;
}

QQuickViewTestUtils::ListRange::ListRange(int start, int end)
    : valid(true)
{
    for (int i=start; i<=end; i++)
        indexes << i;
}

QQuickViewTestUtils::ListRange::~ListRange()
{
}

QQuickViewTestUtils::ListRange QQuickViewTestUtils::ListRange::operator+(const ListRange &other) const
{
    if (other == *this)
        return *this;
    ListRange a(*this);
    a.indexes.append(other.indexes);
    return a;
}

bool QQuickViewTestUtils::ListRange::operator==(const ListRange &other) const
{
    return QSet<int>(indexes.cbegin(), indexes.cend())
        == QSet<int>(other.indexes.cbegin(), other.indexes.cend());
}

bool QQuickViewTestUtils::ListRange::operator!=(const ListRange &other) const
{
    return !(*this == other);
}

bool QQuickViewTestUtils::ListRange::isValid() const
{
    return valid;
}

int QQuickViewTestUtils::ListRange::count() const
{
    return indexes.size();
}

QList<QPair<QString,QString> > QQuickViewTestUtils::ListRange::getModelDataValues(const QaimModel &model)
{
    QList<QPair<QString,QString> > data;
    if (!valid)
        return data;
    for (int i=0; i<indexes.size(); i++)
        data.append(qMakePair(model.name(indexes[i]), model.number(indexes[i])));
    return data;
}

QQuickViewTestUtils::StressTestModel::StressTestModel()
    : QAbstractListModel()
    , m_rowCount(20)
{
    QTimer *t = new QTimer(this);
    t->setInterval(500);
    t->start();

    connect(t, &QTimer::timeout, this, &StressTestModel::updateModel);
}

int QQuickViewTestUtils::StressTestModel::rowCount(const QModelIndex &) const
{
    return m_rowCount;
}

QVariant QQuickViewTestUtils::StressTestModel::data(const QModelIndex &, int) const
{
    return QVariant();
}

void QQuickViewTestUtils::StressTestModel::updateModel()
{
    if (m_rowCount > 10) {
        for (int i = 0; i < 10; ++i) {
            int rnum = QRandomGenerator::global()->bounded(m_rowCount);
            beginRemoveRows(QModelIndex(), rnum, rnum);
            m_rowCount--;
            endRemoveRows();
        }
    }
    if (m_rowCount < 20) {
        for (int i = 0; i < 10; ++i) {
            int rnum = QRandomGenerator::global()->bounded(m_rowCount);
            beginInsertRows(QModelIndex(), rnum, rnum);
            m_rowCount++;
            endInsertRows();
        }
    }
}

#if QT_CONFIG(quick_itemview)
bool QQuickViewTestUtils::testVisibleItems(const QQuickItemViewPrivate *priv, bool *nonUnique, FxViewItem **failItem, int *expectedIdx)
{
    QHash<QQuickItem*, int> uniqueItems;

    int skip = 0;
    for (int i = 0; i < priv->visibleItems.size(); ++i) {
        FxViewItem *item = priv->visibleItems.at(i);
        if (!item) {
            *failItem = nullptr;
            return false;
        }
#if 0
        qDebug() << "\t" << item->index
                 << item->item
                 << item->position()
                 << (!item->item || QQuickItemPrivate::get(item->item)->culled ? "hidden" : "visible");
#endif
        if (item->index == -1) {
            ++skip;
        } else if (item->index != priv->visibleIndex + i - skip) {
            *nonUnique = false;
            *failItem = item;
            *expectedIdx = priv->visibleIndex + i - skip;
            return false;
        } else if (uniqueItems.contains(item->item)) {
            *nonUnique = true;
            *failItem = item;
            *expectedIdx = uniqueItems.find(item->item).value();
            return false;
        }

        uniqueItems.insert(item->item, item->index);
    }

    return true;
}
#endif

namespace QQuickTouchUtils {

    /* QQuickWindow does event compression and only delivers events just
     * before it is about to render the next frame. Since some tests
     * rely on events being delivered immediately AND that no other
     * event processing has occurred in the meanwhile, we flush the
     * event manually and immediately.
     */
    void flush(QQuickWindow *window) {
        if (!window)
            return;
        QQuickDeliveryAgentPrivate *da = QQuickWindowPrivate::get(window)->deliveryAgentPrivate();
        if (!da || !da->delayedTouch)
            return;
        da->deliverDelayedTouchEvent();
    }

}

namespace QTest {
    int Q_TESTLIB_EXPORT defaultMouseDelay();
}

namespace QQuickTest {

    /*! \internal
        Initialize \a view, set \a url, center in available geometry, move mouse away if desired.
        If \a errorMessage is given, QQuickView::errors() will be concatenated into it;
        otherwise, the QWARN messages are generally enough to debug the test.

        Returns \c false if the view fails to load the QML.  That should be fatal in most tests,
        so normally the return value should be checked with QVERIFY.
    */
    bool initView(QQuickView &view, const QUrl &url, bool moveMouseOut, QByteArray *errorMessage)
    {
        view.setSource(url);
        while (view.status() == QQuickView::Loading)
            QTest::qWait(10);
        if (view.status() != QQuickView::Ready) {
            if (errorMessage) {
                for (const QQmlError &e : view.errors())
                    errorMessage->append(e.toString().toLocal8Bit() + '\n');
            }
            return false;
        }
        const QRect screenGeometry = view.screen()->availableGeometry();
        const QSize size = view.size();
        if (view.width() == 0)
            view.setWidth(100);
        if (view.height() == 0)
            view.setHeight(100);
        const QPoint offset = QPoint(size.width() / 2, size.height() / 2);
        view.setFramePosition(screenGeometry.center() - offset);
    #if QT_CONFIG(cursor) // Get the cursor out of the way.
        if (moveMouseOut)
             QCursor::setPos(view.geometry().topRight() + QPoint(100, 100));
    #else
        Q_UNUSED(moveMouseOut);
    #endif
        return true;
    }

    /*! \internal
        Initialize \a view, set \a url, center in available geometry, move mouse away,
        show the \a view, wait for it to be exposed, and verify that its rootObject is not null.

        Returns \c false if anything fails, which should be fatal in most tests.
        The usual way to call this function is
        \code
        QQuickView window;
        QVERIFY(QQuickTest::showView(window, testFileUrl("myitems.qml")));
        \endcode
    */
    bool showView(QQuickView &view, const QUrl &url)
    {
        if (!initView(view, url))
            return false;
        view.show();
        if (!QTest::qWaitForWindowExposed(&view))
            return false;
        if (!view.rootObject())
            return false;
        return true;
    }

    // TODO maybe move the generic pointerPress/Move/Release functions to QTestLib later on

    static Qt::MouseButton pressedTabletButton = Qt::NoButton;
    static Qt::KeyboardModifiers pressedTabletModifiers = Qt::NoModifier;

    void pointerPress(const QPointingDevice *dev, QQuickWindow *window, int pointId, const QPoint &p,
                      Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
    {
        switch (dev->type()) {
        case QPointingDevice::DeviceType::Mouse:
        case QPointingDevice::DeviceType::TouchPad:
            QTest::mousePress(window, button, modifiers, p);
            break;
        case QPointingDevice::DeviceType::TouchScreen:
            QTest::touchEvent(window, const_cast<QPointingDevice *>(dev)).press(pointId, p, window);
            QQuickTouchUtils::flush(window);
            break;
        case QPointingDevice::DeviceType::Puck:
        case QPointingDevice::DeviceType::Stylus:
        case QPointingDevice::DeviceType::Airbrush:
            QTest::lastMouseTimestamp += QTest::defaultMouseDelay();
            pressedTabletButton = button;
            pressedTabletModifiers = modifiers;
            QWindowSystemInterface::handleTabletEvent(window, QTest::lastMouseTimestamp, dev, p, window->mapToGlobal(p),
                                                      button, 0.8, 0, 0, 0, 0, 0, modifiers);
            break;
        default:
            qWarning() << "can't send a press event from" << dev;
            break;
        }
    }

    void pointerMove(const QPointingDevice *dev, QQuickWindow *window, int pointId, const QPoint &p)
    {
        switch (dev->type()) {
        case QPointingDevice::DeviceType::Mouse:
        case QPointingDevice::DeviceType::TouchPad:
            QTest::mouseMove(window, p);
            break;
        case QPointingDevice::DeviceType::TouchScreen:
            QTest::touchEvent(window, const_cast<QPointingDevice *>(dev)).move(pointId, p, window);
            QQuickTouchUtils::flush(window);
            break;
        case QPointingDevice::DeviceType::Puck:
        case QPointingDevice::DeviceType::Stylus:
        case QPointingDevice::DeviceType::Airbrush:
            QTest::lastMouseTimestamp += QTest::defaultMouseDelay();
            QWindowSystemInterface::handleTabletEvent(window, QTest::lastMouseTimestamp, dev, p, window->mapToGlobal(p),
                                                      pressedTabletButton, 0, 0, 0, 0, 0, 0, pressedTabletModifiers);
            break;
        default:
            qWarning() << "can't send a move event from" << dev;
            break;
        }
    }

    void pointerRelease(const QPointingDevice *dev, QQuickWindow *window, int pointId, const QPoint &p,
                        Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
    {
        switch (dev->type()) {
        case QPointingDevice::DeviceType::Mouse:
        case QPointingDevice::DeviceType::TouchPad:
            QTest::mouseRelease(window, button, modifiers, p);
            break;
        case QPointingDevice::DeviceType::TouchScreen:
            QTest::touchEvent(window, const_cast<QPointingDevice *>(dev)).release(pointId, p, window);
            QQuickTouchUtils::flush(window);
            break;
        case QPointingDevice::DeviceType::Puck:
        case QPointingDevice::DeviceType::Stylus:
        case QPointingDevice::DeviceType::Airbrush:
            QTest::lastMouseTimestamp += QTest::defaultMouseDelay();
            QWindowSystemInterface::handleTabletEvent(window, QTest::lastMouseTimestamp, dev, p, window->mapToGlobal(p),
                                                      Qt::NoButton, 0, 0, 0, 0, 0, 0, modifiers);
            break;
        default:
            qWarning() << "can't send a press event from" << dev;
            break;
        }
    }

}

QT_END_NAMESPACE

#include "moc_viewtestutils_p.cpp"
