#pragma execution_character_set("utf-8")
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "XmlRW.h"
#include "ExcelRW.h"
#include "TranslateWorker.h"

#include <QStandardPaths>
#include <QFileDialog>
#include <QCoreApplication>
#include <QProcess>
#include <QListView>
#include <QSslSocket>
#include <QScreen>
#include <QWindow>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QMessageBox>
#include <QShortcut>
#include "ScriptErrorDialog.h"
#include "HelpDialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ========== 创建标题栏控件 ==========
    QPushButton *helpBtn = new QPushButton("?");
    helpBtn->setFixedSize(32, 32);
    helpBtn->setToolTip(tr("帮助"));
    connect(helpBtn, &QPushButton::clicked, this, [this]() {
        HelpDialog *dlg = new HelpDialog(this);
        dlg->show();
        dlg->move(this->geometry().center() - dlg->rect().center());
    });

    QPushButton *minimizeBtn = new QPushButton("—");
    minimizeBtn->setFixedSize(32, 32);
    minimizeBtn->setToolTip(tr("最小化"));
    connect(minimizeBtn, &QPushButton::clicked, this, &MainWindow::showMinimized);

    QPushButton *closeBtn = new QPushButton("X");
    closeBtn->setFixedSize(32, 32);
    closeBtn->setToolTip(tr("关闭"));
    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::close);

    m_titleBarWidget = new QWidget();
    m_titleBarWidget->setFixedHeight(48);
    QHBoxLayout *titleBarLayout = new QHBoxLayout(m_titleBarWidget);
    titleBarLayout->setContentsMargins(24, 0, 24, 0);
    QLabel *titleLabel = new QLabel(tr("自动翻译Tool"));
    titleBarLayout->addWidget(titleLabel, 0, Qt::AlignVCenter);
    titleBarLayout->addStretch();
    titleBarLayout->addWidget(helpBtn, 0, Qt::AlignVCenter);
    titleBarLayout->addWidget(minimizeBtn, 0, Qt::AlignVCenter);
    titleBarLayout->addWidget(closeBtn, 0, Qt::AlignVCenter);

    // ========== 创建Tab页面和布局 ==========
    m_tabWidget = new QTabWidget();

    QWidget *pageSingle = new QWidget();
    QVBoxLayout *pageSingleLayout = new QVBoxLayout(pageSingle);
    pageSingleLayout->addWidget(ui->groupBox);
    pageSingleLayout->addStretch();

    QVBoxLayout *groupBoxLayout = qobject_cast<QVBoxLayout*>(ui->groupBox->layout());
    ui->groupBox->setTitle(tr("翻译一种语言"));

    QVBoxLayout *groupBox3Layout = qobject_cast<QVBoxLayout*>(ui->groupBox_3->layout());
    ui->groupBox_3->setTitle(tr("翻译多种语言"));
    groupBox3Layout->addStretch();

    // 移除旧的按钮布局
    QLayoutItem *oldButtonsItem = nullptr;
    for (int i = 0; i < groupBoxLayout->count(); ++i) {
        QLayoutItem *item = groupBoxLayout->itemAt(i);
        if (item->layout() && item->layout()->objectName() == "verticalLayout_buttons") {
            oldButtonsItem = groupBoxLayout->takeAt(i);
            break;
        }
    }

    // 创建AI翻译按钮
    m_aiTranslateBtn = new QPushButton(tr("使用有道AI翻译"));
    m_aiTranslateBtn->setCheckable(true);
    m_aiTranslateBtn->setFixedWidth(250);

    // 创建单语言快捷模式按钮
    m_singleQuickModeBtn = new QPushButton(tr("切换为快捷模式"));
    m_singleQuickModeBtn->setFixedSize(250, 32);
    connect(m_singleQuickModeBtn, &QPushButton::clicked, this, &MainWindow::slotquickModeChange);

    // 创建单语言合并按钮（快捷模式下显示）
    m_singleMergeStep12Btn = new QPushButton(tr("Step2-1,2: 扫描导入Ts 生成Excel"));
    m_singleMergeStep12Btn->setFixedSize(250, 32);
    m_singleMergeStep12Btn->setVisible(false);
    connect(m_singleMergeStep12Btn, &QPushButton::clicked, this, [this]() {
        QString scriptPath = m_scanTsPathEdit->toPlainText();
        if (scriptPath.isEmpty()) {
            onReceiveMsg("请先导入扫描Ts脚本路径");
            return;
        }
        QFileInfo scriptInfo(scriptPath);
        if (!scriptInfo.exists()) {
            onReceiveMsg("脚本文件不存在: " + scriptPath);
            return;
        }
        QFileInfo info(ui->tsPathEdit->text());
        if (ui->tsPathEdit->text().isEmpty() || !info.isFile() || "ts" != info.suffix()){
            onReceiveMsg("请输入正确的Ts文件路径");
            return;
        }
        m_opeMergeStepOne = true;
        on_scanTsBtn_clicked();
    });

    m_singleMergeStep34Btn = new QPushButton(tr("Step3,4: 译文写入Ts 更新Qm"));
    m_singleMergeStep34Btn->setFixedSize(250, 32);
    m_singleMergeStep34Btn->setVisible(false);
    connect(m_singleMergeStep34Btn, &QPushButton::clicked, this, [this]() {
        if(ui->excelPathEdit->text().isEmpty() || ui->tsPathEdit->text().isEmpty()) {
            onReceiveMsg("请输入正确的Excel文件路径");
            return;
        }
        QString scriptPath = m_genQmPathEdit->toPlainText();
        if (scriptPath.isEmpty()) {
            onReceiveMsg("请先导入生成Qm脚本路径");
            return;
        }
        QFileInfo scriptInfo(scriptPath);
        if (!scriptInfo.exists()) {
            onReceiveMsg("脚本文件不存在: " + scriptPath);
            return;
        }
        m_opeMergeStepTwo = true;
        on_tsUpdateBtn_clicked();
    });

    QVBoxLayout *buttonsLayout = qobject_cast<QVBoxLayout*>(oldButtonsItem ? oldButtonsItem->layout() : nullptr);
    if (buttonsLayout) {
        buttonsLayout->insertWidget(0, m_singleQuickModeBtn, 0, Qt::AlignHCenter);
        buttonsLayout->addWidget(m_singleMergeStep12Btn, 0, Qt::AlignHCenter);
        buttonsLayout->addWidget(m_singleMergeStep34Btn, 0, Qt::AlignHCenter);
        buttonsLayout->addWidget(m_aiTranslateBtn, 0, Qt::AlignHCenter);
    }

    // 底部水平布局：左侧按钮 + 右侧有道翻译
    QHBoxLayout *bottomHLayout = new QHBoxLayout();
    bottomHLayout->setSpacing(24);

    QWidget *buttonsContainer = new QWidget();
    QVBoxLayout *buttonsContainerLayout = new QVBoxLayout(buttonsContainer);
    buttonsContainerLayout->setContentsMargins(0, 0, 0, 0);
    buttonsContainerLayout->setAlignment(Qt::AlignCenter);
    buttonsContainerLayout->addLayout(buttonsLayout);

    bottomHLayout->addWidget(buttonsContainer, 33);
    bottomHLayout->addWidget(ui->groupBox_2, 67);

    ui->groupBox_2->setVisible(false);

    groupBoxLayout->addLayout(bottomHLayout);
    groupBoxLayout->addStretch();

    connect(m_aiTranslateBtn, &QPushButton::toggled, this, [this](bool checked) {
        ui->groupBox_2->setVisible(checked);
        m_aiTranslateBtn->setText(checked ? tr("收起有道AI翻译") : tr("使用有道AI翻译"));
    });

    // 批量翻译页面
    QWidget *pageBatch = new QWidget();
    QVBoxLayout *pageBatchLayout = new QVBoxLayout(pageBatch);
    pageBatchLayout->addWidget(ui->groupBox_3);

    // 底部爱国标语：五颗黄色五角星 + "我爱中国!"
    QLabel *patriotLabel = new QLabel();
    patriotLabel->setAlignment(Qt::AlignCenter);
    patriotLabel->setText(QString(
        "<span style='font-size:28px; font-weight:bold; color:#FFDE00;'>"
        "\u2605 \u2605 \u2605 \u2605 \u2605"
        "</span>"
        "<span style='font-size:28px; font-weight:bold; color:#DE2910; font-family:\"Microsoft YaHei\";'>"
        " 我爱中国! "
        "</span>"
        "<span style='font-size:28px; font-weight:bold; color:#FFDE00;'>"
        "\u2605 \u2605 \u2605 \u2605 \u2605"
        "</span>"
    ));
    pageBatchLayout->addWidget(patriotLabel);
    pageBatchLayout->addStretch();

    m_tabWidget->addTab(pageBatch, tr("翻译多种语言"));
    m_tabWidget->addTab(pageSingle, tr("翻译一种语言"));

    // 替换中央布局
    QLayout *oldLayout = ui->centralWidget->layout();
    if (oldLayout) {
        // 从旧布局中取出所有items，保护widget不被删除
        QList<QLayoutItem*> items;
        while (oldLayout->count() > 0) {
            items.append(oldLayout->takeAt(0));
        }
        // 删除旧布局（此时布局已空，不会连带删除widget）
        delete oldLayout;
        // 清理残留的layout items（不含widget的子布局）
        for (QLayoutItem *item : items) {
            if (item->layout()) {
                // 子布局中的widget已经被重新添加到新布局，只删除空布局
                // 注意：不删除QWidgetItem，因为widget已被新layout接管
            }
            delete item; // QWidgetItem的delete不会删除widget本身
        }
    }

    // 右侧按钮面板：输入框 + 浏览按钮 + 功能按钮
    // 扫描Ts脚本路径输入框（不可复制粘贴，只能通过浏览导入）
    QLabel *scanTsTipLabel = new QLabel(tr("* 如果项目有 新增/删除 需要翻译的字段, 必须先执行此步骤"));
    scanTsTipLabel->setWordWrap(true);
    scanTsTipLabel->setContentsMargins(8, 0, 8, 0);
    scanTsTipLabel->setStyleSheet("QLabel { color: #FF4A4A; font-family: 'Microsoft YaHei'; font-size: 12px; }");
    m_scanTsTipLabel1 = new QLabel(tr("请先导入扫描Ts脚本路径，再点击下方按钮扫描项目文件并更新Ts内容"));
    m_scanTsTipLabel1->setWordWrap(true);
    m_scanTsTipLabel1->setContentsMargins(8, 0, 8, 0);
    m_scanTsTipLabel1->setStyleSheet("QLabel { color: #6D7682; font-family: 'Microsoft YaHei'; font-size: 12px; }");

    m_scanTsPathEdit = new QTextEdit();
    m_scanTsPathEdit->setPlaceholderText(tr("请导入更新Ts文件的脚本路径(updateTsFile.bat)"));
    m_scanTsPathEdit->setReadOnly(true);
    m_scanTsPathEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    m_scanTsPathEdit->setMaximumHeight(60);
    m_scanTsPathEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scanTsPathEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scanTsLookBtn = new QPushButton(tr("导入Ts脚本路径"));
    m_scanTsLookBtn->setFixedSize(225, 36);
    connect(m_scanTsLookBtn, &QPushButton::clicked, this, &MainWindow::on_scanTsLookBtn_clicked);

    QVBoxLayout *scanTsRowLayout = new QVBoxLayout();
    scanTsRowLayout->setSpacing(8);
    scanTsRowLayout->addWidget(scanTsTipLabel);
    scanTsRowLayout->addWidget(m_scanTsPathEdit);
    scanTsRowLayout->addWidget(m_scanTsLookBtn, 0, Qt::AlignHCenter);

    m_scanTsBtn = new QPushButton(tr("开始扫描项目并更新Ts文件"));
    m_scanTsBtn->setFixedSize(225, 36);
    connect(m_scanTsBtn, &QPushButton::clicked, this, &MainWindow::on_scanTsBtn_clicked);

    // 生成Qm脚本路径输入框（不可复制粘贴，只能通过浏览导入）
    QLabel *genQmTipLabel = new QLabel(tr("* 必须执行完左边步骤之后, 必须执行此步骤才可让翻译生效"));
    genQmTipLabel->setWordWrap(true);
    genQmTipLabel->setContentsMargins(8, 0, 8, 0);
    genQmTipLabel->setStyleSheet("QLabel { color: #FF4A4A; font-family: 'Microsoft YaHei'; font-size: 12px; }");
    m_genQmTipLabel1 = new QLabel(tr("请先导入生成Qm脚本路径，再点击下方按钮将Ts文件编译为Qm翻译文件"));
    m_genQmTipLabel1->setWordWrap(true);
    m_genQmTipLabel1->setContentsMargins(8, 0, 8, 0);
    m_genQmTipLabel1->setStyleSheet("QLabel { color: #6D7682; font-family: 'Microsoft YaHei'; font-size: 12px; }");

    m_genQmPathEdit = new QTextEdit();
    m_genQmPathEdit->setPlaceholderText(tr("请导入生成Qm文件的脚本路径(updateQmFile.bat)"));
    m_genQmPathEdit->setReadOnly(true);
    m_genQmPathEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    m_genQmPathEdit->setMaximumHeight(60);
    m_genQmPathEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_genQmPathEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_genQmLookBtn = new QPushButton(tr("导入Qm脚本路径"));
    m_genQmLookBtn->setFixedSize(225, 36);
    connect(m_genQmLookBtn, &QPushButton::clicked, this, &MainWindow::on_genQmLookBtn_clicked);

    QVBoxLayout *genQmRowLayout = new QVBoxLayout();
    genQmRowLayout->setSpacing(8);
    genQmRowLayout->addWidget(genQmTipLabel);
    genQmRowLayout->addWidget(m_genQmPathEdit);
    genQmRowLayout->addWidget(m_genQmLookBtn, 0, Qt::AlignHCenter);

    m_genQmBtn = new QPushButton(tr("开始生成Qm翻译文件"));
    m_genQmBtn->setFixedSize(225, 36);
    connect(m_genQmBtn, &QPushButton::clicked, this, &MainWindow::on_genQmBtn_clicked);

    // 扫描Ts组：GroupBox封装，垂直布局，间距20px
    QGroupBox *scanTsGroupBox = new QGroupBox(tr("Step1:扫描Ts文件"));
    QVBoxLayout *scanTsGroupLayout = new QVBoxLayout(scanTsGroupBox);
    scanTsGroupLayout->setSpacing(10);
    scanTsGroupLayout->setContentsMargins(10, 8, 10, 8);
    scanTsGroupLayout->addLayout(scanTsRowLayout);
    scanTsGroupLayout->addStretch();
    scanTsGroupLayout->addWidget(m_scanTsTipLabel1, 0, Qt::AlignHCenter);
    scanTsGroupLayout->addWidget(m_scanTsBtn, 0, Qt::AlignHCenter);

    // 生成Qm组：GroupBox封装，垂直布局，间距20px
    QGroupBox *genQmGroupBox = new QGroupBox(tr("Step4:生成Qm文件"));
    QVBoxLayout *genQmGroupLayout = new QVBoxLayout(genQmGroupBox);
    genQmGroupLayout->setSpacing(10);
    genQmGroupLayout->setContentsMargins(10, 8, 10, 8);
    genQmGroupLayout->addLayout(genQmRowLayout);
    genQmGroupLayout->addStretch();
    genQmGroupLayout->addWidget(m_genQmTipLabel1, 0, Qt::AlignHCenter);
    genQmGroupLayout->addWidget(m_genQmBtn, 0, Qt::AlignHCenter);

    // 两个GroupBox各占一半高度
    QVBoxLayout *sideBtnLayout = new QVBoxLayout();
    sideBtnLayout->setSpacing(16);
    sideBtnLayout->setContentsMargins(0, 0, 16, 0);
    sideBtnLayout->addWidget(scanTsGroupBox, 1);
    sideBtnLayout->addWidget(genQmGroupBox, 1);

    // 水平布局：tabWidget占3/4，按钮面板占1/4
    QHBoxLayout *contentHLayout = new QHBoxLayout();
    contentHLayout->setSpacing(16);
    contentHLayout->addWidget(m_tabWidget, 75);
    contentHLayout->addLayout(sideBtnLayout, 25);

    QVBoxLayout *newLayout = new QVBoxLayout(ui->centralWidget);
    newLayout->setContentsMargins(1, 1, 1, 1);
    newLayout->setSpacing(16);
    newLayout->addWidget(m_titleBarWidget);
    newLayout->addLayout(contentHLayout, 1);

    // 无边框窗口
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // ========== 初始化UI组件（必须在applyStyles之前） ==========
    m_dragging = false;

    m_toastLabel = new QLabel(this);
    m_toastLabel->setAlignment(Qt::AlignCenter);
    m_toastLabel->setWordWrap(true);
    m_toastLabel->hide();
    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);
    connect(m_toastTimer, &QTimer::timeout, this, [this]() { m_toastLabel->hide(); });

    m_progressBar = new QProgressBar(this);
    m_progressBar->setAlignment(Qt::AlignCenter);
    m_progressBar->setFixedSize(500, 16);
    m_progressBar->hide();

    // 进度标签：显示在进度条上方，显示当前步骤信息
    m_progressLabel = new QLabel(this);
    m_progressLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_progressLabel->setFixedSize(500, 36);
    m_progressLabel->setStyleSheet(
        "QLabel {"
        "  color: #FFFFFF;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  font-family: 'Microsoft YaHei';"
        "  background: transparent;"
        "}"
    );
    m_progressLabel->hide();

    // 遮罩层：进度条显示时覆盖窗口，半透明灰色，阻止点击
    m_overlayWidget = new QWidget(this);
    m_overlayWidget->setStyleSheet("background: rgba(0, 0, 0, 120);");
    m_overlayWidget->hide();

    // ========== 应用所有样式 ==========
    applyStyles();

    // ========== 初始化业务对象 ==========
    m_toLanguage = "en";
    m_pXmlWorker = new XmlRW(this);
    m_pExcelWorker = new ExcelRW(1, 2, 3, this);
    m_pTranslateWorker = new TranslateWorker(m_transList, this);

    ui->youdaoTipLabel->setVisible(false);

    QStandardItemModel *comboModel = new QStandardItemModel(ui->comboBox);
    QStringList comboItems = {"英文", "中文", "日语", "韩语", "法语", "俄语", "葡萄牙语", "西班牙语", "其他"};
    QStringList comboData = {"en", "zh-CN", "ja", "ko", "fr", "ru", "pt", "es", "other"};
    for (int i = 0; i < comboItems.size(); ++i) {
        QStandardItem *item = new QStandardItem(comboItems[i]);
        item->setData(comboData[i], Qt::UserRole);
        item->setTextAlignment(Qt::AlignCenter);
        comboModel->appendRow(item);
    }
    ui->comboBox->setModel(comboModel);
    setWindowTitle("Qt_TSFileEditor");

    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onComboBoxChanged);
    connect(m_pExcelWorker, &ExcelRW::error, this, &MainWindow::onReceiveMsg);
    connect(m_pTranslateWorker, &TranslateWorker::error, this, &MainWindow::onReceiveMsg);

    // 快捷模式按钮手动连接（非 on_objectName_signal 格式，不会自动关联）
    connect(ui->quickModeBtn, &QPushButton::clicked, this, &MainWindow::slotquickModeChange);
    // 初始为普通模式：显示 generateBtn_2 和 tsUpdateBtn_2，隐藏 scanUpdateBtn 和 writeTsGenQmBtn
    ui->scanUpdateBtn->setVisible(false);
    ui->writeTsGenQmBtn->setVisible(false);

    readConfig();

    // Adaptive size: width = 0.5 * screen, height = enough for all content
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    int width = qMax(600, qMin(screenGeometry.width() / 2, 900));

    // Calculate height: ensure all tab pages fit
    ui->groupBox_2->setVisible(true); // temporarily show to measure
    adjustSize();
    int fullHeight = sizeHint().height();
    ui->groupBox_2->setVisible(false); // hide again

    int height = qMax(fullHeight, 500);
    resize(width, height);
    setMinimumWidth(600);

    // Center on screen
    move(screenGeometry.center() - rect().center());

    // Re-adapt when window moves to a different screen
    connect(windowHandle(), &QWindow::screenChanged, this, [this](QScreen *newScreen) {
        if (!newScreen) return;
        QRect sg = newScreen->availableGeometry();
        int w = qMax(600, qMin(sg.width() / 2, 900));
        resize(w, this->height());
        move(sg.center() - rect().center());
    });
}

