// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfiledialogdelegate_p.h"
#include "qquickfiledialogdelegate_p_p.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qmimedata.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtQml/QQmlFile>
#include <QtQml/qqmlexpression.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuick/private/qquickitemview_p_p.h>
#include "qquicksidebar_p.h"
#include "qquicksidebar_p_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

void QQuickFileDialogDelegatePrivate::highlightFile()
{
    Q_Q(QQuickFileDialogDelegate);
    QQuickListViewAttached *attached = static_cast<QQuickListViewAttached*>(
        qmlAttachedPropertiesObject<QQuickListView>(q));
    if (!attached)
        return;

    QQmlContext *delegateContext = qmlContext(q);
    if (!delegateContext)
        return;

    bool converted = false;
    const int index = q->property("index").toInt(&converted);
    if (converted) {
        attached->view()->setCurrentIndex(index);
        if (fileDialog)
            fileDialog->setSelectedFile(file);
        else if (folderDialog)
            folderDialog->setSelectedFolder(file);
    }
}

void QQuickFileDialogDelegatePrivate::chooseFile()
{
    const QFileInfo fileInfo(QQmlFile::urlToLocalFileOrQrc(file));
    if (fileInfo.isDir()) {
        // If it's a directory, navigate to it.
        if (fileDialog)
            fileDialog->setCurrentFolder(file);
        else
            folderDialog->setCurrentFolder(file);
    } else {
        Q_ASSERT(fileDialog);
        // Otherwise it's a file, so select it and close the dialog.
        fileDialog->setSelectedFile(file);

        // Prioritize closing the dialog with QQuickDialogPrivate::handleClick() over QQuickDialog::accept()
        const QQuickFileDialogImplAttached *attached = QQuickFileDialogImplPrivate::get(fileDialog)->attachedOrWarn();
        if (Q_LIKELY(attached)) {
            auto *openButton = attached->buttonBox()->standardButton(QPlatformDialogHelper::Open);
            if (Q_LIKELY(openButton)) {
                emit openButton->clicked();
                return;
            }
        }
        fileDialog->accept();
    }
}

bool QQuickFileDialogDelegatePrivate::acceptKeyClick(Qt::Key key) const
{
    return key == Qt::Key_Return || key == Qt::Key_Enter;
}

QQuickFileDialogDelegate::QQuickFileDialogDelegate(QQuickItem *parent)
    : QQuickItemDelegate(*(new QQuickFileDialogDelegatePrivate), parent)
{
    Q_D(QQuickFileDialogDelegate);
    // Clicking and tabbing should result in it getting focus,
    // as e.g. Ubuntu and Windows both allow tabbing through file dialogs.
    setFocusPolicy(Qt::StrongFocus);
    setCheckable(true);
    QObjectPrivate::connect(this, &QQuickFileDialogDelegate::clicked,
        d, &QQuickFileDialogDelegatePrivate::highlightFile);
    QObjectPrivate::connect(this, &QQuickFileDialogDelegate::doubleClicked,
        d, &QQuickFileDialogDelegatePrivate::chooseFile);
}

QQuickDialog *QQuickFileDialogDelegate::dialog() const
{
    Q_D(const QQuickFileDialogDelegate);
    return d->dialog;
}

void QQuickFileDialogDelegate::setDialog(QQuickDialog *dialog)
{
    Q_D(QQuickFileDialogDelegate);
    if (dialog == d->dialog)
        return;

    d->dialog = dialog;
    d->fileDialog = qobject_cast<QQuickFileDialogImpl*>(dialog);
    d->folderDialog = qobject_cast<QQuickFolderDialogImpl*>(dialog);
    emit dialogChanged();

    if (d->tapHandler)
        d->destroyTapHandler();
    if (d->fileDialog)
        d->initTapHandler();


}

QUrl QQuickFileDialogDelegate::file() const
{
    Q_D(const QQuickFileDialogDelegate);
    return d->file;
}

