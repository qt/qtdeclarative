// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfolderbreadcrumbbar_p.h"
#include "qquickfolderbreadcrumbbar_p_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qloggingcategory.h>
#if QT_CONFIG(shortcut)
#include <QtGui/private/qshortcutmap_p.h>
#endif
#include <QtGui/private/qguiapplication_p.h>
#include <QtQml/QQmlFile>
#include <QtQuick/private/qquicktextinput_p.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>
#include <QtQuickTemplates2/private/qquickpopupitem_p_p.h>
#include <QtQuickTemplates2/private/qquickshortcutcontext_p_p.h>

#include "qquickfiledialogimpl_p.h"
#include "qquickfiledialogimpl_p_p.h"
#include "qquickfolderdialogimpl_p.h"
#include "qquickfolderdialogimpl_p_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcFolderBreadcrumbBar, "qt.quick.dialogs.folderbreadcrumbbar")
Q_LOGGING_CATEGORY(lcContentSize, "qt.quick.dialogs.folderbreadcrumbbar.contentsize")
Q_LOGGING_CATEGORY(lcDelegates, "qt.quick.dialogs.folderbreadcrumbbar.delegates")
Q_LOGGING_CATEGORY(lcShortcuts, "qt.quick.dialogs.folderbreadcrumbbar.shortcuts")
Q_LOGGING_CATEGORY(lcTextInput, "qt.quick.dialogs.folderbreadcrumbbar.textinput")
Q_LOGGING_CATEGORY(lcCurrentItem, "qt.quick.dialogs.folderbreadcrumbbar.currentitem")

QQuickItem *QQuickFolderBreadcrumbBarPrivate::createDelegateItem(QQmlComponent *component, const QVariantMap &initialProperties)
{
    Q_Q(QQuickFolderBreadcrumbBar);
    // If we don't use the correct context, it won't be possible to refer to
    // the control's id from within the delegates.
    QQmlContext *context = component->creationContext();
    // The component might not have been created in QML, in which case
    // the creation context will be null and we have to create it ourselves.
    if (!context)
        context = qmlContext(q);

    // If we have initial properties we assume that all necessary information is passed via
    // initial properties.
    if (!component->isBound() && initialProperties.isEmpty()) {
        context = new QQmlContext(context, q);
        context->setContextObject(q);
    }

    QQuickItem *item = qobject_cast<QQuickItem*>(component->createWithInitialProperties(initialProperties, context));
    if (item)
        QQml_setParent_noEvent(item, q);
    qCDebug(lcDelegates) << "- created delegate item" << item << "with initialProperties" << initialProperties;
    return item;
}

QString QQuickFolderBreadcrumbBarPrivate::folderBaseName(const QString &folderPath)
{
    if (folderPath == QLatin1String("/")) {
        // Unix root.
        return folderPath;
    } else if (folderPath.endsWith(QLatin1String(":/"))) {
        // Windows drive.
        return folderPath.mid(0, folderPath.size() - 1);
    }
    const QString baseName = folderPath.mid(folderPath.lastIndexOf(QLatin1Char('/')) + 1);
    return baseName;
}

/*!
    \internal

    Returns \c { "/foo", "/foo/bar", "/foo/bar/baz" } if \a folder is \c "/foo/bar/baz".
*/
QStringList QQuickFolderBreadcrumbBarPrivate::crumbPathsForFolder(const QUrl &folder)
{
    const QString folderPath = QDir::fromNativeSeparators(QQmlFile::urlToLocalFileOrQrc(folder));
    QDir dir(folderPath);
    // In order to collect the paths for each breadcrumb, we need to work backwards, so we prepend.
    QStringList paths;
    do {
        paths.prepend(dir.absolutePath());
    } while (dir.cdUp());
    return paths;
}