MainWindow::~MainWindow()
{
    saveConfig();

    delete ui;
}

void MainWindow::on_tsLookBtn_clicked()
{
    const QString documentLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString fileName = QFileDialog::getOpenFileName(this, tr("请选择Ts文件"), documentLocation, "Files (*.ts)");

    if(fileName.isEmpty()){
        return;
    }

    ui->tsPathEdit->setText(fileName);
    on_tsImportBtn_clicked();
}

void MainWindow::on_excelLookBtn_clicked()
{
    const QString documentLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString fileName = QFileDialog::getOpenFileName(this, tr("请选择Excel文件"), documentLocation, "Files (*.xlsx)");

    if(fileName.isEmpty()){
        return;
    }
    else{
        QFileInfo info(fileName);
        if ("xlsx" != info.suffix()){
            onReceiveMsg("文件格式不支持, 请确认文件是否存在");
            return;
        }
    }

    ui->excelPathEdit->setText(fileName);
}

void MainWindow::on_generateBtn_clicked()
{
    bool re;

    if(m_opeMergeStepOne){
        m_opeMergeStepOne = false;
    }

    m_pExcelWorker->SetTransColumn(ui->transSpinBox->value());

    //generate excel file
    if(ui->excelPathEdit->text().isEmpty()) {
        const QString documentLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        QString saveName = documentLocation + "/untitled.xlsx";
        QString fileName = QFileDialog::getSaveFileName(this, "excel file path", saveName, "Files (*.xlsx)");
        if (fileName.isEmpty())
        {
            return;
        }
        else{
            ui->excelPathEdit->setText(fileName);
        }
    }

    re = m_pExcelWorker->ExportToXlsx(m_transList, ui->excelPathEdit->text());
    if(re) {
        onReceiveMsg("Excel生成成功");
        ui->youdaoTipLabel->setVisible(false);
    } else {
        onReceiveMsg("Excel生成失败");
    }
}