void QQuickFileDialogDelegate::setFile(const QUrl &file)
{
    Q_D(QQuickFileDialogDelegate);
    QUrl adjustedFile = file;
#ifdef Q_OS_WIN32
    // Work around QTBUG-99105 (FolderListModel uses lowercase drive letter).
    QString path = adjustedFile.path();
    const int driveColonIndex = path.indexOf(QLatin1Char(':'));
    if (driveColonIndex == 2) {
        path.replace(1, 1, path.at(1).toUpper());
        adjustedFile.setPath(path);
    }
#endif
    if (adjustedFile == d->file)
        return;

    d->file = adjustedFile;
    emit fileChanged();
}

void QQuickFileDialogDelegate::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickFileDialogDelegate);
    // We need to respond to being triggered by enter being pressed,
    // but we can't use event->isAccepted() to check, because events are pre-accepted.
    auto connection = QObjectPrivate::connect(this, &QQuickFileDialogDelegate::clicked,
        d, &QQuickFileDialogDelegatePrivate::chooseFile);

    QQuickItemDelegate::keyReleaseEvent(event);

    disconnect(connection);
}

void QQuickFileDialogDelegatePrivate::initTapHandler()
{
    Q_Q(QQuickFileDialogDelegate);
    if (!tapHandler) {
        tapHandler = new QQuickFileDialogTapHandler(q);

        connect(tapHandler, &QQuickTapHandler::longPressed, this,
                                &QQuickFileDialogDelegatePrivate::handleLongPress);
    }
}

void QQuickFileDialogDelegatePrivate::destroyTapHandler()
{
    if (tapHandler)
        delete tapHandler;
}

void QQuickFileDialogDelegatePrivate::handleLongPress()
{
    if (!tapHandler)
        return;

    tapHandler->m_state = QQuickFileDialogTapHandler::Tracking;
    tapHandler->m_longPressed = true;
}

// ----------------------------------------------

QQuickFileDialogTapHandler::QQuickFileDialogTapHandler(QQuickItem *parent)
    : QQuickTapHandler(parent)
{
    // Set a grab permission that stops the flickable from stealing the drag.
    setGrabPermissions(QQuickPointerHandler::CanTakeOverFromAnything);

    // The drag threshold is used by the handler to know when to start a drag.
    // We handle the drag inpendently, so set the threshold to a big number.
    // This will guarantee that QQuickTapHandler::wantsEventPoint always returns
    // true.
    setDragThreshold(1000);
}

QQuickFileDialogImpl *QQuickFileDialogTapHandler::getFileDialogImpl() const
{
    auto *delegate = qobject_cast<QQuickFileDialogDelegate*>(parent());
    auto *delegatePrivate = QQuickFileDialogDelegatePrivate::get(delegate);
    return delegatePrivate->fileDialog;
}

void QQuickFileDialogTapHandler::grabFolder()
{
    if (m_drag.isNull())
        return;

    QQuickPalette *palette = [this]() -> QQuickPalette* {
        auto *delegate = qobject_cast<QQuickFileDialogDelegate*>(parent());
        if (delegate) {
            QQuickDialog *dlg = delegate->dialog();
            if (dlg) {
                QQuickDialogPrivate *priv = QQuickDialogPrivate::get(dlg);
                if (priv)
                    return priv->palette();
            }
        }
        return nullptr;
    }();

    // TODO: use proper @Nx scaling
    const auto src = ":/qt-project.org/imports/QtQuick/Dialogs/quickimpl/images/sidebar-folder.png"_L1;
    QImage img = QImage(src).convertToFormat(QImage::Format_ARGB32);

    if (!img.isNull() && palette) {
        const QColor iconColor = palette->buttonText();
        if (iconColor.alpha() > 0) {
            // similar to what QQuickIconImage does
            QPainter painter(&img);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(img.rect(), iconColor);
        }
    }

    QPixmap pixmap = QPixmap::fromImage(std::move(img));
    auto *mimeData = new QMimeData();
    mimeData->setImageData(pixmap);
    m_drag->setMimeData(mimeData);
    m_drag->setPixmap(pixmap);
}