void QQuickFolderBreadcrumbBarPrivate::repopulate()
{
    Q_Q(QQuickFolderBreadcrumbBar);
    qCDebug(lcDelegates) << "attemping to repopulate breadcrumb bar using folder...";

    if (repopulating)
        return;

    if (!buttonDelegate || !separatorDelegate || !q->contentItem()) {
        qCWarning(lcDelegates) << "Both delegates and contentItem must be set before repopulating";
        return;
    }

    QBoolBlocker repopulateGuard(repopulating);

    auto failureCleanup = [this, q](){
        folderPaths.clear();
        while (q->count() > 0)
            q->removeItem(q->itemAt(0));
    };

    qCDebug(lcDelegates) << "- getting paths for directory" << dialogFolder();
    folderPaths = crumbPathsForFolder(dialogFolder());

    while (q->count() > 0)
        q->removeItem(q->itemAt(0));

    for (int i = 0; i < folderPaths.size(); ++i) {
        const QString &folderPath = folderPaths.at(i);

        QVariantMap initialProperties = {
            { QStringLiteral("index"), QVariant::fromValue(i) },
            { QStringLiteral("folderName"), QVariant::fromValue(folderBaseName(folderPath)) }
        };
        QQuickItem *buttonItem = createDelegateItem(buttonDelegate, initialProperties);
        if (!buttonItem) {
            qCWarning(lcDelegates) << "Failed creating breadcrumb buttonDelegate item:\n" << buttonDelegate->errorString();
            failureCleanup();
            break;
        }
        if (QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton*>(buttonItem)) {
            QObjectPrivate::connect(button, &QQuickAbstractButton::clicked,
                this, &QQuickFolderBreadcrumbBarPrivate::crumbClicked);
        }
        insertItem(q->count(), buttonItem);

        // Don't add a separator for the last button.
        if (i < folderPaths.size() - 1)  {
            initialProperties = {};
            QQuickItem *separatorItem = createDelegateItem(separatorDelegate, initialProperties);
            if (!separatorItem) {
                qCWarning(lcDelegates) << "Failed creating breadcrumb separatorDelegate item:\n" << buttonDelegate->errorString();
                failureCleanup();
                break;
            }
            insertItem(q->count(), separatorItem);
        }
    }

    const int finalCount = q->count();
    // We would do - 2, since separators are included in the count,
    // but as we don't add a separator for the last button, we only need to subtract 1.
    const int newCurrentIndex = finalCount > 2 ? finalCount - 1 : -1;
    qCDebug(lcDelegates) << "- setting currentIndex to" << newCurrentIndex;
    q->setCurrentIndex(newCurrentIndex);

    updateImplicitContentSize();

    qCDebug(lcDelegates) << "... bar now contains" << q->count()
        << "buttons and separators in total, for the following paths:" << folderPaths;
}

void QQuickFolderBreadcrumbBarPrivate::crumbClicked()
{
    Q_Q(QQuickFolderBreadcrumbBar);
    qCDebug(lcCurrentItem) << "updateCurrentIndex called by sender" << q->sender();
    QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton*>(q->sender());
    if (button) {
        const int buttonIndex = contentModel->indexOf(button, nullptr);
        q->setCurrentIndex(buttonIndex);
        const QUrl folderUrl = QUrl::fromLocalFile(folderPaths.at(buttonIndex / 2));
        // TODO: don't repopulate the whole model when clicking on crumbs
        qCDebug(lcCurrentItem) << "setting file dialog's folder to" << folderUrl;
        setDialogFolder(folderUrl);
    }
}

void QQuickFolderBreadcrumbBarPrivate::folderChanged()
{
    if (componentComplete)
        repopulate();
}

static inline QString upButtonName()
{
    return QStringLiteral("upButton");
}

void QQuickFolderBreadcrumbBarPrivate::cancelUpButton()
{
    Q_Q(QQuickFolderBreadcrumbBar);
    quickCancelDeferred(q, upButtonName());
}

void QQuickFolderBreadcrumbBarPrivate::executeUpButton(bool complete)
{
    Q_Q(QQuickFolderBreadcrumbBar);
    if (upButton.wasExecuted())
        return;

    if (!upButton || complete)
        quickBeginDeferred(q, upButtonName(), upButton);
    if (complete)
        quickCompleteDeferred(q, upButtonName(), upButton);
}

void QQuickFolderBreadcrumbBarPrivate::goUp()
{
    QDir dir(QQmlFile::urlToLocalFileOrQrc(dialogFolder()));
    dir.cdUp();
    setDialogFolder(QUrl::fromLocalFile(dir.absolutePath()));
}

