// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicklabsplatformdialog_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>

#include "widgets/qwidgetplatform_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Dialog
    \inherits QtObject
//! \nativetype QQuickLabsPlatformDialog
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \brief The base class of native dialogs.

    The Dialog type provides common QML API for native platform dialogs.

    To show a native dialog, construct an instance of one of the concrete
    Dialog implementations, set the desired properties, and call \l open().
    Dialog emits \l accepted() or \l rejected() when the user is done with
    the dialog.

    \labs
*/

/*!
    \qmlsignal void Qt.labs.platform::Dialog::accepted()

    This signal is emitted when the dialog has been accepted either
    interactively or by calling \l accept().

    \note This signal is \e not emitted when closing the dialog with \l close().

    \sa rejected()
*/

/*!
    \qmlsignal void Qt.labs.platform::Dialog::rejected()

    This signal is emitted when the dialog has been rejected either
    interactively or by calling \l reject().

    \note This signal is \e not emitted when closing the dialog with \l close().

    \sa accepted()
*/

Q_STATIC_LOGGING_CATEGORY(qtLabsPlatformDialogs, "qt.labs.platform.dialogs")

QQuickLabsPlatformDialog::QQuickLabsPlatformDialog(QPlatformTheme::DialogType type, QObject *parent)
    : QObject(parent),
      m_visible(false),
      m_complete(false),
      m_result(0),
      m_parentWindow(nullptr),
      m_flags(Qt::Dialog),
      m_modality(Qt::WindowModal),
      m_type(type),
      m_handle(nullptr)
{
}

QQuickLabsPlatformDialog::~QQuickLabsPlatformDialog()
{
    destroy();
}

QPlatformDialogHelper *QQuickLabsPlatformDialog::handle() const
{
    return m_handle;
}

/*!
    \qmldefault
    \qmlproperty list<QtObject> Qt.labs.platform::Dialog::data

    This default property holds the list of all objects declared as children of
    the dialog.
*/
QQmlListProperty<QObject> QQuickLabsPlatformDialog::data()
{
    return QQmlListProperty<QObject>(this, &m_data);
}

/*!
    \qmlproperty Window Qt.labs.platform::Dialog::parentWindow

    This property holds the parent window of the dialog.

    Unless explicitly set, the window is automatically resolved by iterating
    the QML parent objects until a \l Window or an \l Item that has a window
    is found.
*/
QWindow *QQuickLabsPlatformDialog::parentWindow() const
{
    return m_parentWindow;
}

void QQuickLabsPlatformDialog::setParentWindow(QWindow *window)
{
    if (m_parentWindow == window)
        return;

    m_parentWindow = window;
    emit parentWindowChanged();
}

/*!
    \qmlproperty string Qt.labs.platform::Dialog::title

    This property holds the title of the dialog.
*/
QString QQuickLabsPlatformDialog::title() const
{
    return m_title;
}

void QQuickLabsPlatformDialog::setTitle(const QString &title)
{
    if (m_title == title)
        return;

    m_title = title;
    emit titleChanged();
}

/*!
    \qmlproperty Qt::WindowFlags Qt.labs.platform::Dialog::flags

    This property holds the window flags of the dialog. The default value is \c Qt.Dialog.
*/
Qt::WindowFlags QQuickLabsPlatformDialog::flags() const
{
    return m_flags;
}

void QQuickLabsPlatformDialog::setFlags(Qt::WindowFlags flags)
{
    if (m_flags == flags)
        return;

    m_flags = flags;
    emit flagsChanged();
}

/*!
    \qmlproperty Qt::WindowModality Qt.labs.platform::Dialog::modality

    This property holds the modality of the dialog. The default value is \c Qt.WindowModal.

    Available values:
    \value Qt.NonModal The dialog is not modal and does not block input to other windows.
    \value Qt.WindowModal The dialog is modal to a single window hierarchy and blocks input to its parent window, all grandparent windows, and all siblings of its parent and grandparent windows.
    \value Qt.ApplicationModal The dialog is modal to the application and blocks input to all windows.
*/
Qt::WindowModality QQuickLabsPlatformDialog::modality() const
{
    return m_modality;
}

void QQuickLabsPlatformDialog::setModality(Qt::WindowModality modality)
{
    if (m_modality == modality)
        return;

    m_modality = modality;
    emit modalityChanged();
}

/*!
    \qmlproperty bool Qt.labs.platform::Dialog::visible

    This property holds the visibility of the dialog. The default value is \c false.

    \sa open(), close()
*/
bool QQuickLabsPlatformDialog::isVisible() const
{
    return m_handle && m_visible;
}

