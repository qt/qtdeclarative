// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlclassicpreviewhandler.h"
#include "qqmlpreviewservice.h"

#include <QtCore/qtimer.h>
#include <QtGui/qwindow.h>
#include <QtGui/qguiapplication.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/qquickitem.h>
#include <QtQml/qqmlcomponent.h>

#include <private/qqmlmetatype_p.h>
#include <private/qquickpixmap_p.h>
#include <private/qquickview_p.h>
#include <private/qv4compileddata_p.h>

QT_BEGIN_NAMESPACE

struct QuitLockDisabler
{
    const bool quitLockEnabled;

    Q_NODISCARD_CTOR QuitLockDisabler()
        : quitLockEnabled(QCoreApplication::isQuitLockEnabled())
    {
        QCoreApplication::setQuitLockEnabled(false);
    }

    ~QuitLockDisabler()
    {
        QCoreApplication::setQuitLockEnabled(quitLockEnabled);
    }
};

static void closeAllWindows()
{
    const QWindowList windows = QGuiApplication::allWindows();
    for (QWindow *window : windows)
        window->close();
}

static Qt::WindowFlags fixFlags(Qt::WindowFlags flags)
{
    // If only the type flag is given, some other window flags are automatically assumed. When we
    // add a flag, we need to make those explicit.
    switch (flags) {
    case Qt::Window:
        return flags | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint
                | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint;
    case Qt::Dialog:
    case Qt::Tool:
        return flags | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;
    default:
        return flags;
    }
}

QQmlClassicPreviewHandler::QQmlClassicPreviewHandler(QObject *parent)
    : QQmlPreviewHandler(parent)
{
    m_dummyItem.reset(new QQuickItem);
    const QString platformName = QGuiApplication::platformName();
    m_supportsMultipleWindows = (platformName == QStringLiteral("windows")
                                 || platformName == QStringLiteral("cocoa")
                                 || platformName == QStringLiteral("xcb")
                                 || platformName == QStringLiteral("wayland"));

    QCoreApplication::instance()->installEventFilter(this);
}

QQmlClassicPreviewHandler::~QQmlClassicPreviewHandler()
{
    clear();
}

bool QQmlClassicPreviewHandler::eventFilter(QObject *obj, QEvent *event)
{
    QQuickWindow *window = currentWindow();
    if (window && (event->type() == QEvent::Move) &&
        qobject_cast<QQuickWindow*>(obj) == window) {
        m_lastPosition.takePosition(window);
    }

    return QObject::eventFilter(obj, event);
}

void QQmlClassicPreviewHandler::removeEngine(QQmlEngine *qmlEngine)
{
    QQmlPreviewHandler::removeEngine(qmlEngine);
    for (QObject *obj : m_createdObjects)
        if (obj && ::qmlEngine(obj) == qmlEngine)
            delete obj;
    m_createdObjects.removeAll(nullptr);
}

void QQmlClassicPreviewHandler::connectToService(QQmlPreviewServiceImpl *service)
{
    QQmlPreviewHandler::connectToService(service);
    connect(service, &QQmlPreviewServiceImpl::drop, this, &QQmlClassicPreviewHandler::dropCU);
    connect(service, &QQmlPreviewServiceImpl::rerun, this, &QQmlClassicPreviewHandler::rerun);
    connect(service, &QQmlPreviewServiceImpl::zoom, this, &QQmlClassicPreviewHandler::zoom);
}

void QQmlClassicPreviewHandler::load(const QUrl &url)
{
    QSharedPointer<QuitLockDisabler> disabler(new QuitLockDisabler);

    clear();
    m_component.reset(nullptr);
    QQuickPixmap::purgeCache();

    const QList<QQmlEngine *> seenEngines = engines();
    const int numEngines = seenEngines.size();
    if (numEngines > 1) {
        emit error(QString::fromLatin1("%1 QML engines available. We cannot decide which one "
                                       "should load the component.").arg(numEngines));
        return;
    } else if (numEngines == 0) {
        emit error(QLatin1String("No QML engines found."));
        return;
    }
    m_lastPosition.loadWindowPositionSettings(url);

    QQmlEngine *engine = seenEngines.front();
    engine->clearSingletons();
    engine->clearComponentCache();
    m_component.reset(new QQmlComponent(engine, url, this));

    auto onStatusChanged = [disabler, this](QQmlComponent::Status status) {
        switch (status) {
        case QQmlComponent::Null:
        case QQmlComponent::Loading:
            return true; // try again later
        case QQmlComponent::Ready:
            tryCreateObject();
            break;
        case QQmlComponent::Error:
            emit error(m_component->errorString());
            break;
        default:
            Q_UNREACHABLE();
            break;
        }

        disconnect(m_component.data(), &QQmlComponent::statusChanged, this, nullptr);
        return false; // we're done
    };

    if (onStatusChanged(m_component->status()))
        connect(m_component.data(), &QQmlComponent::statusChanged, this, onStatusChanged);
}