static inline QString textFieldName()
{
    return QStringLiteral("textField");
}

void QQuickFolderBreadcrumbBarPrivate::cancelTextField()
{
    Q_Q(QQuickFolderBreadcrumbBar);
    quickCancelDeferred(q, textFieldName());
}

void QQuickFolderBreadcrumbBarPrivate::executeTextField(bool complete)
{
    Q_Q(QQuickFolderBreadcrumbBar);
    if (textField.wasExecuted())
        return;

    if (!textField || complete)
        quickBeginDeferred(q, textFieldName(), textField);
    if (complete)
        quickCompleteDeferred(q, textFieldName(), textField);
}

void QQuickFolderBreadcrumbBarPrivate::toggleTextFieldVisibility()
{
    textField->setText(QQmlFile::urlToLocalFileOrQrc(dialogFolder()));

    qCDebug(lcTextInput).nospace() << "text field visibility was " << textField->isVisible()
        << "; setting it to " << !textField->isVisible();
    textField->setVisible(!textField->isVisible());

    if (textField->isVisible()) {
        // The text field is now visible, so give it focus,
        // select the text, and let it handle escape/back.
        textField->forceActiveFocus(Qt::ShortcutFocusReason);
        textField->selectAll();
    }

    // We connect to the TextField's visibleChanged signal, so textFieldVisibleChanged()
    // will get called automatically and we don't need to call it here.

    contentItem->setVisible(!textField->isVisible());

    // When the TextField is visible, certain items in the dialog need to be disabled.
    if (auto fileDialog = asFileDialog()) {
        auto fileDialogPrivate = QQuickFileDialogImplPrivate::get(fileDialog);
        fileDialogPrivate->updateEnabled();
    } else if (auto folderDialog = asFolderDialog()) {
        auto folderDialogPrivate = QQuickFolderDialogImplPrivate::get(folderDialog);
        folderDialogPrivate->updateEnabled();
    }
}

void QQuickFolderBreadcrumbBarPrivate::textFieldAccepted()
{
    const QUrl fileUrl = QUrl::fromLocalFile(textField->text());
    const auto fileDialog = asFileDialog();
    const bool mustExist = fileDialog ? fileDialog->options()->acceptMode() != QFileDialogOptions::AcceptSave : true;
    const bool enteredPathIsValidUrl = fileUrl.isValid();
    bool enteredPathExists = false;
    bool enteredPathIsDir = false;
    if (enteredPathIsValidUrl) {
        const QFileInfo fileInfo(textField->text());
        enteredPathExists = fileInfo.exists();
        if (enteredPathExists)
            enteredPathIsDir = fileInfo.isDir();
    }

    qCDebug(lcTextInput).nospace() << "text field accepted -"
        << " text=" << textField->text()
        << " fileUrl=" << fileUrl
        << " mustExist=" << mustExist
        << " enteredPathIsValidUrl=" << enteredPathIsValidUrl
        << " enteredPathExists=" << enteredPathExists
        << " enteredPathIsDir=" << enteredPathIsDir;

    if (enteredPathIsDir && (enteredPathExists || !mustExist)) {
        qCDebug(lcTextInput) << "path entered is a folder; setting folder";
        setDialogFolder(fileUrl);
    } else if (!enteredPathIsDir && (enteredPathExists || !mustExist)) {
        qCDebug(lcTextInput) << "path entered is a file; setting file and calling accept()";
        if (isFileDialog()) {
            auto fileDialog = asFileDialog();
            fileDialog->setSelectedFile(fileUrl);
            fileDialog->accept();
        } else {
            setDialogFolder(fileUrl);
        }
    } else {
        qCDebug(lcTextInput) << "path entered is not valid; not setting file/folder";
    }

    // If the enter key was pressed and the dialog closed, the text input will lose
    // active focus, and textFieldActiveFocusChanged() will toggle its visibility.
    // We should only toggle visibility if the dialog is actually closed, otherwise
    // we'll end up toggling twice, and the text input will be visible the next time
    // the dialog is opened.
    if (dialog->isVisible())
        toggleTextFieldVisibility();
}

