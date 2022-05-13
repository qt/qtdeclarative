// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickplatformmessagedialog_p.h"

#include <QtQuickTemplates2/private/qquickpopup_p_p.h>
#include <QtQuickTemplates2/private/qquickpopupanchors_p.h>

#include "qquickmessagedialogimpl_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQuickPlatformMessageDialog, "qt.quick.dialogs.quickplatformmessagedialog")

QQuickPlatformMessageDialog::QQuickPlatformMessageDialog(QObject *parent)
{
    qCDebug(lcQuickPlatformMessageDialog)
            << "creating non-native Qt Quick MessageDialog with parent" << parent;

    // Set a parent so that we get deleted if we can't be shown for whatever reason.
    // Our eventual parent should be the window, though.
    setParent(parent);

    auto qmlContext = ::qmlContext(parent);
    if (!qmlContext) {
        qmlWarning(parent) << "No QQmlContext for QQuickPlatformMessageDialog; can't create "
                              "non-native MessageDialog implementation";
        return;
    }

    const auto dialogQmlUrl = QUrl(QStringLiteral(
            "qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/qml/MessageDialog.qml"));

    QQmlComponent messageDialogComponent(qmlContext->engine(), dialogQmlUrl, parent);
    if (!messageDialogComponent.isReady()) {
        qmlWarning(parent) << "Failed to load non-native MessageBox implementation:\n"
                           << messageDialogComponent.errorString();
        return;
    }

    m_dialog = qobject_cast<QQuickMessageDialogImpl *>(messageDialogComponent.create());

    if (!m_dialog) {
        qmlWarning(parent) << "Failed to create an instance of the non-native MessageBox:\n"
                           << messageDialogComponent.errorString();
        return;
    }

    m_dialog->setParent(this);

    connect(m_dialog, &QQuickDialog::accepted, this, &QPlatformDialogHelper::accept);
    connect(m_dialog, &QQuickDialog::rejected, this, &QPlatformDialogHelper::reject);
    connect(m_dialog, &QQuickMessageDialogImpl::buttonClicked, this,
            &QQuickPlatformMessageDialog::clicked);
}

void QQuickPlatformMessageDialog::exec()
{
    qCWarning(lcQuickPlatformMessageDialog)
            << "exec() is not supported for the Qt Quick MessageDialog fallback";
}

bool QQuickPlatformMessageDialog::show(Qt::WindowFlags flags, Qt::WindowModality modality,
                                       QWindow *parent)
{
    qCDebug(lcQuickPlatformMessageDialog)
            << "show called with flags" << flags << "modality" << modality << "parent" << parent;
    if (!m_dialog)
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

    QSharedPointer<QMessageDialogOptions> options = QPlatformMessageDialogHelper::options();
    m_dialog->setTitle(options->windowTitle());
    m_dialog->setOptions(options);

    m_dialog->open();
    return true;
}
void QQuickPlatformMessageDialog::hide()
{
    if (isValid())
        m_dialog->close();
}

bool QQuickPlatformMessageDialog::isValid() const
{
    return m_dialog;
}

QQuickMessageDialogImpl *QQuickPlatformMessageDialog::dialog() const
{
    return m_dialog;
}

QT_END_NAMESPACE

#include "moc_qquickplatformmessagedialog_p.cpp"
