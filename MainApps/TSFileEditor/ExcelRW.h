#ifndef EXCELRW_H
#define EXCELRW_H

#include <QObject>
#include <LibXlsxRW/LibXlsxRW.h>
#include "DataModel/TranslateModel.h"

#define MAX_ERROR_COUNT 10
#define MAX_GROUP 16
#define MAX_GROUP_LENGTH 2

class ExcelRW : public QObject
{
    Q_OBJECT
public:
    enum ERROR_TYPE{
        EXCEL_EMPTY_ERROR = 0,
    };

public:
    explicit ExcelRW(int keyColumn = 1, int sourceColumn = 2, int transColumn = 3, QObject *parent = 0);

    bool  ImportFromXlsx(QList<TranslateModel>& list, QString strPath);
    bool  ExportToXlsx(QList<TranslateModel>& list, QString strPath);
    bool  ExportToXlsxMultiColumn(const QMap<QString, QList<TranslateModel>>& tsDataMap, const QMap<QString, int>& tsColumnMap, QString strPath);
    void  SetTransColumn(int column);

signals:
    void error(const QString& msg);

public slots:

private:
    bool checkAccountName(QString string);
    bool checkPassword(QString string);
    bool checkIsNumber(QString string);
    bool checkWebSite(QString site);
    QString CheckFormat(const QString &Str);
    double calculateRowHeight(const QString& text, const QString& text_trans,double columnWidthChars = 90, double fontSize = 10.0, double minHeight = 20.0);

    int m_TotalCount;

    int m_KeyColumn;
    int m_SourceColumn;
    int m_TransColumn;
    int m_success_count = 0;
};

#endif // EXCELRW_H