void QQuickFolderBreadcrumbBarPrivate::textFieldVisibleChanged()
{
    qCDebug(lcShortcuts) << "text field was either hidden or shown";

    if (textField && textField->isVisible())
        handleTextFieldShown();
    else
        handleTextFieldHidden();
}

void QQuickFolderBreadcrumbBarPrivate::textFieldActiveFocusChanged()
{
    qCDebug(lcTextInput) << "text field activeFocus changed to" << textField->hasActiveFocus();

    // When the text field loses focus, it should be hidden.
    if (!textField->hasActiveFocus() && textField->isVisible())
        toggleTextFieldVisibility();
}

/*
    When the text field is visible:

    - Ctrl+L should do nothing (matches e.g. Ubuntu and Windows)
    - Escape/back should hide it
*/
void QQuickFolderBreadcrumbBarPrivate::handleTextFieldShown()
{
#if QT_CONFIG(shortcut)
    if (editPathToggleShortcutId == 0)
        return;

    qCDebug(lcShortcuts) << "text field was shown; ungrabbing edit path shortcut";
    ungrabEditPathShortcut();
#endif
}

/*
    When the text field is not visible:

    - Ctrl+L should make it visible
    - Escape/back should close the dialog
*/
void QQuickFolderBreadcrumbBarPrivate::handleTextFieldHidden()
{
#if QT_CONFIG(shortcut)
    Q_Q(QQuickFolderBreadcrumbBar);

    QGuiApplicationPrivate *appPrivate = QGuiApplicationPrivate::instance();
    qCDebug(lcShortcuts) << "text field was hidden; grabbing edit path shortcut";

    if (editPathToggleShortcutId == 0) {
        editPathToggleShortcutId = appPrivate->shortcutMap.addShortcut(
            q, Qt::CTRL | Qt::Key_L, Qt::WindowShortcut, QQuickShortcutContext::matcher);
    }

    qCDebug(lcShortcuts).nospace() << "... editPathToggleShortcutId=" << editPathToggleShortcutId;
#endif
}

void QQuickFolderBreadcrumbBarPrivate::ungrabEditPathShortcut()
{
#if QT_CONFIG(shortcut)
    Q_Q(QQuickFolderBreadcrumbBar);
    QGuiApplicationPrivate *appPrivate = QGuiApplicationPrivate::instance();
    if (editPathToggleShortcutId != 0) {
        appPrivate->shortcutMap.removeShortcut(editPathToggleShortcutId, q);
        editPathToggleShortcutId = 0;
    }
#endif
}

QQuickFileDialogImpl *QQuickFolderBreadcrumbBarPrivate::asFileDialog() const
{
    return qobject_cast<QQuickFileDialogImpl*>(dialog);
}

QQuickFolderDialogImpl *QQuickFolderBreadcrumbBarPrivate::asFolderDialog() const
{
    return qobject_cast<QQuickFolderDialogImpl*>(dialog);
}

bool QQuickFolderBreadcrumbBarPrivate::isFileDialog() const
{
    return asFileDialog();
}

QUrl QQuickFolderBreadcrumbBarPrivate::dialogFolder() const
{
    return dialog->property("currentFolder").toUrl();
}

void QQuickFolderBreadcrumbBarPrivate::setDialogFolder(const QUrl &folder)
{
    Q_Q(QQuickFolderBreadcrumbBar);
    if (!dialog->setProperty("currentFolder", folder))
        qmlWarning(q) << "Failed to set currentFolder property of dialog" << dialog->objectName() << "to" << folder;
}

qreal QQuickFolderBreadcrumbBarPrivate::getContentWidth() const
{
    Q_Q(const QQuickFolderBreadcrumbBar);
    const int count = contentModel->count();
    qreal totalWidth = qMax(0, count - 1) * spacing;
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = q->itemAt(i);
        if (item) {
            QQuickItemPrivate *p = QQuickItemPrivate::get(item);
            if (!p->widthValid())
                totalWidth += item->implicitWidth();
            else
                totalWidth += item->width();
        }
    }
    qCDebug(lcContentSize) << "content width:" << totalWidth;
    return totalWidth;
}

