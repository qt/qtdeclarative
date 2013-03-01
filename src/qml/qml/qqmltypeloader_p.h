/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLTYPELOADER_P_H
#define QQMLTYPELOADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtCore/qatomic.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtQml/qqmlerror.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlfile.h>

#include <private/qv8_p.h>
#include <private/qhashedstring_p.h>
#include <private/qqmlscript_p.h>
#include <private/qqmlimport_p.h>
#include <private/qqmlcleanup_p.h>
#include <private/qqmldirparser_p.h>
#include <private/qqmlbundle_p.h>
#include <private/qflagpointer_p.h>
#include <private/qqmlabstracturlinterceptor_p.h>

QT_BEGIN_NAMESPACE

class QQmlScriptData;
class QQmlScriptBlob;
class QQmlQmldirData;
class QQmlTypeLoader;
class QQmlCompiledData;
class QQmlComponentPrivate;
class QQmlTypeData;
class QQmlDataLoader;
class QQmlExtensionInterface;

class Q_QML_PRIVATE_EXPORT QQmlDataBlob : public QQmlRefCount
{
public:
    enum Status {
        Null,                    // Prior to QQmlDataLoader::load()
        Loading,                 // Prior to data being received and dataReceived() being called
        WaitingForDependencies,  // While there are outstanding addDependency()s
        Complete,                // Finished
        Error                    // Error
    };

    enum Type { //Matched in QQmlAbstractUrlInterceptor
        QmlFile = QQmlAbstractUrlInterceptor::QmlFile,
        JavaScriptFile = QQmlAbstractUrlInterceptor::JavaScriptFile,
        QmldirFile = QQmlAbstractUrlInterceptor::QmldirFile
    };

    QQmlDataBlob(const QUrl &, Type);
    virtual ~QQmlDataBlob();

    void startLoading(QQmlDataLoader* manager);

    Type type() const;

    Status status() const;
    bool isNull() const;
    bool isLoading() const;
    bool isWaiting() const;
    bool isComplete() const;
    bool isError() const;
    bool isCompleteOrError() const;

    qreal progress() const;

    QUrl url() const;
    QUrl finalUrl() const;
    QString finalUrlString() const;

    QList<QQmlError> errors() const;

    class Data {
    public:
        inline const char *data() const;
        inline int size() const;

        inline QByteArray asByteArray() const;

        inline bool isFile() const;
        inline QQmlFile *asFile() const;

    private:
        friend class QQmlDataBlob;
        friend class QQmlDataLoader;
        inline Data();
        Data(const Data &);
        Data &operator=(const Data &);
        QBiPointer<const QByteArray, QQmlFile> d;
    };

protected:
    // Can be called from within callbacks
    void setError(const QQmlError &);
    void setError(const QList<QQmlError> &errors);
    void addDependency(QQmlDataBlob *);

    // Callbacks made in load thread
    virtual void dataReceived(const Data &) = 0;
    virtual void done();
    virtual void networkError(QNetworkReply::NetworkError);
    virtual void dependencyError(QQmlDataBlob *);
    virtual void dependencyComplete(QQmlDataBlob *);
    virtual void allDependenciesDone();

    // Callbacks made in main thread
    virtual void downloadProgressChanged(qreal);
    virtual void completed();
private:
    friend class QQmlDataLoader;
    friend class QQmlDataLoaderThread;

    void tryDone();
    void cancelAllWaitingFor();
    void notifyAllWaitingOnMe();
    void notifyComplete(QQmlDataBlob *);

    struct ThreadData {
        inline ThreadData();
        inline QQmlDataBlob::Status status() const;
        inline void setStatus(QQmlDataBlob::Status);
        inline bool isAsync() const;
        inline void setIsAsync(bool);
        inline quint8 progress() const;
        inline void setProgress(quint8);

    private:
        QAtomicInt _p;
    };
    ThreadData m_data;

    // m_errors should *always* be written before the status is set to Error.
    // We use the status change as a memory fence around m_errors so that locking
    // isn't required.  Once the status is set to Error (or Complete), m_errors 
    // cannot be changed.
    QList<QQmlError> m_errors;

    Type m_type;

    QUrl m_url;
    QUrl m_finalUrl;
    mutable QString m_finalUrlString;

