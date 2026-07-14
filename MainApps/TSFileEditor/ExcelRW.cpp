#include "ExcelRW.h"
#include <QRegExpValidator>
#include <QFileInfo>
#include <QDebug>
#include <QtGlobal>
#include <QtMath>

ExcelRW::ExcelRW(int keyColumn, int sourceColumn, int transColumn, QObject *parent) : QObject(parent)
{
    m_TotalCount = 0;
    m_KeyColumn = keyColumn;
    m_SourceColumn = sourceColumn;
    m_TransColumn = transColumn;
}

bool ExcelRW::ImportFromXlsx(QList<TranslateModel> &list, QString strPath)
{
    bool bSuccess = true;
    //    int nErrLine = 1;
    list.clear();

    QString strKey, strSource, strTranslate;
    QXlsx::CellRange cellRange;

    QXlsx::Document* m_pDoc = new QXlsx::Document(strPath);
    if(nullptr == m_pDoc)
    {
        return bSuccess;
    }
    if(m_pDoc->sheetNames().isEmpty())
    {
        return bSuccess;
    }
    m_pDoc->selectSheet(m_pDoc->sheetNames().first());
    cellRange = m_pDoc->currentWorksheet()->dimension();

    do
    {
        if(1 == cellRange.lastRow() && (nullptr == m_pDoc->cellAt(cellRange.lastRow(), cellRange.lastColumn())))
        {
            bSuccess = true;
            break;
        }

        for(int i = 2; i<=cellRange.lastRow(); i++)
        {
            m_TotalCount++;

            if(m_pDoc->cellAt(i, m_KeyColumn) == nullptr)
            {
                strKey = "";
            }else
            {
                strKey = m_pDoc->cellAt(i, m_KeyColumn)->value().toString();
            }

            if(m_pDoc->cellAt(i, m_SourceColumn) == nullptr)
            {
                strSource = "";
            }else
            {
                strSource = m_pDoc->cellAt(i, m_SourceColumn)->value().toString();
            }

            if(m_pDoc->cellAt(i, m_TransColumn) == nullptr)
            {
                strTranslate = "";
            }else
            {
                strTranslate = m_pDoc->cellAt(i, m_TransColumn)->value().toString();
            }

            //            nErrLine = i;

            //            if(strSite.isEmpty())
            //            {
            //                addError(EXCEL_EMPTY_ERROR, nErrLine);
            //                bSuccess = false;
            //                continue;
            //            }


            TranslateModel model;
            model.SetKey(strKey);
            //model.SetSource(strSource);
            model.SetSource(strKey);
            model.SetTranslate(strTranslate);
            list.append(model);
        }
    }while(0);

    delete m_pDoc;
    return bSuccess;
}

