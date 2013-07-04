/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qquickqfiledialog_p.h"
#include "qquickitem.h"

#include <private/qguiapplication_p.h>
#include <private/qqmlcontext_p.h>
#include <QWindow>
#include <QQuickWindow>
#include <QFileDialog>

QT_BEGIN_NAMESPACE

class QFileDialogHelper : public QPlatformFileDialogHelper
{
public:
    QFileDialogHelper() :
        QPlatformFileDialogHelper()
    {
        connect(&m_dialog, SIGNAL(currentChanged(const QString&)), this, SIGNAL(currentChanged(const QString&)));
        connect(&m_dialog, SIGNAL(directoryEntered(const QString&)), this, SIGNAL(directoryEntered(const QString&)));
        connect(&m_dialog, SIGNAL(fileSelected(const QString&)), this, SIGNAL(fileSelected(const QString&)));
        connect(&m_dialog, SIGNAL(filesSelected(const QStringList&)), this, SIGNAL(filesSelected(const QStringList&)));
        connect(&m_dialog, SIGNAL(filterSelected(const QString&)), this, SIGNAL(filterSelected(const QString&)));
        connect(&m_dialog, SIGNAL(accepted()), this, SIGNAL(accept()));
        connect(&m_dialog, SIGNAL(rejected()), this, SIGNAL(reject()));
    }

    virtual bool defaultNameFilterDisables() const { return true; }
    virtual void setDirectory(const QString &dir) { m_dialog.setDirectory(dir); }
    virtual QString directory() const { return m_dialog.directory().absolutePath(); }
    virtual void selectFile(const QString &f) { m_dialog.selectFile(f); }
    virtual QStringList selectedFiles() const { return m_dialog.selectedFiles(); }

    virtual void setFilter() {
        m_dialog.setWindowTitle(QPlatformFileDialogHelper::options()->windowTitle());
        if (QPlatformFileDialogHelper::options()->isLabelExplicitlySet(QFileDialogOptions::LookIn))
            m_dialog.setLabelText(m_dialog.LookIn, QPlatformFileDialogHelper::options()->labelText(QFileDialogOptions::LookIn));
        if (QPlatformFileDialogHelper::options()->isLabelExplicitlySet(QFileDialogOptions::FileName))
            m_dialog.setLabelText(m_dialog.FileName, QPlatformFileDialogHelper::options()->labelText(QFileDialogOptions::FileName));
        if (QPlatformFileDialogHelper::options()->isLabelExplicitlySet(QFileDialogOptions::FileType))
            m_dialog.setLabelText(m_dialog.FileType, QPlatformFileDialogHelper::options()->labelText(QFileDialogOptions::FileType));
        if (QPlatformFileDialogHelper::options()->isLabelExplicitlySet(QFileDialogOptions::Accept))
            m_dialog.setLabelText(m_dialog.Accept, QPlatformFileDialogHelper::options()->labelText(QFileDialogOptions::Accept));
        if (QPlatformFileDialogHelper::options()->isLabelExplicitlySet(QFileDialogOptions::Reject))
            m_dialog.setLabelText(m_dialog.Reject, QPlatformFileDialogHelper::options()->labelText(QFileDialogOptions::Reject));
        m_dialog.setFilter(QPlatformFileDialogHelper::options()->filter());
        m_dialog.setNameFilters(QPlatformFileDialogHelper::options()->nameFilters());
        m_dialog.selectNameFilter(QPlatformFileDialogHelper::options()->initiallySelectedNameFilter());
        m_dialog.setFileMode(QFileDialog::FileMode(QPlatformFileDialogHelper::options()->fileMode()));
        m_dialog.setOptions((QFileDialog::Options)((int)(QPlatformFileDialogHelper::options()->options())));
        m_dialog.setAcceptMode(QFileDialog::AcceptMode(QPlatformFileDialogHelper::options()->acceptMode()));
    }

    virtual void selectNameFilter(const QString &f) { m_dialog.selectNameFilter(f); }
    virtual QString selectedNameFilter() const { return m_dialog.selectedNameFilter(); }
    virtual void exec() { m_dialog.exec(); }

    virtual bool show(Qt::WindowFlags f, Qt::WindowModality m, QWindow *parent) {
        m_dialog.winId();
        QWindow *window = m_dialog.windowHandle();
        Q_ASSERT(window);
        window->setTransientParent(parent);
        window->setFlags(f);
        m_dialog.setWindowModality(m);
        m_dialog.show();
        return m_dialog.isVisible();
    }

    virtual void hide() { m_dialog.hide(); }

private:
    QFileDialog m_dialog;
};

/*!
    \qmltype QtFileDialog
    \instantiates QQuickQFileDialog
    \inqmlmodule QtQuick.PrivateWidgets 1
    \ingroup qtquick-visual
    \brief Dialog component for choosing files from a local filesystem.
    \since 5.1
    \internal

    QtFileDialog provides a means to instantiate and manage a QFileDialog.
    It is not recommended to be used directly; it is an implementation
    detail of \l FileDialog in the \l QtQuick.Dialogs module.

    To use this type, you will need to import the module with the following line:
    \code
    import QtQuick.PrivateWidgets 1.0
    \endcode
*/

/*!
    \qmlsignal QtQuick::Dialogs::FileDialog::accepted

    The \a accepted signal is emitted when the user has finished using the
    dialog. You can then inspect the \a fileUrl or \a fileUrls properties to
    get the selection.

    Example:

    \qml
    FileDialog {
        onAccepted: { console.log("Selected file: " + fileUrl) }
    }
    \endqml
*/

/*!
    \qmlsignal QtQuick::Dialogs::FileDialog::rejected

    The \a rejected signal is emitted when the user has dismissed the dialog,
    either by closing the dialog window or by pressing the Cancel button.
*/

/*!
    \class QQuickQFileDialog
    \inmodule QtQuick.PrivateWidgets
    \internal

    \brief The QQuickQFileDialog class is a wrapper for a QFileDialog.

    \since 5.1
*/

/*!
    Constructs a file dialog with parent window \a parent.
*/
QQuickQFileDialog::QQuickQFileDialog(QObject *parent)
    : QQuickAbstractFileDialog(parent)
{
}

/*!
    Destroys the file dialog.
*/
QQuickQFileDialog::~QQuickQFileDialog()
{
    if (m_dlgHelper)
        m_dlgHelper->hide();
    delete m_dlgHelper;
}

QPlatformFileDialogHelper *QQuickQFileDialog::helper()
{
    QQuickItem *parentItem = qobject_cast<QQuickItem *>(parent());
    if (parentItem)
        m_parentWindow = parentItem->window();

    if (!m_dlgHelper) {
        m_dlgHelper = new QFileDialogHelper();
        connect(m_dlgHelper, SIGNAL(directoryEntered(QString)), this, SIGNAL(folderChanged()));
        connect(m_dlgHelper, SIGNAL(filterSelected(QString)), this, SIGNAL(filterSelected()));
        connect(m_dlgHelper, SIGNAL(accept()), this, SLOT(accept()));
        connect(m_dlgHelper, SIGNAL(reject()), this, SLOT(reject()));
    }

    return m_dlgHelper;
}

QT_END_NAMESPACE
