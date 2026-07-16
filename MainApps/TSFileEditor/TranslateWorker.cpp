#include "TranslateWorker.h"
#include <NetWorker.h>
#include <QCryptographicHash>
#include <QUuid>
#include <QTimer>
#include <QNetworkRequest>

TranslateWorker::TranslateWorker(QList<TranslateModel> &list, QObject *parent) : QObject(parent),
    m_list(list)
{
    m_pNetWorker = NetWorker::instance();

    connect(this, &TranslateWorker::translateResult, this, &TranslateWorker::onTranslateResult);
}

TranslateWorker::~TranslateWorker()
{

}

void TranslateWorker::SetIdKey(const QString &id, const QString &key)
{
    m_appId = id;
    m_appKey = key;
}

bool TranslateWorker::YoudaoTranslate(const QString &from, const QString &to)
{
    if(m_list.count() <=0) {
        emit error("translate list is empty");
        return false;
    }

    m_fromLang = from;
    m_toLang = to;
    qDebug() << "total size:" << m_list.size();
    for(int i=0; i<m_list.count(); i++) {
        if(m_list.at(i).GetTranslate().isEmpty())
            YoudaoTranslate(i, m_list.at(i).GetSource());
        else
            m_already_count++;
    }
    return true;
}

void TranslateWorker::YoudaoTranslate(int index, const QString &_source)
{
    // ??????
    // ??????????1000ms?
    //    qint64 now = QDateTime::currentMSecsSinceEpoch();
    //    if (now - m_lastRequestTime < 1000) {
    //        QTimer::singleShot(1000 - (now - m_lastRequestTime), this, [=]() {
    //            YoudaoTranslate(index, source); // ?????
    //        });
    //        return;
    //    }
    //    m_lastRequestTime = now;

    QString baseUrl = QString("http://openapi.youdao.com/api");
    //QString baseUrl = QString("http://fanyi.youdao.com/openapi");
    QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString timestamp = QString::number(QDateTime::currentDateTime().toSecsSinceEpoch());
    QString source = _source;

    QUrlQuery query;
    //query.addQueryItem("q", source.toUtf8().toPercentEncoding());//QUrl::toPercentEncoding(source)
    query.addQueryItem("q", QUrl::toPercentEncoding(source.toUtf8()));
    query.addQueryItem("from", m_fromLang.trimmed());
    query.addQueryItem("to", m_toLang.trimmed());
    query.addQueryItem("appKey", m_appId.trimmed());
    query.addQueryItem("salt", uuid);
    query.addQueryItem("sign", GetYoudaoSign(source, uuid, timestamp));
    query.addQueryItem("signType", "v3");
    query.addQueryItem("curtime", timestamp);

    // ??POST ?? ??????Content-Type?
    //    QNetworkRequest request;
    //    request.setUrl(QUrl("https://openapi.youdao.com/api"));
    //    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    //    QNetworkReply *pReply = m_pNetWorker->post(
    //        baseUrl,
    //        query.toString(QUrl::FullyEncoded).toUtf8()
    //    );

    // get??
    QNetworkReply *pReply = (m_pNetWorker->get(baseUrl, query));
    connect(pReply, &QNetworkReply::finished, this, [=](){
        if (pReply->error() != QNetworkReply::NoError) {
            emit error("Http error: " + pReply->errorString());
        } else {
            QByteArray replyData = pReply->readAll();

            QJsonParseError jsonError;
            QJsonDocument jsonDocument = QJsonDocument::fromJson(replyData, &jsonError);
            if (jsonError.error == QJsonParseError::NoError) {
                if (!(jsonDocument.isNull() || jsonDocument.isEmpty()) && jsonDocument.isObject()) {
                    QVariantMap data = jsonDocument.toVariant().toMap();
                    int errorcode = data[QLatin1String("errorCode")].toInt();

                    if(0 == errorcode){
                        QVariantList detailList = data[QLatin1String("translation")].toList();
                        QString str = detailList.first().toString();

                        emit translateResult(index, str);
                    }
                    else{
                        //qDebug() << replyData;
                        if (errorcode == 411 || errorcode == 110 || errorcode == 108) { // ??411??
                            //qWarning() << "Frequency limit hit, retrying...";
                            QTimer::singleShot(500, this, [=]() {
                                YoudaoTranslate(index, source);
                            });
                        }
                        else
                        {
                            QVariantMap data = jsonDocument.toVariant().toMap();
                            int errorcode = data[QLatin1String("errorCode")].toInt();
                            qDebug() << "Fail translator error " << errorcode << "----" << m_failure_count++  << "---" << source;
                        }

                    }

                }
            } else {
                emit error(jsonError.errorString());
            }
        }

        pReply->deleteLater();
    });
}

QString TranslateWorker::CheckFormat(const QString &Str)
{
    QString str = Str;
    if (str.isEmpty())
        return Str;
    // ???????/?????
    if (str.startsWith('\"')) {
        str.remove(0, 1);
    }
    // ??????????
    if (str.endsWith('\"')) {
        str.chop(1);
    }

    // ??%???
    if(str.contains(" % ") || str.contains("% "))
        str.replace(" % ","%").replace("% ","%");

    // ?????
    str[0] = str[0].toUpper();

    // ?? () ??
    if (str.count('(') >  str.count(')'))
        return str + ")";

    return str;
}

QString TranslateWorker::GetYoudaoSign(const QString &q, const QString &salt, const QString &curtime) {
    QString input = m_appId + q + salt + curtime + m_appKey; // ?????
    QByteArray hash = QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

void TranslateWorker::onTranslateResult(int index, const QString &str)
{
    TranslateModel model = m_list.at(index);
    model.SetTranslate(CheckFormat(str));
    m_list.replace(index, model);
    m_finish_count++;

    qDebug() << "No:" << QString("current = %1, failure = %2, already = %3 , total = %4").arg(m_finish_count).arg(m_failure_count).arg(m_already_count).arg(m_list.size()) << "source str: " << m_list.at(index).GetSource() << "\ttranslationStr: " << m_list.at(index).GetTranslate();
}
