/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qquickmessagedialogimpl_p.h"
#include "qquickmessagedialogimpl_p_p.h"

#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p_p.h>

QT_BEGIN_NAMESPACE

QQuickMessageDialogImplPrivate::QQuickMessageDialogImplPrivate() { }

void QQuickMessageDialogImplPrivate::handleClick(QQuickAbstractButton *button)
{
    Q_Q(QQuickMessageDialogImpl);

    if (const QQuickMessageDialogImplAttached *attached = attachedOrWarn()) {
        QPlatformDialogHelper::StandardButton standardButton =
                QQuickDialogButtonBoxPrivate::get(attached->buttonBox())->standardButton(button);
        QPlatformDialogHelper::ButtonRole role = buttonRole(button);

        emit q->buttonClicked(standardButton, role);
    }

    QQuickDialogPrivate::handleClick(button);
}

QQuickMessageDialogImplAttached *QQuickMessageDialogImplPrivate::attachedOrWarn()
{
    Q_Q(QQuickMessageDialogImpl);
    QQuickMessageDialogImplAttached *attached = static_cast<QQuickMessageDialogImplAttached *>(
            qmlAttachedPropertiesObject<QQuickMessageDialogImpl>(q));
    if (!attached)
        qmlWarning(q) << "Expected MessageDialogImpl attached object to be present on" << this;
    return attached;
}

QQuickMessageDialogImpl::QQuickMessageDialogImpl(QObject *parent)
    : QQuickDialog(*(new QQuickMessageDialogImplPrivate), parent)
{
}

QSharedPointer<QMessageDialogOptions> QQuickMessageDialogImpl::options() const
{
    Q_D(const QQuickMessageDialogImpl);
    return d->options;
}

void QQuickMessageDialogImpl::setOptions(const QSharedPointer<QMessageDialogOptions> &options)
{
    Q_D(QQuickMessageDialogImpl);
    d->options = options;

    QQuickMessageDialogImplAttached *attached = d->attachedOrWarn();

    if (options && attached) {
        attached->detailedTextButton()->setVisible(!d->options->detailedText().isEmpty());
        attached->buttonBox()->setStandardButtons(d->options->standardButtons());
    }

    if (showDetailedText())
        toggleShowDetailedText();

    emit optionsChanged();
}

QString QQuickMessageDialogImpl::text() const
{
    Q_D(const QQuickMessageDialogImpl);
    return d->options ? d->options->text() : QString();
}

QString QQuickMessageDialogImpl::informativeText() const
{
    Q_D(const QQuickMessageDialogImpl);
    return d->options ? d->options->informativeText() : QString();
}

QString QQuickMessageDialogImpl::detailedText() const
{
    Q_D(const QQuickMessageDialogImpl);
    return d->options ? d->options->detailedText() : QString();
}

bool QQuickMessageDialogImpl::showDetailedText() const
{
    Q_D(const QQuickMessageDialogImpl);
    return d->m_showDetailedText;
}

void QQuickMessageDialogImpl::toggleShowDetailedText()
{
    Q_D(QQuickMessageDialogImpl);
    d->m_showDetailedText = !d->m_showDetailedText;
    emit showDetailedTextChanged();
}

QQuickMessageDialogImplAttached *QQuickMessageDialogImpl::qmlAttachedProperties(QObject *object)
{
    return new QQuickMessageDialogImplAttached(object);
}

QQuickMessageDialogImplAttached::QQuickMessageDialogImplAttached(QObject *parent)
    : QObject(*(new QQuickMessageDialogImplAttachedPrivate), parent)
{
    if (!qobject_cast<QQuickMessageDialogImpl *>(parent)) {
        qmlWarning(this) << "MessageDialogImpl attached properties should only be "
                         << "accessed through the root MessageDialogImpl instance";
    }
}

QQuickDialogButtonBox *QQuickMessageDialogImplAttached::buttonBox() const
{
    Q_D(const QQuickMessageDialogImplAttached);
    return d->buttonBox;
}

void QQuickMessageDialogImplAttached::setButtonBox(QQuickDialogButtonBox *buttons)
{
    Q_D(QQuickMessageDialogImplAttached);
    if (d->buttonBox == buttons)
        return;

    if (d->buttonBox) {
        QQuickMessageDialogImpl *messageDialogImpl =
                qobject_cast<QQuickMessageDialogImpl *>(parent());
        if (messageDialogImpl) {
            auto dialogPrivate = QQuickDialogPrivate::get(messageDialogImpl);
            QObjectPrivate::disconnect(d->buttonBox, &QQuickDialogButtonBox::accepted,
                                       dialogPrivate, &QQuickDialogPrivate::handleAccept);
            QObjectPrivate::disconnect(d->buttonBox, &QQuickDialogButtonBox::rejected,
                                       dialogPrivate, &QQuickDialogPrivate::handleReject);
            QObjectPrivate::disconnect(d->buttonBox, &QQuickDialogButtonBox::clicked, dialogPrivate,
                                       &QQuickDialogPrivate::handleClick);
        }
    }

    d->buttonBox = buttons;

    if (d->buttonBox) {
        QQuickMessageDialogImpl *messageDialogImpl =
                qobject_cast<QQuickMessageDialogImpl *>(parent());
        if (messageDialogImpl) {
            auto dialogPrivate = QQuickDialogPrivate::get(messageDialogImpl);
            QObjectPrivate::connect(d->buttonBox, &QQuickDialogButtonBox::accepted, dialogPrivate,
                                    &QQuickDialogPrivate::handleAccept);
            QObjectPrivate::connect(d->buttonBox, &QQuickDialogButtonBox::rejected, dialogPrivate,
                                    &QQuickDialogPrivate::handleReject);
            QObjectPrivate::connect(d->buttonBox, &QQuickDialogButtonBox::clicked, dialogPrivate,
                                    &QQuickDialogPrivate::handleClick);
        }
    }

    emit buttonBoxChanged();
}

QQuickButton *QQuickMessageDialogImplAttached::detailedTextButton() const
{
    Q_D(const QQuickMessageDialogImplAttached);
    return d->detailedTextButton;
}
void QQuickMessageDialogImplAttached::setDetailedTextButton(QQuickButton *detailedTextButton)
{
    Q_D(QQuickMessageDialogImplAttached);

    if (d->detailedTextButton == detailedTextButton)
        return;

    if (d->detailedTextButton) {
        QQuickMessageDialogImpl *messageDialogImpl =
                qobject_cast<QQuickMessageDialogImpl *>(parent());
        if (messageDialogImpl)
            disconnect(d->detailedTextButton, &QQuickAbstractButton::clicked, messageDialogImpl,
                       &QQuickMessageDialogImpl::toggleShowDetailedText);
    }

    d->detailedTextButton = detailedTextButton;

    if (d->detailedTextButton) {
        QQuickMessageDialogImpl *messageDialogImpl =
                qobject_cast<QQuickMessageDialogImpl *>(parent());
        if (messageDialogImpl)
            connect(d->detailedTextButton, &QQuickAbstractButton::clicked, messageDialogImpl,
                    &QQuickMessageDialogImpl::toggleShowDetailedText);
    }

    emit detailedTextButtonChanged();
}

QT_END_NAMESPACE

#include "moc_qquickmessagedialogimpl_p.cpp"