QUrl QQuickFileDialogTapHandler::getFolderUrlAtPress() const
{
    return qobject_cast<QQuickFileDialogDelegate*>(parent())->file().toLocalFile();
}

void QQuickFileDialogTapHandler::handleDrag(QQuickDragEvent *event)
{
    if (m_state == Dragging) {
        auto *fileDialogImpl = getFileDialogImpl();
        auto *attached = QQuickFileDialogImplPrivate::get(fileDialogImpl)->attachedOrWarn();
        if (Q_LIKELY(attached)) {
            auto *sideBar = attached->sideBar();
            if (Q_LIKELY(sideBar)) {
                auto *sideBarPrivate = QQuickSideBarPrivate::get(sideBar);
                const auto pos = QPoint(event->x(), event->y());
                sideBarPrivate->setShowAddFavoriteDelegate(sideBar->contains(pos));
                if (sideBarPrivate->showAddFavoriteDelegate()) {
                    const QList<QQuickItem *> items = sideBar->contentChildren().toList<QList<QQuickItem *>>();
                    // iterate through the children until we have counted all the folder paths. The next button will
                    // be the addFavoriteDelegate
                    const int addFavoritePos = sideBar->effectiveFolderPaths().size();
                    int currentPos = 0;
                    for (int i = 0; i < items.length(); i++) {
                        if (auto *button = qobject_cast<QQuickAbstractButton *>(items.at(i))) {
                            if (currentPos == addFavoritePos) {
                                // check if the pointer position is within the add favorite button
                                const QRectF bBox = button->mapRectToItem(sideBar, button->boundingRect());
                                sideBarPrivate->setAddFavoriteDelegateHovered(bBox.contains(pos));
                                break;
                            } else {
                                currentPos++;
                            }
                        }
                    }
                }
            }
        }
    }
}

void QQuickFileDialogTapHandler::handleDrop(QQuickDragEvent *event)
{
    Q_UNUSED(event);
    if (m_state == Dragging) {
        if (!m_sourceUrl.isEmpty()) {
            auto *fileDialogImpl = getFileDialogImpl();
            auto *attached = QQuickFileDialogImplPrivate::get(fileDialogImpl)->attachedOrWarn();
            if (Q_LIKELY(attached)) {
                auto *sideBar = attached->sideBar();
                if (Q_LIKELY(sideBar)) {
                    auto *sideBarPrivate = QQuickSideBarPrivate::get(sideBar);
                    // this cannot be handled in handleDrag because handleDrag is connected to the drop
                    // area, and so won't run when the cursor is outside of it
                    if (sideBarPrivate->addFavoriteDelegateHovered())
                        sideBarPrivate->addFavorite(m_sourceUrl);
                    sideBarPrivate->setShowAddFavoriteDelegate(false);
                }
            }
        }
        m_state = DraggingFinished;
    }
}

void QQuickFileDialogTapHandler::handleContainsDragChanged()
{
    if (m_state == Dragging) {
        auto *fileDialogImpl = getFileDialogImpl();
        auto *attached = QQuickFileDialogImplPrivate::get(fileDialogImpl)->attachedOrWarn();
        if (Q_LIKELY(attached)) {
            auto *sideBar = attached->sideBar();
            if (Q_LIKELY(sideBar && m_dropArea)) {
                auto *sideBarPrivate = QQuickSideBarPrivate::get(sideBar);
                if (m_dropArea->containsDrag()) {
                    sideBarPrivate->setShowAddFavoriteDelegate(true);
                } else {
                    sideBarPrivate->setShowAddFavoriteDelegate(false);
                    sideBarPrivate->setAddFavoriteDelegateHovered(false);
                }
            }
        }
    }
}

