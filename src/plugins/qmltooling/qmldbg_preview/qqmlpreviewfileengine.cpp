// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpreviewfileengine.h"
#include "qqmlpreviewservice.h"

#include <QtCore/qlibraryinfo.h>
#include <QtCore/qthread.h>
#include <QtCore/qwaitcondition.h>

#include <cstring>

QT_BEGIN_NAMESPACE

static bool isRelative(const QString &path)
{
    if (path.isEmpty())
        return true;
    if (path.at(0) == '/')
        return false;
    if (path.at(0) == ':' && path.size() >= 2 && path.at(1) == '/')
        return false;
#ifdef Q_OS_WIN
    if (path.length() >= 2 && path.at(1) == ':')
        return false;
#endif
    return true;
}

static QString absolutePath(const QString &path)
{
    return QDir::cleanPath(isRelative(path) ? (QDir::currentPath() + '/' + path) : path);
}

bool isRootPath(const QString &path)
{
    return QFileSystemEntry::isRootPath(path);
}

class QQmlPreviewFileEngineIterator : public QAbstractFileEngineIterator
{
public:
    QQmlPreviewFileEngineIterator(const QString &path, QDirListing::IteratorFlags filters,
                                  const QStringList &filterNames, const QStringList &m_entries);
    ~QQmlPreviewFileEngineIterator();

    bool advance() override;
    QString currentFileName() const override;

private:
    const QStringList m_entries;
    int m_index;
};

QQmlPreviewFileEngineIterator::QQmlPreviewFileEngineIterator(const QString &path,
                                                             QDirListing::IteratorFlags filters,
                                                             const QStringList &filterNames,
                                                             const QStringList &entries)
    : QAbstractFileEngineIterator(path, filters, filterNames), m_entries(entries), m_index(0)
{
}

QQmlPreviewFileEngineIterator::~QQmlPreviewFileEngineIterator()
{
}

bool QQmlPreviewFileEngineIterator::advance()
{
    if (m_index >= m_entries.size())
        return false;

    ++m_index;
    return true;
}

QString QQmlPreviewFileEngineIterator::currentFileName() const
{
    if (m_index == 0 || m_index > m_entries.size())
        return QString();
    return m_entries.at(m_index - 1);
}

QQmlPreviewFileEngine::QQmlPreviewFileEngine(const QString &file, const QString &absolute,
                                             QQmlPreviewFileLoader *loader) :
    m_name(file), m_absolute(absolute), m_loader(loader)
{
    load();
}

void QQmlPreviewFileEngine::setFileName(const QString &file)
{
    m_name = file;
    m_absolute = absolutePath(file);
    m_fallback.reset();
    m_contents.close();
    m_contents.setData(QByteArray());
    m_entries.clear();
    load();
}

bool QQmlPreviewFileEngine::open(QIODevice::OpenMode flags,
                                 std::optional<QFile::Permissions> permissions)
{
    switch (m_result) {
    case QQmlPreviewFileLoader::File:
        return m_contents.open(flags);
    case QQmlPreviewFileLoader::Directory:
        return false;
    case QQmlPreviewFileLoader::Fallback:
        return m_fallback->open(flags, permissions);
    default:
        Q_UNREACHABLE_RETURN(false);
    }
}

bool QQmlPreviewFileEngine::close()
{
    switch (m_result) {
    case QQmlPreviewFileLoader::Fallback:
        return m_fallback->close();
    case QQmlPreviewFileLoader::File:
        m_contents.close();
        return true;
    case QQmlPreviewFileLoader::Directory:
        return false;
    default:
        Q_UNREACHABLE_RETURN(false);
    }
}

qint64 QQmlPreviewFileEngine::size() const
{
    return m_fallback ? m_fallback->size() : m_contents.size();
}

qint64 QQmlPreviewFileEngine::pos() const
{
    return m_fallback ? m_fallback->pos() : m_contents.pos();
}

bool QQmlPreviewFileEngine::seek(qint64 newPos)
{
    return m_fallback? m_fallback->seek(newPos) : m_contents.seek(newPos);
}

qint64 QQmlPreviewFileEngine::read(char *data, qint64 maxlen)
{
    return m_fallback ? m_fallback->read(data, maxlen) : m_contents.read(data, maxlen);
}