void MainWindow::on_tsUpdateBtn_clicked()
{
    bool re;

    m_pExcelWorker->SetTransColumn(ui->transSpinBox->value());

    //import excel file
    if(ui->excelPathEdit->text().isEmpty()) {
        on_excelLookBtn_clicked();
    }

    re = m_pExcelWorker->ImportFromXlsx(m_transList, ui->excelPathEdit->text());
    if(re) {
        onReceiveMsg("Excel文件导入成功");
        ui->youdaoTipLabel->setVisible(false);
    } else {
        onReceiveMsg("Excel文件导入失败");
    }

    //update ts file
    if(ui->tsPathEdit->text().isEmpty()) {
        on_tsLookBtn_clicked();
    }

    re = m_pXmlWorker->ExportToTS(m_transList, ui->tsPathEdit->text());

    if(re) {
        onReceiveMsg("译文成功写入Ts文件");
        if(m_opeMergeStepTwo){
            on_genQmBtn_clicked();
        }
    } else {
        onReceiveMsg("译文写入失败");
        m_opeMergeStepTwo = false;
    }

}

void MainWindow::on_translateBtn_clicked()
{
    bool re;

    m_pExcelWorker->SetTransColumn(ui->transSpinBox->value());

    //import excel file
    if(ui->excelPathEdit->text().isEmpty()) {
        on_excelLookBtn_clicked();
    }

    re = m_pExcelWorker->ImportFromXlsx(m_transList, ui->excelPathEdit->text());
    if(re) {
        onReceiveMsg("Excel文件导入成功");
    }
    else {
        onReceiveMsg("Excel文件导入失败");
    }

    //    m_pTranslateWorker->YoudaoTranslate("??", "auto", m_toLanguage);

    //translate excel file
    m_pTranslateWorker->SetIdKey(ui->youdaoAppIdlineEdit->text(), ui->youdaoKeylineEdit->text());
    re = m_pTranslateWorker->YoudaoTranslate("auto", m_toLanguage);
    if(re) {
        onReceiveMsg("有道翻译成功");
        ui->youdaoTipLabel->setVisible(true);
    }
    else {
        onReceiveMsg("有道翻译失败");
    }
}

void MainWindow::onComboBoxChanged(int)
{
    QString langCode = ui->comboBox->currentData().toString();

    if ("other" == langCode) {
        m_toLanguage = ui->otherLineEdit->text();
    } else {
        m_toLanguage = langCode;
    }

}

void MainWindow::on_tsImportBtn_clicked()
{
    bool re;

    QFileInfo info(ui->tsPathEdit->text());
    if (ui->tsPathEdit->text().isEmpty() || !info.isFile() || "ts" != info.suffix()){
        onReceiveMsg("请输入正确的Ts文件路径");
        m_opeMergeStepOne = false;
        return;
    }

    m_transList.clear();
    re = m_pXmlWorker->ImportFromTS(m_transList, ui->tsPathEdit->text());

    if(re) {
        onReceiveMsg("Ts文件导入成功");
        if(m_opeMergeStepOne){
            on_generateBtn_clicked();
        }
    } else {
        onReceiveMsg("Ts文件导入失败");
        m_opeMergeStepOne = false;
    }
}

