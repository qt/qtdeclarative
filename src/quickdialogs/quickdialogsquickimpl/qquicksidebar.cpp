// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicksidebar_p.h"
#include "qquicksidebar_p_p.h"
#include "qquickfiledialogimpl_p_p.h"
#include <QtQml/qqmllist.h>
#if QT_CONFIG(settings)
#include <QtCore/qsettings.h>
#endif

#include <QtQuickTemplates2/private/qquickaction_p.h>
#include <QtQuickTemplates2/private/qquickcontextmenu_p.h>

/*!
    \internal

 Private class for the sidebar in a file dialog.

 Given a FileDialog, SideBar creates a ListView that appears on the left hand side of the
 of the FileDialog's content item. The ListView has two halves. The first half contains
 standard paths and the second half contains favorites. Favorites can be added by dragging
 and dropping a directory from the main FileDialog ListView into the SideBar. Favorites are
 removed by right clicking and selecting 'Remove' from the context menu.
*/

using namespace Qt::Literals::StringLiterals;

static std::initializer_list<QStandardPaths::StandardLocation> s_defaultPaths = {
   QStandardPaths::HomeLocation,     QStandardPaths::DesktopLocation,
   QStandardPaths::DownloadLocation, QStandardPaths::DocumentsLocation,
   QStandardPaths::MusicLocation,    QStandardPaths::PicturesLocation,
   QStandardPaths::MoviesLocation,
};

QQuickSideBar::QQuickSideBar(QQuickItem *parent)
    : QQuickContainer(*(new QQuickSideBarPrivate), parent)
{
    Q_D(QQuickSideBar);
    d->folderPaths = s_defaultPaths;

    QObject::connect(this, &QQuickContainer::currentIndexChanged, [d](){
        d->currentButtonClickedUrl.clear();
    });

    // read in the favorites
#if QT_CONFIG(settings)
    d->readSettings();
#endif
}

QQuickSideBar::~QQuickSideBar()
{
    Q_D(QQuickSideBar);

#if QT_CONFIG(settings)
    d->writeSettings();
#endif
}

QQuickDialog *QQuickSideBar::dialog() const
{
    Q_D(const QQuickSideBar);
    return d->dialog;
}

void QQuickSideBar::setDialog(QQuickDialog *dialog)
{
    Q_D(QQuickSideBar);
    if (dialog == d->dialog)
        return;

    if (auto fileDialog = qobject_cast<QQuickFileDialogImpl *>(d->dialog))
        QObjectPrivate::disconnect(fileDialog, &QQuickFileDialogImpl::currentFolderChanged, d,
                                   &QQuickSideBarPrivate::folderChanged);

    d->dialog = dialog;

    if (auto fileDialog = qobject_cast<QQuickFileDialogImpl *>(d->dialog))
        QObjectPrivate::connect(fileDialog, &QQuickFileDialogImpl::currentFolderChanged, d,
                                &QQuickSideBarPrivate::folderChanged);

    emit dialogChanged();
}

QList<QStandardPaths::StandardLocation> QQuickSideBar::folderPaths() const
{
    Q_D(const QQuickSideBar);
    return d->folderPaths;
}

void QQuickSideBar::setFolderPaths(const QList<QStandardPaths::StandardLocation> &folderPaths)
{
    Q_D(QQuickSideBar);
    if (folderPaths == d->folderPaths)
        return;

    const auto oldEffective = effectiveFolderPaths();

    d->folderPaths = folderPaths;
    emit folderPathsChanged();

    if (oldEffective != effectiveFolderPaths())
        emit effectiveFolderPathsChanged();

    d->repopulate();
}

QList<QStandardPaths::StandardLocation> QQuickSideBar::effectiveFolderPaths() const
{
    QList<QStandardPaths::StandardLocation> effectivePaths;

    // The home location is never returned as empty
    const QString homeLocation = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    bool homeFound = false;
    for (auto &path : folderPaths()) {
        if (!homeFound && path == QStandardPaths::HomeLocation) {
            effectivePaths.append(path);
            homeFound = true;
        } else if (QStandardPaths::writableLocation(path) != homeLocation) {
            // if a standard path is not found, it will be resolved to home location
            effectivePaths.append(path);
        }
    }

    return effectivePaths;
}

