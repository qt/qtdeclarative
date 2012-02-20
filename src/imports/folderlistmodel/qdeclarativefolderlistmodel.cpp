/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//![code]
#include "qdeclarativefolderlistmodel.h"
#include "fileinfothread_p.h"
#include "fileproperty_p.h"
#include <QDebug>
#include <qdeclarativecontext.h>

QT_BEGIN_NAMESPACE

class QDeclarativeFolderListModelPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeFolderListModel)

public:
    QDeclarativeFolderListModelPrivate(QDeclarativeFolderListModel *q)
        : q_ptr(q),
          sortField(QDeclarativeFolderListModel::Name), sortReversed(false), showDirs(true), showDirsFirst(false), showDots(false), showOnlyReadable(false)
    {
        nameFilters << QLatin1String("*");
    }


    QDeclarativeFolderListModel *q_ptr;
    QUrl currentDir;
    QUrl rootDir;
    FileInfoThread fileInfoThread;
    QList<FileProperty> data;
    QHash<int, QByteArray> roleNames;
    QDeclarativeFolderListModel::SortField sortField;
    QStringList nameFilters;
    bool sortReversed;
    bool showDirs;
    bool showDirsFirst;
    bool showDots;
    bool showOnlyReadable;

    ~QDeclarativeFolderListModelPrivate() {}
    void init();
    void updateSorting();

    // private slots
    void _q_directoryChanged(const QString &directory, const QList<FileProperty> &list);
    void _q_directoryUpdated(const QString &directory, const QList<FileProperty> &list, int fromIndex, int toIndex);
    void _q_sortFinished(const QList<FileProperty> &list);
};


void QDeclarativeFolderListModelPrivate::init()
{
    Q_Q(QDeclarativeFolderListModel);
    qRegisterMetaType<QList<FileProperty> >("QList<FileProperty>");
    q->connect(&fileInfoThread, SIGNAL(directoryChanged(QString, QList<FileProperty>)),
               q, SLOT(_q_directoryChanged(QString, QList<FileProperty>)));
    q->connect(&fileInfoThread, SIGNAL(directoryUpdated(QString, QList<FileProperty>, int, int)),
               q, SLOT(_q_directoryUpdated(QString, QList<FileProperty>, int, int)));
    q->connect(&fileInfoThread, SIGNAL(sortFinished(QList<FileProperty>)),
               q, SLOT(_q_sortFinished(QList<FileProperty>)));
}


void QDeclarativeFolderListModelPrivate::updateSorting()
{
    Q_Q(QDeclarativeFolderListModel);

    QDir::SortFlags flags = 0;

    switch (sortField) {
        case QDeclarativeFolderListModel::Unsorted:
            flags |= QDir::Unsorted;
            break;
        case QDeclarativeFolderListModel::Name:
            flags |= QDir::Name;
            break;
        case QDeclarativeFolderListModel::Time:
            flags |= QDir::Time;
            break;
        case QDeclarativeFolderListModel::Size:
            flags |= QDir::Size;
            break;
        case QDeclarativeFolderListModel::Type:
            flags |= QDir::Type;
            break;
        default:
            break;
    }

    emit q->layoutAboutToBeChanged();

    if (sortReversed)
        flags |= QDir::Reversed;

    fileInfoThread.setSortFlags(flags);
}

void QDeclarativeFolderListModelPrivate::_q_directoryChanged(const QString &directory, const QList<FileProperty> &list)
{
    Q_Q(QDeclarativeFolderListModel);
    Q_UNUSED(directory);

    data = list;
    q->endResetModel();
    emit q->rowCountChanged();
    emit q->folderChanged();
}


void QDeclarativeFolderListModelPrivate::_q_directoryUpdated(const QString &directory, const QList<FileProperty> &list, int fromIndex, int toIndex)
{
    Q_Q(QDeclarativeFolderListModel);
    Q_UNUSED(directory);

    QModelIndex parent;
    if (data.size() > list.size()) {
        //File(s) removed. Since I do not know how many
        //or where I need to update the whole list from the first item.
        data = list;
        q->beginRemoveRows(parent, fromIndex, toIndex);
        q->endRemoveRows();
        q->beginInsertRows(parent, fromIndex, list.size()-1);
        q->endInsertRows();
        emit q->rowCountChanged();
    } else if (data.size() < list.size()) {
        //qDebug() << "File added. FromIndex: " << fromIndex << " toIndex: " << toIndex << " list size: " << list.size();
        //File(s) added. Calculate how many and insert
        //from the first changed one.
        toIndex = fromIndex + (list.size() - data.size()-1);
        q->beginInsertRows(parent, fromIndex, toIndex);
        q->endInsertRows();
        data = list;
        emit q->rowCountChanged();
        QModelIndex modelIndexFrom = q->createIndex(fromIndex, 0);
        QModelIndex modelIndexTo = q->createIndex(toIndex, 0);
        emit q->dataChanged(modelIndexFrom, modelIndexTo);
    } else {
        //qDebug() << "File has been updated";
        QModelIndex modelIndexFrom = q->createIndex(fromIndex, 0);
        QModelIndex modelIndexTo = q->createIndex(toIndex, 0);
        data = list;
        emit q->dataChanged(modelIndexFrom, modelIndexTo);
    }
}