qreal QQuickFolderBreadcrumbBarPrivate::getContentHeight() const
{
    Q_Q(const QQuickFolderBreadcrumbBar);
    const int count = contentModel->count();
    qreal maxHeight = 0;
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = q->itemAt(i);
        if (item)
            maxHeight = qMax(maxHeight, item->implicitHeight());
    }
    qCDebug(lcContentSize) << "content height:" << maxHeight;
    return maxHeight;
}

void QQuickFolderBreadcrumbBarPrivate::resizeContent()
{
    Q_Q(QQuickFolderBreadcrumbBar);
    if (contentItem) {
        const int upButtonSpace = q->upButton() ? q->upButton()->width() + upButtonSpacing : 0;
        contentItem->setPosition(QPointF(q->leftPadding() + upButtonSpace, q->topPadding()));
        contentItem->setSize(QSizeF(q->availableWidth() - upButtonSpace, q->availableHeight()));

        if (textField) {
            textField->setPosition(contentItem->position());
            textField->setSize(contentItem->size());
        }
    }
}

void QQuickFolderBreadcrumbBarPrivate::itemGeometryChanged(QQuickItem *item, QQuickGeometryChange change, const QRectF &diff)
{
    QQuickContainerPrivate::itemGeometryChanged(item, change, diff);
    if (change.sizeChange())
        updateImplicitContentSize();
}

void QQuickFolderBreadcrumbBarPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    QQuickContainerPrivate::itemImplicitWidthChanged(item);
    if (item != contentItem)
        updateImplicitContentWidth();
}

void QQuickFolderBreadcrumbBarPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    QQuickContainerPrivate::itemImplicitHeightChanged(item);
    if (item != contentItem)
        updateImplicitContentHeight();
}

/*!
    \internal

    Private class for breadcrumb navigation of a directory.

    Given a FileDialog, FolderBreadCrumbbar creates breadcrumb buttons and
    separators from the specified delegate components.
*/

QQuickFolderBreadcrumbBar::QQuickFolderBreadcrumbBar(QQuickItem *parent)
    : QQuickContainer(*(new QQuickFolderBreadcrumbBarPrivate), parent)
{
    Q_D(QQuickFolderBreadcrumbBar);
    d->changeTypes |= QQuickItemPrivate::Geometry | QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight;
}

QQuickDialog *QQuickFolderBreadcrumbBar::dialog() const
{
    Q_D(const QQuickFolderBreadcrumbBar);
    return d->dialog;
}

void QQuickFolderBreadcrumbBar::setDialog(QQuickDialog *dialog)
{
    Q_D(QQuickFolderBreadcrumbBar);
    if (dialog == d->dialog)
        return;

    if (d->dialog) {
        if (auto fileDialog = d->asFileDialog()) {
            // TODO: rename impl's currentFolder too, when name is decided
            QObjectPrivate::disconnect(fileDialog, &QQuickFileDialogImpl::currentFolderChanged,
                d, &QQuickFolderBreadcrumbBarPrivate::folderChanged);
        } else if (auto folderDialog = d->asFolderDialog()) {
            QObjectPrivate::disconnect(folderDialog, &QQuickFolderDialogImpl::currentFolderChanged,
                d, &QQuickFolderBreadcrumbBarPrivate::folderChanged);
        }
    }

    d->dialog = dialog;

    if (d->dialog) {
        if (auto fileDialog = d->asFileDialog()) {
            QObjectPrivate::connect(fileDialog, &QQuickFileDialogImpl::currentFolderChanged,
                d, &QQuickFolderBreadcrumbBarPrivate::folderChanged);
        } else if (auto folderDialog = d->asFolderDialog()) {
            QObjectPrivate::connect(folderDialog, &QQuickFolderDialogImpl::currentFolderChanged,
                d, &QQuickFolderBreadcrumbBarPrivate::folderChanged);
        }
    }

    emit dialogChanged();
}

QQmlComponent *QQuickFolderBreadcrumbBar::buttonDelegate()
{
    Q_D(QQuickFolderBreadcrumbBar);
    return d->buttonDelegate;
}