void MainWindow::onReceiveMsg(const QString &msg)
{
    bool success = msg.contains("成功") || msg.contains("完成");
    showToast(msg, success);
}

void MainWindow::showToast(const QString &msg, bool success)
{
    m_toastTimer->stop();
    m_toastLabel->setText(msg);

    // Max width = 2/3 of window width
    int maxW = width() * 2 / 3;
    m_toastLabel->setMaximumWidth(maxW);
    m_toastLabel->setMinimumWidth(300);

    // Success: same style as generateBtn (blue theme)
    // Failure: same style as tsUpdateBtn (red theme)
    QString style;
    if (success) {
        style =
            "QLabel {"
            "  border: 1px solid #00BA00;"
            "  border-radius: 4px;"
            "  padding: 8px 16px;"
            "  background: #E5FAE1;"
            "  color: #00BA00;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "}";
    } else {
        style =
            "QLabel {"
            "  border: 1px solid #FF4A4A;"
            "  border-radius: 4px;"
            "  padding: 8px 16px;"
            "  background: #FFF0F0;"
            "  color: #FF4A4A;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "}";
    }
    m_toastLabel->setStyleSheet(style);

    // Adjust size to fit content
    m_toastLabel->adjustSize();

    // Position at top center, below the tab bar
    int x = (width() - m_toastLabel->width()) / 2;
    int tabBarBottom = m_tabWidget->pos().y() + m_tabWidget->tabBar()->height();
    int y = tabBarBottom + 8;
    m_toastLabel->move(x, y);
    m_toastLabel->raise();
    m_toastLabel->show();

    m_toastTimer->start(3000);
}

void MainWindow::on_tsDirLookBtn_clicked()
{
    const QString documentLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString dirName = QFileDialog::getExistingDirectory(this, tr("请选择Ts目录"), documentLocation);

    if(dirName.isEmpty()){
        return;
    }

    ui->tsDirEdit->setText(dirName);
}

void MainWindow::on_excelDirBtn_clicked()
{
    const QString documentLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString fileName = QFileDialog::getOpenFileName(this, tr("请选择Excel文件"), documentLocation, "Files (*.xlsx)");

    if(fileName.isEmpty()){
        return;
    }
    else{
        QFileInfo info(fileName);
        if ("xlsx" != info.suffix()){
            onReceiveMsg("文件类型不支持,请确认文件是否存在");
            return;
        }
    }

    ui->excelDirEdit->setText(fileName);
}

void MainWindow::on_generateBtn_2_clicked()
{
    if(m_opeMergeStepOne){
        m_opeMergeStepOne = false;
    }
    QFileInfo tsDirinfo(ui->tsDirEdit->text());
    if (!tsDirinfo.isDir()){
        onReceiveMsg("Ts目录为空");
        return;
    }

    QFileInfo excelinfo(ui->excelDirEdit->text());
    if (!excelinfo.exists()){
        onReceiveMsg("Excel文件路径不存在");
        return;
    }

    QStringList filters;
    filters << QString("*.ts");
    QDir tsdir(ui->tsDirEdit->text());
    tsdir.setFilter(QDir::Files | QDir::NoSymLinks);
    tsdir.setNameFilters(filters);

    if (tsdir.count() <= 0) {
        onReceiveMsg("Ts目录下未发现Ts文件");
        return;
    }

    // 显示遮罩层和进度条
    QFileInfoList fileList = tsdir.entryInfoList();
    int total = fileList.size();
    showProgress(true, tr("正在从Ts文件读取翻译并生成Excel..."), total);

    // Read all ts files into a map: tsFileName -> TranslateModel list
    QMap<QString, QList<TranslateModel>> tsDataMap;
    int current = 0;
    for (QFileInfo info : fileList) {
        QList<TranslateModel> transList;
        bool re = m_pXmlWorker->ImportFromTS(transList, info.absoluteFilePath());
        if (re) {
            tsDataMap[info.fileName()] = transList;
        }

        current++;
        m_progressBar->setValue(current);
        QCoreApplication::processEvents();
    }

    // Export all translations into one Excel with different columns
    bool re = m_pExcelWorker->ExportToXlsxMultiColumn(tsDataMap, m_tsColumnMap, ui->excelDirEdit->text());

    showProgress(false, re ? "所有Ts文件翻译已成功写入Excel" : "Excel生成失败");
}

void MainWindow::on_tsUpdateBtn_2_clicked()
{
    bool re;

    QFileInfo tsDirinfo(ui->tsDirEdit->text());
    if (!tsDirinfo.isDir()){
        onReceiveMsg("ts目录为空");
        return;
    }

    QFileInfo excelDirinfo(ui->excelDirEdit->text());
    if (!excelDirinfo.exists()){
        onReceiveMsg("Excel文件路径不存在");
        return;
    }

    QStringList filters;
    filters << QString("*.ts");
    QDir tsdir(ui->tsDirEdit->text());
    tsdir.setFilter(QDir::Files | QDir::NoSymLinks);
    tsdir.setNameFilters(filters);

    if (tsdir.count() <= 0) {
        onReceiveMsg("Ts目录下未发现Ts文件");
        return;
    }

    // 显示进度条
    QFileInfoList fileList = tsdir.entryInfoList();
    int total = fileList.size();
    showProgress(true, tr("正在批量译文写入Ts文件..."), total);

    int current = 0;
    for (QFileInfo info : fileList) {
        qDebug() << "[BatchUpdate] Processing ts file:" << info.fileName() << "column:" << m_tsColumnMap[info.fileName()];
        QString path = info.fileName();
        if (!m_tsColumnMap.contains(info.fileName())) {
            qDebug() << "[BatchUpdate] Skipped, no column mapping for:" << info.fileName();
            current++;
            m_progressBar->setValue(current);
            QCoreApplication::processEvents();
            continue;
        }

        //import ts file
        m_transList.clear();
        re = m_pXmlWorker->ImportFromTS(m_transList, info.absoluteFilePath());
        qDebug() << "[BatchUpdate] ImportFromTS result:" << re << "list size:" << m_transList.size();

        if(!re) {
            qCritical() << "[BatchUpdate] ImportFromTS failed for:" << info.fileName();
            current++;
            m_progressBar->setValue(current);
            QCoreApplication::processEvents();
            continue;
        }

        m_pExcelWorker->SetTransColumn(m_tsColumnMap[info.fileName()]);
        re = m_pExcelWorker->ImportFromXlsx(m_transList, ui->excelDirEdit->text());
        qDebug() << "[BatchUpdate] ImportFromXlsx result:" << re << "list size:" << m_transList.size();

        if(!re) {
            qCritical() << "[BatchUpdate] ImportFromXlsx failed for:" << info.fileName();
            current++;
            m_progressBar->setValue(current);
            QCoreApplication::processEvents();
            continue;
        }

        re = m_pXmlWorker->ExportToTS(m_transList, info.absoluteFilePath());
        qDebug() << "[BatchUpdate] ExportToTS result:" << re;

        if(!re) {
            qCritical() << "[BatchUpdate] ExportToTS failed for:" << info.fileName();
            current++;
            m_progressBar->setValue(current);
            QCoreApplication::processEvents();
            continue;
        }

        current++;
        m_progressBar->setValue(current);
        QCoreApplication::processEvents(); // 刷新UI
    }

    showProgress(false, "所有Ts文件已翻译完成");
    if(m_opeMergeStepTwo){
        on_genQmBtn_clicked();
    }
}

void MainWindow::on_scanUpdateBtn_clicked()
{
    // Step 1: Scan project and update Ts files (sync)
    QString scriptPath = m_scanTsPathEdit->toPlainText();
    if (scriptPath.isEmpty()) {
        onReceiveMsg("请先导入扫描Ts脚本路径");
        return;
    }
    QFileInfo scriptInfo(scriptPath);
    if (!scriptInfo.exists()) {
        onReceiveMsg("脚本文件不存在: " + scriptPath);
        return;
    }
    QFileInfo tsDirinfo(ui->tsDirEdit->text());
    if (!tsDirinfo.isDir()){
        onReceiveMsg("Ts目录为空");
        return;
    }

    QFileInfo excelinfo(ui->excelDirEdit->text());
    if (!excelinfo.exists()){
        onReceiveMsg("Excel文件路径不存在");
        return;
    }

    // 合并扫描项目更新Ts和写入Excel操作
    m_opeMergeStepOne = true;
    on_scanTsBtn_clicked();
}

