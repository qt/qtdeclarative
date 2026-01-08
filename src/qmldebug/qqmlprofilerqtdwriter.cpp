// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlprofilerqtdwriter_p.h"

#include <private/qobject_p.h>

#include <QtCore/qfile.h>
#include <QtCore/qqueue.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qurl.h>
#include <QtCore/qxmlstream.h>
#include <QtCore/qxpfunctional.h>

#include <limits>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
    "DebugMessage",
    "Quick3D", // Not "Quick3DFrame" unfortunately. See precedence in Qt Creator.
};

Q_STATIC_ASSERT(sizeof(MESSAGE_STRINGS) == MaximumMessage * sizeof(const char *));

/////////////////////////////////////////////////////////////////
class QQmlProfilerQtdWriterPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlProfilerQtdWriter)
public:
    enum State {
        Empty,
        AcquiringData,
        ProcessingData,
        Done
    };

    // data storage
    QList<QQmlProfilerEventType> eventTypes;
    QList<QQmlProfilerEvent> events;

    qint64 traceStartTime = std::numeric_limits<qint64>::max();
    qint64 traceEndTime = std::numeric_limits<qint64>::min();

    // internal state while collecting events
    qint64 qmlMeasuredTime = 0;
    State state = Empty;

    bool isEmpty() const { return events.isEmpty(); }
    void sortStartTimes();
    void computeQmlTime();
    void setState(State state);
};

/////////////////////////////////////////////////////////////////
QQmlProfilerQtdWriter::QQmlProfilerQtdWriter(QObject *parent) :
    QQmlProfilerEventReceiver(*new QQmlProfilerQtdWriterPrivate, parent)
{
}

QQmlProfilerQtdWriter::~QQmlProfilerQtdWriter() = default;

void QQmlProfilerQtdWriter::clear()
{
    Q_D(QQmlProfilerQtdWriter);

    // Do not clear the types. They persist for the whole session.
    d->traceStartTime = std::numeric_limits<qint64>::max();
    d->traceEndTime = std::numeric_limits<qint64>::min();
    d->events.clear();
    d->qmlMeasuredTime = 0;
    d->setState(QQmlProfilerQtdWriterPrivate::Empty);
}

void QQmlProfilerQtdWriter::startTrace(qint64 time, const QList<int> &engineIds)
{
    QQmlProfilerEventReceiver::startTrace(time, engineIds);

    Q_D(QQmlProfilerQtdWriter);
    if (time < d->traceStartTime)
        d->traceStartTime = time;
}

void QQmlProfilerQtdWriter::endTrace(qint64 time, const QList<int> &engineIds)
{
    QQmlProfilerEventReceiver::endTrace(time, engineIds);

    Q_D(QQmlProfilerQtdWriter);
    if (time > d->traceEndTime)
        d->traceEndTime = time;
}


static QString qmlRangeTypeAsString(RangeType type)
{
    if (type * sizeof(char *) < sizeof(RANGE_TYPE_STRINGS))
        return QLatin1String(RANGE_TYPE_STRINGS[type]);
    else
        return QString::number(type);
}

static QString qmlMessageAsString(Message type)
{
    if (type * sizeof(char *) < sizeof(MESSAGE_STRINGS))
        return QLatin1String(MESSAGE_STRINGS[type]);
    else
        return QString::number(type);
}

void QQmlProfilerQtdWriter::addEvent(const QQmlProfilerEvent &event)
{
    Q_D(QQmlProfilerQtdWriter);
    d->setState(QQmlProfilerQtdWriterPrivate::AcquiringData);
    d->events.append(event);
}

void QQmlProfilerQtdWriter::addEventType(const QQmlProfilerEventType &type)
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
    case Quick3DFrame:
        displayName = QString::fromLatin1("Quick3DFrame:%1").arg(type.detailType());
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
    d_func()->eventTypes.append(newType);
}

