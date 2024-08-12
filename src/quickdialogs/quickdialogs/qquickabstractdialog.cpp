// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickabstractdialog_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickDialogs2QuickImpl/private/qquickdialogimplfactory_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcDialogs, "qt.quick.dialogs")

/*!
    \internal

    A dialog that can be backed by different implementations.

    Each dialog has a QPlatformDialogHelper handle, which is created in create():

    - First we attempt to create a native dialog (e.g. QWindowsFileDialogHelper) through
      QGuiApplicationPrivate::platformTheme()->createPlatformDialogHelper().
    - If that fails, we try to create the Qt Quick fallback dialog (e.g. QQuickPlatformFileDialog)
      through QQuickDialogImplFactory::createPlatformDialogHelper().

    The handle acts as an intermediary between the QML-facing dialog object
    and the native/widget/quick implementation:

            +---------------------------+
            | FileDialog created in QML |
            +---------------------------+
                         |
                         |
                         v                         +----------------------+
                +------------------+               | attempt to create    |     +------+
                |useNativeDialog()?|-----false---->| QQuickPlatformDialog |---->| done |
                +------------------+               | instance and set     |     +------+
                         |                         | m_handle to it       |
                         |                         +----------------------+
                         v                                  ^
                        true                                |
                         |                                  |
                         v                                  |
               +---------------------+                      |
               | attempt to create   |                      |
               | QWindowsFileDialog- |                      |
               | Helper instance and |                      |
               | set m_handle to it  |                      |
               +---------------------+                      |
                         |                                  |
                         v                                  |
                 +-----------------+                        |
                 | m_handle valid? |--------------------->false
                 +-----------------+                        ^
                         |                                  |
                         v                                  |
                        true                                |
                         |                                  |
                 +-------------------+                      |
                 | m_handle->show()? |------------------->false
                 +-------------------+
                         |
                         v
                        true
                         |
                      +------+
                      | done |
                      +------+

    If QWindowsFileDialogHelper is created, it creates a native dialog.
    If QQuickPlatformDialog is created, it creates a non-native QQuickFileDialogImpl.
*/

/*!
    \qmltype Dialog
    \inherits QtObject
//! \instantiates QQuickAbstractDialog
    \inqmlmodule QtQuick.Dialogs
    \since 6.2
    \brief The base class of native dialogs.

    The Dialog type provides common QML API for native platform dialogs.
    For the non-native dialog, see \l [QML QtQuickControls]{Dialog}.

    To show a native dialog, construct an instance of one of the concrete
    Dialog implementations, set the desired properties, and call \l open().
    Dialog emits \l accepted() or \l rejected() when the user is done with
    the dialog.
*/

/*!
    \qmlsignal void QtQuick.Dialogs::Dialog::accepted()

    This signal is emitted when the dialog has been accepted either
    interactively or by calling \l accept().

    \sa rejected()
*/

/*!
    \qmlsignal void QtQuick.Dialogs::Dialog::rejected()

    This signal is emitted when the dialog has been rejected either
    interactively or by calling \l reject().

    This signal is also emitted when closing the dialog with \l close().

    \sa accepted()
*/

Q_DECLARE_LOGGING_CATEGORY(lcDialogs)

QQuickAbstractDialog::QQuickAbstractDialog(QQuickDialogType type, QObject *parent)
    : QObject(parent),
      m_type(type)
{
}

QQuickAbstractDialog::~QQuickAbstractDialog()
{
    destroy();
}

QPlatformDialogHelper *QQuickAbstractDialog::handle() const
{
    return m_handle.get();
}

/*!
    \qmldefault
    \qmlproperty list<QtObject> QtQuick.Dialogs::Dialog::data

    This default property holds the list of all objects declared as children of
    the dialog.
*/
QQmlListProperty<QObject> QQuickAbstractDialog::data()
{
    return QQmlListProperty<QObject>(this, &m_data);
}

/*!
    \qmlproperty Window QtQuick.Dialogs::Dialog::parentWindow

    This property holds the parent window of the dialog.

    Unless explicitly set, the window is automatically resolved by iterating
    the QML parent objects until a \l Window or an \l Item that has a window
    is found.
*/
QWindow *QQuickAbstractDialog::parentWindow() const
{
    return m_parentWindow;
}

void QQuickAbstractDialog::setParentWindow(QWindow *window)
{
    qCDebug(lcDialogs) << "set parent window to" << window;
    m_parentWindowExplicitlySet = bool(window);

    if (m_parentWindow == window)
        return;

    m_parentWindow = window;
    emit parentWindowChanged();
}

void QQuickAbstractDialog::resetParentWindow()
{
    m_parentWindowExplicitlySet = false;

    if (!m_parentWindow)
        return;

    m_parentWindow = nullptr;
    emit parentWindowChanged();
}