void MainWindow::on_writeTsGenQmBtn_clicked()
{
    // Step 1: Batch write translations to Ts files (sync)
    // 合并译文写入Ts和生成QM文件
    QFileInfo tsDirinfo(ui->tsDirEdit->text());
    if (!tsDirinfo.isDir()){
        onReceiveMsg("ts目录为空");
        return;
    }

    QFileInfo excelDirinfo(ui->excelDirEdit->text());
    if (!excelDirinfo.exists()){
        onReceiveMsg("Excel文件路径不存在");
        return;
    }

    QStringList filters;
    filters << QString("*.ts");
    QDir tsdir(ui->tsDirEdit->text());
    tsdir.setFilter(QDir::Files | QDir::NoSymLinks);
    tsdir.setNameFilters(filters);

    if (tsdir.count() <= 0) {
        onReceiveMsg("Ts目录下未发现Ts文件");
        return;
    }
    QString scriptPath = m_genQmPathEdit->toPlainText();
    if (scriptPath.isEmpty()) {
        onReceiveMsg("请先导入生成Qm脚本路径");
        return;
    }
    QFileInfo scriptInfo(scriptPath);
    if (!scriptInfo.exists()) {
        onReceiveMsg("脚本文件不存在: " + scriptPath);
        return;
    }
    m_opeMergeStepTwo = true;
    on_tsUpdateBtn_2_clicked();
    // Step 2: Generate Qm files
    //on_genQmBtn_clicked();
}

void MainWindow::on_scanTsLookBtn_clicked()
{
    const QString documentLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString fileName = QFileDialog::getOpenFileName(this, tr("请选择扫描Ts脚本"), documentLocation, "Batch Files (*.bat)");
    if (fileName.isEmpty()) {
        return;
    }
    m_scanTsPathEdit->setPlainText(fileName);
}

void MainWindow::on_genQmLookBtn_clicked()
{
    const QString documentLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString fileName = QFileDialog::getOpenFileName(this, tr("请选择生成Qm脚本"), documentLocation, "Batch Files (*.bat)");
    if (fileName.isEmpty()) {
        return;
    }
    m_genQmPathEdit->setPlainText(fileName);
}

void MainWindow::slotquickModeChange()
{
    m_quickMode = !m_quickMode;
    if (m_quickMode) {
        // 翻译多种语言 tab
        ui->quickModeBtn->setText(tr("切换为普通模式"));
        ui->scanUpdateBtn->setVisible(true);
        ui->writeTsGenQmBtn->setVisible(true);
        m_genQmBtn->setVisible(false);
        m_scanTsBtn->setVisible(false);
        m_genQmTipLabel1->setVisible(false);
        m_scanTsTipLabel1->setVisible(false);
        ui->generateBtn_2->setVisible(false);
        ui->tsUpdateBtn_2->setVisible(false);

        // 翻译一种语言 tab
        m_singleQuickModeBtn->setText(tr("切换为普通模式"));
        m_singleMergeStep12Btn->setVisible(true);
        m_singleMergeStep34Btn->setVisible(true);
        ui->tsImportBtn->setVisible(false);
        ui->generateBtn->setVisible(false);
        ui->tsUpdateBtn->setVisible(false);
        m_aiTranslateBtn->setVisible(false);
    } else {
        // 翻译多种语言 tab
        ui->quickModeBtn->setText(tr("切换为快捷模式"));
        ui->scanUpdateBtn->setVisible(false);
        ui->writeTsGenQmBtn->setVisible(false);
        ui->generateBtn_2->setVisible(true);
        ui->tsUpdateBtn_2->setVisible(true);
        m_genQmTipLabel1->setVisible(true);
        m_scanTsTipLabel1->setVisible(true);
        m_genQmBtn->setVisible(true);
        m_scanTsBtn->setVisible(true);

        // 翻译一种语言 tab
        m_singleQuickModeBtn->setText(tr("切换为快捷模式"));
        m_singleMergeStep12Btn->setVisible(false);
        m_singleMergeStep34Btn->setVisible(false);
        ui->tsImportBtn->setVisible(true);
        ui->generateBtn->setVisible(true);
        ui->tsUpdateBtn->setVisible(true);
        m_aiTranslateBtn->setVisible(true);
    }
}

void MainWindow::on_scanTsBtn_clicked()
{
    QString scriptPath = m_scanTsPathEdit->toPlainText();
    if (scriptPath.isEmpty()) {
        onReceiveMsg("请先导入扫描Ts脚本路径");
        return;
    }
    QFileInfo scriptInfo(scriptPath);
    if (!scriptInfo.exists()) {
        onReceiveMsg("脚本文件不存在: " + scriptPath);
        return;
    }

    m_scanTsBtn->setEnabled(false);
    m_genQmBtn->setEnabled(false);

    // 显示进度条和进度标签
    showProgress(true, tr("正在开始扫描项目并更新Ts文件..."), 3);

    QProcess *process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    QSharedPointer<int> currentStep = QSharedPointer<int>::create(0);
    QSharedPointer<bool> finished = QSharedPointer<bool>::create(false);
    QSharedPointer<QString> allOutput = QSharedPointer<QString>::create();

    connect(process, &QProcess::readyReadStandardOutput, this, [this, process, currentStep, finished, allOutput]() {
        if (*finished) return;
        QString output = QString::fromLocal8Bit(process->readAllStandardOutput());
        allOutput->append(output);
        QStringList lines = output.split('\n', QString::SkipEmptyParts);
        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) continue;

            QRegularExpression re("\\[(\\d+)/(\\d+)\\]");
            QRegularExpressionMatch match = re.match(trimmed);
            if (match.hasMatch()) {
                *currentStep = match.captured(1).toInt();
                int total = match.captured(2).toInt();
                m_progressBar->setMaximum(total);
                m_progressBar->setValue(*currentStep - 1);
            }

            if (trimmed.contains("[Success]")) {
                m_progressBar->setValue(*currentStep);
            } else if (trimmed.contains("Error occurs")) {
                *finished = true;
                showProgress(false);
                m_scanTsBtn->setEnabled(true);
                m_genQmBtn->setEnabled(true);
                // 弹出错误详情对话框，方便查错
                showScriptError(tr("脚本执行失败"), *allOutput);
                m_opeMergeStepOne = false;
                process->kill();
                process->deleteLater();
                return;
            } else if (trimmed.contains("Bat Finish") || trimmed.contains("All Steps Complete")) {
                *finished = true;
                showProgress(false, "扫描项目文件更新Ts内容完成");
                m_scanTsBtn->setEnabled(true);
                m_genQmBtn->setEnabled(true);
                process->kill();
                process->deleteLater();
                if(m_opeMergeStepOne){
                    if(m_tabWidget->currentIndex() == 0){
                        on_generateBtn_2_clicked();
                    }
                    else{
                        on_tsImportBtn_clicked();
                    }
                }
                return;
            }
        }
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process, currentStep, finished, allOutput](int exitCode, QProcess::ExitStatus) {
        if (!*finished) {
            m_scanTsBtn->setEnabled(true);
            m_genQmBtn->setEnabled(true);
            if (exitCode == 0) {
                showProgress(false, "扫描项目文件更新Ts内容完成");
            } else {
                showProgress(false, "扫描项目文件更新Ts内容失败");
                showScriptError(tr("脚本执行失败"), *allOutput);
            }
        }
        process->deleteLater();
    });

    process->setWorkingDirectory(scriptInfo.absolutePath());
    process->start("cmd.exe", QStringList() << "/c" << scriptPath);

    if (!process->waitForStarted()) {
        showProgress(false, "无法启动脚本");
        m_scanTsBtn->setEnabled(true);
        m_genQmBtn->setEnabled(true);
        process->deleteLater();
        return;
    }
}

