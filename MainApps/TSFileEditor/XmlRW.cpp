#include "XmlRW.h"

#include <QDebug>
#include <QDomNodeList>

#define ROOT_ELEMENT        "TS"
#define CONTEXT_ELEMENT     "context"
#define MESSAGE_ELEMENT     "message"
#define LOCATION_ELEMENT    "location"
#define SOURCE_ELEMENT      "source"
#define TRANSLATION_ELEMENT "translation"

XmlRW::XmlRW(QObject *parent) : QObject(parent)
{

}

void XmlRW::UpdateTranslateMap(QList<TranslateModel>& list)
{
    m_translateMap.clear();
    int i = 0;
    foreach (TranslateModel model, list) {
        m_translateMap[model.GetKey()] = model.GetTranslate();
        i++;
    }
}

bool XmlRW::ImportFromTS(QList<TranslateModel>& list, QString strPath)
{
    QFile file(strPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    else {
        m_vanishCount = 0;
        xml.setDevice(&file);
        //        m_translateMap.clear();

        if (xml.readNextStartElement()) {
            QString strName = xml.name().toString();
            if (strName== ROOT_ELEMENT) {
                QXmlStreamAttributes attributes = xml.attributes();
                if (attributes.hasAttribute("version")) {
                    QString strVersion = attributes.value("version").toString();
                    qDebug() << "version : " << strVersion;
                }
                if (attributes.hasAttribute("language")) {
                    QString strLanguage = attributes.value("language").toString();
                }

                ReadXBEL();
            } else {
                xml.raiseError("XML file format error.");
            }
        }

        file.close();
        list.clear();

        for (auto i = m_translateMap.constBegin(); i != m_translateMap.constEnd(); i++) {
            TranslateModel model;
            model.SetKey(i.key());
            model.SetSource(i.key());
            model.SetTranslate(i.value());
            list.append(model);
        }

        // Remove vanish messages from the source .ts file
        if (m_vanishCount > 0) {
            RemoveVanishMessages(strPath);
        }

        return true;
    }
}

bool XmlRW::ExportToTS(QList<TranslateModel>& list, QString strPath)
{
    QFile file(strPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    else {
        if(list.count() <=0) {
            qDebug() << "[ExportToTS] translate list is empty, path:" << strPath;
            return false;
        }

        qDebug() << "[ExportToTS] Start, path:" << strPath << "list count:" << list.count();

        UpdateTranslateMap(list);

        qDebug() << "[ExportToTS] TranslateMap size:" << m_translateMap.size();

        QDomDocument doc;
        if(!doc.setContent(&file))
        {
            qCritical() << "[ExportToTS] xml parsing error, path:" << strPath;
            return false;
        }
        file.close();

        QDomElement root=doc.documentElement();
        QDomNodeList list=root.elementsByTagName("message");

        qDebug() << "[ExportToTS] message nodes in ts file:" << list.count();

        int overwriteCount = 0;
        int keepOriginalCount = 0;
        int unfinishedCount = 0;

        QDomNode node;
        for(int i=0; i < list.count(); i++) {
            node = list.at(i);
            QDomNodeList childList = node.childNodes();
            QString strKey = childList.at(childList.count()-2).toElement().text();
            QString strTranslation = node.lastChild().toElement().text();
            QString strValue = CheckFormat(m_translateMap.value(strKey));


            //if(strTranslation.isEmpty() && !strValue.isEmpty()) {

            QDomNode oldNode = node.lastChild();

            QDomElement newElement = doc.createElement("translation");
            if(strTranslation.isEmpty() && strValue.isEmpty())
            {
                newElement.setAttribute("type", "unfinished");
                unfinishedCount++;
            }
            else if(!strValue.isEmpty())
            {
                overwriteCount++;
            }
            else
            {
                keepOriginalCount++;
            }

            QDomText text = doc.createTextNode(strValue); // ???????Excel?????
            newElement.appendChild(text);
            node.replaceChild(newElement, oldNode);
            //}
        }

        qDebug() << "[ExportToTS] Overwrite:" << overwriteCount
                 << "KeepOriginal:" << keepOriginalCount << "Unfinished:" << unfinishedCount;

        if(!file.open(QFile::WriteOnly|QFile::Truncate)) {
            qCritical() << "[ExportToTS] Failed to open file for writing:" << strPath;
            return false;
        }

        QTextStream outStream(&file);
        doc.save(outStream, 4);
        file.close();

        qDebug() << "[ExportToTS] Success, path:" << strPath;
        return true;
    }
}

QString XmlRW::ErrorString() const
{
    return QString("Error:%1  Line:%2  Column:%3")
    .arg(xml.errorString())
        .arg(xml.lineNumber())
        .arg(xml.columnNumber());
}

void XmlRW::ReadXBEL()
{
    Q_ASSERT(xml.isStartElement() && xml.name().toString() == ROOT_ELEMENT);

    while (xml.readNextStartElement()) {
        if (xml.name().toString() == CONTEXT_ELEMENT) {
            ReadContext();
        } else {
            xml.skipCurrentElement();
        }
    }
}

void XmlRW::ReadContext()
{
    Q_ASSERT(xml.isStartElement() && xml.name().toString() == CONTEXT_ELEMENT);

    while (xml.readNextStartElement()) {
        if (xml.name().toString() == MESSAGE_ELEMENT) {
            ReadMessage();
        }
        else {
            xml.skipCurrentElement();
        }
    }
}

void XmlRW::ReadMessage()
{
    Q_ASSERT(xml.isStartElement() && xml.name().toString() == MESSAGE_ELEMENT);

    QString strSource, strTranslation, strLoaction;
    bool isVanish = false;

    while (xml.readNextStartElement()) {
        if (xml.name().toString() == SOURCE_ELEMENT) {
            strSource = xml.readElementText();
        } else if (xml.name().toString() == TRANSLATION_ELEMENT) {
            QXmlStreamAttributes attributes = xml.attributes();
            if (attributes.hasAttribute("type") && attributes.value("type").toString() == "vanished") {
                isVanish = true;
                xml.skipCurrentElement();
            } else {
                strTranslation = CheckFormat(xml.readElementText());
            }
        } else if (xml.name().toString() == LOCATION_ELEMENT) {
            strLoaction.clear();

            QXmlStreamAttributes attributes = xml.attributes();
            if (attributes.hasAttribute("filename")) {
                QString strFileName = attributes.value("filename").toString();
                strLoaction.append(QString("fileName: %1; ").arg(strFileName));
            }
            if (attributes.hasAttribute("line")) {
                QString strLine = attributes.value("line").toString();
                strLoaction.append(QString("line: %1; ").arg(strLine));
            }

            xml.skipCurrentElement();
        } else {
            xml.skipCurrentElement();
        }
    }

    if (isVanish) {
        m_vanishCount++;
    } else {
        m_translateMap.insert(strSource, strTranslation);
    }
}

void XmlRW::RemoveVanishMessages(const QString &strPath)
{
    QFile file(strPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QDomDocument doc;
    if (!doc.setContent(&file)) {
        file.close();
        return;
    }
    file.close();

    QDomElement root = doc.documentElement();
    QDomNodeList contextList = root.elementsByTagName(CONTEXT_ELEMENT);

    int removedCount = 0;
    for (int i = 0; i < contextList.count(); i++) {
        QDomElement contextElem = contextList.at(i).toElement();
        QDomNodeList messageList = contextElem.elementsByTagName(MESSAGE_ELEMENT);

        for (int j = messageList.count() - 1; j >= 0; j--) {
            QDomElement messageElem = messageList.at(j).toElement();
            QDomNodeList translationList = messageElem.elementsByTagName(TRANSLATION_ELEMENT);

            if (translationList.count() > 0) {
                QDomElement transElem = translationList.at(0).toElement();
                if (transElem.attribute("type") == "vanished") {
                    contextElem.removeChild(messageElem);
                    removedCount++;
                }
            }
        }
    }

    if (removedCount > 0) {
        if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
            return;
        }
        QTextStream outStream(&file);
        doc.save(outStream, 4);
        file.close();
        qDebug() << "Removed" << removedCount << "vanish messages from" << strPath;
    }
}

void XmlRW::printDomNode(const QDomNode& node) {
    // ????????
    if (node.isNull()) {
        qDebug() << "????";
        return;
    }

    // ????????
    switch (node.nodeType()) {
    case QDomNode::ElementNode: {
        // ?????????????
        QDomElement elem = node.toElement();
        qDebug() << "????:" << elem.tagName();
        QDomNamedNodeMap attrs = elem.attributes();
        for (int i = 0; i < attrs.size(); ++i) {
            QDomAttr attr = attrs.item(i).toAttr();
            qDebug() << "  ??:" << attr.name() << "=" << attr.value();
        }
        break;
    }
    case QDomNode::TextNode: {
        // ???????????
        qDebug() << "????:" << node.toText().data();
        break;
    }
    case QDomNode::CommentNode: {
        // ????
        qDebug() << "??:" << node.toComment().data();
        break;
    }
    default:
        qDebug() << "??????:" << node.nodeType();
    }
}


QString XmlRW::CheckFormat(const QString &Str)
{
    QString str = Str;
    if (str.isEmpty())
        return Str;
    if(Str.contains("% "))
        qDebug() << "---------" << str;
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