QList<QUrl> QQuickSideBar::favoritePaths() const
{
    Q_D(const QQuickSideBar);
    return d->favoritePaths;
}

void QQuickSideBar::setFavoritePaths(const QList<QUrl> &favoritePaths)
{
    Q_D(QQuickSideBar);
    if (favoritePaths == d->favoritePaths)
        return;

    d->favoritePaths = favoritePaths;
    emit favoritePathsChanged();

#if QT_CONFIG(settings)
    d->writeSettings();
#endif
    d->repopulate();
}

QQmlComponent *QQuickSideBar::buttonDelegate() const
{
    Q_D(const QQuickSideBar);
    return d->buttonDelegate;
}

void QQuickSideBar::setButtonDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickSideBar);
    if (d->componentComplete || delegate == d->buttonDelegate)
        return;

    d->buttonDelegate = delegate;
    emit buttonDelegateChanged();
}

QQmlComponent *QQuickSideBar::separatorDelegate() const
{
    Q_D(const QQuickSideBar);
    return d->separatorDelegate;
}

void QQuickSideBar::setSeparatorDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickSideBar);
    if (d->componentComplete || delegate == d->separatorDelegate)
        return;

    d->separatorDelegate = delegate;
    emit separatorDelegateChanged();
}

QQmlComponent *QQuickSideBar::addFavoriteDelegate() const
{
    Q_D(const QQuickSideBar);
    return d->addFavoriteDelegate;
}

void QQuickSideBar::setAddFavoriteDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickSideBar);
    if (d->componentComplete || delegate == d->addFavoriteDelegate)
        return;

    d->addFavoriteDelegate = delegate;
    emit addFavoriteDelegateChanged();

    if (d->showAddFavoriteDelegate())
        d->repopulate();
}

QQuickItem *QQuickSideBarPrivate::createDelegateItem(QQmlComponent *component,
                                                     const QVariantMap &initialProperties)
{
    Q_Q(QQuickSideBar);
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

    QQuickItem *item = qobject_cast<QQuickItem *>(
            component->createWithInitialProperties(initialProperties, context));
    if (item)
        QQml_setParent_noEvent(item, q);
    return item;
}

void QQuickSideBarPrivate::repopulate()
{
    Q_Q(QQuickSideBar);

    if (repopulating || !buttonDelegate || !separatorDelegate || !addFavoriteDelegate || !q->contentItem())
        return;

    QBoolBlocker repopulateGuard(repopulating);

    auto updateIconSourceAndSize = [this](QQuickAbstractButton *button, const QUrl &iconUrl) {
        // we need to preserve the default binding on icon.color, so
        // we just take the default-created icon, and update its source
        // and size
        QQuickIcon icon = button->icon();
        icon.setSource(iconUrl);
        const QSize iconSize = dialogIconSize();
        icon.setWidth(iconSize.width());
        icon.setHeight(iconSize.height());
        button->setIcon(icon);
    };

    auto createButtonDelegate = [this, q, &updateIconSourceAndSize](int index, const QString &folderPath, const QUrl &iconUrl) {
        const QString displayName = displayNameFromFolderPath(folderPath);
        QVariantMap initialProperties = {
                                          { "index"_L1, QVariant::fromValue(index) },
                                          { "folderName"_L1, QVariant::fromValue(displayName) },
                                        };

        if (QQuickItem *buttonItem = createDelegateItem(buttonDelegate, initialProperties)) {
            if (QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(buttonItem)) {
                QObjectPrivate::connect(button, &QQuickAbstractButton::clicked, this,
                                        &QQuickSideBarPrivate::buttonClicked);
                updateIconSourceAndSize(button, iconUrl);
            }
            insertItem(q->count(), buttonItem);
        }
    };

    // clean up previous state
    while (q->count() > 0)
        q->removeItem(q->itemAt(0));

    // repopulate
    const auto folders = q->effectiveFolderPaths();
    const auto favorites = q->favoritePaths();
    showSeparator = !folders.isEmpty() && (!favorites.isEmpty() || showAddFavoriteDelegate());
    int insertIndex = 0;

    for (auto &folder : folders)
        createButtonDelegate(insertIndex++, QStandardPaths::displayName(folder), folderIconSource(folder));

    if (showSeparator)
        if (QQuickItem *separatorItem = createDelegateItem(separatorDelegate, {}))
            insertItem(insertIndex++, separatorItem);

    if (showAddFavoriteDelegate()) {
        // the variant needs to be QString, not a QLatin1StringView
        const QString labelText = QCoreApplication::translate("FileDialog", "Add Favorite");
        QVariantMap initialProperties = {
                                          { "labelText"_L1, QVariant::fromValue(labelText) },
                                          { "dragHovering"_L1, QVariant::fromValue(addFavoriteDelegateHovered()) },
                                        };
        if (auto *addFavoriteDelegateItem = createDelegateItem(addFavoriteDelegate, initialProperties)) {
            if (QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(addFavoriteDelegateItem))
                updateIconSourceAndSize(button, addFavoriteIconUrl());
            insertItem(insertIndex++, addFavoriteDelegateItem);
        }
    }

    // calculate the starting index for the favorites
    for (auto &favorite : favorites)
        createButtonDelegate(insertIndex++, favorite.toLocalFile(), folderIconSource());

    q->setCurrentIndex(-1);
}