void MainWindow::on_genQmBtn_clicked()
{
    if(m_opeMergeStepTwo){
        m_opeMergeStepTwo = false;
    }
    QString scriptPath = m_genQmPathEdit->toPlainText();
    if (scriptPath.isEmpty()) {
        onReceiveMsg("请先导入生成Qm脚本路径");
        return;
    }
    QFileInfo scriptInfo(scriptPath);
    if (!scriptInfo.exists()) {
        onReceiveMsg("脚本文件不存在: " + scriptPath);
        return;
    }

    m_scanTsBtn->setEnabled(false);
    m_genQmBtn->setEnabled(false);

    // 显示进度条和进度标签
    showProgress(true, tr("正在开始生成Qm翻译文件..."), 10);

    QProcess *process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    QSharedPointer<int> currentStep = QSharedPointer<int>::create(0);
    QSharedPointer<bool> finished = QSharedPointer<bool>::create(false);
    QSharedPointer<QString> allOutput = QSharedPointer<QString>::create();

    connect(process, &QProcess::readyReadStandardOutput, this, [this, process, currentStep, finished, allOutput]() {
        if (*finished) return;
        QString output = QString::fromLocal8Bit(process->readAllStandardOutput());
        allOutput->append(output);
        QStringList lines = output.split('\n', QString::SkipEmptyParts);
        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) continue;

            QRegularExpression re("\\[(\\d+)/(\\d+)\\]");
            QRegularExpressionMatch match = re.match(trimmed);
            if (match.hasMatch()) {
                *currentStep = match.captured(1).toInt();
                int total = match.captured(2).toInt();
                m_progressBar->setMaximum(total);
                m_progressBar->setValue(*currentStep - 1);
            }

            if (trimmed.contains("[Success]")) {
                m_progressBar->setValue(*currentStep);
            } else if (trimmed.contains("Error occurs")) {
                *finished = true;
                showProgress(false);
                m_scanTsBtn->setEnabled(true);
                m_genQmBtn->setEnabled(true);
                showScriptError(tr("脚本执行失败"), *allOutput);
                process->kill();
                process->deleteLater();
                return;
            } else if (trimmed.contains("Bat Finish") || trimmed.contains("All Steps Complete")) {
                *finished = true;
                showProgress(false,"生成Qm文件完成");
                m_scanTsBtn->setEnabled(true);
                m_genQmBtn->setEnabled(true);
                process->kill();
                process->deleteLater();
                return;
            }
        }
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process, currentStep, finished, allOutput](int exitCode, QProcess::ExitStatus) {
        if (!*finished) {
            
            m_scanTsBtn->setEnabled(true);
            m_genQmBtn->setEnabled(true);
            if (exitCode == 0) {
                showProgress(false, "生成Qm文件完成");
            } else {
                showProgress(false, "生成Qm文件失败");
                showScriptError(tr("脚本执行失败"), *allOutput);
            }
        }
        process->deleteLater();
    });

    process->setWorkingDirectory(scriptInfo.absolutePath());
    process->start("cmd.exe", QStringList() << "/c" << scriptPath);

    if (!process->waitForStarted()) {
        showProgress(false, "无法启动脚本");
        m_scanTsBtn->setEnabled(true);
        m_genQmBtn->setEnabled(true);
        process->deleteLater();
        return;
    }
}

void MainWindow::readConfig()
{
    QString configPath = QApplication::applicationDirPath();
    QSettings settings(configPath + "/config.ini", QSettings::IniFormat);
    settings.beginGroup("path");
    if(settings.value("tsPath").toString().isEmpty()){
        ui->tsPathEdit->setText("C:/QA_Working/Firmware/trunk/VmsTools_v2/VmsTools/translations/VmsTsFile/VmsTranslator_zh_CN.ts");
    }
    else{
        ui->tsPathEdit->setText(settings.value("tsPath").toString());
    }
    if(settings.value("tsDir").toString().isEmpty()){
        ui->tsDirEdit->setText("C:/QA_Working/Firmware/trunk/VmsTools_v2/VmsTools/translations/VmsTsFile");
    }
    else{
        ui->tsDirEdit->setText(settings.value("tsDir").toString());
    }

    if(settings.value("excelPath").toString().isEmpty()){
        ui->excelPathEdit->setText("C:/Work/Kinto/TsFileEditor/tsFile/excel/VMSTools.xlsx");
    }
    else{
        ui->excelPathEdit->setText(settings.value("excelPath").toString());
    }
    if(settings.value("excelPath2").toString().isEmpty()){
        ui->excelDirEdit->setText("C:/Work/Kinto/TsFileEditor/tsFile/excel/VMSTools.xlsx");
    }
    else{
        ui->excelDirEdit->setText(settings.value("excelPath2").toString());
    }

    if(settings.value("qmBatPath").toString().isEmpty()){
        m_genQmPathEdit->setPlainText("C:/QA_Working/Firmware/trunk/VmsTools_v2/VmsTools/translations/updateQmFile.bat");
    }
    else{
        m_genQmPathEdit->setPlainText(settings.value("qmBatPath").toString());
    }
    if(settings.value("tsBatPath").toString().isEmpty()){
        m_scanTsPathEdit->setPlainText("C:/QA_Working/Firmware/trunk/VmsTools_v2/VmsTools/translations/updateTsFile.bat");
    }
    else{
        m_scanTsPathEdit->setPlainText(settings.value("tsBatPath").toString());
    }
    settings.endGroup();

    m_tsColumnMap.clear();
    QString tsDir = ui->tsDirEdit->text();
    if (!tsDir.isEmpty()) {
        QDir dir(tsDir);
        QStringList filters;
        filters << "*.ts";
        dir.setNameFilters(filters);
        dir.setFilter(QDir::Files | QDir::NoSymLinks);
        for (const QFileInfo &info : dir.entryInfoList()) {
            qDebug() << "222222" << info.fileName() << getColumnIndex(info.fileName());
            m_tsColumnMap[info.fileName()] = getColumnIndex(info.fileName());
        }
    }
}

int MainWindow::getColumnIndex(const QString &fileName)
{
    if(fileName.isEmpty()){
        return 3;
    }
    if(fileName.contains("VmsTranslator_zh_CN")){
        return 3;
    }
    if(fileName.contains("VmsTranslator_ESP")){
        return 4;
    }
    if(fileName.contains("VmsTranslator_FR")){
        return 5;
    }
    if(fileName.contains("VmsTranslator_TH")){
        return 6;
    }
    if(fileName.contains("VmsTranslator_DE")){
        return 7;
    }
    if(fileName.contains("VmsTranslator_JA")){
        return 8;
    }
    if(fileName.contains("VmsTranslator_VN")){
        return 9;
    }
    if(fileName.contains("VmsTranslator_RU")){
        return 10;
    }
    if(fileName.contains("VmsTranslator_PT")){
        return 11;
    }
    if(fileName.contains("VmsTranslator_MS")){
        return 12;
    }
    if(fileName.contains("VmsTranslator_IN")){
        return 13;
    }
    if(fileName.contains("VmsTranslator_KR")){
        return 14;
    }
    if(fileName.contains("VmsTranslator_IT")){
        return 15;
    }
    if(fileName.contains("VmsTranslator_ID")){
        return 16;
    }
    if(fileName.contains("VmsTranslator_HE")){
        return 17;
    }
    if(fileName.contains("VmsTranslator_AR")){
        return 18;
    }
    if(fileName.contains("VmsTranslator_PL")){
        return 19;
    }
    if(fileName.contains("VmsTranslator_NL")){
        return 20;
    }
}

void MainWindow::saveConfig()
{
    QString configPath = QApplication::applicationDirPath();
    QSettings settings(configPath + "/config.ini", QSettings::IniFormat);
    settings.beginGroup("path");
    settings.setValue("tsPath", ui->tsPathEdit->text());
    settings.setValue("tsDir", ui->tsDirEdit->text());
    settings.setValue("excelPath", ui->excelPathEdit->text());
    settings.setValue("excelPath2", ui->excelDirEdit->text());
    settings.setValue("qmBatPath", m_genQmPathEdit->toPlainText());
    settings.setValue("tsBatPath", m_scanTsPathEdit->toPlainText());
    settings.endGroup();

}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    event->accept();
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor("#ffffff"));
    painter.setPen(QPen(QColor("#b0b0b0"), 1));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 12, 12);
}

// ========== 样式初始化函数 ==========
void MainWindow::showScriptError(const QString& title, const QString& output)
{
    ScriptErrorDialog *dlg = new ScriptErrorDialog(title, output, this);
    dlg->show();
    dlg->move(this->geometry().center() - dlg->rect().center());
}

void MainWindow::showProgress(bool show, const QString& labelText, int maximum)
{
    if (show) {
        m_overlayWidget->setGeometry(0, 0, width(), height());
        m_overlayWidget->raise();
        m_overlayWidget->show();
        m_progressBar->setMaximum(maximum);
        m_progressBar->setValue(0);
        int x = (width() - m_progressBar->width()) / 2;
        int y = (height() - m_progressBar->height()) / 2;
        m_progressBar->move(x, y);
        m_progressBar->raise();
        m_progressBar->show();
        m_progressLabel->move(x, y - 60);
        m_progressLabel->raise();
        m_progressLabel->show();
        m_progressLabel->setText(labelText);
    } else {
        m_overlayWidget->hide();
        m_progressBar->hide();
        m_progressLabel->hide();
        if (!labelText.isEmpty()) {
            onReceiveMsg(labelText);
        }
    }
}

