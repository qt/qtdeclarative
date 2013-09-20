/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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
#include <QStringList>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <private/qqmlengine_p.h>
#include <QDebug>
#include <private/qv8engine_p.h>
#include <QtSql/qsqldatabase.h>
#include <QtSql/qsqlquery.h>
#include <QtSql/qsqlerror.h>
#include <QtSql/qsqlrecord.h>
#include <QtSql/qsqlfield.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qstack.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qsettings.h>
#include <QtCore/qdir.h>
#include <private/qv4sqlerrors_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4object_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4objectproto_p.h>
#include <private/qv4exception_p.h>
#include <private/qv4scopedvalue_p.h>

using namespace QV4;

#define V4THROW_SQL(error, desc) { \
    QV4::Scoped<String> v(scope, Value::fromString(ctx, desc)); \
    QV4::Scoped<Object> ex(scope, ctx->engine->newErrorObject(v.asValue())); \
    ex->put(ctx->engine->newIdentifier(QStringLiteral("code")), Value::fromInt32(error)); \
    ctx->throwError(ex); \
}

#define V4THROW_REFERENCE(string) { \
    QV4::Scoped<String> v(scope, Value::fromString(ctx, string)); \
    ctx->throwReferenceError(v); \
}


class QQmlSqlDatabaseData : public QV8Engine::Deletable
{
public:
    QQmlSqlDatabaseData(QV8Engine *engine);
    ~QQmlSqlDatabaseData();

    PersistentValue databaseProto;
    PersistentValue queryProto;
    PersistentValue rowsProto;
};

V8_DEFINE_EXTENSION(QQmlSqlDatabaseData, databaseData)

class QQmlSqlDatabaseWrapper : public Object
{
    Q_MANAGED

public:
    enum Type { Database, Query, Rows };
    QQmlSqlDatabaseWrapper(QV8Engine *e)
        : Object(QV8Engine::getV4(e)), type(Database), inTransaction(false), readonly(false), forwardOnly(false)
    {
        vtbl = &static_vtbl;
    }

    ~QQmlSqlDatabaseWrapper() {
    }

    static ReturnedValue getIndexed(Managed *m, uint index, bool *hasProperty);
    static void destroy(Managed *that) {
        static_cast<QQmlSqlDatabaseWrapper *>(that)->~QQmlSqlDatabaseWrapper();
    }

    Type type;
    QSqlDatabase database;

    QString version; // type == Database

    bool inTransaction; // type == Query
    bool readonly;   // type == Query

    QSqlQuery sqlQuery; // type == Rows
    bool forwardOnly; // type == Rows
};

DEFINE_MANAGED_VTABLE(QQmlSqlDatabaseWrapper);

static ReturnedValue qmlsqldatabase_version(SimpleCallContext *ctx)
{
    QV4::Scope scope(ctx);

    QQmlSqlDatabaseWrapper *r = ctx->thisObject.as<QQmlSqlDatabaseWrapper>();
    if (!r || r->type != QQmlSqlDatabaseWrapper::Database)
        V4THROW_REFERENCE("Not a SQLDatabase object");

    return Value::fromString(ctx->engine->newString(r->version)).asReturnedValue();
}

static ReturnedValue qmlsqldatabase_rows_length(SimpleCallContext *ctx)
{
    QV4::Scope scope(ctx);

    QQmlSqlDatabaseWrapper *r = ctx->thisObject.as<QQmlSqlDatabaseWrapper>();
    if (!r || r->type != QQmlSqlDatabaseWrapper::Rows)
        V4THROW_REFERENCE("Not a SQLDatabase::Rows object");

    int s = r->sqlQuery.size();
    if (s < 0) {
        // Inefficient
        if (r->sqlQuery.last()) {
            s = r->sqlQuery.at() + 1;
        } else {
            s = 0;
        }
    }
    return Encode(s);
}

static ReturnedValue qmlsqldatabase_rows_forwardOnly(SimpleCallContext *ctx)
{
    QV4::Scope scope(ctx);

    QQmlSqlDatabaseWrapper *r = ctx->thisObject.as<QQmlSqlDatabaseWrapper>();
    if (!r || r->type != QQmlSqlDatabaseWrapper::Rows)
        V4THROW_REFERENCE("Not a SQLDatabase::Rows object");
    return Encode(r->sqlQuery.isForwardOnly());
}

