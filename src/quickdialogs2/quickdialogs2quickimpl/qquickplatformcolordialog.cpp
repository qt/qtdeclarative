/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickplatformcolordialog_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qwindow.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlinfo.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickTemplates2/private/qquickdialog_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p_p.h>
#include <QtQuickTemplates2/private/qquickpopupanchors_p.h>

#include "qquickcolordialogimpl_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQuickPlatformColorDialog, "qt.quick.dialogs.quickplatformcolordialog")

QQuickPlatformColorDialog::QQuickPlatformColorDialog(QObject *parent)
{
    qCDebug(lcQuickPlatformColorDialog)
            << "creating non-native Qt Quick ColorDialog with parent" << parent;

    // Set a parent so that we get deleted if we can't be shown for whatever reason.
    // Our eventual parent should be the window, however.
    setParent(parent);

    auto qmlContext = ::qmlContext(parent);
    if (!qmlContext) {
        qmlWarning(parent) << "No QQmlContext for QQuickPlatformColorDialog; can't create "
                              "non-native ColorDialog implementation";
        return;
    }

    const auto dialogQmlUrl = QUrl(QStringLiteral(
            "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/qml/ColorDialog.qml"));
    QQmlComponent colorDialogComponent(qmlContext->engine(), dialogQmlUrl, parent);
    if (!colorDialogComponent.isReady()) {
        qmlWarning(parent) << "Failed to load non-native ColorDialog implementation:\n"
                           << colorDialogComponent.errorString();
        return;
    }

    m_dialog = qobject_cast<QQuickColorDialogImpl *>(colorDialogComponent.create());
    if (!m_dialog) {
        qmlWarning(parent) << "Failed to create an instance of the non-native ColorDialog:\n"
                           << colorDialogComponent.errorString();
        return;
    }
    // Give it a parent until it's parented to the window in show().
    m_dialog->setParent(this);

    connect(m_dialog, &QQuickDialog::accepted, this, &QPlatformDialogHelper::accept);
    connect(m_dialog, &QQuickDialog::rejected, this, &QPlatformDialogHelper::reject);

    connect(m_dialog, &QQuickColorDialogImpl::colorChanged, this,
            &QQuickPlatformColorDialog::currentColorChanged);
}

bool QQuickPlatformColorDialog::isValid() const
{
    return m_dialog;
}

void QQuickPlatformColorDialog::setCurrentColor(const QColor &color)
{
    if (m_dialog)
        m_dialog->setColor(color);
}

QColor QQuickPlatformColorDialog::currentColor() const
{
    return m_dialog ? m_dialog->color().toRgb() : QColor();
}

void QQuickPlatformColorDialog::exec()
{
    qCWarning(lcQuickPlatformColorDialog)
            << "exec() is not supported for the Qt Quick ColorDialog fallback";
}

bool QQuickPlatformColorDialog::show(Qt::WindowFlags flags, Qt::WindowModality modality,
                                     QWindow *parent)
{
    qCDebug(lcQuickPlatformColorDialog)
            << "show called with flags" << flags << "modality" << modality << "parent" << parent;

    if (!m_dialog || !parent)
        return false;

    auto quickWindow = qobject_cast<QQuickWindow *>(parent);

    if (!quickWindow) {
        qmlInfo(this->parent()) << "Parent window (" << parent
                                << ") of non-native dialog is not a QQuickWindow";
        return false;
    }

    m_dialog->setParent(parent);
    m_dialog->resetParentItem();

    auto popupPrivate = QQuickPopupPrivate::get(m_dialog);
    popupPrivate->getAnchors()->setCenterIn(m_dialog->parentItem());

    QSharedPointer<QColorDialogOptions> options = QPlatformColorDialogHelper::options();

    m_dialog->setTitle(options->windowTitle());
    m_dialog->setOptions(options);

    m_dialog->open();
    return true;
}

void QQuickPlatformColorDialog::hide()
{
    if (!m_dialog)
        return;

    m_dialog->close();
}

QQuickColorDialogImpl *QQuickPlatformColorDialog::dialog() const
{
    return m_dialog;
}

QT_END_NAMESPACE