void QQmlClassicPreviewHandler::dropCU(const QUrl &url)
{
    // Drop any existing compilation units for this URL from the type registry.
    // There can be multiple, one for each engine.
    while (const auto cu = QQmlMetaType::obtainCompilationUnit(url))
        QQmlMetaType::unregisterInternalCompositeType(cu);
}

void QQmlClassicPreviewHandler::rerun()
{
    if (m_component.isNull() || !m_component->isReady()) {
        emit error(QLatin1String("Component is not ready."));
        return;
    }

    QuitLockDisabler disabler;
    Q_UNUSED(disabler);
    clear();
    tryCreateObject();
}

void QQmlClassicPreviewHandler::clear()
{
    qDeleteAll(m_createdObjects);
    m_createdObjects.clear();
    setCurrentWindow(nullptr);
}

void QQmlClassicPreviewHandler::tryCreateObject()
{
    if (!m_supportsMultipleWindows)
        closeAllWindows();
    QObject *object = m_component->create();
    m_createdObjects.append(object);
    showObject(object);
}

void QQmlClassicPreviewHandler::showObject(QObject *object)
{
    if (QWindow *window = qobject_cast<QWindow *>(object)) {
        setCurrentWindow(qobject_cast<QQuickWindow *>(window));
        for (QWindow *otherWindow : QGuiApplication::allWindows()) {
            if (QQuickWindow *quickWindow = qobject_cast<QQuickWindow *>(otherWindow)) {
                if (quickWindow == currentWindow())
                    continue;
                quickWindow->setVisible(false);
                quickWindow->setFlags(quickWindow->flags() & ~Qt::WindowStaysOnTopHint);
            }
        }
    } else if (QQuickItem *item = qobject_cast<QQuickItem *>(object)) {
        setCurrentWindow(nullptr);
        for (QWindow *window : QGuiApplication::allWindows()) {
            if (QQuickWindow *quickWindow = qobject_cast<QQuickWindow *>(window)) {
                if (currentWindow() != nullptr) {
                    emit error(QLatin1String("Multiple QQuickWindows available. We cannot "
                                             "decide which one to use."));
                    return;
                }
                setCurrentWindow(quickWindow);
            } else {
                window->setVisible(false);
                window->setFlag(Qt::WindowStaysOnTopHint, false);
            }
        }

        if (currentWindow() == nullptr) {
            setCurrentWindow(new QQuickWindow);
            m_createdObjects.append(currentWindow());
        }

        for (QQuickItem *oldItem : currentWindow()->contentItem()->childItems())
            oldItem->setParentItem(m_dummyItem.data());

        // Special case for QQuickView, as that keeps a "root" pointer around, and uses it to
        // automatically resize the window or the item.
        if (QQuickView *view = qobject_cast<QQuickView *>(currentWindow()))
            QQuickViewPrivate::get(view)->setRootObject(item);
        else
            item->setParentItem(currentWindow()->contentItem());

        currentWindow()->resize(item->size().toSize());
        // used by debug translation service to get the states
        setCurrentRootItem(item);
    } else {
        emit error(QLatin1String("Created object is neither a QWindow nor a QQuickItem."));
    }

    if (QQuickWindow *window = currentWindow()) {
        m_lastPosition.initLastSavedWindowPosition(window);
        window->setFlags(fixFlags(window->flags()) | Qt::WindowStaysOnTopHint);
        window->setVisible(true);
    }
}

void QQmlClassicPreviewHandler::zoom(qreal newFactor)
{
    m_zoomFactor = newFactor;
    QTimer::singleShot(0, this, [this, newFactor]() {
        zoomWindow(currentWindow(), newFactor, &m_lastPosition);
    });
}

QT_END_NAMESPACE

#include "moc_qqmlclassicpreviewhandler.cpp"
