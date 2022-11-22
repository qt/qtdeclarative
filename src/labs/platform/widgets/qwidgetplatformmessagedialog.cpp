// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwidgetplatformmessagedialog_p.h"
#include "qwidgetplatformdialog_p.h"

#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qabstractbutton.h>

QT_BEGIN_NAMESPACE

QWidgetPlatformMessageDialog::QWidgetPlatformMessageDialog(QObject *parent)
    : m_dialog(new QMessageBox)
{
    setParent(parent);

    connect(m_dialog.data(), &QMessageBox::accepted, this, &QPlatformDialogHelper::accept);
    connect(m_dialog.data(), &QMessageBox::rejected, this, &QPlatformDialogHelper::reject);
    connect(m_dialog.data(), &QMessageBox::buttonClicked, [this](QAbstractButton *button) {
        QMessageBox::ButtonRole role = m_dialog->buttonRole(button);
        QMessageBox::StandardButton standardButton = m_dialog->standardButton(button);
        emit clicked(static_cast<StandardButton>(standardButton), static_cast<ButtonRole>(role));
    });
}

QWidgetPlatformMessageDialog::~QWidgetPlatformMessageDialog()
{
}
void QWidgetPlatformMessageDialog::exec()
{
    m_dialog->exec();
}

bool QWidgetPlatformMessageDialog::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    QSharedPointer<QMessageDialogOptions> options = QPlatformMessageDialogHelper::options();
    m_dialog->setWindowTitle(options->windowTitle());
    m_dialog->setIcon(static_cast<QMessageBox::Icon>(options->standardIcon()));
    m_dialog->setText(options->text());
    m_dialog->setInformativeText(options->informativeText());
#if QT_CONFIG(textedit)
    m_dialog->setDetailedText(options->detailedText());
#endif
    m_dialog->setStandardButtons(static_cast<QMessageBox::StandardButtons>(int(options->standardButtons())));

    return QWidgetPlatformDialog::show(m_dialog.data(), flags, modality, parent);
}

void QWidgetPlatformMessageDialog::hide()
{
    m_dialog->hide();
}

QT_END_NAMESPACE

#include "moc_qwidgetplatformmessagedialog_p.cpp"