void MainWindow::applyStyles()
{
    // ---- 标题栏样式 ----
    // 标题栏背景样式：浅蓝色背景，顶部圆角12px
    m_titleBarWidget->setStyleSheet(
        "QWidget {"
        "  background: #E5F5FF;"
        "  border: none;"
        "  border-top-left-radius: 12px;"
        "  border-top-right-radius: 12px;"
        "  border-bottom-left-radius: 0px;"
        "  border-bottom-right-radius: 0px;"
        "}"
    );

    // 标题文本样式：16px粗体深灰色
    QLabel *titleLabel = m_titleBarWidget->findChild<QLabel*>();
    if (titleLabel) {
        titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #333333; background: transparent;");
    }
    QString titleStyle = QString(
        "QPushButton {"
            "  border: none;"
            "  border-radius: 4px;"
            "  background: transparent;"
            "  color: #666666;"
            "  font-size: 20px;"
            "  padding: 4px;"
            "}"
            "QPushButton:hover {"
            "  background: #e0e0e0;"
            "  color: #333333;"
            "}"
    );

    // 标题栏按钮样式
    QList<QPushButton*> titleBtns = m_titleBarWidget->findChildren<QPushButton*>();
    for(auto &btn : titleBtns){
        btn->setStyleSheet(titleStyle);
    }

    // ---- AI翻译按钮样式 ----
    // 浅蓝色主题，与生成Excel表格按钮风格类似
    m_aiTranslateBtn->setStyleSheet(
            "QPushButton {"
            "  border: 1px solid #62C0FF;"
            "  border-radius: 4px;"
            "  padding: 5px 16px;"
            "  min-height: 26px;"
            "  background: #EAF6FF;"
            "  color: #62C0FF;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background: #62C0FF;"
            "  color: #ffffff;"
            "}"
            "QPushButton:pressed {"
            "  background: #4AA8E8;"
            "  color: #ffffff;"
            "}"
            "QPushButton:checked {"
            "  border: 1px solid #DCDCDC;"
            "  background: #FFFFFF;"
            "  color: #DCDCDC;"
            "}"
            "QPushButton:checked:hover {"
            "  background: #f5f5f5;"
            "  color: #999999;"
            "}"
        );

    // ---- 功能按钮样式 ----
    // 导入Ts文件按钮：暖黄色主题
    ui->tsImportBtn->setStyleSheet(
        "QPushButton {"
        "  border: 1px solid #FFEEC2;"
        "  border-radius: 4px;"
        "  padding: 5px 16px;"
        "  min-height: 26px;"
        "  background: #FFF8E8;"
        "  color: #B8860B;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #FFEEC2;"
        "  color: #8B6914;"
        "}"
        "QPushButton:pressed {"
        "  background: #FFE0A0;"
        "  color: #8B6914;"
        "}"
    );

    // 生成Excel表格按钮：蓝色主题
    ui->generateBtn->setStyleSheet(
        "QPushButton {"
        "  border: 1px solid #0099FF;"
        "  border-radius: 4px;"
        "  padding: 5px 16px;"
        "  min-height: 26px;"
        "  background: #E6F4FF;"
        "  color: #0099FF;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #0099FF;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background: #0077CC;"
        "  color: #ffffff;"
        "}"
    );

    // 译文写入Ts文件按钮：红色主题
    ui->tsUpdateBtn->setStyleSheet(
        "QPushButton {"
        "  border: 1px solid #FF4A4A;"
        "  border-radius: 4px;"
        "  padding: 5px 16px;"
        "  min-height: 26px;"
        "  background: #FFF0F0;"
        "  color: #FF4A4A;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #FF4A4A;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background: #CC3333;"
        "  color: #ffffff;"
        "}"
    );

    // ---- 右侧面板按钮样式 ----
    m_scanTsPathEdit->setReadOnly(true);
    m_genQmPathEdit->setReadOnly(true);

    // 路径输入框样式：圆角4px，边框#DCDCDC，微软雅黑11px
    QString pathEditStyle =
        "QTextEdit {"
        "  border: 1px solid #DCDCDC;"
        "  border-radius: 4px;"
        "  padding: 2px 2px;"
        "  background: #fafafa;"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 11px;"
        "  color: #333333;"
        "}"
        "QTextEdit:focus {"
        "  border: 1px solid #4a90d9;"
        "  background: #ffffff;"
        "}"
        ;
    m_scanTsPathEdit->setStyleSheet(pathEditStyle);
    m_genQmPathEdit->setStyleSheet(pathEditStyle);

    // 导入路径按钮样式：与生成Excel表格按钮（蓝色主题）一样
    QString importPathBtnStyle =
        "QPushButton {"
        "  border: 1px solid #0099FF;"
        "  border-radius: 4px;"
        "  padding: 5px 16px;"
        "  min-height: 26px;"
        "  background: #E6F4FF;"
        "  color: #0099FF;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #0099FF;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background: #0077CC;"
        "  color: #ffffff;"
        "}"
        ;
    m_scanTsLookBtn->setStyleSheet(importPathBtnStyle);
    m_genQmLookBtn->setStyleSheet(importPathBtnStyle);

    // 扫描项目文件按钮：与译文写入Ts文件按钮（红色主题）一样
    QString actionBtnStyle =
        "QPushButton {"
        "  border: 1px solid #FF4A4A;"
        "  border-radius: 4px;"
        "  padding: 5px 16px;"
        "  min-height: 26px;"
        "  background: #FFF0F0;"
        "  color: #FF4A4A;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #FF4A4A;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background: #CC3333;"
        "  color: #ffffff;"
        "}"
        ;
    m_scanTsBtn->setStyleSheet(actionBtnStyle);
    m_genQmBtn->setStyleSheet(actionBtnStyle);

    // ---- 有道翻译按钮样式 ----
    // 与生成Excel表格按钮相同的蓝色主题
    ui->translateBtn->setStyleSheet(
        "QPushButton {"
        "  border: 1px solid #0099FF;"
        "  border-radius: 4px;"
        "  padding: 5px 16px;"
        "  min-height: 26px;"
        "  background: #E6F4FF;"
        "  color: #0099FF;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #0099FF;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background: #0077CC;"
        "  color: #ffffff;"
        "}"
    );

    // ---- 浏览按钮样式 ----
    // 蓝色背景白色文字，统一样式
    QString browseBtnStyle =
        "QPushButton {"
        "  border: 1px solid #0099FF;"
        "  border-radius: 4px;"
        "  padding: 5px 10px;"
        "  min-height: 36px;"
        "  background: #0099FF;"
        "  color: #ffffff;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #0088DD;"
        "}"
        "QPushButton:pressed {"
        "  background: #0077BB;"
        "}"
        ;
    ui->tsLookBtn->setStyleSheet(browseBtnStyle);
    ui->excelLookBtn->setStyleSheet(browseBtnStyle);
    ui->tsDirLookBtn->setStyleSheet(browseBtnStyle);
    ui->excelDirBtn->setStyleSheet(browseBtnStyle);

    // ---- 按钮宽度设置 ----
    // 单文件模块按钮宽度150px，浏览按钮宽度150px
    ui->tsImportBtn->setFixedWidth(250);
    ui->generateBtn->setFixedWidth(250);
    ui->tsUpdateBtn->setFixedWidth(250);
    ui->tsLookBtn->setFixedWidth(150);
    ui->excelLookBtn->setFixedWidth(150);
    ui->tsDirLookBtn->setFixedWidth(150);
    ui->excelDirBtn->setFixedWidth(150);

    // 批量模块按钮：继承单文件按钮样式，宽度225px，高度36px
    ui->generateBtn_2->setStyleSheet(ui->generateBtn->styleSheet());
    ui->tsUpdateBtn_2->setStyleSheet(ui->tsUpdateBtn->styleSheet());
    ui->generateBtn_2->setFixedWidth(250);
    ui->generateBtn_2->setFixedHeight(36);
    ui->tsUpdateBtn_2->setFixedWidth(250);
    ui->tsUpdateBtn_2->setFixedHeight(36);

    // 新增批量按钮样式
    ui->scanUpdateBtn->setStyleSheet(ui->generateBtn->styleSheet());
    ui->scanUpdateBtn->setFixedWidth(250);
    ui->scanUpdateBtn->setFixedHeight(36);
    ui->writeTsGenQmBtn->setStyleSheet(ui->tsUpdateBtn->styleSheet());
    ui->writeTsGenQmBtn->setFixedWidth(250);
    ui->writeTsGenQmBtn->setFixedHeight(36);

    // 快捷模式按钮样式
    ui->quickModeBtn->setStyleSheet(
        "QPushButton {"
        "  background: #3B82F6;"
        "  color: #FFFFFF;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  padding: 4px 16px;"
        "}"
        "QPushButton:hover {"
        "  background: #2563EB;"
        "}"
        "QPushButton:pressed {"
        "  background: #1D4ED8;"
        "}"
    );
    ui->quickModeBtn->setFixedWidth(250);
    ui->quickModeBtn->setFixedHeight(36);

    // 单语言快捷模式按钮样式（与多语言一致）
    m_singleQuickModeBtn->setStyleSheet(ui->quickModeBtn->styleSheet());
    m_singleMergeStep12Btn->setStyleSheet(ui->generateBtn->styleSheet());
    m_singleMergeStep34Btn->setStyleSheet(ui->tsUpdateBtn->styleSheet());

    // ---- 生成图标资源 ----
    // SpinBox加减号图标和ComboBox下拉箭头图标
    QString tempDir = QDir::tempPath().replace("\\", "/");

    QPixmap plusPixmap(16, 16);
    plusPixmap.fill(Qt::transparent);
    QPainter plusPainter(&plusPixmap);
    plusPainter.setRenderHint(QPainter::Antialiasing);
    plusPainter.setPen(QPen(QColor("#666666"), 2));
    plusPainter.drawLine(4, 8, 12, 8);
    plusPainter.drawLine(8, 4, 8, 12);
    plusPixmap.save(tempDir + "/spinbox_plus.png", "PNG");

    QPixmap minusPixmap(16, 16);
    minusPixmap.fill(Qt::transparent);
    QPainter minusPainter(&minusPixmap);
    minusPainter.setRenderHint(QPainter::Antialiasing);
    minusPainter.setPen(QPen(QColor("#666666"), 2));
    minusPainter.drawLine(4, 8, 12, 8);
    minusPixmap.save(tempDir + "/spinbox_minus.png", "PNG");

    QPixmap arrowPixmap(16, 16);
    arrowPixmap.fill(Qt::transparent);
    QPainter arrowPainter(&arrowPixmap);
    arrowPainter.setRenderHint(QPainter::Antialiasing);
    arrowPainter.setPen(QPen(QColor("#666666"), 2));
    arrowPainter.drawLine(3, 10, 8, 4);
    arrowPainter.drawLine(8, 4, 13, 10);
    arrowPixmap.save(tempDir + "/combobox_arrow.png", "PNG");

    // ---- 全局样式表 ----
    // 包含：字体、窗口圆角边框、GroupBox、输入框、按钮、下拉框、Tab、状态栏等
    setStyleSheet(QString(
        "* {"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 14px;"
        "}"
        "QMainWindow {"
        "  background: #ffffff;"
        "  border: 1px solid #b0b0b0;"
        "  border-radius: 12px;"
        "}"
        "QGroupBox {"
        "  font-weight: bold;"
        "  font-size: 16px;"
        "  border: 1px solid #d0d0d0;"
        "  border-radius: 6px;"
        "  margin-top: 10px;"
        "  padding-top: 14px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 24px;"
        "  padding: 0 4px;"
        "  font-size: 16px;"
        "}"
        "QLineEdit {"
        "  border: 1px solid #c0c0c0;"
        "  border-radius: 4px;"
        "  padding: 2px 8px;"
        "  min-height: 36px;"
        "  background: #fafafa;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid #4a90d9;"
        "  background: #ffffff;"
        "}"
        "QPushButton {"
        "  border: 1px solid #b0b0b0;"
        "  border-radius: 4px;"
        "  padding: 2px 16px;"
        "  min-height: 26px;"
        "  background: #f5f5f5;"
        "}"
        "QPushButton:hover {"
        "  background: #e8e8e8;"
        "  border: 1px solid #909090;"
        "}"
        "QPushButton:pressed {"
        "  background: #d8d8d8;"
        "}"
        "QComboBox {"
        "  border: 1px solid #c0c0c0;"
        "  border-radius: 4px;"
        "  padding: 2px 2px;"
        "  min-height: 26px;"
        "  min-width: 68;"
        "  background: #fafafa;"
        "}"
        "QComboBox:hover {"
        "  border: 1px solid #909090;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 20px;"
        "  border: none;"
        "  border-left: 1px solid #d0d0d0;"
        "}"
        "QComboBox::down-arrow {"
        "  image: url(%1/combobox_arrow.png);"
        "}"
        "QComboBox QAbstractItemView {"
        "  border: 1px solid #c0c0c0;"
        "  border-radius: 4px;"
        "  background: #ffffff;"
        "  selection-background-color: #4a90d9;"
        "  selection-color: #ffffff;"
        "  outline: none;"
        "  padding: 2px;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "  min-height: 28px;"
        "  color: #4a90d9;"
        "  padding: 0px;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "  background: #e0ecf7;"
        "  color: #333333;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "  background: #4a90d9;"
        "  color: #ffffff;"
        "}"
        "QLabel {"
        "  min-height: 30px;"
        "}"
        "QTabWidget::pane {"
        "  border: 1px solid #d0d0d0;"
        "  border-radius: 4px;"
        "  background: #ffffff;"
        "}"
        "QTabWidget::tab-bar {"
        "  alignment: center;"
        "}"
        "QTabBar::tab {"
        "  background: #ffffff;"
        "  border: 1px solid #DCDCDC;"
        "  border-bottom: none;"
        "  border-top-left-radius: 4px;"
        "  border-top-right-radius: 4px;"
        "  padding: 4px 40px;"
        "  min-height: 32px;"
        "  min-width: 120px;"
        "  margin-right: 0px;"
        "  color: #6D7682;"
        "  font-size: 16px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #0099FF;"
        "  color: #ffffff;"
        "  border: 1px solid #0099FF;"
        "  border-bottom: none;"
        "  font-weight: bold;"
        "}"
        "QTabBar::tab:!selected:hover {"
        "  background: #E6F4FF;"
        "  color: #0099FF;"
        "  border: 1px solid #0099FF;"
        "  border-bottom: none;"
        "}"
        "QStatusBar {"
        "  color: #FF0000;"
        "  background: transparent;"
        "}"
    ).arg(tempDir));

    // ---- SpinBox样式 ----
    // 翻译列SpinBox：带加减号图标
    ui->transSpinBox->setStyleSheet(
        QString(
        "QSpinBox {"
        "  border: 1px solid #c0c0c0;"
        "  border-radius: 4px;"
        "  padding: 2px 6px;"
        "  min-height: 26px;"
        "  background: #fafafa;"
        "}"
        "QSpinBox:hover {"
        "  border: 1px solid #909090;"
        "}"
        "QSpinBox::up-button {"
        "  subcontrol-origin: border;"
        "  subcontrol-position: top right;"
        "  width: 22px;"
        "  border: 1px solid #c0c0c0;"
        "  border-top-right-radius: 4px;"
        "  background: #f5f5f5;"
        "}"
        "QSpinBox::up-button:hover {"
        "  background: #e8e8e8;"
        "  border: 1px solid #909090;"
        "}"
        "QSpinBox::up-arrow {"
        "  image: url(%1/spinbox_plus.png);"
        "}"
        "QSpinBox::down-button {"
        "  subcontrol-origin: border;"
        "  subcontrol-position: bottom right;"
        "  width: 22px;"
        "  border: 1px solid #c0c0c0;"
        "  border-bottom-right-radius: 4px;"
        "  background: #f5f5f5;"
        "}"
        "QSpinBox::down-button:hover {"
        "  background: #e8e8e8;"
        "  border: 1px solid #909090;"
        "}"
        "QSpinBox::down-arrow {"
        "  image: url(%1/spinbox_minus.png);"
        "}"
        ).arg(tempDir.replace("\\", "/"))
    );

    // ---- 进度条样式 ----
    // 蓝色主题进度条，固定500x16，圆角8px
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  border: none;"
        "  border-radius: 8px;"
        "  background: #e0e0e0;"
        "  color: #333333;"
        "  font-weight: bold;"
        "}"
        "QProgressBar::chunk {"
        "  background: #0099FF;"
        "  border-radius: 8px;"
        "}"
    );

    // ---- ComboBox样式 ----
    // 语言选择下拉框：可编辑，居中显示，无边框输入框
    ui->comboBox->setEditable(true);
    QListView *comboBoxView = new QListView();
    ui->comboBox->setView(comboBoxView);
    ui->comboBox->lineEdit()->setReadOnly(true);
    ui->comboBox->lineEdit()->setAlignment(Qt::AlignCenter);
    ui->comboBox->lineEdit()->setStyleSheet("QLineEdit { border: none; background: transparent; }");
}
