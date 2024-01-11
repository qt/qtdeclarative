// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerdata.h"

#include <QtCore/qfile.h>
#include <QtCore/qqueue.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qurl.h>
#include <QtCore/qxmlstream.h>
#include <QtCore/qxpfunctional.h>

#include <limits>

const char PROFILER_FILE_VERSION[] = "1.02";

static const char *RANGE_TYPE_STRINGS[] = {
    "Painting",
    "Compiling",
    "Creating",
    "Binding",
    "HandlingSignal",
    "Javascript"
};

Q_STATIC_ASSERT(sizeof(RANGE_TYPE_STRINGS) == MaximumRangeType * sizeof(const char *));

static const char *MESSAGE_STRINGS[] = {
    "Event",
    "RangeStart",
    "RangeData",
    "RangeLocation",
    "RangeEnd",
    "Complete",
    "PixmapCache",
    "SceneGraph",
    "MemoryAllocation",
    "DebugMessage"
};

Q_STATIC_ASSERT(sizeof(MESSAGE_STRINGS) == MaximumMessage * sizeof(const char *));

/////////////////////////////////////////////////////////////////
class QmlProfilerDataPrivate
{
public:
    QmlProfilerDataPrivate(QmlProfilerData *qq){ Q_UNUSED(qq); }

    // data storage
    QVector<QQmlProfilerEventType> eventTypes;
    QVector<QQmlProfilerEvent> events;

    qint64 traceStartTime;
    qint64 traceEndTime;

    // internal state while collecting events
    qint64 qmlMeasuredTime;
    QmlProfilerData::State state;
};

/////////////////////////////////////////////////////////////////
QmlProfilerData::QmlProfilerData(QObject *parent) :
    QQmlProfilerEventReceiver(parent), d(new QmlProfilerDataPrivate(this))
{
    d->state = Empty;
    clear();
}

QmlProfilerData::~QmlProfilerData()
{
    clear();
    delete d;
}

void QmlProfilerData::clear()
{
    d->events.clear();

    d->traceEndTime = std::numeric_limits<qint64>::min();
    d->traceStartTime = std::numeric_limits<qint64>::max();
    d->qmlMeasuredTime = 0;

    setState(Empty);
}

QString QmlProfilerData::qmlRangeTypeAsString(RangeType type)
{
    if (type * sizeof(char *) < sizeof(RANGE_TYPE_STRINGS))
        return QLatin1String(RANGE_TYPE_STRINGS[type]);
    else
        return QString::number(type);
}

QString QmlProfilerData::qmlMessageAsString(Message type)
{
    if (type * sizeof(char *) < sizeof(MESSAGE_STRINGS))
        return QLatin1String(MESSAGE_STRINGS[type]);
    else
        return QString::number(type);
}

void QmlProfilerData::setTraceStartTime(qint64 time)
{
    if (time < d->traceStartTime)
        d->traceStartTime = time;
}

void QmlProfilerData::setTraceEndTime(qint64 time)
{
    if (time > d->traceEndTime)
        d->traceEndTime = time;
}

qint64 QmlProfilerData::traceStartTime() const
{
    return d->traceStartTime;
}

qint64 QmlProfilerData::traceEndTime() const
{
    return d->traceEndTime;
}

void QmlProfilerData::addEvent(const QQmlProfilerEvent &event)
{
    setState(AcquiringData);
    d->events.append(event);
}