/*!
    \qmlproperty string QtQuick.Dialogs::Dialog::title

    This property holds the title of the dialog.
*/
QString QQuickAbstractDialog::title() const
{
    return m_title;
}

void QQuickAbstractDialog::setTitle(const QString &title)
{
    if (m_title == title)
        return;

    m_title = title;
    emit titleChanged();
}

/*!
    \qmlproperty Qt::WindowFlags QtQuick.Dialogs::Dialog::flags

    This property holds the window flags of the dialog. The default value is \c Qt.Dialog.
*/
Qt::WindowFlags QQuickAbstractDialog::flags() const
{
    return m_flags;
}

void QQuickAbstractDialog::setFlags(Qt::WindowFlags flags)
{
    if (m_flags == flags)
        return;

    m_flags = flags;
    emit flagsChanged();
}

/*!
    \qmlproperty Qt::WindowModality QtQuick.Dialogs::Dialog::modality

    This property holds the modality of the dialog. The default value is \c Qt.WindowModal.

    Available values:
    \value Qt.NonModal The dialog is not modal and does not block input to other windows.
    \value Qt.WindowModal The dialog is modal to a single window hierarchy and blocks input to its parent window, all grandparent windows, and all siblings of its parent and grandparent windows.
    \value Qt.ApplicationModal The dialog is modal to the application and blocks input to all windows.
*/
Qt::WindowModality QQuickAbstractDialog::modality() const
{
    return m_modality;
}

void QQuickAbstractDialog::setModality(Qt::WindowModality modality)
{
    if (m_modality == modality)
        return;

    m_modality = modality;
    emit modalityChanged();
}

/*!
    \qmlproperty bool QtQuick.Dialogs::Dialog::visible

    This property holds the visibility of the dialog. The default value is \c false.

    \sa open(), close()
*/
bool QQuickAbstractDialog::isVisible() const
{
    return m_handle && m_visible;
}

void QQuickAbstractDialog::setVisible(bool visible)
{
    qCDebug(lcDialogs) << "setVisible called with" << visible;

    if (visible) {
        // Don't try to open before component completion, as we won't have a window yet,
        // and open() sets m_visible to false if it fails.
        if (!m_complete)
            m_visibleRequested = true;
        else
            open();
    } else {
        close();
    }
}

/*!
    \qmlproperty int QtQuick.Dialogs::Dialog::result

    This property holds the result code.

    Standard result codes:
    \value Dialog.Accepted
    \value Dialog.Rejected

    \note MessageDialog sets the result to the value of the clicked standard
          button instead of using the standard result codes.
*/
int QQuickAbstractDialog::result() const
{
    return m_result;
}

void QQuickAbstractDialog::setResult(int result)
{
    if (m_result == result)
        return;

    m_result = result;
    emit resultChanged();
}

/*!
    \qmlmethod void QtQuick.Dialogs::Dialog::open()

    Opens the dialog.

    \sa visible, close()
*/
void QQuickAbstractDialog::open()
{
    qCDebug(lcDialogs) << "open called";
    if (m_visible || !create())
        return;

    onShow(m_handle.get());

    m_visible = m_handle->show(m_flags, m_modality, windowForOpen());
    if (!m_visible && useNativeDialog()) {
        // Fall back to non-native dialog
        destroy();
        if (!create(CreateOptions::DontTryNativeDialog))
            return;

        onShow(m_handle.get());
        m_visible = m_handle->show(m_flags, m_modality, windowForOpen());

        if (m_visible) {
            // The conditions that caused the non-native fallback might have
            // changed the next time open() is called, so we should try again
            // with a native dialog when that happens.
            QObject::connect(this, &QQuickAbstractDialog::visibleChanged,
                m_handle.get(), [this]{ if (!isVisible()) destroy(); });
        }
    }
    if (m_visible) {
        m_result = Rejected; // in case an accepted dialog gets re-opened, then closed
        emit visibleChanged();
    }
}

/*!
    \qmlmethod void QtQuick.Dialogs::Dialog::close()

    Closes the dialog and emits either the \l accepted() or \l rejected()
    signal.

    \sa visible, open()
*/
void QQuickAbstractDialog::close()
{
    if (!m_handle || !m_visible)
        return;

    onHide(m_handle.get());
    m_handle->hide();
    m_visible = false;
    if (!m_parentWindowExplicitlySet)
        m_parentWindow = nullptr;
    emit visibleChanged();

    if (dialogCode() == Accepted)
        emit accepted();
    else if (dialogCode() == Rejected)
        emit rejected();
}

/*!
    \qmlmethod void QtQuick.Dialogs::Dialog::accept()

    Closes the dialog and emits the \l accepted() signal.

    \sa reject()
*/
void QQuickAbstractDialog::accept()
{
    done(Accepted);
}

/*!
    \qmlmethod void QtQuick.Dialogs::Dialog::reject()

    Closes the dialog and emits the \l rejected() signal.

    \sa accept()
*/
void QQuickAbstractDialog::reject()
{
    done(Rejected);
}