void QQuickFolderBreadcrumbBar::setButtonDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickFolderBreadcrumbBar);
    qCDebug(lcFolderBreadcrumbBar) << "setButtonDelegate called with" << delegate;
    if (d->componentComplete) {
        // Simplify the code by disallowing this.
        qCWarning(lcFolderBreadcrumbBar) << "BreadcrumbBar does not support setting delegates after component completion";
        return;
    }

    if (delegate == d->buttonDelegate)
        return;

    d->buttonDelegate = delegate;
    emit buttonDelegateChanged();
}

QQmlComponent *QQuickFolderBreadcrumbBar::separatorDelegate()
{
    Q_D(QQuickFolderBreadcrumbBar);
    return d->separatorDelegate;
}

void QQuickFolderBreadcrumbBar::setSeparatorDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickFolderBreadcrumbBar);
    qCDebug(lcFolderBreadcrumbBar) << "setSeparatorDelegate called with" << delegate;
    if (d->componentComplete) {
        qCWarning(lcFolderBreadcrumbBar) << "BreadcrumbBar does not support setting delegates after component completion";
        return;
    }

    if (delegate == d->separatorDelegate)
        return;

    d->separatorDelegate = delegate;
    emit separatorDelegateChanged();
}

QQuickAbstractButton *QQuickFolderBreadcrumbBar::upButton()
{
    Q_D(QQuickFolderBreadcrumbBar);
    if (!d->upButton)
        d->executeUpButton();
    return d->upButton;
}

void QQuickFolderBreadcrumbBar::setUpButton(QQuickAbstractButton *upButton)
{
    Q_D(QQuickFolderBreadcrumbBar);
    if (upButton == d->upButton)
        return;

    if (!d->upButton.isExecuting())
        d->cancelUpButton();

    if (d->upButton) {
        QObjectPrivate::disconnect(d->upButton.data(), &QQuickAbstractButton::clicked,
            d, &QQuickFolderBreadcrumbBarPrivate::goUp);
    }

    QQuickControlPrivate::hideOldItem(d->upButton);
    d->upButton = upButton;
    if (d->upButton) {
        if (!d->upButton->parentItem())
            d->upButton->setParentItem(this);

        QObjectPrivate::connect(d->upButton.data(), &QQuickAbstractButton::clicked,
            d, &QQuickFolderBreadcrumbBarPrivate::goUp);
    }
    if (!d->upButton.isExecuting())
        emit upButtonChanged();
}

int QQuickFolderBreadcrumbBar::upButtonSpacing() const
{
    Q_D(const QQuickFolderBreadcrumbBar);
    return d->upButtonSpacing;
}

void QQuickFolderBreadcrumbBar::setUpButtonSpacing(int upButtonSpacing)
{
    Q_D(QQuickFolderBreadcrumbBar);
    if (upButtonSpacing == d->upButtonSpacing)
        return;

    d->upButtonSpacing = upButtonSpacing;
    emit upButtonSpacingChanged();
}

QQuickTextField *QQuickFolderBreadcrumbBar::textField()
{
    Q_D(QQuickFolderBreadcrumbBar);
    return d->textField;
}

void QQuickFolderBreadcrumbBar::setTextField(QQuickTextField *textField)
{
    Q_D(QQuickFolderBreadcrumbBar);
    if (textField == d->textField)
        return;

    if (!d->textField.isExecuting())
        d->cancelUpButton();

    if (d->textField)
        d->handleTextFieldHidden();

    if (d->textField) {
        QObjectPrivate::disconnect(d->textField.data(), &QQuickTextInput::visibleChanged,
            d, &QQuickFolderBreadcrumbBarPrivate::textFieldVisibleChanged);
        QObjectPrivate::disconnect(d->textField.data(), &QQuickTextInput::activeFocusChanged,
            d, &QQuickFolderBreadcrumbBarPrivate::textFieldActiveFocusChanged);
        QObjectPrivate::disconnect(d->textField.data(), &QQuickTextInput::accepted,
            d, &QQuickFolderBreadcrumbBarPrivate::textFieldAccepted);
    }

    QQuickControlPrivate::hideOldItem(d->textField);
    d->textField = textField;
    if (d->textField) {
        if (!d->textField->parentItem())
            d->textField->setParentItem(this);

        d->textField->setVisible(false);

        QObjectPrivate::connect(d->textField.data(), &QQuickTextInput::visibleChanged,
            d, &QQuickFolderBreadcrumbBarPrivate::textFieldVisibleChanged);
        QObjectPrivate::connect(d->textField.data(), &QQuickTextInput::activeFocusChanged,
            d, &QQuickFolderBreadcrumbBarPrivate::textFieldActiveFocusChanged);
        QObjectPrivate::connect(d->textField.data(), &QQuickTextInput::accepted,
            d, &QQuickFolderBreadcrumbBarPrivate::textFieldAccepted);
    }
    if (!d->textField.isExecuting())
        emit textFieldChanged();
}