void QmlProfilerData::addEventType(const QQmlProfilerEventType &type)
{
    QQmlProfilerEventType newType = type;

    QString details;
    // generate details string
    if (!type.data().isEmpty()) {
        details = type.data().simplified();
        QRegularExpression rewrite(QStringLiteral("^\\(function \\$(\\w+)\\(\\) \\{ (return |)(.+) \\}\\)$"));
        QRegularExpressionMatch match = rewrite.match(details);
        if (match.hasMatch()) {
            details = match.captured(1) +QLatin1String(": ") + match.captured(3);
        }
        if (details.startsWith(QLatin1String("file://")))
            details = details.mid(details.lastIndexOf(QLatin1Char('/')) + 1);
    }

    newType.setData(details);

    QString displayName;
    switch (type.message()) {
    case Event: {
        switch (type.detailType()) {
        case Mouse:
        case Key:
            displayName = QString::fromLatin1("Input:%1").arg(type.detailType());
            break;
        case AnimationFrame:
            displayName = QString::fromLatin1("AnimationFrame");
            break;
        default:
            displayName = QString::fromLatin1("Unknown");
        }
        break;
    }
    case RangeStart:
    case RangeData:
    case RangeLocation:
    case RangeEnd:
    case Complete:
        Q_UNREACHABLE();
        break;
    case PixmapCacheEvent: {
        const QString filePath = QUrl(type.location().filename()).path();
        displayName = QStringView{filePath}.mid(filePath.lastIndexOf(QLatin1Char('/')) + 1)
                + QLatin1Char(':') + QString::number(type.detailType());
        break;
    }
    case SceneGraphFrame:
        displayName = QString::fromLatin1("SceneGraph:%1").arg(type.detailType());
        break;
    case MemoryAllocation:
        displayName = QString::fromLatin1("MemoryAllocation:%1").arg(type.detailType());
        break;
    case DebugMessage:
        displayName = QString::fromLatin1("DebugMessage:%1").arg(type.detailType());
        break;
    case MaximumMessage: {
        const QQmlProfilerEventLocation eventLocation = type.location();
        // generate hash
        if (eventLocation.filename().isEmpty()) {
            displayName = QString::fromLatin1("Unknown");
        } else {
            const QString filePath = QUrl(eventLocation.filename()).path();
            displayName = QStringView{filePath}.mid(
                        filePath.lastIndexOf(QLatin1Char('/')) + 1) +
                        QLatin1Char(':') + QString::number(eventLocation.line());
        }
        break;
    }
    }

    newType.setDisplayName(displayName);
    d->eventTypes.append(newType);
}

