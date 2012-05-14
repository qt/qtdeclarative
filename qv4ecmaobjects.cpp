
#include "qv4ecmaobjects_p.h"
#include <qmath.h>
#include <qnumeric.h>
#include <cassert>

using namespace QQmlJS::VM;

//
// Object
//
ObjectCtor::ObjectCtor(Context *scope)
    : FunctionObject(scope)
{
}

void ObjectCtor::construct(Context *ctx)
{
    __qmljs_init_object(ctx, &ctx->thisObject, new Object());
}

void ObjectCtor::call(Context *)
{
    assert(!"not here");
}


ObjectPrototype::ObjectPrototype(Context *ctx, FunctionObject *ctor)
{
    setProperty(ctx, QLatin1String("constructor"), Value::object(ctx, ctor));
}

//
// String
//
StringCtor::StringCtor(Context *scope)
    : FunctionObject(scope)
{
}

void StringCtor::construct(Context *ctx)
{
    Value arg = ctx->argument(0);
    __qmljs_to_string(ctx, &arg, &arg);
    __qmljs_init_object(ctx, &ctx->thisObject, new StringObject(arg));
}

void StringCtor::call(Context *ctx)
{
    const Value arg = ctx->argument(0);
    if (arg.is(UNDEFINED_TYPE))
        __qmljs_init_string(ctx, &ctx->result, String::get(ctx, QString()));
    else
        __qmljs_to_string(ctx, &ctx->result, &arg);
}

StringPrototype::StringPrototype(Context *ctx, FunctionObject *ctor)
{
    setProperty(ctx, QLatin1String("constructor"), Value::object(ctx, ctor));
    setProperty(ctx, QLatin1String("toString"), method_toString);
    setProperty(ctx, QLatin1String("valueOf"), method_valueOf);
    setProperty(ctx, QLatin1String("charAt"), method_charAt);
    setProperty(ctx, QLatin1String("charCodeAt"), method_charCodeAt);
    setProperty(ctx, QLatin1String("concat"), method_concat);
    setProperty(ctx, QLatin1String("indexOf"), method_indexOf);
    setProperty(ctx, QLatin1String("lastIndexOf"), method_lastIndexOf);
    setProperty(ctx, QLatin1String("localeCompare"), method_localeCompare);
    setProperty(ctx, QLatin1String("match"), method_match);
    setProperty(ctx, QLatin1String("replace"), method_replace);
    setProperty(ctx, QLatin1String("search"), method_search);
    setProperty(ctx, QLatin1String("slice"), method_slice);
    setProperty(ctx, QLatin1String("split"), method_split);
    setProperty(ctx, QLatin1String("substr"), method_substr);
    setProperty(ctx, QLatin1String("substring"), method_substring);
    setProperty(ctx, QLatin1String("toLowerCase"), method_toLowerCase);
    setProperty(ctx, QLatin1String("toLocaleLowerCase"), method_toLocaleLowerCase);
    setProperty(ctx, QLatin1String("toUpperCase"), method_toUpperCase);
    setProperty(ctx, QLatin1String("toLocaleUpperCase"), method_toLocaleUpperCase);
    setProperty(ctx, QLatin1String("fromCharCode"), method_fromCharCode);
}

QString StringPrototype::getThisString(Context *ctx)
{
    Value v;
    __qmljs_to_string(ctx, &v, &ctx->thisObject);
    assert(v.is(STRING_TYPE));
    return v.stringValue->toQString();
}

void StringPrototype::method_toString(Context *ctx)
{
    __qmljs_to_string(ctx, &ctx->result, &ctx->thisObject);
}

void StringPrototype::method_valueOf(Context *ctx)
{
    ctx->thisObject.objectValue->defaultValue(&ctx->result, STRING_HINT);
}

void StringPrototype::method_charAt(Context *ctx)
{
    const QString str = getThisString(ctx);

    int pos = 0;
    if (ctx->argumentCount > 0)
        pos = (int) ctx->argument(0).toInteger(ctx);

    QString result;
    if (pos >= 0 && pos < str.length())
        result += str.at(pos);

    ctx->result = Value::string(ctx, result);
}

void StringPrototype::method_charCodeAt(Context *ctx)
{
    const QString str = getThisString(ctx);

    int pos = 0;
    if (ctx->argumentCount > 0)
        pos = (int) ctx->argument(0).toInteger(ctx);

    double result = qSNaN();

    if (pos >= 0 && pos < str.length())
        result = str.at(pos).unicode();

    __qmljs_init_number(ctx, &ctx->result, result);
}

void StringPrototype::method_concat(Context *ctx)
{
    QString value = getThisString(ctx);

    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        Value v;
        __qmljs_to_string(ctx, &v, &ctx->arguments[i]);
        assert(v.is(STRING_TYPE));
        value += v.stringValue->toQString();
    }

    ctx->result = Value::string(ctx, value);
}