void QDeclarativeFolderListModelPrivate::_q_sortFinished(const QList<FileProperty> &list)
{
    Q_Q(QDeclarativeFolderListModel);

    QModelIndex parent;
    q->beginRemoveRows(parent, 0, data.size()-1);
    data.clear();
    q->endRemoveRows();

    q->beginInsertRows(parent, 0, list.size()-1);
    data = list;
    q->endInsertRows();
}


/*!
    \qmlclass FolderListModel QDeclarativeFolderListModel
    \ingroup qml-working-with-data
    \brief The FolderListModel provides a model of the contents of a file system folder.

    FolderListModel provides access to information about the contents of a folder
    in the local file system, exposing a list of files to views and other data components.

    \note This type is made available by importing the \c Qt.labs.folderlistmodel module.
    \e{Elements in the Qt.labs module are not guaranteed to remain compatible
    in future versions.}

    \bold{import Qt.labs.folderlistmodel 1.0}

    The \l folder property specifies the folder to access. Information about the
    files and directories in the folder is supplied via the model's interface.
    Components access names and paths via the following roles:

    \list
    \o \c fileName
    \o \c filePath
    \o \c fileBaseName
    \o \c fileSuffix
    \o \c fileSize
    \o \c fileModified
    \o \c fileAccessed
    \o \c fileIsDir
    \endlist

    Additionally a file entry can be differentiated from a folder entry via the
    isFolder() method.

    \section1 Filtering

    Various properties can be set to filter the number of files and directories
    exposed by the model.

    The \l nameFilters property can be set to contain a list of wildcard filters
    that are applied to names of files and directories, causing only those that
    match the filters to be exposed.

    Directories can be included or excluded using the \l showDirs property, and
    navigation directories can also be excluded by setting the \l showDotAndDotDot
    property to false.

    It is sometimes useful to limit the files and directories exposed to those
    that the user can access. The \l showOnlyReadable property can be set to
    enable this feature.

    \section1 Example Usage

    The following example shows a FolderListModel being used to provide a list
    of QML files in a \l ListView:

    \snippet doc/src/snippets/declarative/folderlistmodel.qml 0

    \section1 Path Separators

    Qt uses "/" as a universal directory separator in the same way that "/" is
    used as a path separator in URLs. If you always use "/" as a directory
    separator, Qt will translate your paths to conform to the underlying
    operating system.

    \sa {QML Data Models}
*/

QDeclarativeFolderListModel::QDeclarativeFolderListModel(QObject *parent)
    : QAbstractListModel(parent), d_ptr(new QDeclarativeFolderListModelPrivate(this))
{
    Q_D(QDeclarativeFolderListModel);
    d->roleNames[FileNameRole] = "fileName";
    d->roleNames[FilePathRole] = "filePath";
    d->roleNames[FileBaseNameRole] = "fileBaseName";
    d->roleNames[FileSuffixRole] = "fileSuffix";
    d->roleNames[FileSizeRole] = "fileSize";
    d->roleNames[FileLastModifiedRole] = "fileModified";
    d->roleNames[FileLastReadRole] = "fileAccessed";
    d->roleNames[FileIsDirRole] = "fileIsDir";
    setRoleNames(d->roleNames);

    d->init();
}

QDeclarativeFolderListModel::~QDeclarativeFolderListModel()
{
}

QVariant QDeclarativeFolderListModel::data(const QModelIndex &index, int role) const
{
    Q_D(const QDeclarativeFolderListModel);
    QVariant rv;

    if (index.row() >= d->data.size())
        return rv;

    switch (role)
    {
        case FileNameRole:
            rv = d->data.at(index.row()).fileName();
            break;
        case FilePathRole:
            rv = d->data.at(index.row()).filePath();
            break;
        case FileBaseNameRole:
            rv = d->data.at(index.row()).baseName();
            break;
        case FileSuffixRole:
            rv = d->data.at(index.row()).suffix();
            break;
        case FileSizeRole:
            rv = d->data.at(index.row()).size();
            break;
        case FileLastModifiedRole:
            rv = d->data.at(index.row()).lastModified().date().toString(Qt::ISODate) + " " + d->data.at(index.row()).lastModified().time().toString();
            break;
        case FileLastReadRole:
            rv = d->data.at(index.row()).lastRead().date().toString(Qt::ISODate) + " " + d->data.at(index.row()).lastRead().time().toString();
            break;
        case FileIsDirRole:
            rv = d->data.at(index.row()).isDir();
            break;
        default:
            break;
    }
    return rv;
}