void QmlProfilerData::computeQmlTime()
{
    // compute levels
    qint64 level0Start = -1;
    int level = 0;

    for (const QQmlProfilerEvent &event : std::as_const(d->events)) {
        const QQmlProfilerEventType &type = d->eventTypes.at(event.typeIndex());
        if (type.message() != MaximumMessage)
            continue;

        switch (type.rangeType()) {
        case Compiling:
        case Creating:
        case Binding:
        case HandlingSignal:
        case Javascript:
            switch (event.rangeStage()) {
            case RangeStart:
                if (level++ == 0)
                    level0Start = event.timestamp();
                break;
            case RangeEnd:
                if (--level == 0)
                    d->qmlMeasuredTime += event.timestamp() - level0Start;
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }
}

bool compareStartTimes(const QQmlProfilerEvent &t1, const QQmlProfilerEvent &t2)
{
    return t1.timestamp() < t2.timestamp();
}

void QmlProfilerData::sortStartTimes()
{
    if (d->events.size() < 2)
        return;

    // assuming startTimes is partially sorted
    // identify blocks of events and sort them with quicksort
    QVector<QQmlProfilerEvent>::iterator itFrom = d->events.end() - 2;
    QVector<QQmlProfilerEvent>::iterator itTo = d->events.end() - 1;

    while (itFrom != d->events.begin() && itTo != d->events.begin()) {
        // find block to sort
        while (itFrom != d->events.begin() && itTo->timestamp() > itFrom->timestamp()) {
            --itTo;
            itFrom = itTo - 1;
        }

        // if we're at the end of the list
        if (itFrom == d->events.begin())
            break;

        // find block length
        while (itFrom != d->events.begin() && itTo->timestamp() <= itFrom->timestamp())
            --itFrom;

        if (itTo->timestamp() <= itFrom->timestamp())
            std::sort(itFrom, itTo + 1, compareStartTimes);
        else
            std::sort(itFrom + 1, itTo + 1, compareStartTimes);

        // move to next block
        itTo = itFrom;
        itFrom = itTo - 1;
    }
}

void QmlProfilerData::complete()
{
    setState(ProcessingData);
    sortStartTimes();
    computeQmlTime();
    setState(Done);
    emit dataReady();
}

bool QmlProfilerData::isEmpty() const
{
    return d->events.isEmpty();
}

struct StreamWriter {
    QString error;

    StreamWriter(const QString &filename)
    {
        if (!filename.isEmpty()) {
            file.setFileName(filename);
            if (!file.open(QIODevice::WriteOnly)) {
                error = QmlProfilerData::tr("Could not open %1 for writing").arg(filename);
                return;
            }
        } else {
            if (!file.open(stdout, QIODevice::WriteOnly)) {
                error = QmlProfilerData::tr("Could not open stdout for writing");
                return;
            }
        }

        stream.setDevice(&file);
        stream.setAutoFormatting(true);
        stream.writeStartDocument();
        writeStartElement("trace");
    }

    ~StreamWriter() {
        writeEndElement();
        stream.writeEndDocument();
        file.close();
    }

    template<typename Number>
    void writeAttribute(const char *name, Number number)
    {
        stream.writeAttribute(QLatin1String(name), QString::number(number));
    }

    void writeAttribute(const char *name, const char *value)
    {
        stream.writeAttribute(QLatin1String(name), QLatin1String(value));
    }

    void writeAttribute(const char *name, const QQmlProfilerEvent &event, int i, bool printZero = true)
    {
        const qint64 number = event.number<qint64>(i);
        if (printZero || number != 0)
            writeAttribute(name, number);
    }

    template<typename Number>
    void writeTextElement(const char *name, Number number)
    {
        writeTextElement(name, QString::number(number));
    }

    void writeTextElement(const char *name, const char *value)
    {
        stream.writeTextElement(QLatin1String(name), QLatin1String(value));
    }

    void writeTextElement(const char *name, const QString &value)
    {
        stream.writeTextElement(QLatin1String(name), value);
    }

    void writeStartElement(const char *name)
    {
        stream.writeStartElement(QLatin1String(name));
    }

    void writeEndElement()
    {
        stream.writeEndElement();
    }

private:
    QFile file;
    QXmlStreamWriter stream;
};

struct DataIterator
{
    DataIterator(
            const QmlProfilerDataPrivate *d,
            qxp::function_ref<void(const QQmlProfilerEvent &, qint64)> &&sendEvent)
        : d(d)
        , sendEvent(std::move(sendEvent))
    {}

    void run();

private:
    void handleRangeEvent(const QQmlProfilerEvent &event, const QQmlProfilerEventType &type);
    void sendPending();
    void endLevel0();

    const QmlProfilerDataPrivate *d = nullptr;
    const qxp::function_ref<void(const QQmlProfilerEvent &, qint64)> sendEvent;

    QQueue<QQmlProfilerEvent> pointEvents;
    QList<QQmlProfilerEvent> rangeStarts[MaximumRangeType];
    QList<qint64> rangeEnds[MaximumRangeType];

    int level = 0;
};

void DataIterator::handleRangeEvent(
        const QQmlProfilerEvent &event, const QQmlProfilerEventType &type)
{
    QList<QQmlProfilerEvent> &starts = rangeStarts[type.rangeType()];
    switch (event.rangeStage()) {
    case RangeStart: {
        ++level;
        starts.append(event);
        break;
    }
    case RangeEnd: {
        const qint64 invalidTimestamp = -1;
        QList<qint64> &ends = rangeEnds[type.rangeType()];

               // -1 because all valid timestamps are >= 0.
        ends.resize(starts.size(), invalidTimestamp);

        qsizetype i = starts.size();
        while (ends[--i] != invalidTimestamp) {}

        Q_ASSERT(i >= 0);
        Q_ASSERT(starts[i].timestamp() <= event.timestamp());

        ends[i] = event.timestamp();
        if (--level == 0)
            endLevel0();
        break;
    }
    default:
        break;
    }
}

void DataIterator::sendPending()
{
    // Send all pending events in the order of their start times.

    qsizetype index[MaximumRangeType] = { 0, 0, 0, 0, 0, 0 };
    while (true) {

        // Find the range type with the minimum start time.
        qsizetype minimum = MaximumRangeType;
        qint64 minimumTime = std::numeric_limits<qint64>::max();
        for (qsizetype i = 0; i < MaximumRangeType; ++i) {
            const QList<QQmlProfilerEvent> &starts = rangeStarts[i];
            if (starts.size() == index[i])
                continue;
            const qint64 timestamp = starts[index[i]].timestamp();
            if (timestamp < minimumTime) {
                minimumTime = timestamp;
                minimum = i;
            }
        }
        if (minimum == MaximumRangeType)
            break;

        // Send all point events that happened before the range we've found.
        while (!pointEvents.isEmpty() && pointEvents.front().timestamp() < minimumTime)
            sendEvent(pointEvents.dequeue(), 0);

        // Send the range itself
        sendEvent(rangeStarts[minimum][index[minimum]],
                  rangeEnds[minimum][index[minimum]] - minimumTime);

        // Bump the index so that we don't send the same range again
        ++index[minimum];
    }
}

void DataIterator::endLevel0()
{
    sendPending();
    for (qsizetype i = 0; i < MaximumRangeType; ++i) {
        rangeStarts[i].clear();
        rangeEnds[i].clear();
    }
}

void DataIterator::run()
{
    for (const QQmlProfilerEvent &event : std::as_const(d->events)) {
        const QQmlProfilerEventType &type = d->eventTypes.at(event.typeIndex());
        if (type.rangeType() != MaximumRangeType)
            handleRangeEvent(event, type);
        else if (level == 0)
            sendEvent(event, 0);
        else
            pointEvents.enqueue(event);
    }

    for (qsizetype i = 0; i < MaximumRangeType; ++i) {
        while (rangeEnds[i].size() < rangeStarts[i].size()) {
            rangeEnds[i].append(d->traceEndTime);
            --level;
        }
    }

    sendPending();
}

bool QmlProfilerData::save(const QString &filename)
{
    if (isEmpty()) {
        emit error(tr("No data to save"));
        return false;
    }

    StreamWriter stream(filename);
    if (!stream.error.isEmpty()) {
        emit error(stream.error);
        return false;
    }

    stream.writeAttribute("version", PROFILER_FILE_VERSION);
    stream.writeAttribute("traceStart", traceStartTime());
    stream.writeAttribute("traceEnd", traceEndTime());

    stream.writeStartElement("eventData");
    stream.writeAttribute("totalTime", d->qmlMeasuredTime);

    for (int typeIndex = 0, end = d->eventTypes.size(); typeIndex < end; ++typeIndex) {
        const QQmlProfilerEventType &eventData = d->eventTypes.at(typeIndex);
        stream.writeStartElement("event");
        stream.writeAttribute("index", typeIndex);
        if (!eventData.displayName().isEmpty())
            stream.writeTextElement("displayname", eventData.displayName());

        stream.writeTextElement("type", eventData.rangeType() == MaximumRangeType
                                ? qmlMessageAsString(eventData.message())
                                : qmlRangeTypeAsString(eventData.rangeType()));

        const QQmlProfilerEventLocation location = eventData.location();
        if (!location.filename().isEmpty())
            stream.writeTextElement("filename", location.filename());
        if (location.line() >= 0)
            stream.writeTextElement("line", location.line());
        if (location.column() >= 0)
            stream.writeTextElement("column", location.column());
        if (!eventData.data().isEmpty())
            stream.writeTextElement("details", eventData.data());
        if (eventData.rangeType() == Binding)
            stream.writeTextElement("bindingType", eventData.detailType());
        else if (eventData.message() == Event) {
            switch (eventData.detailType()) {
            case AnimationFrame:
                stream.writeTextElement("animationFrame", eventData.detailType());
                break;
            case Key:
                stream.writeTextElement("keyEvent", eventData.detailType());
                break;
            case Mouse:
                stream.writeTextElement("mouseEvent", eventData.detailType());
                break;
            }
        } else if (eventData.message() == PixmapCacheEvent)
            stream.writeTextElement("cacheEventType", eventData.detailType());
        else if (eventData.message() == SceneGraphFrame)
            stream.writeTextElement("sgEventType", eventData.detailType());
        else if (eventData.message() == MemoryAllocation)
            stream.writeTextElement("memoryEventType", eventData.detailType());
        stream.writeEndElement();
    }
    stream.writeEndElement(); // eventData

    stream.writeStartElement("profilerDataModel");

    auto sendEvent = [&](const QQmlProfilerEvent &event, qint64 duration = 0) {
        Q_ASSERT(duration >= 0);
        const QQmlProfilerEventType &type = d->eventTypes.at(event.typeIndex());
        stream.writeStartElement("range");
        stream.writeAttribute("startTime", event.timestamp());
        if (duration != 0)
            stream.writeAttribute("duration", duration);
        stream.writeAttribute("eventIndex", event.typeIndex());
        if (type.message() == Event) {
            if (type.detailType() == AnimationFrame) {
                // special: animation frame
                stream.writeAttribute("framerate", event, 0);
                stream.writeAttribute("animationcount", event, 1);
                stream.writeAttribute("thread", event, 2);
            } else if (type.detailType() == Key || type.detailType() == Mouse) {
                // numerical value here, to keep the format a bit more compact
                stream.writeAttribute("type", event, 0);
                stream.writeAttribute("data1", event, 1);
                stream.writeAttribute("data2", event, 2);
            }
        } else if (type.message() == PixmapCacheEvent) {
            // special: pixmap cache event
            if (type.detailType() == PixmapSizeKnown) {
                stream.writeAttribute("width", event, 0);
                stream.writeAttribute("height", event, 1);
            } else if (type.detailType() == PixmapReferenceCountChanged
                       || type.detailType() == PixmapCacheCountChanged) {
                stream.writeAttribute("refCount", event, 1);
            }
        } else if (type.message() == SceneGraphFrame) {
            stream.writeAttribute("timing1", event, 0, false);
            stream.writeAttribute("timing2", event, 1, false);
            stream.writeAttribute("timing3", event, 2, false);
            stream.writeAttribute("timing4", event, 3, false);
            stream.writeAttribute("timing5", event, 4, false);
        } else if (type.message() == MemoryAllocation) {
            stream.writeAttribute("amount", event, 0);
        }
        stream.writeEndElement();
    };

    DataIterator(d, std::move(sendEvent)).run();

    stream.writeEndElement(); // profilerDataModel

    return true;
}

void QmlProfilerData::setState(QmlProfilerData::State state)
{
    // It's not an error, we are continuously calling "AcquiringData" for example
    if (d->state == state)
        return;

    switch (state) {
    case Empty:
        // if it's not empty, complain but go on
        if (!isEmpty())
            emit error("Invalid qmlprofiler state change (Empty)");
        break;
    case AcquiringData:
        // we're not supposed to receive new data while processing older data
        if (d->state == ProcessingData)
            emit error("Invalid qmlprofiler state change (AcquiringData)");
        break;
    case ProcessingData:
        if (d->state != AcquiringData)
            emit error("Invalid qmlprofiler state change (ProcessingData)");
        break;
    case Done:
        if (d->state != ProcessingData && d->state != Empty)
            emit error("Invalid qmlprofiler state change (Done)");
        break;
    default:
        emit error("Trying to set unknown state in events list");
        break;
    }

    d->state = state;
    emit stateChanged();

    // special: if we were done with an empty list, clean internal data and go back to empty
    if (d->state == Done && isEmpty()) {
        clear();
    }
    return;
}

int QmlProfilerData::numLoadedEventTypes() const
{
    return d->eventTypes.size();
}

#include "moc_qmlprofilerdata.cpp"