void QQuickSideBarPrivate::buttonClicked()
{
    Q_Q(QQuickSideBar);
    if (QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(q->sender())) {
        const int buttonIndex = contentModel->indexOf(button, nullptr);
        q->setCurrentIndex(buttonIndex);

        currentButtonClickedUrl = QUrl();
        // calculate the starting index for the favorites
        const int offset = q->effectiveFolderPaths().size() + (showSeparator ? 1 : 0);
        if (buttonIndex >= offset)
            currentButtonClickedUrl = q->favoritePaths().at(buttonIndex - offset);
        else
            currentButtonClickedUrl = QUrl::fromLocalFile(
                    QStandardPaths::writableLocation(q->effectiveFolderPaths().at(buttonIndex)));

        currentButtonClickedUrl.setScheme("file"_L1);
        setDialogFolder(currentButtonClickedUrl);
    }
}

void QQuickSideBarPrivate::folderChanged()
{
    Q_Q(QQuickSideBar);

    if (dialog->property("currentFolder").toUrl() != currentButtonClickedUrl)
        q->setCurrentIndex(-1);
}

QString QQuickSideBarPrivate::displayNameFromFolderPath(const QString &folderPath)
{
    return folderPath.section(QLatin1Char('/'), -1);
}

QUrl QQuickSideBarPrivate::dialogFolder() const
{
    return dialog->property("currentFolder").toUrl();
}

void QQuickSideBarPrivate::setDialogFolder(const QUrl &folder)
{
    Q_Q(QQuickSideBar);
    if (!dialog->setProperty("currentFolder", folder))
        qmlWarning(q) << "Failed to set currentFolder property of dialog" << dialog->objectName()
                      << "to" << folder;
}

void QQuickSideBar::componentComplete()
{
    Q_D(QQuickSideBar);
    QQuickContainer::componentComplete();
    d->repopulate();
    d->initContextMenu();
}

QUrl QQuickSideBarPrivate::folderIconSource() const
{
    return QUrl("qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/sidebar-folder.png"_L1);
}

QUrl QQuickSideBarPrivate::folderIconSource(QStandardPaths::StandardLocation stdLocation) const
{
    switch (stdLocation) {
    case QStandardPaths::DesktopLocation:
        return QUrl("qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/sidebar-desktop.png"_L1);
    case QStandardPaths::DocumentsLocation:
        return QUrl("qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/sidebar-documents.png"_L1);
    case QStandardPaths::MusicLocation:
        return QUrl("qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/sidebar-music.png"_L1);
    case QStandardPaths::MoviesLocation:
        return QUrl("qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/sidebar-video.png"_L1);
    case QStandardPaths::PicturesLocation:
        return QUrl("qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/sidebar-photo.png"_L1);
    case QStandardPaths::HomeLocation:
        return QUrl("qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/sidebar-home.png"_L1);
    case QStandardPaths::DownloadLocation:
        return QUrl("qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/sidebar-downloads.png"_L1);
    default:
        return QUrl("qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/sidebar-folder.png"_L1);
    }
}

QSize QQuickSideBarPrivate::dialogIconSize() const
{
    return QSize(16, 16);
}

