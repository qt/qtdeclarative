// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickplatformfontdialog_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qwindow.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlinfo.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickTemplates2/private/qquickdialog_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p_p.h>
#include <QtQuickTemplates2/private/qquickpopupanchors_p.h>

#include "qquickfontdialogimpl_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQuickPlatformFontDialog, "qt.quick.dialogs.quickplatformfontdialog")

/*!
    \class QQuickPlatformFontDialog
    \internal

    An interface that QQuickFontDialog can use to access the non-native Qt Quick FontDialog.

    Both this and the native implementations are created in QQuickAbstractDialog::create().

*/
QQuickPlatformFontDialog::QQuickPlatformFontDialog(QObject *parent)
{
    qCDebug(lcQuickPlatformFontDialog)
            << "creating non-native Qt Quick FontDialog with parent" << parent;

    // Set a parent so that we get deleted if we can't be shown for whatever reason.
    // Our eventual parent should be the window, though.
    setParent(parent);

    auto qmlContext = ::qmlContext(parent);
    if (!qmlContext) {
        qmlWarning(parent) << "No QQmlContext for QQuickPlatformFontDialog; can't create "
                              "non-native FontDialog implementation";
        return;
    }

    const auto dialogQmlUrl = QUrl(QStringLiteral(
            "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/qml/FontDialog.qml"));

    QQmlComponent fontDialogComponent(qmlContext->engine(), dialogQmlUrl, parent);
    if (!fontDialogComponent.isReady()) {
        qmlWarning(parent) << "Failed to load non-native FontDialog implementation:\n"
                           << fontDialogComponent.errorString();
        return;
    }

    m_dialog = qobject_cast<QQuickFontDialogImpl *>(fontDialogComponent.create());

    if (!m_dialog) {
        qmlWarning(parent) << "Failed to create an instance of the non-native FontDialog:\n"
                           << fontDialogComponent.errorString();
        return;
    }

    m_dialog->setParent(this);

    connect(m_dialog, &QQuickDialog::accepted, this, &QPlatformDialogHelper::accept);
    connect(m_dialog, &QQuickDialog::rejected, this, &QPlatformDialogHelper::reject);

    connect(m_dialog, &QQuickFontDialogImpl::currentFontChanged,
            this, &QQuickPlatformFontDialog::currentFontChanged);
}

bool QQuickPlatformFontDialog::isValid() const
{
    return m_dialog;
}

void QQuickPlatformFontDialog::setCurrentFont(const QFont &font)
{
    if (m_dialog) {
        if (!m_dialog->options())
            m_dialog->setOptions(QPlatformFontDialogHelper::options());
        m_dialog->setCurrentFont(font, true);
    }
}

QFont QQuickPlatformFontDialog::currentFont() const
{
    return m_dialog ? m_dialog->currentFont() : QFont();
}

void QQuickPlatformFontDialog::exec()
{
    qCWarning(lcQuickPlatformFontDialog)
            << "exec() is not supported for the Qt Quick FontDialog fallback";
}

bool QQuickPlatformFontDialog::show(Qt::WindowFlags flags, Qt::WindowModality modality,
                                    QWindow *parent)
{
    qCDebug(lcQuickPlatformFontDialog)
            << "show called with flags" << flags << "modality" << modality << "parent" << parent;

    if (!isValid())
        return false;

    if (!parent)
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

    QSharedPointer<QFontDialogOptions> options = QPlatformFontDialogHelper::options();
    m_dialog->setTitle(options->windowTitle());
    m_dialog->setOptions(options);

    m_dialog->init();

    m_dialog->open();
    return true;
}

void QQuickPlatformFontDialog::hide()
{
    if (isValid())
        m_dialog->close();
}

QQuickFontDialogImpl *QQuickPlatformFontDialog::dialog() const
{
    return m_dialog;
}

QT_END_NAMESPACE

#include "moc_qquickplatformfontdialog_p.cpp"