void QQuickLabsPlatformDialog::setVisible(bool visible)
{
    if (visible)
        open();
    else
        close();
}

/*!
    \qmlproperty int Qt.labs.platform::Dialog::result

    This property holds the result code.

    Standard result codes:
    \value Dialog.Accepted
    \value Dialog.Rejected

    \note MessageDialog sets the result to the value of the clicked standard
          button instead of using the standard result codes.
*/
int QQuickLabsPlatformDialog::result() const
{
    return m_result;
}

void QQuickLabsPlatformDialog::setResult(int result)
{
    if (m_result == result)
        return;

    m_result = result;
    emit resultChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::Dialog::open()

    Opens the dialog.

    \sa visible, close()
*/
void QQuickLabsPlatformDialog::open()
{
    if (m_visible || !create())
        return;

    onShow(m_handle);
    m_visible = m_handle->show(m_flags, m_modality, m_parentWindow);
    if (m_visible)
        emit visibleChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::Dialog::close()

    Closes the dialog.

    \sa visible, open()
*/
void QQuickLabsPlatformDialog::close()
{
    if (!m_handle || !m_visible)
        return;

    onHide(m_handle);
    m_handle->hide();
    m_visible = false;
    emit visibleChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::Dialog::accept()

    Closes the dialog and emits the \l accepted() signal.

    \sa reject()
*/
void QQuickLabsPlatformDialog::accept()
{
    done(Accepted);
}

/*!
    \qmlmethod void Qt.labs.platform::Dialog::reject()

    Closes the dialog and emits the \l rejected() signal.

    \sa accept()
*/
void QQuickLabsPlatformDialog::reject()
{
    done(Rejected);
}

/*!
    \qmlmethod void Qt.labs.platform::Dialog::done(int result)

    Closes the dialog and sets the \a result.

    \sa accept(), reject(), result
*/
void QQuickLabsPlatformDialog::done(int result)
{
    close();
    setResult(result);

    if (result == Accepted)
        emit accepted();
    else if (result == Rejected)
        emit rejected();
}

void QQuickLabsPlatformDialog::classBegin()
{
}

void QQuickLabsPlatformDialog::componentComplete()
{
    m_complete = true;
    if (!m_parentWindow)
        setParentWindow(findParentWindow());
}

static const char *qmlTypeName(const QObject *object)
{
    return object->metaObject()->className() + qstrlen("QQuickLabsPlatform");
}

bool QQuickLabsPlatformDialog::create()
{
    if (!m_handle) {
        if (useNativeDialog())
            m_handle = QGuiApplicationPrivate::platformTheme()->createPlatformDialogHelper(m_type);
        if (!m_handle)
            m_handle = QWidgetPlatform::createDialog(m_type, this);
        qCDebug(qtLabsPlatformDialogs) << qmlTypeName(this) << "->" << m_handle;
        if (m_handle) {
            onCreate(m_handle);
            connect(m_handle, &QPlatformDialogHelper::accept, this, &QQuickLabsPlatformDialog::accept);
            connect(m_handle, &QPlatformDialogHelper::reject, this, &QQuickLabsPlatformDialog::reject);
        }
    }
    return m_handle;
}

void QQuickLabsPlatformDialog::destroy()
{
    delete m_handle;
    m_handle = nullptr;
}

bool QQuickLabsPlatformDialog::useNativeDialog() const
{
    return !QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs)
            && QGuiApplicationPrivate::platformTheme()->usePlatformNativeDialog(m_type);
}

void QQuickLabsPlatformDialog::onCreate(QPlatformDialogHelper *dialog)
{
    Q_UNUSED(dialog);
}

void QQuickLabsPlatformDialog::onShow(QPlatformDialogHelper *dialog)
{
    Q_UNUSED(dialog);
}

void QQuickLabsPlatformDialog::onHide(QPlatformDialogHelper *dialog)
{
    Q_UNUSED(dialog);
}

QWindow *QQuickLabsPlatformDialog::findParentWindow() const
{
    QObject *obj = parent();
    while (obj) {
        QWindow *window = qobject_cast<QWindow *>(obj);
        if (window)
            return window;
        QQuickItem *item = qobject_cast<QQuickItem *>(obj);
        if (item && item->window())
            return item->window();
        obj = obj->parent();
    }
    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qquicklabsplatformdialog_p.cpp"