static ReturnedValue qmlsqldatabase_rows_setForwardOnly(SimpleCallContext *ctx)
{
    QV4::Scope scope(ctx);

    QQmlSqlDatabaseWrapper *r = ctx->thisObject.as<QQmlSqlDatabaseWrapper>();
    if (!r || r->type != QQmlSqlDatabaseWrapper::Rows)
        V4THROW_REFERENCE("Not a SQLDatabase::Rows object");
    if (ctx->argumentCount < 1)
        ctx->throwTypeError();

    r->sqlQuery.setForwardOnly(ctx->arguments[0].toBoolean());
    return Encode::undefined();
}

QQmlSqlDatabaseData::~QQmlSqlDatabaseData()
{
}

static QString qmlsqldatabase_databasesPath(QV8Engine *engine)
{
    return engine->engine()->offlineStoragePath() +
           QDir::separator() + QLatin1String("Databases");
}

static void qmlsqldatabase_initDatabasesPath(QV8Engine *engine)
{
    QDir().mkpath(qmlsqldatabase_databasesPath(engine));
}

static QString qmlsqldatabase_databaseFile(const QString& connectionName, QV8Engine *engine)
{
    return qmlsqldatabase_databasesPath(engine) + QDir::separator() + connectionName;
}

static ReturnedValue qmlsqldatabase_rows_index(QQmlSqlDatabaseWrapper *r, ExecutionEngine *v4, quint32 index, bool *hasProperty = 0)
{
    QV8Engine *v8 = v4->v8Engine;

    if (r->sqlQuery.at() == (int)index || r->sqlQuery.seek(index)) {
        QSqlRecord record = r->sqlQuery.record();
        // XXX optimize
        Object *row = v4->newObject();
        for (int ii = 0; ii < record.count(); ++ii) {
            QVariant v = record.value(ii);
            if (v.isNull()) {
                row->put(v4->newIdentifier(record.fieldName(ii)), Value::nullValue());
            } else {
                row->put(v4->newIdentifier(record.fieldName(ii)), Value::fromReturnedValue(v8->fromVariant(v)));
            }
        }
        if (hasProperty)
            *hasProperty = true;
        return Value::fromObject(row).asReturnedValue();
    } else {
        if (hasProperty)
            *hasProperty = false;
        return Value::undefinedValue().asReturnedValue();
    }
}

ReturnedValue QQmlSqlDatabaseWrapper::getIndexed(Managed *m, uint index, bool *hasProperty)
{
    QQmlSqlDatabaseWrapper *r = m->as<QQmlSqlDatabaseWrapper>();
    if (!r || r->type != QQmlSqlDatabaseWrapper::Rows)
        return Object::getIndexed(m, index, hasProperty);

    return qmlsqldatabase_rows_index(r, m->engine(), index, hasProperty);
}

static ReturnedValue qmlsqldatabase_rows_item(SimpleCallContext *ctx)
{
    QV4::Scope scope(ctx);
    QQmlSqlDatabaseWrapper *r = ctx->thisObject.as<QQmlSqlDatabaseWrapper>();
    if (!r || r->type != QQmlSqlDatabaseWrapper::Rows)
        V4THROW_REFERENCE("Not a SQLDatabase::Rows object");

    return qmlsqldatabase_rows_index(r, ctx->engine, ctx->argumentCount ? ctx->arguments[0].toUInt32() : 0);
}

