#ifndef TRANSLATEWORKER_H
#define TRANSLATEWORKER_H

#include <QObject>
#include "DataModel/TranslateModel.h"

class NetWorker;

class TranslateWorker : public QObject
{
    Q_OBJECT
public:

    explicit TranslateWorker(QList<TranslateModel> &list, QObject *parent = nullptr);
    ~TranslateWorker();

    void SetIdKey(const QString& id, const QString& key);
    bool YoudaoTranslate(const QString &from = QString("auto"), const QString &to = QString("en"));

signals:
    void translateResult(int index, const QString &str);
    void error(const QString& msg);

private slots:
    void onTranslateResult(int index, const QString &str);

private:
    //QByteArray GetYoudaoSign(const QString &source, const QString& uuid, const QString& timestamp);
    QString GetYoudaoSign(const QString &source, const QString& uuid, const QString& timestamp);
    void YoudaoTranslate(int index, const QString &source);
    QString CheckFormat(const QString& str);

    NetWorker*              m_pNetWorker;

    QString                 m_fromLang;
    QString                 m_toLang;
    QString                 m_appId;
    QString                 m_appKey;
    QList<TranslateModel>   &m_list;
    int m_lastRequestTime = 0;
    int m_finish_count = 0;
    int m_already_count = 0;
    int m_failure_count = 0;
};

#endif // TRANSLATEWORKER_H