bool QQuickFolderBreadcrumbBar::event(QEvent *event)
{
#if QT_CONFIG(shortcut)
    Q_D(QQuickFolderBreadcrumbBar);
    if (event->type() == QEvent::Shortcut) {
        QShortcutEvent *shortcutEvent = static_cast<QShortcutEvent *>(event);
        if (shortcutEvent->shortcutId() == d->editPathToggleShortcutId) {
            d->toggleTextFieldVisibility();
            return true;
        } else if (shortcutEvent->shortcutId() == d->goUpShortcutId) {
            d->goUp();
        }
    }
#endif
    return QQuickItem::event(event);
}

void QQuickFolderBreadcrumbBar::keyPressEvent(QKeyEvent *event)
{
#if QT_CONFIG(shortcut)
    Q_D(QQuickFolderBreadcrumbBar);

    if (event->matches(QKeySequence::Cancel) && d->textField->isVisible()) {
        d->toggleTextFieldVisibility();
        event->accept();
    } else
#endif
    {
        QQuickContainer::keyPressEvent(event);
    }
}

void QQuickFolderBreadcrumbBar::componentComplete()
{
    Q_D(QQuickFolderBreadcrumbBar);
    qCDebug(lcFolderBreadcrumbBar) << "componentComplete";
    QQuickContainer::componentComplete();
    d->repopulate();

    if (d->textField) {
        // Force it to be updated as setTextField() is too early to do it.
        d->textFieldVisibleChanged();
    }
}

void QQuickFolderBreadcrumbBar::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    Q_D(QQuickFolderBreadcrumbBar);
    QQuickContainer::itemChange(change, data);

    if (change == QQuickItem::ItemVisibleHasChanged && isComponentComplete()) {
        if (data.boolValue && d->dialog->isVisible()) {
            // It's visible.
            d->handleTextFieldHidden();

#if QT_CONFIG(shortcut)
            d->goUpShortcutId = QGuiApplicationPrivate::instance()->shortcutMap.addShortcut(
                this, QKeySequence(Qt::ALT | Qt::Key_Up), Qt::WindowShortcut, QQuickShortcutContext::matcher);
#endif
        } else {
            // It's hidden.
            // Hide the text field so that when the dialog gets opened again, it's not still visible.
            if (d->textField)
                d->textField->setVisible(false);

            // Make the ListView visible again.
            if (d->contentItem)
                d->contentItem->setVisible(true);

            // We also need to ungrab the edit path shortcut when we're not visible.
            d->ungrabEditPathShortcut();

#if QT_CONFIG(shortcut)
            if (d->goUpShortcutId != 0) {
                QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(d->goUpShortcutId, this);
                d->goUpShortcutId = 0;
            }
#endif
        }
    }
}

bool QQuickFolderBreadcrumbBar::isContent(QQuickItem *item) const
{
    if (!qmlContext(item))
        return false;

    if (QQuickItemPrivate::get(item)->isTransparentForPositioner())
        return false;

    return true;
}

QFont QQuickFolderBreadcrumbBar::defaultFont() const
{
    // TODO
    return QQuickTheme::font(QQuickTheme::TabBar);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickFolderBreadcrumbBar::accessibleRole() const
{
    // TODO
    return QAccessible::PageTabList;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickfolderbreadcrumbbar_p.cpp"
