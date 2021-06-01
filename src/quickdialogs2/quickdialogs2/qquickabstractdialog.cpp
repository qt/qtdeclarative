/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

    Each dialog has a handle to QPlatformDialogHelper, which is created in create().
    The helper acts as an intermediary between the QML-facing dialog object
    and the native/widget/quick implementation:

    +------------+      +------------------------------------+      +-------------------------------------+
    |            |      |                                    |      |                                     |
    | FileDialog |----->| Native/Widget/Quick QPlatformFile- |----->| Native OS dialog/QQuickFileDialog/  |
    |            |      | DialogHelper subclass              |      | QQuickFileDialogImpl                |
    |            |      |                                    |      |                                     |
    +------------+      +------------------------------------+      +-------------------------------------+
*/

/*!
    \qmltype Dialog
    \inherits QtObject
//! \instantiates QQuickAbstractDialog
    \inqmlmodule QtQuick.Dialogs
    \since 6.2
    \brief The base class of native dialogs.

    The Dialog type provides common QML API for native platform dialogs.

    To show a native dialog, construct an instance of one of the concrete
    Dialog implementations, set the desired properties, and call \l open().
    Dialog emits \l accepted() or \l rejected() when the user is done with
    the dialog.
*/

/*!
    \qmlsignal void QtQuick.Dialogs::Dialog::accepted()

    This signal is emitted when the dialog has been accepted either
    interactively or by calling \l accept().

    \note This signal is \e not emitted when closing the dialog with \l close().

    \sa rejected()
*/

/*!
    \qmlsignal void QtQuick.Dialogs::Dialog::rejected()

    This signal is emitted when the dialog has been rejected either
    interactively or by calling \l reject().

    \note This signal is \e not emitted when closing the dialog with \l close().

    \sa accepted()
*/

Q_DECLARE_LOGGING_CATEGORY(lcDialogs)

QQuickAbstractDialog::QQuickAbstractDialog(QPlatformTheme::DialogType type, QObject *parent)
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
    \qmlproperty list<Object> QtQuick.Dialogs::Dialog::data

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
    if (m_parentWindow == window)
        return;

    m_parentWindow = window;
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
    \qmlproperty StandardCode QtQuick.Dialogs::Dialog::result

    This property holds the result code.

    Standard result codes:
    \value Dialog.Accepted
    \value Dialog.Rejected

    \note MessageDialog sets the result to the value of the clicked standard
          button instead of using the standard result codes.
*/
QQuickAbstractDialog::StandardCode QQuickAbstractDialog::result() const
{
    return m_result;
}

void QQuickAbstractDialog::setResult(StandardCode result)
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
    m_visible = m_handle->show(m_flags, m_modality, m_parentWindow);
    if (m_visible)
        emit visibleChanged();
}

/*!
    \qmlmethod void QtQuick.Dialogs::Dialog::close()

    Closes the dialog.

    \sa visible, open()
*/
void QQuickAbstractDialog::close()
{
    if (!m_handle || !m_visible)
        return;

    onHide(m_handle.get());
    m_handle->hide();
    m_visible = false;
    emit visibleChanged();
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
    \qmlmethod void QtQuick.Dialogs::Dialog::done(StandardCode result)

    Closes the dialog and sets the \a result.

    \sa accept(), reject(), result
*/
void QQuickAbstractDialog::done(StandardCode result)
{
    close();
    setResult(result);

    if (result == Accepted)
        emit accepted();
    else if (result == Rejected)
        emit rejected();
}

void QQuickAbstractDialog::classBegin()
{
}

void QQuickAbstractDialog::componentComplete()
{
    qCDebug(lcDialogs) << "componentComplete";
    m_complete = true;

    if (!m_parentWindow) {
        qCDebug(lcDialogs) << "- no parent window; searching for one";
        setParentWindow(findParentWindow());
    }

    if (m_visibleRequested) {
        qCDebug(lcDialogs) << "visible was bound to true before component completion; opening dialog";
        open();
        m_visibleRequested = false;
    }
}

static const char *qmlTypeName(const QObject *object)
{
    return object->metaObject()->className() + qstrlen("QQuickPlatform");
}

bool QQuickAbstractDialog::create()
{
    qCDebug(lcDialogs) << qmlTypeName(this) << "attempting to create dialog backend of type"
        << m_type << "with parent window" << m_parentWindow;
    if (m_handle)
        return m_handle.get();

    qCDebug(lcDialogs) << "- attempting to create a native dialog";
    if (useNativeDialog())
        m_handle.reset(QGuiApplicationPrivate::platformTheme()->createPlatformDialogHelper(m_type));

    if (!m_handle) {
        qCDebug(lcDialogs) << "- attempting to create a quick dialog";
        m_handle.reset(QQuickDialogImplFactory::createPlatformDialogHelper(m_type, this));
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

    if (!QGuiApplicationPrivate::platformTheme()->usePlatformNativeDialog(m_type)) {
        qCDebug(lcDialogs) << "  - the platform theme told us a native dialog isn't available; not using native dialog";
        return false;
    }

    return true;
}

void QQuickAbstractDialog::onCreate(QPlatformDialogHelper *dialog)
{
    Q_UNUSED(dialog);
}

void QQuickAbstractDialog::onShow(QPlatformDialogHelper *dialog)
{
    Q_UNUSED(dialog);
}

void QQuickAbstractDialog::onHide(QPlatformDialogHelper *dialog)
{
    Q_UNUSED(dialog);
}

QWindow *QQuickAbstractDialog::findParentWindow() const
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