/*!
    \qmlproperty int FolderListModel::count

    Returns the number of items in the current folder that match the
    filter criteria.
*/
int QDeclarativeFolderListModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const QDeclarativeFolderListModel);
    Q_UNUSED(parent);
    return d->data.size();
}

QModelIndex QDeclarativeFolderListModel::index(int row, int , const QModelIndex &) const
{
    return createIndex(row, 0);
}

/*!
    \qmlproperty string FolderListModel::folder

    The \a folder property holds a URL for the folder that the model is
    currently providing.

    The value is a URL expressed as a string, and must be a \c file: or \c qrc:
    URL, or a relative URL.

    By default, the value is an invalid URL.
*/
QUrl QDeclarativeFolderListModel::folder() const
{
    Q_D(const QDeclarativeFolderListModel);
    return d->currentDir;
}

void QDeclarativeFolderListModel::setFolder(const QUrl &folder)
{
    Q_D(QDeclarativeFolderListModel);

    if (folder == d->currentDir)
        return;

    QString resolvedPath = QDir::cleanPath(folder.path());

    beginResetModel();

    //Remove the old path for the file system watcher
    if (!d->currentDir.isEmpty())
        d->fileInfoThread.removePath(d->currentDir.path());

    d->currentDir = folder;

    QFileInfo info(resolvedPath);
    if (!info.exists() || !info.isDir()) {
        d->data.clear();
        endResetModel();
        emit rowCountChanged();
        return;
    }

    d->fileInfoThread.setPath(resolvedPath);
}


/*!
   \qmlproperty string QDeclarativeFolderListModel::rootFolder

   When the rootFolder is set, then this folder will
   be threated as the root in the file system, so that
   you can only travers sub folders from this rootFolder.
*/
QUrl QDeclarativeFolderListModel::rootFolder() const
{
    Q_D(const QDeclarativeFolderListModel);
    return d->rootDir;
}

void QDeclarativeFolderListModel::setRootFolder(const QUrl &path)
{
    Q_D(QDeclarativeFolderListModel);

    if (path.isEmpty())
        return;

    QString resolvedPath = QDir::cleanPath(path.path());

    QFileInfo info(resolvedPath);
    if (!info.exists() || !info.isDir())
        return;

    d->fileInfoThread.setRootPath(resolvedPath);
    d->rootDir = path;
}


/*!
    \qmlproperty url FolderListModel::parentFolder

    Returns the URL of the parent of of the current \l folder.
*/
QUrl QDeclarativeFolderListModel::parentFolder() const
{
    Q_D(const QDeclarativeFolderListModel);

    QString localFile = d->currentDir.toLocalFile();
    if (!localFile.isEmpty()) {
        QDir dir(localFile);
#if defined(Q_OS_WIN)
        if (dir.isRoot())
            dir.setPath("");
        else
#endif
            dir.cdUp();
        localFile = dir.path();
    } else {
        int pos = d->currentDir.path().lastIndexOf(QLatin1Char('/'));
        if (pos == -1)
            return QUrl();
        localFile = d->currentDir.path().left(pos);
    }
    return QUrl::fromLocalFile(localFile);
}

/*!
    \qmlproperty list<string> FolderListModel::nameFilters

    The \a nameFilters property contains a list of file name filters.
    The filters may include the ? and * wildcards.

    The example below filters on PNG and JPEG files:

    \qml
    FolderListModel {
        nameFilters: [ "*.png", "*.jpg" ]
    }
    \endqml

    \note Directories are not excluded by filters.
*/
QStringList QDeclarativeFolderListModel::nameFilters() const
{
    Q_D(const QDeclarativeFolderListModel);
    return d->nameFilters;
}

void QDeclarativeFolderListModel::setNameFilters(const QStringList &filters)
{
    Q_D(QDeclarativeFolderListModel);
    d->fileInfoThread.setNameFilters(filters);
    d->nameFilters = filters;
}

void QDeclarativeFolderListModel::classBegin()
{
}

void QDeclarativeFolderListModel::componentComplete()
{
    Q_D(QDeclarativeFolderListModel);

    if (!d->currentDir.isValid() || d->currentDir.toLocalFile().isEmpty() || !QDir().exists(d->currentDir.toLocalFile()))
        setFolder(QUrl(QLatin1String("file://")+QDir::currentPath()));
}

/*!
    \qmlproperty enumeration FolderListModel::sortField

    The \a sortField property contains field to use for sorting.  sortField
    may be one of:
    \list
    \o Unsorted - no sorting is applied.
    \o Name - sort by filename
    \o LastModified - sort by time modified
    \o Size - sort by file size
    \o Type - sort by file type (extension)
    \endlist

    \sa sortReversed
*/
QDeclarativeFolderListModel::SortField QDeclarativeFolderListModel::sortField() const
{
    Q_D(const QDeclarativeFolderListModel);
    return d->sortField;
}

