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

    qDebug() << "[ImportFromXlsx] Start, path:" << strPath
             << "KeyCol:" << m_KeyColumn << "SourceCol:" << m_SourceColumn << "TransCol:" << m_TransColumn;

    QXlsx::Document* m_pDoc = new QXlsx::Document(strPath);
    if(nullptr == m_pDoc)
    {
        qCritical() << "[ImportFromXlsx] Failed to create Document object";
        return bSuccess;
    }
    if(m_pDoc->sheetNames().isEmpty())
    {
        qCritical() << "[ImportFromXlsx] No sheets found in Excel file";
        return bSuccess;
    }
    m_pDoc->selectSheet(m_pDoc->sheetNames().first());
    cellRange = m_pDoc->currentWorksheet()->dimension();

    qDebug() << "[ImportFromXlsx] Sheet:" << m_pDoc->sheetNames().first()
             << "Range:" << cellRange.toString() << "rows:" << cellRange.lastRow() << "cols:" << cellRange.lastColumn();

    do
    {
        if(1 == cellRange.lastRow() && (nullptr == m_pDoc->cellAt(cellRange.lastRow(), cellRange.lastColumn())))
        {
            qDebug() << "[ImportFromXlsx] Empty sheet, only 1 row";
            bSuccess = true;
            break;
        }

        int readCount = 0;
        int emptyTransCount = 0;
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
            readCount++;
            if(strTranslate.trimmed().isEmpty())
                emptyTransCount++;
        }
        qDebug() << "[ImportFromXlsx] Read completed, total rows:" << readCount
                 << "empty translate:" << emptyTransCount << "list size:" << list.size();
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

    // Open existing file if it exists (preserves row 1 content), otherwise create new
    QXlsx::Document xlsx(strPath);
    if (xlsx.sheetNames().isEmpty())
        xlsx.addSheet("Sheet1");
    xlsx.selectSheet(xlsx.sheetNames().first());
    // Row 1 is preserved as-is, headers start from row 2
    xlsx.write(2, m_KeyColumn, QVariant(strHeaderkey));
    xlsx.write(2, m_SourceColumn, QVariant(strHeaderSource));
    xlsx.write(2, m_TransColumn, QVariant(strHeaderTranslate));
    xlsx.setColumnWidth(2, 3, 90);
    xlsx.setColumnHidden(1, 1, true);
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
        int row = idx + 3; // data starts from row 3 (row 1 empty, row 2 header)
        for(int j=1; j<=3; j++){
            xlsx.write(row, m_KeyColumn, QVariant(list[i].GetKey()));
            xlsx.write(row, m_SourceColumn, QVariant(list[i].GetSource()),wrapFormat);
            xlsx.write(row, m_TransColumn, QVariant(CheckFormat(list[i].GetTranslate())),wrapFormat);
        }
        if(!list[i].GetTranslate().trimmed().isEmpty())
            m_success_count++;
        else
            qDebug() << "Excel empty translate row" << row << "key:" << list[i].GetKey();
        xlsx.setRowHeight(row, list.count() + 10, calculateRowHeight(QString(list[i].GetSource()), QString(list[i].GetTranslate())));
    }

    // Set row height to 30 for all rows
    int totalRows = sortedRows.count() + 2; // +2 for empty row 1 and header row 2
    xlsx.setRowHeight(1, totalRows, 30);

    xlsx.saveAs(strPath);
    qDebug() << "EXCEL SIZE " << m_success_count;
    return true;
}

void ExcelRW::SetTransColumn(int column)
{
    m_TransColumn = column;
}