static ReturnedValue qmlsqldatabase_executeSql(SimpleCallContext *ctx)
{
    QV4::Scope scope(ctx);
    QQmlSqlDatabaseWrapper *r = ctx->thisObject.as<QQmlSqlDatabaseWrapper>();
    if (!r || r->type != QQmlSqlDatabaseWrapper::Query)
        V4THROW_REFERENCE("Not a SQLDatabase::Query object");

    QV8Engine *engine = ctx->engine->v8Engine;

    if (!r->inTransaction)
        V4THROW_SQL(SQLEXCEPTION_DATABASE_ERR,QQmlEngine::tr("executeSql called outside transaction()"));

    QSqlDatabase db = r->database;

    QString sql = ctx->argument(0).toQStringNoThrow();

    if (r->readonly && !sql.startsWith(QLatin1String("SELECT"),Qt::CaseInsensitive)) {
        V4THROW_SQL(SQLEXCEPTION_SYNTAX_ERR, QQmlEngine::tr("Read-only Transaction"));
    }

    QSqlQuery query(db);
    bool err = false;

    ScopedValue result(scope, Value::undefinedValue());

    if (query.prepare(sql)) {
        if (ctx->argumentCount > 1) {
            Value values = ctx->arguments[1];
            if (ArrayObject *array = values.asArrayObject()) {
                quint32 size = array->arrayLength();
                for (quint32 ii = 0; ii < size; ++ii)
                    query.bindValue(ii, engine->toVariant(QV4::Value::fromReturnedValue(array->getIndexed(ii)), -1));
            } else if (Object *object = values.asObject()) {
                ObjectIterator it(object, ObjectIterator::WithProtoChain|ObjectIterator::EnumerableOnly);
                ScopedValue key(scope);
                while (1) {
                    Value value;
                    key = it.nextPropertyName(&value);
                    if (key->isNull())
                        break;
                    QVariant v = engine->toVariant(value, -1);
                    if (key->isString()) {
                        query.bindValue(key->stringValue()->toQString(), v);
                    } else {
                        assert(key->isInteger());
                        query.bindValue(key->integerValue(), v);
                    }
                }
            } else {
                query.bindValue(0, engine->toVariant(values, -1));
            }
        }
        if (query.exec()) {
            QQmlSqlDatabaseWrapper *rows = new (ctx->engine->memoryManager) QQmlSqlDatabaseWrapper(engine);
            rows->setPrototype(databaseData(engine)->rowsProto.value().asObject());
            rows->type = QQmlSqlDatabaseWrapper::Rows;
            rows->database = db;
            rows->sqlQuery = query;

            Object *resultObject = ctx->engine->newObject();
            result = Value::fromObject(resultObject);
            // XXX optimize
            resultObject->put(ctx->engine->newIdentifier("rowsAffected"), Value::fromInt32(query.numRowsAffected()));
            resultObject->put(ctx->engine->newIdentifier("insertId"), engine->toString(query.lastInsertId().toString()));
            resultObject->put(ctx->engine->newIdentifier("rows"), Value::fromObject(rows));
        } else {
            err = true;
        }
    } else {
        err = true;
    }
    if (err)
        V4THROW_SQL(SQLEXCEPTION_DATABASE_ERR,query.lastError().text());

    return result.asReturnedValue();
}

static ReturnedValue qmlsqldatabase_changeVersion(SimpleCallContext *ctx)
{
    if (ctx->argumentCount < 2)
        return Encode::undefined();

    Scope scope(ctx);

    QQmlSqlDatabaseWrapper *r = ctx->thisObject.as<QQmlSqlDatabaseWrapper>();
    if (!r || r->type != QQmlSqlDatabaseWrapper::Database)
        V4THROW_REFERENCE("Not a SQLDatabase object");

    QV8Engine *engine = ctx->engine->v8Engine;

    QSqlDatabase db = r->database;
    QString from_version = ctx->arguments[0].toQStringNoThrow();
    QString to_version = ctx->arguments[1].toQStringNoThrow();
    Value callback = ctx->argument(2);

    if (from_version != r->version)
        V4THROW_SQL(SQLEXCEPTION_VERSION_ERR, QQmlEngine::tr("Version mismatch: expected %1, found %2").arg(from_version).arg(r->version));

    QQmlSqlDatabaseWrapper *w = new (ctx->engine->memoryManager) QQmlSqlDatabaseWrapper(engine);
    w->setPrototype(databaseData(engine)->queryProto.value().asObject());
    w->type = QQmlSqlDatabaseWrapper::Query;
    w->database = db;
    w->version = r->version;
    w->inTransaction = true;

    bool ok = true;
    if (FunctionObject *f = callback.asFunctionObject()) {
        ok = false;
        db.transaction();

        ScopedCallData callData(scope, 1);
        callData->thisObject = engine->global();
        callData->args[0] = Value::fromObject(w);
        try {
            f->call(callData);
        } catch (Exception &) {
            db.rollback();
            throw;
        }
        if (!db.commit()) {
            db.rollback();
            V4THROW_SQL(SQLEXCEPTION_UNKNOWN_ERR,QQmlEngine::tr("SQL transaction failed"));
        } else {
            ok = true;
        }
    }

    w->inTransaction = false;

    if (ok) {
        w->version = to_version;
#ifndef QT_NO_SETTINGS
        QSettings ini(qmlsqldatabase_databaseFile(db.connectionName(),engine) + QLatin1String(".ini"), QSettings::IniFormat);
        ini.setValue(QLatin1String("Version"), to_version);
#endif
    }

    return Encode::undefined();
}

