#ifndef QLISTPROVIDER_H
#define QLISTPROVIDER_H

#include <QtQml/qqmlengine.h>

#include <QtCore/qobject.h>
#include <QtCore/qjsonarray.h>

class QListProvider : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ListProvider)
    Q_PROPERTY(QJsonArray json READ json WRITE setJson NOTIFY jsonChanged FINAL)
    Q_PROPERTY(QStringList strings READ strings WRITE setStrings NOTIFY stringsChanged FINAL)

public:
    explicit QListProvider(QObject *parent = nullptr) : QObject(parent) { }

    Q_INVOKABLE QList<int> intList() const
    {
        QList<int> list;
        for (int i = 0; i < 3; ++i)
            list.append(i);
        return list;
    }

    QStringList strings() const { return m_strings; }
    void setStrings(const QStringList &strings)
    {
        if (strings != m_strings) {
            m_strings = strings;
            emit stringsChanged();
        }
    }

    QJsonArray json() const { return m_json; }
    void setJson(const QJsonArray &json)
    {
        if (json != m_json) {
            m_json = json;
            emit jsonChanged();
        }
    }

signals:
    void jsonChanged();
    void stringsChanged();

private:
    QJsonArray m_json;
    QStringList m_strings;
};

#endif // QLISTPROVIDER_H