    // List of QQmlDataBlob's that are waiting for me to complete.
    QList<QQmlDataBlob *> m_waitingOnMe;

    // List of QQmlDataBlob's that I am waiting for to complete.
    QList<QQmlDataBlob *> m_waitingFor;

    // Manager that is currently fetching data for me
    QQmlDataLoader *m_manager;
    int m_redirectCount:30;
    bool m_inCallback:1;
    bool m_isDone:1;
};

class QQmlDataLoaderThread;
class QQmlDataLoader
{
public:
    QQmlDataLoader(QQmlEngine *);
    ~QQmlDataLoader();

    void lock();
    void unlock();

    bool isConcurrent() const { return true; }

    enum Mode { PreferSynchronous, Asynchronous };

    void load(QQmlDataBlob *, Mode = PreferSynchronous);
    void loadWithStaticData(QQmlDataBlob *, const QByteArray &, Mode = PreferSynchronous);

    QQmlEngine *engine() const;
    void initializeEngine(QQmlExtensionInterface *, const char *);

protected:
    void shutdownThread();

private:
    friend class QQmlDataBlob;
    friend class QQmlDataLoaderThread;
    friend class QQmlDataLoaderNetworkReplyProxy;

    void loadThread(QQmlDataBlob *);
    void loadWithStaticDataThread(QQmlDataBlob *, const QByteArray &);
    void networkReplyFinished(QNetworkReply *);
    void networkReplyProgress(QNetworkReply *, qint64, qint64);
    
    typedef QHash<QNetworkReply *, QQmlDataBlob *> NetworkReplies;

    void setData(QQmlDataBlob *, const QByteArray &);
    void setData(QQmlDataBlob *, QQmlFile *);
    void setData(QQmlDataBlob *, const QQmlDataBlob::Data &);

    QQmlEngine *m_engine;
    QQmlDataLoaderThread *m_thread;
    NetworkReplies m_networkReplies;
};

class QQmlBundleData : public QQmlBundle,
                       public QQmlRefCount
{
public:
    QQmlBundleData(const QString &);
    QString fileName;
};

class QQmlTypeLoader : public QQmlDataLoader
{
    Q_DECLARE_TR_FUNCTIONS(QQmlTypeLoader)
public:
    class Q_QML_PRIVATE_EXPORT Blob : public QQmlDataBlob
    {
    public:
        Blob(const QUrl &url, QQmlDataBlob::Type type, QQmlTypeLoader *loader);
        ~Blob();

        QQmlTypeLoader *typeLoader() const { return m_typeLoader; }
        const QQmlImports &imports() const { return m_imports; }

    protected:
        bool addImport(const QQmlScript::Import &import, QList<QQmlError> *errors);

        bool fetchQmldir(const QUrl &url, const QQmlScript::Import *import, int priority, QList<QQmlError> *errors);
        bool updateQmldir(QQmlQmldirData *data, const QQmlScript::Import *import, QList<QQmlError> *errors);

    private:
        virtual bool qmldirDataAvailable(QQmlQmldirData *, QList<QQmlError> *);

        virtual void scriptImported(QQmlScriptBlob *, const QQmlScript::Location &, const QString &, const QString &) {}

        virtual void dependencyError(QQmlDataBlob *);
        virtual void dependencyComplete(QQmlDataBlob *);

    protected:
        QQmlTypeLoader *m_typeLoader;
        QQmlImports m_imports;
        QHash<const QQmlScript::Import *, int> m_unresolvedImports;
        QList<QQmlQmldirData *> m_qmldirs;
    };

    class QmldirContent
    {
    private:
        friend class QQmlTypeLoader;
        QmldirContent();

        void setContent(const QString &location, const QString &content);
        void setError(const QQmlError &);

    public:
        bool hasError() const;
        QList<QQmlError> errors(const QString &uri) const;

        QString typeNamespace() const;

        QQmlDirComponents components() const;
        QQmlDirScripts scripts() const;
        QQmlDirPlugins plugins() const;

        QString pluginLocation() const;

    private:
        QQmlDirParser m_parser;
        QString m_location;
    };

    QQmlTypeLoader(QQmlEngine *);
    ~QQmlTypeLoader();

    enum Option {
        None,
        PreserveParser
    };
    Q_DECLARE_FLAGS(Options, Option)

    QQmlImportDatabase *importDatabase();