static ReturnedValue qmlsqldatabase_transaction_shared(SimpleCallContext *ctx, bool readOnly)
{
    QV4::Scope scope(ctx);
    QQmlSqlDatabaseWrapper *r = ctx->thisObject.as<QQmlSqlDatabaseWrapper>();
    if (!r || r->type != QQmlSqlDatabaseWrapper::Database)
        V4THROW_REFERENCE("Not a SQLDatabase object");

    QV8Engine *engine = ctx->engine->v8Engine;

    FunctionObject *callback = ctx->argumentCount ? ctx->arguments[0].asFunctionObject() : 0;
    if (!callback)
        V4THROW_SQL(SQLEXCEPTION_UNKNOWN_ERR, QQmlEngine::tr("transaction: missing callback"));

    QSqlDatabase db = r->database;

    QQmlSqlDatabaseWrapper *w = new (ctx->engine->memoryManager) QQmlSqlDatabaseWrapper(engine);
    w->setPrototype(databaseData(engine)->queryProto.value().asObject());
    w->type = QQmlSqlDatabaseWrapper::Query;
    w->database = db;
    w->version = r->version;
    w->readonly = readOnly;
    w->inTransaction = true;

    db.transaction();
    if (callback) {
        ScopedCallData callData(scope, 1);
        callData->thisObject = engine->global();
        callData->args[0] = Value::fromObject(w);
        try {
            callback->call(callData);
        } catch (Exception &) {
            w->inTransaction = false;
            db.rollback();
            throw;
        }

        w->inTransaction = false;

        if (!db.commit())
            db.rollback();
    }

    return Encode::undefined();
}

static ReturnedValue qmlsqldatabase_transaction(SimpleCallContext *ctx)
{
    return qmlsqldatabase_transaction_shared(ctx, false);
}

static ReturnedValue qmlsqldatabase_read_transaction(SimpleCallContext *ctx)
{
    return qmlsqldatabase_transaction_shared(ctx, true);
}