#if QT_CONFIG(settings)
void QQuickSideBarPrivate::writeSettings() const
{
    QSettings settings("QtProject"_L1, "qquickfiledialog"_L1);
    settings.beginWriteArray("favorites");

    for (int i = 0; i < favoritePaths.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("favorite", favoritePaths.at(i));
    }
    settings.endArray();
}

void QQuickSideBarPrivate::readSettings()
{
    favoritePaths.clear();
    QSettings settings("QtProject"_L1, "qquickfiledialog"_L1);
    const int size = settings.beginReadArray("favorites");

    QList<QUrl> newPaths;

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        const QUrl favorite = settings.value("favorite").toUrl();
        const QFileInfo info(favorite.toLocalFile());

        if (info.isDir())
            // check it is not a duplicate
            if (!newPaths.contains(favorite))
                newPaths.append(favorite);
    }
    settings.endArray();

    favoritePaths = newPaths;
}
#endif

void QQuickSideBarPrivate::addFavorite(const QUrl &favorite)
{
    Q_Q(QQuickSideBar);
    QList<QUrl> newPaths = q->favoritePaths();
    const QFileInfo info(favorite.toLocalFile());
    if (info.isDir()) {
        // check it is not a duplicate
        if (!newPaths.contains(favorite)) {
            newPaths.prepend(favorite);
            q->setFavoritePaths(newPaths);
        }
    }
}

void QQuickSideBarPrivate::removeFavorite(const QUrl &favorite)
{
    Q_Q(QQuickSideBar);
    QList<QUrl> paths = q->favoritePaths();
    bool success = paths.removeOne(favorite);
    if (success)
        q->setFavoritePaths(paths);
    else
        qmlWarning(q) << "Failed to remove favorite path" << favorite;
}

bool QQuickSideBarPrivate::showAddFavoriteDelegate() const
{
    return addFavoriteDelegateVisible;
}

void QQuickSideBarPrivate::setShowAddFavoriteDelegate(bool show)
{
    if (show == addFavoriteDelegateVisible)
        return;

    addFavoriteDelegateVisible = show;
    repopulate();
}

bool QQuickSideBarPrivate::addFavoriteDelegateHovered() const
{
    return addFavoriteHovered;
}

void QQuickSideBarPrivate::setAddFavoriteDelegateHovered(bool hovered)
{
    if (hovered == addFavoriteHovered)
        return;

    addFavoriteHovered = hovered;
    repopulate();
}

QUrl QQuickSideBarPrivate::addFavoriteIconUrl() const
{
    return QUrl("qrc:/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/sidebar-plus.png"_L1);
}

void QQuickSideBarPrivate::initContextMenu()
{
    Q_Q(QQuickSideBar);
    contextMenu = new QQuickContextMenu(q);
    connect(contextMenu, &QQuickContextMenu::requested, this, &QQuickSideBarPrivate::handleContextMenuRequested);
}

void QQuickSideBarPrivate::handleContextMenuRequested(QPointF pos)
{
    Q_Q(QQuickSideBar);
    const int offset = q->effectiveFolderPaths().size() + (showSeparator ? 1 : 0);
    for (int i = offset; i < q->count(); ++i) {
        QQuickItem *itm = q->itemAt(i);
        if (itm->contains(itm->mapFromItem(q, pos))) {
            auto favorites = q->favoritePaths();
            urlToBeRemoved = favorites.value(i - offset);

            if (!urlToBeRemoved.isEmpty() && !menu) {
                QQmlEngine *eng = qmlEngine(q);
                Q_ASSERT(eng);
                QQmlContext *context = qmlContext(q);
                QQmlComponent component(eng);
                component.loadFromModule("QtQuick.Controls", "Menu");
                menu = qobject_cast<QQuickMenu*>(component.create(context));
                if (menu) {
                    auto *removeAction = new QQuickAction(menu);
                    removeAction->setText(QCoreApplication::translate("FileDialog", "Remove"));
                    menu->addAction(removeAction);
                    connect(removeAction, &QQuickAction::triggered, this, &QQuickSideBarPrivate::handleRemoveAction);
                }
            }
            contextMenu->setMenu(menu);
            return;
        }
    }
    contextMenu->setMenu(nullptr);  // prevent the Context menu from popping up otherwise
}

void QQuickSideBarPrivate::handleRemoveAction()
{
    if (!urlToBeRemoved.isEmpty())
        removeFavorite(urlToBeRemoved);
    urlToBeRemoved.clear();
}