bool ExcelRW::ExportToXlsxMultiColumn(const QMap<QString, QList<TranslateModel>>& tsDataMap, const QMap<QString, int>& tsColumnMap, QString strPath)
{
    if(strPath.isEmpty()) {
        emit error("Export path cannot be empty");
        return false;
    }

    if (tsDataMap.isEmpty()) {
        emit error("No ts data to export");
        return false;
    }

    // Use the first ts file's data as the base for key/source rows
    QString firstTsKey = tsDataMap.keys().first();
    const QList<TranslateModel>& baseList = tsDataMap[firstTsKey];
    if (baseList.isEmpty()) {
        emit error("Base ts file is empty");
        return false;
    }

    // Open existing file if it exists (preserves row 1 content), otherwise create new
    QXlsx::Document xlsx(strPath);
    if (xlsx.sheetNames().isEmpty())
        xlsx.addSheet("Sheet1");
    xlsx.selectSheet(xlsx.sheetNames().first());

    // Row 1 is preserved as-is, headers start from row 2
    xlsx.write(2, 1, QVariant(tr("Key")));
    xlsx.write(2, 2, QVariant(tr("Source")));

    // Build reverse map: column -> ts filename for header writing
    QMap<int, QString> columnHeaderMap;
    for (auto it = tsColumnMap.constBegin(); it != tsColumnMap.constEnd(); ++it) {
        if (tsDataMap.contains(it.key())) {
            columnHeaderMap[it.value()] = it.key();
        }
    }
    for (auto it = columnHeaderMap.constBegin(); it != columnHeaderMap.constEnd(); ++it) {
        QString header = it.value();
        header.remove(".ts", Qt::CaseInsensitive);
        xlsx.write(2, it.key(), QVariant(header));
    }

    // Set column widths to 40 for all columns
    int maxCol = 2;
    for (auto it = columnHeaderMap.constBegin(); it != columnHeaderMap.constEnd(); ++it) {
        if (it.key() > maxCol) maxCol = it.key();
    }
    for (int c = 1; c <= maxCol; ++c) {
        xlsx.setColumnWidth(c, c, 40);
    }
    xlsx.setColumnHidden(1, 1, true);

    QXlsx::Format wrapFormat;
    wrapFormat.setTextWarp(true);

    // Find zh_CN column for sorting: rows with empty zh_CN translation go last
    int zhCnCol = 3; // default
    for (auto it = tsColumnMap.constBegin(); it != tsColumnMap.constEnd(); ++it) {
        if (it.key().contains("VmsTranslator_zh_CN")) {
            zhCnCol = it.value();
            break;
        }
    }

    // Build zh_CN key->translate map
    QMap<QString, QString> zhCnTransMap;
    for (auto it = tsDataMap.constBegin(); it != tsDataMap.constEnd(); ++it) {
        if (it.key().contains("VmsTranslator_zh_CN")) {
            for (const TranslateModel& model : it.value()) {
                zhCnTransMap[model.GetKey()] = model.GetTranslate();
            }
            break;
        }
    }

    // Separate rows: zh_CN non-empty first, empty last
    QList<int> translatedRows;
    QList<int> untranslatedRows;
    for (int i = 0; i < baseList.count(); ++i) {
        QString zhTrans = zhCnTransMap.value(baseList[i].GetKey(), "");
        if (zhTrans.trimmed().isEmpty())
            untranslatedRows.append(i);
        else
            translatedRows.append(i);
    }
    QList<int> sortedRows = translatedRows + untranslatedRows;

    // Write base key/source data in sorted order
    for (int idx = 0; idx < sortedRows.count(); ++idx) {
        int i = sortedRows[idx];
        int row = idx + 3; // data starts from row 3 (row 1 empty, row 2 header)
        xlsx.write(row, 1, QVariant(baseList[i].GetKey()));
        xlsx.write(row, 2, QVariant(baseList[i].GetSource()), wrapFormat);
    }

    // Write translations from each ts file into its corresponding column
    for (auto it = tsDataMap.constBegin(); it != tsDataMap.constEnd(); ++it) {
        QString tsFileName = it.key();
        if (!tsColumnMap.contains(tsFileName)) continue;

        int col = tsColumnMap[tsFileName];
        const QList<TranslateModel>& transList = it.value();

        // Build a map from key -> translate for quick lookup
        QMap<QString, QString> keyToTrans;
        for (const TranslateModel& model : transList) {
            keyToTrans[model.GetKey()] = model.GetTranslate();
        }

        // Write translations matching by key, in sorted order
        for (int idx = 0; idx < sortedRows.count(); ++idx) {
            int i = sortedRows[idx];
            int row = idx + 3; // data starts from row 3 (row 1 empty, row 2 header)
            QString key = baseList[i].GetKey();
            QString trans = keyToTrans.value(key, "");
            xlsx.write(row, col, QVariant(CheckFormat(trans)), wrapFormat);
        }
    }

    // Set row height to 30 for all rows
    int totalRows = sortedRows.count() + 2; // +2 for empty row 1 and header row 2
    xlsx.setRowHeight(1, totalRows, 30);

    xlsx.saveAs(strPath);
    return true;
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