QQmlSqlDatabaseData::QQmlSqlDatabaseData(QV8Engine *engine)
{
    ExecutionEngine *v4 = QV8Engine::getV4(engine);
    {
        Object *proto = v4->newObject();
        proto->defineDefaultProperty(v4, QStringLiteral("transaction"), qmlsqldatabase_transaction);
        proto->defineDefaultProperty(v4, QStringLiteral("readTransaction"), qmlsqldatabase_read_transaction);
        Property *p = proto->insertMember(v4->newString(QStringLiteral("version")),
                                          Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
        p->setGetter(v4->newBuiltinFunction(v4->rootContext, v4->newString(QStringLiteral("version")), qmlsqldatabase_version));
        proto->defineDefaultProperty(v4, QStringLiteral("changeVersion"), qmlsqldatabase_changeVersion);
        databaseProto = Value::fromObject(proto);
    }

    {
        Object *proto = v4->newObject();
        proto->defineDefaultProperty(v4, QStringLiteral("executeSql"), qmlsqldatabase_executeSql);
        queryProto = Value::fromObject(proto);
    }
    {
        Object *proto = v4->newObject();
        proto->defineDefaultProperty(v4, QStringLiteral("item"), qmlsqldatabase_rows_item);
        Property *p = proto->insertMember(v4->newString(QStringLiteral("length")),
                                          Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
        p->setGetter(v4->newBuiltinFunction(v4->rootContext, v4->newString(QStringLiteral("length")), qmlsqldatabase_rows_length));
        p = proto->insertMember(v4->newString(QStringLiteral("forwardOnly")),
                                Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
        p->setGetter(v4->newBuiltinFunction(v4->rootContext, v4->newString(QStringLiteral("forwardOnly")), qmlsqldatabase_rows_forwardOnly));
        p->setSetter(v4->newBuiltinFunction(v4->rootContext, v4->newString(QStringLiteral("setForwardOnly")), qmlsqldatabase_rows_setForwardOnly));
        rowsProto = Value::fromObject(proto);
    }
}

/*
HTML5 "spec" says "rs.rows[n]", but WebKit only impelments "rs.rows.item(n)". We do both (and property iterator).
We add a "forwardOnly" property that stops Qt caching results (code promises to only go forward
through the data.
*/


/*!
    \qmlmodule QtQuick.LocalStorage 2
    \title Qt Quick Local Storage QML Types
    \ingroup qmlmodules
    \brief Provides a JavaScript object singleton type for accessing a local
    SQLite database

    This is a singleton type for reading and writing to SQLite databases.


    \section1 Methods

    \list
    \li object \b{\l{#openDatabaseSync}{openDatabaseSync}}(string name, string version, string description, int estimated_size, jsobject callback(db))
    \endlist


    \section1 Detailed Description

    To use the types in this module, import the module and call the
    relevant functions using the \c LocalStorage type:

    \code
    import QtQuick.LocalStorage 2.0
    import QtQuick 2.0

    Item {
        Component.onCompleted: {
            var db = LocalStorage.openDatabaseSync(...)
        }
    }
    \endcode


These databases are user-specific and QML-specific, but accessible to all QML applications.
They are stored in the \c Databases subdirectory
of QQmlEngine::offlineStoragePath(), currently as SQLite databases.

Database connections are automatically closed during Javascript garbage collection.

The API can be used from JavaScript functions in your QML:

\snippet localstorage/localstorage/hello.qml 0

The API conforms to the Synchronous API of the HTML5 Web Database API,
\link http://www.w3.org/TR/2009/WD-webdatabase-20091029/ W3C Working Draft 29 October 2009\endlink.

The \l{localstorage/localstorage}{SQL Local Storage example} demonstrates the basics of
using the Offline Storage API.

\section3 Open or create a databaseData
\code
import QtQuick.LocalStorage 2.0 as Sql

db = Sql.openDatabaseSync(identifier, version, description, estimated_size, callback(db))
\endcode
The above code returns the database identified by \e identifier. If the database does not already exist, it
is created, and the function \e callback is called with the database as a parameter. \e description
and \e estimated_size are written to the INI file (described below), but are otherwise currently
unused.

May throw exception with code property SQLException.DATABASE_ERR, or SQLException.VERSION_ERR.

When a database is first created, an INI file is also created specifying its characteristics:

\table
\header \li \b {Key} \li \b {Value}
\row \li Name \li The name of the database passed to \c openDatabase()
\row \li Version \li The version of the database passed to \c openDatabase()
\row \li Description \li The description of the database passed to \c openDatabase()
\row \li EstimatedSize \li The estimated size (in bytes) of the database passed to \c openDatabase()
\row \li Driver \li Currently "QSQLITE"
\endtable

This data can be used by application tools.

\section3 db.changeVersion(from, to, callback(tx))

This method allows you to perform a \e{Scheme Upgrade}.

If the current version of \e db is not \e from, then an exception is thrown.

Otherwise, a database transaction is created and passed to \e callback. In this function,
you can call \e executeSql on \e tx to upgrade the database.

May throw exception with code property SQLException.DATABASE_ERR or SQLException.UNKNOWN_ERR.

\section3 db.transaction(callback(tx))

This method creates a read/write transaction and passed to \e callback. In this function,
you can call \e executeSql on \e tx to read and modify the database.

If the callback throws exceptions, the transaction is rolled back.

\section3 db.readTransaction(callback(tx))

This method creates a read-only transaction and passed to \e callback. In this function,
you can call \e executeSql on \e tx to read the database (with SELECT statements).

\section3 results = tx.executeSql(statement, values)

This method executes a SQL \e statement, binding the list of \e values to SQL positional parameters ("?").

It returns a results object, with the following properties:

\table
\header \li \b {Type} \li \b {Property} \li \b {Value} \li \b {Applicability}
\row \li int \li rows.length \li The number of rows in the result \li SELECT
\row \li var \li rows.item(i) \li Function that returns row \e i of the result \li SELECT
\row \li int \li rowsAffected \li The number of rows affected by a modification \li UPDATE, DELETE
\row \li string \li insertId \li The id of the row inserted \li INSERT
\endtable

May throw exception with code property SQLException.DATABASE_ERR, SQLException.SYNTAX_ERR, or SQLException.UNKNOWN_ERR.


\section1 Method Documentation

\target openDatabaseSync
\code
object openDatabaseSync(string name, string version, string description, int estimated_size, jsobject callback(db))
\endcode

Opens or creates a local storage sql database by the given parameters.

\list
\li \c name is the database name
\li \c version is the database version
\li \c description is the database display name
\li \c estimated_size is the database's estimated size, in bytes
\li \c callback is an optional parameter, which is invoked if the database has not yet been created.
\endlist

Returns the created database object.

*/
class QQuickLocalStorage : public QObject
{
    Q_OBJECT
public:
    QQuickLocalStorage(QObject *parent=0) : QObject(parent)
    {
    }
    ~QQuickLocalStorage() {
    }

   Q_INVOKABLE void openDatabaseSync(QQmlV4Function* args);
};

void QQuickLocalStorage::openDatabaseSync(QQmlV4Function *args)
{
#ifndef QT_NO_SETTINGS
    QV8Engine *engine = args->engine();
    ExecutionContext *ctx = QV8Engine::getV4(engine)->current;
    QV4::Scope scope(ctx);
    if (engine->engine()->offlineStoragePath().isEmpty())
        V4THROW_SQL(SQLEXCEPTION_DATABASE_ERR, QQmlEngine::tr("SQL: can't create database, offline storage is disabled."));

    qmlsqldatabase_initDatabasesPath(engine);

    QSqlDatabase database;

    QString dbname = (*args)[0].toQStringNoThrow();
    QString dbversion = (*args)[1].toQStringNoThrow();
    QString dbdescription = (*args)[2].toQStringNoThrow();
    int dbestimatedsize = (*args)[3].toInt32();
    FunctionObject *dbcreationCallback = (*args)[4].asFunctionObject();

    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(dbname.toUtf8());
    QString dbid(QLatin1String(md5.result().toHex()));

    QString basename = qmlsqldatabase_databaseFile(dbid, engine);
    bool created = false;
    QString version = dbversion;

    {
        QSettings ini(basename+QLatin1String(".ini"),QSettings::IniFormat);

        if (QSqlDatabase::connectionNames().contains(dbid)) {
            database = QSqlDatabase::database(dbid);
            version = ini.value(QLatin1String("Version")).toString();
            if (version != dbversion && !dbversion.isEmpty() && !version.isEmpty())
                V4THROW_SQL(SQLEXCEPTION_VERSION_ERR, QQmlEngine::tr("SQL: database version mismatch"));
        } else {
            created = !QFile::exists(basename+QLatin1String(".sqlite"));
            database = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), dbid);
            if (created) {
                ini.setValue(QLatin1String("Name"), dbname);
                if (dbcreationCallback)
                    version = QString();
                ini.setValue(QLatin1String("Version"), version);
                ini.setValue(QLatin1String("Description"), dbdescription);
                ini.setValue(QLatin1String("EstimatedSize"), dbestimatedsize);
                ini.setValue(QLatin1String("Driver"), QLatin1String("QSQLITE"));
            } else {
                if (!dbversion.isEmpty() && ini.value(QLatin1String("Version")) != dbversion) {
                    // Incompatible
                    V4THROW_SQL(SQLEXCEPTION_VERSION_ERR,QQmlEngine::tr("SQL: database version mismatch"));
                }
                version = ini.value(QLatin1String("Version")).toString();
            }
            database.setDatabaseName(basename+QLatin1String(".sqlite"));
        }
        if (!database.isOpen())
            database.open();
    }

    QQmlSqlDatabaseWrapper *db = new (ctx->engine->memoryManager) QQmlSqlDatabaseWrapper(engine);
    db->setPrototype(databaseData(engine)->databaseProto.value().asObject());
    db->database = database;
    db->version = version;

    if (created && dbcreationCallback) {
        Scope scope(ctx);
        ScopedCallData callData(scope, 1);
        callData->thisObject = engine->global();
        callData->args[0] = Value::fromObject(db);
        dbcreationCallback->call(callData);
    }

    args->setReturnValue(Value::fromObject(db));
#endif // QT_NO_SETTINGS
}

static QObject *module_api_factory(QQmlEngine *engine, QJSEngine *scriptEngine)
{
   Q_UNUSED(engine)
   Q_UNUSED(scriptEngine)
   QQuickLocalStorage *api = new QQuickLocalStorage();

   return api;
}

class QQmlLocalStoragePlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    QQmlLocalStoragePlugin()
    {
    }

    void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == "QtQuick.LocalStorage");
        qmlRegisterSingletonType<QQuickLocalStorage>(uri, 2, 0, "LocalStorage", module_api_factory);
    }
};

#include "plugin.moc"