/*!
    \qmlmethod void QtQuick.Dialogs::Dialog::done(int result)

    Closes the dialog and sets the \a result.

    \sa accept(), reject(), result
*/
void QQuickAbstractDialog::done(int result)
{
    setResult(result);
    close();
}

void QQuickAbstractDialog::classBegin()
{
}

void QQuickAbstractDialog::componentComplete()
{
    qCDebug(lcDialogs) << "componentComplete";
    m_complete = true;

    if (!m_visibleRequested)
        return;

    m_visibleRequested = false;

    if (windowForOpen()) {
        open();
        return;
    }

    // Since visible were set to true by the user, we want the dialog to be open by default.
    // There is no guarantee that the dialog will work when it exists in a object tree that lacks a window,
    // and since qml components are sometimes instantiated before they're given a window
    // (which is the case when using QQuickView), we want to delay the call to open(), until the window is provided.
    if (const auto parentItem = findParentItem())
        connect(parentItem, &QQuickItem::windowChanged, this, &QQuickAbstractDialog::deferredOpen, Qt::SingleShotConnection);
}

static const char *qmlTypeName(const QObject *object)
{
    return object->metaObject()->className() + qstrlen("QQuickPlatform");
}

QPlatformTheme::DialogType toPlatformDialogType(QQuickDialogType quickDialogType)
{
    return quickDialogType == QQuickDialogType::FolderDialog
        ? QPlatformTheme::FileDialog : static_cast<QPlatformTheme::DialogType>(quickDialogType);
}

bool QQuickAbstractDialog::create(CreateOptions createOptions)
{
    qCDebug(lcDialogs) << qmlTypeName(this) << "attempting to create dialog backend of type"
        << int(m_type) << "with parent window" << m_parentWindow;
    if (m_handle)
        return m_handle.get();

    if ((createOptions != CreateOptions::DontTryNativeDialog) && useNativeDialog()) {
        qCDebug(lcDialogs) << "- attempting to create a native dialog";
        m_handle.reset(QGuiApplicationPrivate::platformTheme()->createPlatformDialogHelper(
            toPlatformDialogType(m_type)));
    }

    if (!m_handle) {
        qCDebug(lcDialogs) << "- attempting to create a quick dialog";
        m_handle = QQuickDialogImplFactory::createPlatformDialogHelper(m_type, this);
    }

    qCDebug(lcDialogs) << qmlTypeName(this) << "created ->" << m_handle.get();
    if (m_handle) {
        onCreate(m_handle.get());
        connect(m_handle.get(), &QPlatformDialogHelper::accept, this, &QQuickAbstractDialog::accept);
        connect(m_handle.get(), &QPlatformDialogHelper::reject, this, &QQuickAbstractDialog::reject);
    }
    return m_handle.get();
}

void QQuickAbstractDialog::destroy()
{
    m_handle.reset();
}

bool QQuickAbstractDialog::useNativeDialog() const
{
    if (QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs)) {
        qCDebug(lcDialogs) << "  - Qt::AA_DontUseNativeDialogs was set; not using native dialog";
        return false;
    }

    if (!QGuiApplicationPrivate::platformTheme()->usePlatformNativeDialog(toPlatformDialogType(m_type))) {
        qCDebug(lcDialogs) << "  - the platform theme told us a native dialog isn't available; not using native dialog";
        return false;
    }

    return true;
}

/*!
    \internal

    Called at the end of \l create().
*/
void QQuickAbstractDialog::onCreate(QPlatformDialogHelper *dialog)
{
    Q_UNUSED(dialog);
}

/*!
    \internal

    Called by \l open(), after the call to \l create() and before
    the handle/helper's \c show function is called.
*/
void QQuickAbstractDialog::onShow(QPlatformDialogHelper *dialog)
{
    Q_UNUSED(dialog);
    m_firstShow = false;
}

void QQuickAbstractDialog::onHide(QPlatformDialogHelper *dialog)
{
    Q_UNUSED(dialog);
}

int QQuickAbstractDialog::dialogCode() const { return m_result; }

QQuickItem *QQuickAbstractDialog::findParentItem() const
{
    QObject *obj = parent();
    while (obj) {
        QQuickItem *item = qobject_cast<QQuickItem *>(obj);
        if (item)
            return item;
        obj = obj->parent();
    }
    return nullptr;
}

QWindow *QQuickAbstractDialog::windowForOpen() const
{
    if (m_parentWindowExplicitlySet)
        return m_parentWindow;
    else if (auto parentItem = findParentItem())
        return parentItem->window();
    return m_parentWindow;
}

void QQuickAbstractDialog::deferredOpen(QWindow *window)
{
    m_parentWindow = window;
    open();
}

QT_END_NAMESPACE

#include "moc_qquickabstractdialog_p.cpp"