void StringPrototype::method_indexOf(Context *ctx)
{
    QString value = getThisString(ctx);

    QString searchString;
    if (ctx->argumentCount)
        searchString = ctx->argument(0).toString(ctx)->toQString();

    int pos = 0;
    if (ctx->argumentCount > 1)
        pos = (int) ctx->argument(1).toInteger(ctx);

    int index = -1;
    if (! value.isEmpty())
        index = value.indexOf(searchString, qMin(qMax(pos, 0), value.length()));

    __qmljs_init_number(ctx, &ctx->result, index);
}

void StringPrototype::method_lastIndexOf(Context *ctx)
{
    const QString value = getThisString(ctx);

    QString searchString;
    if (ctx->argumentCount) {
        Value v;
        __qmljs_to_string(ctx, &v, &ctx->arguments[0]);
        searchString = v.stringValue->toQString();
    }

    Value posArg = ctx->argument(1);
    double position = __qmljs_to_number(ctx, &posArg);
    if (qIsNaN(position))
        position = +qInf();
    else
        position = trunc(position);

    int pos = trunc(qMin(qMax(position, 0.0), double(value.length())));
    if (!searchString.isEmpty() && pos == value.length())
        --pos;
    int index = value.lastIndexOf(searchString, pos);
    __qmljs_init_number(ctx, &ctx->result, index);
}

void StringPrototype::method_localeCompare(Context *ctx)
{
    const QString value = getThisString(ctx);
    const QString that = ctx->argument(0).toString(ctx)->toQString();
    __qmljs_init_number(ctx, &ctx->result, QString::localeAwareCompare(value, that));
}

void StringPrototype::method_match(Context *)
{
    // requires Regexp
    Q_UNIMPLEMENTED();
}

void StringPrototype::method_replace(Context *)
{
    // requires Regexp
    Q_UNIMPLEMENTED();
}

void StringPrototype::method_search(Context *)
{
    // requires Regexp
    Q_UNIMPLEMENTED();
}

void StringPrototype::method_slice(Context *ctx)
{
    const QString text = getThisString(ctx);
    const int length = text.length();

    int start = int (ctx->argument(0).toInteger(ctx));
    int end = ctx->argument(1).isUndefined()
              ? length : int (ctx->argument(1).toInteger(ctx));

    if (start < 0)
        start = qMax(length + start, 0);
    else
        start = qMin(start, length);

    if (end < 0)
        end = qMax(length + end, 0);
    else
        end = qMin(end, length);

    int count = qMax(0, end - start);
    ctx->result = Value::string(ctx, text.mid(start, count));
}

void StringPrototype::method_split(Context *)
{
    // requires Array
    Q_UNIMPLEMENTED();
}

void StringPrototype::method_substr(Context *ctx)
{
    const QString value = getThisString(ctx);

    double start = 0;
    if (ctx->argumentCount > 0)
        start = ctx->argument(0).toInteger(ctx);

    double length = +qInf();
    if (ctx->argumentCount > 1)
        length = ctx->argument(1).toInteger(ctx);

    double count = value.length();
    if (start < 0)
        start = qMax(count + start, 0.0);

    length = qMin(qMax(length, 0.0), count - start);

    qint32 x = Value::toInt32(start);
    qint32 y = Value::toInt32(length);
    ctx->result = Value::string(ctx, value.mid(x, y));
}

void StringPrototype::method_substring(Context *ctx)
{
    QString value = getThisString(ctx);
    int length = value.length();

    double start = 0;
    double end = length;

    if (ctx->argumentCount > 0)
        start = ctx->argument(0).toInteger(ctx);

    if (ctx->argumentCount > 1)
        end = ctx->argument(1).toInteger(ctx);

    if (qIsNaN(start) || start < 0)
        start = 0;

    if (qIsNaN(end) || end < 0)
        end = 0;

    if (start > length)
        start = length;

    if (end > length)
        end = length;

    if (start > end) {
        double was = start;
        start = end;
        end = was;
    }

    qint32 x = Value::toInt32(start);
    qint32 y = Value::toInt32(end - start);
    ctx->result = Value::string(ctx, value.mid(x, y));
}

void StringPrototype::method_toLowerCase(Context *ctx)
{
    QString value = getThisString(ctx);
    ctx->result = Value::string(ctx, value.toLower());
}

void StringPrototype::method_toLocaleLowerCase(Context *ctx)
{
    method_toLowerCase(ctx);
}

void StringPrototype::method_toUpperCase(Context *ctx)
{
    QString value = getThisString(ctx);
    ctx->result = Value::string(ctx, value.toUpper());
}

void StringPrototype::method_toLocaleUpperCase(Context *ctx)
{
    method_toUpperCase(ctx);
}

void StringPrototype::method_fromCharCode(Context *ctx)
{
    QString str;
    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        QChar c(ctx->argument(i).toUInt16(ctx));
        str += c;
    }
    ctx->result = Value::string(ctx, str);
}