QAbstractFileEngine::FileFlags QQmlPreviewFileEngine::fileFlags(
        QAbstractFileEngine::FileFlags type) const
{
    if (m_fallback)
        return m_fallback->fileFlags(type);

    QAbstractFileEngine::FileFlags ret;

    if (type & PermsMask) {
        ret |= QAbstractFileEngine::FileFlags(
                    ReadOwnerPerm | ReadUserPerm | ReadGroupPerm | ReadOtherPerm);
    }

    if (type & TypesMask) {
        if (m_result == QQmlPreviewFileLoader::Directory)
            ret |= DirectoryType;
        else
            ret |= FileType;
    }

    if (type & FlagsMask) {
        ret |= ExistsFlag;
        if (isRootPath(m_name))
            ret |= RootFlag;
    }

    return ret;
}

QString QQmlPreviewFileEngine::fileName(QAbstractFileEngine::FileName file) const
{
    if (m_fallback)
        return m_fallback->fileName(file);

    if (file == BaseName) {
        int slashPos = m_name.lastIndexOf('/');
        if (slashPos == -1)
            return m_name;
        return m_name.mid(slashPos + 1);
    } else if (file == PathName || file == AbsolutePathName) {
        const QString path = (file == AbsolutePathName) ? m_absolute : m_name;
        const int slashPos = path.lastIndexOf('/');
        if (slashPos == -1)
            return QString();
        else if (slashPos == 0)
            return "/";
        return path.left(slashPos);
    } else if (file == CanonicalName || file == CanonicalPathName) {
        if (file == CanonicalPathName) {
            const int slashPos = m_absolute.lastIndexOf('/');
            if (slashPos != -1)
                return m_absolute.left(slashPos);
        }
        return m_absolute;
    }
    return m_name;
}

uint QQmlPreviewFileEngine::ownerId(QAbstractFileEngine::FileOwner owner) const
{
    return m_fallback ? m_fallback->ownerId(owner) : static_cast<uint>(-2);
}

QAbstractFileEngine::IteratorUniquePtr QQmlPreviewFileEngine::beginEntryList(
        const QString &path, QDirListing::IteratorFlags filters, const QStringList &filterNames)
{
    return m_fallback ? m_fallback->beginEntryList(path, filters, filterNames)
                      : std::make_unique<QQmlPreviewFileEngineIterator>(
                              path, filters, filterNames, m_entries);
}

QAbstractFileEngine::IteratorUniquePtr QQmlPreviewFileEngine::endEntryList()
{
    return m_fallback ? m_fallback->endEntryList() : nullptr;
}

bool QQmlPreviewFileEngine::flush()
{
    return m_fallback ? m_fallback->flush() : true;
}

bool QQmlPreviewFileEngine::syncToDisk()
{
    return m_fallback ? m_fallback->syncToDisk() : false;
}

bool QQmlPreviewFileEngine::isSequential() const
{
    return m_fallback ? m_fallback->isSequential() : m_contents.isSequential();
}

bool QQmlPreviewFileEngine::remove()
{
    return m_fallback ? m_fallback->remove() : false;
}

bool QQmlPreviewFileEngine::copy(const QString &newName)
{
    return m_fallback ? m_fallback->copy(newName) : false;
}

bool QQmlPreviewFileEngine::rename(const QString &newName)
{
    return m_fallback ? m_fallback->rename(newName) : false;
}

bool QQmlPreviewFileEngine::renameOverwrite(const QString &newName)
{
    return m_fallback ? m_fallback->renameOverwrite(newName) : false;
}

bool QQmlPreviewFileEngine::link(const QString &newName)
{
    return m_fallback ? m_fallback->link(newName) : false;
}

bool QQmlPreviewFileEngine::mkdir(const QString &dirName, bool createParentDirectories,
                                  std::optional<QFile::Permissions> permissions) const
{
    return m_fallback ? m_fallback->mkdir(dirName, createParentDirectories, permissions) : false;
}

bool QQmlPreviewFileEngine::rmdir(const QString &dirName, bool recurseParentDirectories) const
{
    return m_fallback ? m_fallback->rmdir(dirName, recurseParentDirectories) : false;
}

bool QQmlPreviewFileEngine::setSize(qint64 size)
{
    switch (m_result) {
    case QQmlPreviewFileLoader::Fallback:
        return m_fallback->setSize(size);
    case QQmlPreviewFileLoader::File:
        if (size < 0 || size > std::numeric_limits<int>::max())
            return false;
        m_contents.buffer().resize(static_cast<int>(size));
        return true;
    case QQmlPreviewFileLoader::Directory:
        return false;
    default:
        Q_UNREACHABLE_RETURN(false);
    }
}