    QQmlTypeData *getType(const QUrl &url, Mode mode = PreferSynchronous);
    QQmlTypeData *getType(const QByteArray &, const QUrl &url, Options = None);

    QQmlScriptBlob *getScript(const QUrl &);
    QQmlQmldirData *getQmldir(const QUrl &);

    QQmlBundleData *getBundle(const QString &);
    QQmlBundleData *getBundle(const QHashedStringRef &);
    void addBundle(const QString &, const QString &);

    QString absoluteFilePath(const QString &path);
    bool directoryExists(const QString &path);

    const QmldirContent *qmldirContent(const QString &filePath, const QString &uriHint);
    void setQmldirContent(const QString &filePath, const QString &content);

    void clearCache();
    void trimCache();

    bool isTypeLoaded(const QUrl &url) const;
    bool isScriptLoaded(const QUrl &url) const;

private:
    void addBundleNoLock(const QString &, const QString &);
    QString bundleIdForQmldir(const QString &qmldir, const QString &uriHint);

    template<typename T>
    struct TypedCallback
    {
        TypedCallback(T *object, void (T::*func)(QQmlTypeData *)) : o(object), mf(func) {}

        static void redirect(void *arg, QQmlTypeData *type)
        {
            TypedCallback<T> *self = reinterpret_cast<TypedCallback<T> *>(arg);
            ((self->o)->*(self->mf))(type);
        }

    private:
        T *o;
        void (T::*mf)(QQmlTypeData *);
    };

    typedef QHash<QUrl, QQmlTypeData *> TypeCache;
    typedef QHash<QUrl, QQmlScriptBlob *> ScriptCache;
    typedef QHash<QUrl, QQmlQmldirData *> QmldirCache;
    typedef QStringHash<bool> StringSet;
    typedef QStringHash<StringSet*> ImportDirCache;
    typedef QStringHash<QmldirContent *> ImportQmlDirCache;
    typedef QStringHash<QQmlBundleData *> BundleCache;
    typedef QStringHash<QString> QmldirBundleIdCache;

    TypeCache m_typeCache;
    ScriptCache m_scriptCache;
    QmldirCache m_qmldirCache;
    ImportDirCache m_importDirCache;
    ImportQmlDirCache m_importQmlDirCache;
    BundleCache m_bundleCache;
    QmldirBundleIdCache m_qmldirBundleIdCache;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlTypeLoader::Options)

class Q_AUTOTEST_EXPORT QQmlTypeData : public QQmlTypeLoader::Blob
{
public:
    struct TypeReference
    {
        TypeReference() : type(0), majorVersion(0), minorVersion(0), typeData(0) {}

        QQmlScript::Location location;
        QQmlType *type;
        int majorVersion;
        int minorVersion;
        QQmlTypeData *typeData;
    };

    struct ScriptReference
    {
        ScriptReference() : script(0) {}

        QQmlScript::Location location;
        QString qualifier;
        QQmlScriptBlob *script;
    };

private:
    friend class QQmlTypeLoader;

    QQmlTypeData(const QUrl &, QQmlTypeLoader::Options, QQmlTypeLoader *);

public:
    ~QQmlTypeData();

    const QQmlScript::Parser &parser() const;

    const QList<TypeReference> &resolvedTypes() const;
    const QList<ScriptReference> &resolvedScripts() const;
    const QSet<QString> &namespaces() const;

    QQmlCompiledData *compiledData() const;

    // Used by QQmlComponent to get notifications
    struct TypeDataCallback {
        virtual ~TypeDataCallback();
        virtual void typeDataProgress(QQmlTypeData *, qreal) {}
        virtual void typeDataReady(QQmlTypeData *) {}
    };
    void registerCallback(TypeDataCallback *);
    void unregisterCallback(TypeDataCallback *);

protected:
    virtual void done();
    virtual void completed();
    virtual void dataReceived(const Data &);
    virtual void allDependenciesDone();
    virtual void downloadProgressChanged(qreal);

private:
    void resolveTypes();
    void compile();

    virtual void scriptImported(QQmlScriptBlob *blob, const QQmlScript::Location &location, const QString &qualifier, const QString &nameSpace);

    QQmlTypeLoader::Options m_options;

    QQmlScript::Parser scriptParser;

    QList<ScriptReference> m_scripts;

    QSet<QString> m_namespaces;