void QQmlProfilerQtdWriterPrivate::computeQmlTime()
{
    // compute levels
    qint64 level0Start = -1;
    int level = 0;

    for (const QQmlProfilerEvent &event : std::as_const(events)) {
        const QQmlProfilerEventType &type = eventTypes.at(event.typeIndex());
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
                    qmlMeasuredTime += event.timestamp() - level0Start;
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

void QQmlProfilerQtdWriterPrivate::sortStartTimes()
{
    if (events.size() < 2)
        return;

    // assuming startTimes is partially sorted
    // identify blocks of events and sort them with quicksort
    QList<QQmlProfilerEvent>::iterator itFrom = events.end() - 2;
    QList<QQmlProfilerEvent>::iterator itTo = events.end() - 1;

    while (itFrom != events.begin() && itTo != events.begin()) {
        // find block to sort
        while (itFrom != events.begin() && itTo->timestamp() > itFrom->timestamp()) {
            --itTo;
            itFrom = itTo - 1;
        }

        // if we're at the end of the list
        if (itFrom == events.begin())
            break;

        // find block length
        while (itFrom != events.begin() && itTo->timestamp() <= itFrom->timestamp())
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

void QQmlProfilerQtdWriter::complete(qint64 maximumTime)
{
    Q_D(QQmlProfilerQtdWriter);
    d->setState(QQmlProfilerQtdWriterPrivate::ProcessingData);
    d->sortStartTimes();
    d->computeQmlTime();
    d->setState(QQmlProfilerQtdWriterPrivate::Done);
    QQmlProfilerEventReceiver::complete(maximumTime);
}

struct StreamWriter {
    QString error;

    StreamWriter(const QString &filename)
    {
        if (!filename.isEmpty()) {
            file.setFileName(filename);
            if (!file.open(QIODevice::WriteOnly)) {
                error = QQmlProfilerQtdWriter::tr("Could not open %1 for writing").arg(filename);
                return;
            }
        } else {
            if (!file.open(stdout, QIODevice::WriteOnly)) {
                error = QQmlProfilerQtdWriter::tr("Could not open stdout for writing");
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
            const QQmlProfilerQtdWriterPrivate *d,
            qxp::function_ref<void(const QQmlProfilerEvent &, qint64)> &&sendEvent)
        : d(d)
        , sendEvent(std::move(sendEvent))
    {}

    void run();

private:
    void handleRangeEvent(const QQmlProfilerEvent &event, const QQmlProfilerEventType &type);
    void sendPending();
    void endLevel0();

    const QQmlProfilerQtdWriterPrivate *d = nullptr;
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

bool QQmlProfilerQtdWriter::save(const QString &filename)
{
    Q_D(QQmlProfilerQtdWriter);

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
    stream.writeAttribute("traceStart", d->traceStartTime);
    stream.writeAttribute("traceEnd", d->traceEndTime);

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

void QQmlProfilerQtdWriterPrivate::setState(State newState)
{
    // It's not an error, we are continuously calling "AcquiringData" for example
    if (state == newState)
        return;

    Q_Q(QQmlProfilerQtdWriter);
    switch (newState) {
    case Empty:
        // if it's not empty, complain but go on
        if (!isEmpty())
            emit q->error("Invalid qmlprofiler state change (Empty)"_L1);
        break;
    case AcquiringData:
        // we're not supposed to receive new data while processing older data
        if (state == ProcessingData)
            emit q->error("Invalid qmlprofiler state change (AcquiringData)"_L1);
        break;
    case ProcessingData:
        if (state != AcquiringData)
            emit q->error("Invalid qmlprofiler state change (ProcessingData)"_L1);
        break;
    case Done:
        if (state != ProcessingData && state != Empty)
            emit q->error("Invalid qmlprofiler state change (Done)"_L1);
        break;
    default:
        emit q->error("Trying to set unknown state in events list"_L1);
        break;
    }

    state = newState;

    // special: if we were done with an empty list, clean internal data and go back to empty
    if (state == Done && isEmpty())
        q->clear();
    return;
}

qsizetype QQmlProfilerQtdWriter::numLoadedEventTypes() const
{
    return d_func()->eventTypes.size();
}

qsizetype QQmlProfilerQtdWriter::numLoadedEvents() const
{
    return d_func()->events.size();
}


QT_END_NAMESPACE

#include "moc_qqmlprofilerqtdwriter_p.cpp"