bool QQmlPreviewFileEngine::caseSensitive() const
{
    return m_fallback ? m_fallback->caseSensitive() : true;
}

bool QQmlPreviewFileEngine::isRelativePath() const
{
    return m_fallback ? m_fallback->isRelativePath() : isRelative(m_name);
}

QStringList QQmlPreviewFileEngine::entryList(QDir::Filters filters,
                                             const QStringList &filterNames) const
{
    return m_fallback ? m_fallback->entryList(filters, filterNames)
                      : QAbstractFileEngine::entryList(filters, filterNames);
}

bool QQmlPreviewFileEngine::setPermissions(uint perms)
{
    return m_fallback ? m_fallback->setPermissions(perms) : false;
}

QByteArray QQmlPreviewFileEngine::id() const
{
    return m_fallback ? m_fallback->id() : QByteArray();
}

QString QQmlPreviewFileEngine::owner(FileOwner owner) const
{
    return m_fallback ? m_fallback->owner(owner) : QString();
}

QDateTime QQmlPreviewFileEngine::fileTime(QFile::FileTime time) const
{
    // Files we replace are always newer than the ones we had before. This makes the QML engine
    // actually recompile them, rather than pick them from the cache.
    return m_fallback ? m_fallback->fileTime(time) : QDateTime::currentDateTime();
}

int QQmlPreviewFileEngine::handle() const
{
    return m_fallback ? m_fallback->handle() : -1;
}

qint64 QQmlPreviewFileEngine::readLine(char *data, qint64 maxlen)
{
    return m_fallback ? m_fallback->readLine(data, maxlen) : m_contents.readLine(data, maxlen);
}

qint64 QQmlPreviewFileEngine::write(const char *data, qint64 len)
{
    return m_fallback ? m_fallback->write(data, len) : m_contents.write(data, len);
}

bool QQmlPreviewFileEngine::extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output)
{
    return m_fallback ? m_fallback->extension(extension, option, output) : false;
}

bool QQmlPreviewFileEngine::supportsExtension(Extension extension) const
{
    return m_fallback ? m_fallback->supportsExtension(extension) : false;
}

void QQmlPreviewFileEngine::load() const
{
    // We can get here from different threads on different instances of QQmlPreviewFileEngine.
    // However, there is only one loader per QQmlPreviewFileEngineHandler and it is not thread-safe.
    // Its content mutex doesn't help us here because we explicitly wait on it in load(), which
    // causes it to be released. Therefore, lock the load mutex first.
    // This doesn't cause any deadlocks because the only thread that wakes the loader on the content
    // mutex never calls load(). It's the QML debug server thread that handles the debug protocol.
    QMutexLocker loadLocker(m_loader->loadMutex());

    m_result = m_loader->load(m_absolute);
    switch (m_result) {
    case QQmlPreviewFileLoader::File:
        m_contents.setData(m_loader->contents());
        break;
    case QQmlPreviewFileLoader::Directory:
        m_entries = m_loader->entries();
        break;
    case QQmlPreviewFileLoader::Fallback:
        m_fallback = QAbstractFileEngine::create(m_name);
        break;
    case QQmlPreviewFileLoader::Unknown:
        Q_UNREACHABLE();
        break;
    }
}

QQmlPreviewFileEngineHandler::QQmlPreviewFileEngineHandler(QQmlPreviewFileLoader *loader)
    : m_loader(loader)
{
}

std::unique_ptr<QAbstractFileEngine> QQmlPreviewFileEngineHandler::create(
        const QString &fileName) const
{
    using namespace Qt::StringLiterals;
    static QList<QLatin1StringView> prohibitedSuffixes {
        // Don't load compiled QML/JS over the network
        ".qmlc"_L1, ".jsc"_L1, ".mjsc"_L1,

        // Don't load plugins over the network
        ".dll"_L1, ".so"_L1, ".dylib"_L1
    };

    for (QLatin1StringView suffix : prohibitedSuffixes) {
        if (fileName.endsWith(suffix))
            return nullptr;
    }

    if (isRootPath(fileName))
        return nullptr;

    QString relative = fileName;
    while (relative.endsWith('/'))
        relative.chop(1);

    if (relative.isEmpty() || relative == ":")
        return nullptr;

    const QString absolute = relative.startsWith(':') ? relative : absolutePath(relative);

    if (m_loader->isBlacklisted(absolute))
        return {};

    return std::make_unique<QQmlPreviewFileEngine>(relative, absolute, m_loader.data());
}

QT_END_NAMESPACE