    QList<TypeReference> m_types;
    bool m_typesResolved:1;

    QQmlCompiledData *m_compiledData;

    QList<TypeDataCallback *> m_callbacks;

    QQmlScript::Import *m_implicitImport;
    bool m_implicitImportLoaded;
    bool loadImplicitImport();
};

// QQmlScriptData instances are created, uninitialized, by the loader in the 
// load thread.  The first time they are used by the VME, they are initialized which
// creates their v8 objects and they are referenced and added to the  engine's cleanup
// list.  During QQmlCleanup::clear() all v8 resources are destroyed, and the 
// reference that was created is released but final deletion only occurs once all the
// references as released.  This is all intended to ensure that the v8 resources are
// only created and destroyed in the main thread :)
class Q_AUTOTEST_EXPORT QQmlScriptData : public QQmlCleanup, public QQmlRefCount
{
private:
    friend class QQmlTypeLoader;

    QQmlScriptData();

public:
    ~QQmlScriptData();

    QUrl url;
    QString urlString;
    QQmlTypeNameCache *importCache;
    QList<QQmlScriptBlob *> scripts;
    QQmlScript::Object::ScriptBlock::Pragmas pragmas;

    bool isInitialized() const { return hasEngine(); }
    void initialize(QQmlEngine *);

    bool hasError() const { return m_error.isValid(); }
    void setError(const QQmlError &error) { m_error = error; }
    QQmlError error() const { return m_error; }

protected:
    virtual void clear(); // From QQmlCleanup

private:
    friend class QQmlVME;
    friend class QQmlScriptBlob;

    bool m_loaded;
    QByteArray m_programSource;
    v8::Persistent<v8::Script> m_program;
    v8::Persistent<v8::Object> m_value;
    QQmlError m_error;
};

class Q_AUTOTEST_EXPORT QQmlScriptBlob : public QQmlTypeLoader::Blob
{
private:
    friend class QQmlTypeLoader;

    QQmlScriptBlob(const QUrl &, QQmlTypeLoader *);

public:
    ~QQmlScriptBlob();

    struct ScriptReference
    {
        ScriptReference() : script(0) {}

        QQmlScript::Location location;
        QString qualifier;
        QString nameSpace;
        QQmlScriptBlob *script;
    };

    QQmlScript::Object::ScriptBlock::Pragmas pragmas() const;

    QQmlScriptData *scriptData() const;

protected:
    virtual void dataReceived(const Data &);
    virtual void done();

private:
    virtual void scriptImported(QQmlScriptBlob *blob, const QQmlScript::Location &location, const QString &qualifier, const QString &nameSpace);

    QString m_source;
    QQmlScript::Parser::JavaScriptMetaData m_metadata;

    QList<ScriptReference> m_scripts;
    QQmlScriptData *m_scriptData;
};

class Q_AUTOTEST_EXPORT QQmlQmldirData : public QQmlTypeLoader::Blob
{
private:
    friend class QQmlTypeLoader;

    QQmlQmldirData(const QUrl &, QQmlTypeLoader *);

public:
    const QString &content() const;

    const QQmlScript::Import *import() const;
    void setImport(const QQmlScript::Import *);

    int priority() const;
    void setPriority(int);

protected:
    virtual void dataReceived(const Data &);

private:
    QString m_content;
    const QQmlScript::Import *m_import;
    int m_priority;
};

QQmlDataBlob::Data::Data()
{
}

const char *QQmlDataBlob::Data::data() const
{
    Q_ASSERT(!d.isNull());

    if (d.isT1()) return d.asT1()->constData();
    else return d.asT2()->data();
}

int QQmlDataBlob::Data::size() const
{
    Q_ASSERT(!d.isNull());

    if (d.isT1()) return d.asT1()->size();
    else return d.asT2()->size();
}

bool QQmlDataBlob::Data::isFile() const
{
    return d.isT2();
}

QByteArray QQmlDataBlob::Data::asByteArray() const
{
    Q_ASSERT(!d.isNull());

    if (d.isT1()) return *d.asT1();
    else return d.asT2()->dataByteArray();
}

QQmlFile *QQmlDataBlob::Data::asFile() const
{
    if (d.isT2()) return d.asT2();
    else return 0;
}

QT_END_NAMESPACE

#endif // QQMLTYPELOADER_P_H