bool ExcelRW::ExportToXlsx(QList<TranslateModel>& list, QString strPath)
{
    if(strPath.isEmpty()) {
        emit error("Export path cannot be empty");
        return false;
    }

    if (list.count() <= 0)
    {
        emit error("*.ts file is empty");
        return false;
    }
    m_success_count = 0;
    QString strHeaderkey = tr("Key");
    QString strHeaderSource = tr("Source");
    QString strHeaderTranslate = tr("Translate");

    QXlsx::Document xlsx;
    xlsx.addSheet("Sheet1");
    xlsx.write(1, m_KeyColumn, QVariant(strHeaderkey));
    xlsx.write(1, m_SourceColumn, QVariant(strHeaderSource));
    xlsx.write(1, m_TransColumn, QVariant(strHeaderTranslate));
    xlsx.setColumnWidth(2, 3, 90); // ??1???100??????15??
    xlsx.setColumnHidden(1, 1, true); // ??Key??
    // ????????
    QXlsx::Format wrapFormat;
    wrapFormat.setTextWarp(true);

    // Separate rows: translated (non-empty) first, untranslated (empty) last
    QList<int> translatedRows;
    QList<int> untranslatedRows;
    for(int i=0; i < list.count(); i++)
    {
        if(list[i].GetTranslate().trimmed().isEmpty())
            untranslatedRows.append(i);
        else
            translatedRows.append(i);
    }
    QList<int> sortedRows = translatedRows + untranslatedRows;

    qDebug() << "ExportToXlsx: total=" << list.count()
             << "translated=" << translatedRows.count()
             << "untranslated=" << untranslatedRows.count();

    for(int idx=0; idx < sortedRows.count(); idx++)
    {
        int i = sortedRows[idx];
        for(int j=1; j<=3; j++){
            xlsx.write(idx+2, m_KeyColumn, QVariant(list[i].GetKey()));
            xlsx.write(idx+2, m_SourceColumn, QVariant(list[i].GetSource()),wrapFormat);
            xlsx.write(idx+2, m_TransColumn, QVariant(CheckFormat(list[i].GetTranslate())),wrapFormat);
        }
        if(!list[i].GetTranslate().trimmed().isEmpty())
            m_success_count++;
        else
            qDebug() << "Excel empty translate row" << idx+2 << "key:" << list[i].GetKey();
        xlsx.setRowHeight(idx+2, list.count() + 10, calculateRowHeight(QString(list[i].GetSource()), QString(list[i].GetTranslate())));
    }

    xlsx.saveAs(strPath);
    qDebug() << "EXCEL SIZE " << m_success_count;
    return true;
}

void ExcelRW::SetTransColumn(int column)
{
    m_TransColumn = column;
}

bool ExcelRW::checkAccountName(QString string)
{
    if(string.isEmpty())
    {
        return false;
    }

    QRegExp regp("^[a-zA-Z_0-9]+$");
    QRegExpValidator validator(regp,nullptr);
    int pos = 0;
    if(QValidator::Acceptable != validator.validate(string, pos)){
        return false;
    }

    return true;
}

bool ExcelRW::checkPassword(QString string)
{
    if(string.isEmpty())
    {
        return false;
    }

    QRegExp regp("^[\\x21-\\x7E]+$");
    QRegExpValidator validator(regp, this);
    int pos = 0;
    if(QValidator::Acceptable != validator.validate(string, pos)){
        return false;
    }
    return true;
}

bool ExcelRW::checkIsNumber(QString string)
{
    if(string.isEmpty())
    {
        return false;
    }
    QRegExp regp("^[0-9]+$");
    QRegExpValidator validator(regp,nullptr);
    int pos=0;
    if(QValidator::Acceptable != validator.validate(string,pos))
    {
        return false;
    }
    return true;
}

bool ExcelRW::checkWebSite(QString site)
{
    if(site.isEmpty())
    {
        return false;
    }
    QRegExp regp("^([a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?\\.)+[a-zA-Z]{2,6}$");
    QRegExpValidator validator(regp,nullptr);
    int pos=0;
    if(QValidator::Acceptable != validator.validate(site, pos))
    {
        return false;
    }

    return true;
}

QString ExcelRW::CheckFormat(const QString &Str)
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

double ExcelRW::calculateRowHeight(const QString& text, const QString& text_trans, double columnWidthChars, double fontSize, double minHeight)
{
    // ???????????Excel?1???7??
    double columnWidthPoints = columnWidthChars * 7.0;

    // ??????????????
    bool hasChinese = text.contains(QRegExp("[\\u4e00-\\u9fa5]"));
    double avgCharWidth = fontSize * (hasChinese ? 1.2 : 0.6);

    // ???????????
    double charsPerLine = columnWidthPoints / avgCharWidth;

    // ??????????1??
    int lines = qMax(1, qCeil(text.length() / charsPerLine));
    int endlines = qMax(lines, qCeil(text_trans.length() / charsPerLine));

    // ?????????????????1.3??
    double requiredHeight = endlines * fontSize * 1.3;

    // ???????????
    return qMax(minHeight, requiredHeight);
}
