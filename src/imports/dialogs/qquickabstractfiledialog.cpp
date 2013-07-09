/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick.Dialogs module of the Qt Toolkit.
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

#include "qquickabstractfiledialog_p.h"
#include "qquickitem.h"

#include <private/qguiapplication_p.h>
#include <QWindow>
#include <QQuickWindow>

QT_BEGIN_NAMESPACE

QQuickAbstractFileDialog::QQuickAbstractFileDialog(QObject *parent)
    : QQuickAbstractDialog(parent)
    , m_dlgHelper(0)
    , m_options(QSharedPointer<QFileDialogOptions>(new QFileDialogOptions()))
    , m_selectExisting(true)
    , m_selectMultiple(false)
    , m_selectFolder(false)
{
    connect(this, SIGNAL(accepted()), this, SIGNAL(selectionAccepted()));
}

QQuickAbstractFileDialog::~QQuickAbstractFileDialog()
{
}

void QQuickAbstractFileDialog::setVisible(bool v)
{
    if (helper() && v) {
        m_dlgHelper->setOptions(m_options);
        m_dlgHelper->setFilter();
        emit filterSelected();
    }
    QQuickAbstractDialog::setVisible(v);
}

QString QQuickAbstractFileDialog::title() const
{
    return m_options->windowTitle();
}

void QQuickAbstractFileDialog::setTitle(const QString &t)
{
    if (m_options->windowTitle() == t) return;
    m_options->setWindowTitle(t);
    emit titleChanged();
}

void QQuickAbstractFileDialog::setSelectExisting(bool selectExisting)
{
    if (selectExisting == m_selectExisting) return;
    m_selectExisting = selectExisting;
    updateModes();
}

void QQuickAbstractFileDialog::setSelectMultiple(bool selectMultiple)
{
    if (selectMultiple == m_selectMultiple) return;
    m_selectMultiple = selectMultiple;
    updateModes();
}

void QQuickAbstractFileDialog::setSelectFolder(bool selectFolder)
{
    if (selectFolder == m_selectFolder) return;
    m_selectFolder = selectFolder;
    updateModes();
}

QUrl QQuickAbstractFileDialog::folder()
{
    if (m_dlgHelper && !m_dlgHelper->directory().isEmpty())
        return QUrl::fromLocalFile(m_dlgHelper->directory());
    return QUrl::fromLocalFile(m_options->initialDirectory());
}

void QQuickAbstractFileDialog::setFolder(const QUrl &f)
{
    QString dir = f.path();
    if (m_dlgHelper)
        m_dlgHelper->setDirectory(dir);
    m_options->setInitialDirectory(dir);
    emit folderChanged();
}

void QQuickAbstractFileDialog::setNameFilters(const QStringList &f)
{
    m_options->setNameFilters(f);
    if (f.isEmpty())
        selectNameFilter(QString());
    else if (!f.contains(selectedNameFilter()))
        selectNameFilter(f.first());
    emit nameFiltersChanged();
}

QString QQuickAbstractFileDialog::selectedNameFilter()
{
    QString ret;
    if (m_dlgHelper)
        ret = m_dlgHelper->selectedNameFilter();
    if (ret.isEmpty())
        return m_options->initiallySelectedNameFilter();
    return ret;
}

void QQuickAbstractFileDialog::selectNameFilter(const QString &f)
{
    // This should work whether the dialog is currently being shown already, or ahead of time.
    m_options->setInitiallySelectedNameFilter(f);
    if (m_dlgHelper)
        m_dlgHelper->selectNameFilter(f);
    emit filterSelected();
}

QUrl QQuickAbstractFileDialog::fileUrl()
{
    QList<QUrl> urls = fileUrls();
    return (urls.count() == 1) ? urls[0] : QUrl();
}

QList<QUrl> QQuickAbstractFileDialog::fileUrls()
{
    QList<QUrl> ret;
    if (m_dlgHelper)
        foreach (QString path, m_dlgHelper->selectedFiles())
            ret << QUrl::fromLocalFile(path);
    return ret;
}

void QQuickAbstractFileDialog::updateModes()
{
    // The 4 possible modes are AnyFile, ExistingFile, Directory, ExistingFiles
    // Assume AnyFile until we find a reason to the contrary
    QFileDialogOptions::FileMode mode = QFileDialogOptions::AnyFile;

    if (m_selectFolder) {
        mode = QFileDialogOptions::Directory;
        m_options->setOption(QFileDialogOptions::ShowDirsOnly);
        m_selectMultiple = false;
        m_selectExisting = true;
        setNameFilters(QStringList());
    } else if (m_selectExisting) {
        mode = m_selectMultiple ?
            QFileDialogOptions::ExistingFiles : QFileDialogOptions::ExistingFile;
        m_options->setOption(QFileDialogOptions::ShowDirsOnly, false);
    } else if (m_selectMultiple) {
        m_selectExisting = true;
    }
    if (!m_selectExisting)
        m_selectMultiple = false;
    m_options->setFileMode(mode);
    m_options->setAcceptMode(m_selectExisting ?
                           QFileDialogOptions::AcceptOpen : QFileDialogOptions::AcceptSave);
    emit fileModeChanged();
}

QT_END_NAMESPACE