void QDeclarativeFolderListModel::setSortField(SortField field)
{
    Q_D(QDeclarativeFolderListModel);
    if (field != d->sortField) {
        d->sortField = field;
        d->updateSorting();
    }
}

int QDeclarativeFolderListModel::roleFromString(const QString &roleName) const
{
    Q_D(const QDeclarativeFolderListModel);
    return d->roleNames.key(roleName.toLatin1(), -1);
}

/*!
    \qmlproperty bool FolderListModel::sortReversed

    If set to true, reverses the sort order.  The default is false.

    \sa sortField
*/
bool QDeclarativeFolderListModel::sortReversed() const
{
    Q_D(const QDeclarativeFolderListModel);
    return d->sortReversed;
}

void QDeclarativeFolderListModel::setSortReversed(bool rev)
{
    Q_D(QDeclarativeFolderListModel);

    if (rev != d->sortReversed) {
        d->sortReversed = rev;
        d->updateSorting();
    }
}

/*!
    \qmlmethod bool FolderListModel::isFolder(int index)

    Returns true if the entry \a index is a folder; otherwise
    returns false.
*/
bool QDeclarativeFolderListModel::isFolder(int index) const
{
    if (index != -1) {
        QModelIndex idx = createIndex(index, 0);
        if (idx.isValid()) {
            QVariant var = data(idx, FileIsDirRole);
            if (var.isValid())
                return var.toBool();
        }
    }
    return false;
}

/*!
    \qmlproperty bool FolderListModel::showDirs

    If true, directories are included in the model; otherwise only files
    are included.

    By default, this property is true.

    Note that the nameFilters are not applied to directories.

    \sa showDotAndDotDot
*/
bool QDeclarativeFolderListModel::showDirs() const
{
    Q_D(const QDeclarativeFolderListModel);
    return d->showDirs;
}

void  QDeclarativeFolderListModel::setShowDirs(bool on)
{
    Q_D(QDeclarativeFolderListModel);

    d->fileInfoThread.setShowDirs(on);
    d->showDirs = on;
}

/*!
    \qmlproperty bool FolderListModel::showDirsFirst

    If true, if directories are included in the model they will
    always be shown first, then the files.

    By default, this property is false.

*/
bool QDeclarativeFolderListModel::showDirsFirst() const
{
    Q_D(const QDeclarativeFolderListModel);
    return d->showDirsFirst;
}

void  QDeclarativeFolderListModel::setShowDirsFirst(bool on)
{
    Q_D(QDeclarativeFolderListModel);

    d->fileInfoThread.setShowDirsFirst(on);
    d->showDirsFirst = on;
}


/*!
    \qmlproperty bool FolderListModel::showDotAndDotDot

    If true, the "." and ".." directories are included in the model; otherwise
    they are excluded.

    By default, this property is false.

    \sa showDirs
*/
bool QDeclarativeFolderListModel::showDotAndDotDot() const
{
    Q_D(const QDeclarativeFolderListModel);
    return d->showDots;
}

void  QDeclarativeFolderListModel::setShowDotAndDotDot(bool on)
{
    Q_D(QDeclarativeFolderListModel);

    if (on != d->showDots) {
        d->fileInfoThread.setShowDotDot(on);
    }
}

/*!
    \qmlproperty bool FolderListModel::showOnlyReadable

    If true, only readable files and directories are shown; otherwise all files
    and directories are shown.

    By default, this property is false.

    \sa showDirs
*/
bool QDeclarativeFolderListModel::showOnlyReadable() const
{
    Q_D(const QDeclarativeFolderListModel);
    return d->showOnlyReadable;
}

void QDeclarativeFolderListModel::setShowOnlyReadable(bool on)
{
    Q_D(QDeclarativeFolderListModel);

    if (on != d->showOnlyReadable) {
        d->fileInfoThread.setShowOnlyReadable(on);
    }
}

/*!
    \qmlmethod QVariant QDeclarativeFolderListModel::get(int idx, const QString &property) const

    Get the folder property for the given index. The following properties
    are available.

    \list
        \o \c fileName
        \o \c filePath
        \o \c fileBaseName
        \o \c fileSuffix
        \o \c fileSize
        \o \c fileModified
        \o \c fileAccessed
        \o \c fileIsDir
    \endlist
*/
QVariant QDeclarativeFolderListModel::get(int idx, const QString &property) const
{
    int role = roleFromString(property);
    if (role >= 0 && idx >= 0)
        return data(index(idx, 0), role);
    else
        return QVariant();
}

#include "moc_qdeclarativefolderlistmodel.cpp"

//![code]
QT_END_NAMESPACE
