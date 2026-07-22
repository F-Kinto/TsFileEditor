#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QMap>
#include <QMouseEvent>
#include <QPainter>
#include <QLabel>
#include <QTextEdit>
#include <QTimer>
#include <QPushButton>
#include <QProgressBar>

#include "DataModel/TranslateModel.h"

namespace Ui {
class MainWindow;
}

class XmlRW;
class ExcelRW;
class TranslateWorker;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void on_tsLookBtn_clicked();
    void on_excelLookBtn_clicked();
    void on_generateBtn_clicked();
    void on_tsUpdateBtn_clicked();
    void on_translateBtn_clicked();
    void on_tsImportBtn_clicked();

    void onComboBoxChanged(int);
    void onReceiveMsg(const QString& msg);

    void on_tsDirLookBtn_clicked();
    void on_excelDirBtn_clicked();
    void on_generateBtn_2_clicked();
    void on_tsUpdateBtn_2_clicked();
    void on_scanUpdateBtn_clicked();
    void on_writeTsGenQmBtn_clicked();
    void on_scanTsBtn_clicked();
    void on_genQmBtn_clicked();
    void on_scanTsLookBtn_clicked();
    void on_genQmLookBtn_clicked();
    void slotquickModeChange();

private:
    Ui::MainWindow*         ui;

    QList<TranslateModel>  m_transList;

    QString                 m_toLanguage;

    XmlRW*                  m_pXmlWorker;
    ExcelRW*                m_pExcelWorker;
    TranslateWorker*        m_pTranslateWorker;

    QMap<QString, int>      m_tsColumnMap;

    bool                    m_dragging;
    bool                    m_opeMergeStepOne{false};
    bool                    m_opeMergeStepTwo{false};
    QPoint                  m_dragPosition;

    QLabel*                 m_toastLabel;
    QTimer*                 m_toastTimer;
    QTabWidget*             m_tabWidget;
    QPushButton*            m_aiTranslateBtn;
    QProgressBar*           m_progressBar;
    QLabel*                 m_progressLabel;
    QWidget*                m_titleBarWidget;
    QWidget*                m_overlayWidget;
    QTextEdit*              m_scanTsPathEdit;
    QTextEdit*              m_genQmPathEdit;
    QPushButton*            m_scanTsLookBtn;
    QPushButton*            m_genQmLookBtn;
    QPushButton*            m_scanTsBtn;
    QPushButton*            m_genQmBtn;
    QPushButton*            m_quickModeBtn;
    QPushButton*            m_singleQuickModeBtn;
    QPushButton*            m_singleMergeStep12Btn;
    QPushButton*            m_singleMergeStep34Btn;
    QLabel*                 m_genQmTipLabel1;
    QLabel*                 m_scanTsTipLabel1;
    QLabel*                 m_scanTsTipLabel;
    QLabel*                 m_genQmTipLabel;
    bool                    m_quickMode{true};

    void readConfig();
    void saveConfig();
    int getColumnIndex(const QString& columnName);
    void showToast(const QString& msg, bool success);
    void showScriptError(const QString& title, const QString& output);
    void showProgress(bool show, const QString& labelText = "", int maximum = 0);
    void applyStyles();
    QLabel* createFlagLabel(int width = 288, int height = 192);
    QLabel* createPatriotLabel();
};
#endif // MAINWINDOW_H