void QQuickFileDialogTapHandler::resetDragData()
{
    if (m_state != Listening) {
        m_state = Listening;
        m_sourceUrl = QUrl();
        if (!m_drag.isNull()) {
            m_drag->disconnect();
            delete m_drag;
        }
        if (!m_dropArea.isNull()) {
            m_dropArea->disconnect();
            delete m_dropArea;
        }

        m_longPressed = false;
    }
}

void QQuickFileDialogTapHandler::handleEventPoint(QPointerEvent *event, QEventPoint &point)
{
    QQuickTapHandler::handleEventPoint(event, point);

    auto *fileDialogDelegate = qobject_cast<QQuickFileDialogDelegate *>(parent());
    auto *fileDialogImpl = getFileDialogImpl();
    if (Q_UNLIKELY(!fileDialogImpl))
        return;
    auto *fileDialogImplPrivate = QQuickFileDialogImplPrivate::get(fileDialogImpl);
    QQuickFileDialogImplAttached *attached = fileDialogImplPrivate->attachedOrWarn();
    if (Q_UNLIKELY(!attached || !attached->sideBar()))
        return;

    if (m_state == DraggingFinished)
        resetDragData();

    if (point.state() == QEventPoint::Pressed) {
        resetDragData();
        // Activate the passive grab to get further move updates
        setPassiveGrab(event, point, true);
    } else if (point.state() == QEventPoint::Released) {
        resetDragData();
    } else if (point.state() == QEventPoint::Updated && m_longPressed) {
        // Check to see that the movement can be considered as dragging
        const qreal distX = point.position().x() - point.pressPosition().x();
        const qreal distY = point.position().y() - point.pressPosition().y();
        // consider the squared distance to be optimal
        const qreal dragDistSq = distX * distX + distY * distY;
        if (dragDistSq > qPow(qApp->styleHints()->startDragDistance(), 2)) {
            switch (m_state) {
            case Tracking: {
                // set the drag
                if (m_drag.isNull())
                    m_drag = new QDrag(fileDialogDelegate);
                // Set the drop area
                if (m_dropArea.isNull()) {
                    auto *sideBar = attached->sideBar();
                    m_dropArea = new QQuickDropArea(sideBar);
                    m_dropArea->setSize(sideBar->size());
                    connect(m_dropArea, &QQuickDropArea::positionChanged, this,
                            &QQuickFileDialogTapHandler::handleDrag);
                    connect(m_dropArea, &QQuickDropArea::dropped, this,
                            &QQuickFileDialogTapHandler::handleDrop);
                    connect(m_dropArea, &QQuickDropArea::containsDragChanged, this,
                            &QQuickFileDialogTapHandler::handleContainsDragChanged);
                }

                m_sourceUrl = fileDialogDelegate->file();
                // set up the grab
                grabFolder();
                m_state = DraggingStarted;
            } break;

            case DraggingStarted: {
                if (m_drag && m_drag->mimeData()) {
                    if (auto *item = qobject_cast<QQuickItem *>(m_drag->source())) {
                        Q_UNUSED(item);
                        m_state = Dragging;
                        // start the drag
                        m_drag->exec();
                        // If the state still remains dragging, then the drop happened outside
                        // the drop area
                        if (m_state == Dragging)
                            resetDragData();
                    }
                }
            } break;

            default:
                break;
            }
        }
    }
}

bool QQuickFileDialogTapHandler::wantsEventPoint(const QPointerEvent *event, const QEventPoint &point)
{
    if (!QQuickTapHandler::wantsEventPoint(event, point) || event->type() == QEvent::Type::Wheel)
        return false;

    // we only want the event point if the delegate contains a directory and not a file
    auto *fileDialogDelegate = qobject_cast<QQuickFileDialogDelegate *>(parent());
    QFileInfo info(fileDialogDelegate->file().toLocalFile());
    if (info.isDir())
        return true;

    return false;
}

QT_END_NAMESPACE

#include "moc_qquickfiledialogdelegate_p.cpp"
