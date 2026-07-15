#pragma execution_character_set("utf-8")
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "XmlRW.h"
#include "ExcelRW.h"
#include "TranslateWorker.h"

#include <QStandardPaths>
#include <QFileDialog>
#include <QListView>
#include <QSslSocket>
#include <QScreen>
#include <QWindow>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ========== 创建标题栏控件 ==========
    QPushButton *minimizeBtn = new QPushButton("—");
    minimizeBtn->setFixedSize(32, 32);
    minimizeBtn->setToolTip(tr("最小化"));
    connect(minimizeBtn, &QPushButton::clicked, this, &MainWindow::showMinimized);

    QPushButton *closeBtn = new QPushButton("X");
    closeBtn->setFixedSize(32, 32);
    closeBtn->setToolTip(tr("关闭"));
    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::close);

    QWidget *titleBarWidget = new QWidget();
    titleBarWidget->setFixedHeight(48);
    QHBoxLayout *titleBarLayout = new QHBoxLayout(titleBarWidget);
    titleBarLayout->setContentsMargins(24, 0, 24, 0);
    QLabel *titleLabel = new QLabel(tr("自动翻译Tool"));
    titleBarLayout->addWidget(titleLabel, 0, Qt::AlignVCenter);
    titleBarLayout->addStretch();
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

    // 将输入框行包裹在75%宽度居中布局中
    auto wrapRowCentered = [](QVBoxLayout *parentLayout, int index) {
        QLayoutItem *item = parentLayout->itemAt(index);
        if (!item || !item->layout()) return;
        QHBoxLayout *rowLayout = qobject_cast<QHBoxLayout*>(item->layout());
        if (!rowLayout) return;

        QWidget *container = new QWidget();
        QHBoxLayout *containerLayout = new QHBoxLayout(container);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setSpacing(rowLayout->spacing());

        parentLayout->takeAt(index);

        while (rowLayout->count() > 0) {
            QLayoutItem *child = rowLayout->takeAt(0);
            if (child->widget()) {
                containerLayout->addWidget(child->widget());
            } else if (child->layout()) {
                containerLayout->addLayout(child->layout());
            } else if (child->spacerItem()) {
                containerLayout->addSpacerItem(child->spacerItem());
            }
        }
        delete rowLayout;

        QHBoxLayout *wrapperLayout = new QHBoxLayout();
        wrapperLayout->setContentsMargins(0, 0, 0, 0);
        wrapperLayout->setSpacing(24);
        wrapperLayout->addStretch(1);
        wrapperLayout->addWidget(container, 6);
        wrapperLayout->addStretch(1);

        parentLayout->insertLayout(index, wrapperLayout);
    };

    wrapRowCentered(groupBoxLayout, 0);
    wrapRowCentered(groupBoxLayout, 1);
    wrapRowCentered(groupBoxLayout, 2);

    QVBoxLayout *groupBox3Layout = qobject_cast<QVBoxLayout*>(ui->groupBox_3->layout());
    ui->groupBox_3->setTitle(tr("翻译多种语言"));
    wrapRowCentered(groupBox3Layout, 0);
    wrapRowCentered(groupBox3Layout, 1);
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
    m_aiTranslateBtn->setFixedWidth(150);

    QVBoxLayout *buttonsLayout = qobject_cast<QVBoxLayout*>(oldButtonsItem ? oldButtonsItem->layout() : nullptr);
    if (buttonsLayout) {
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

    // 底部布局包裹在75%居中布局中
    QWidget *bottomContainer = new QWidget();
    QHBoxLayout *bottomContainerLayout = new QHBoxLayout(bottomContainer);
    bottomContainerLayout->setContentsMargins(0, 0, 0, 0);
    bottomContainerLayout->setSpacing(0);
    bottomContainerLayout->addLayout(bottomHLayout);

    QHBoxLayout *bottomWrapper = new QHBoxLayout();
    bottomWrapper->setContentsMargins(0, 0, 0, 0);
    bottomWrapper->setSpacing(0);
    bottomWrapper->addStretch(1);
    bottomWrapper->addWidget(bottomContainer, 6);
    bottomWrapper->addStretch(1);
    groupBoxLayout->addLayout(bottomWrapper);
    groupBoxLayout->addStretch();

    connect(m_aiTranslateBtn, &QPushButton::toggled, this, [this](bool checked) {
        ui->groupBox_2->setVisible(checked);
        m_aiTranslateBtn->setText(checked ? tr("收起有道AI翻译") : tr("使用有道AI翻译"));
    });

    // 批量翻译页面
    QWidget *pageBatch = new QWidget();
    QVBoxLayout *pageBatchLayout = new QVBoxLayout(pageBatch);
    pageBatchLayout->addWidget(ui->groupBox_3);
    pageBatchLayout->addStretch();

    m_tabWidget->addTab(pageSingle, tr("翻译一种语言"));
    m_tabWidget->addTab(pageBatch, tr("翻译多种语言"));

    // 替换中央布局
    QLayout *oldLayout = ui->centralWidget->layout();
    while (oldLayout->count() > 0) {
        QLayoutItem *item = oldLayout->takeAt(0);
        if (item->layout()) {
            delete item->layout();
        }
        delete item;
    }
    delete oldLayout;

    QVBoxLayout *newLayout = new QVBoxLayout(ui->centralWidget);
    newLayout->setContentsMargins(1, 1, 1, 1);
    newLayout->setSpacing(16);
    newLayout->addWidget(titleBarWidget);
    newLayout->addWidget(m_tabWidget, 1);

    // 无边框窗口
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // ========== 应用所有样式 ==========
    applyStyles();

    // ========== 初始化业务对象 ==========
    m_dragging = false;

    m_toastLabel = new QLabel(this);
    m_toastLabel->setAlignment(Qt::AlignCenter);
    m_toastLabel->setWordWrap(true);
    m_toastLabel->hide();
    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);
    connect(m_toastTimer, &QTimer::timeout, this, [this]() { m_toastLabel->hide(); });

    m_toLanguage = "en";
    m_pXmlWorker = new XmlRW(this);
    m_pExcelWorker = new ExcelRW(1, 2, 3, this);
    m_pTranslateWorker = new TranslateWorker(m_transList, this);

    ui->youdaoTipLabel->setVisible(false);

    QStandardItemModel *comboModel = new QStandardItemModel(ui->comboBox);
    QStringList comboItems = {"??", "??", "??", "??", "??", "??", "????", "????", "??"};
    QStringList comboData = {"en", "zh-CHS", "ja", "ko", "fr", "ru", "pt", "es", "other"};
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

    m_pExcelWorker->SetTransColumn(ui->transSpinBox->value());

    //generate excel file
    if(ui->excelPathEdit->text().isEmpty()) {
        const QString documentLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        QString saveName = documentLocation + "/untitled.xlsx";
        QString fileName = QFileDialog::getSaveFileName(this, "excel file path", saveName, "Files (*.xlsx)");
        qDebug() << "sace name path" << saveName << "---filename " << fileName;
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
    } else {
        onReceiveMsg("译文写入失败");
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

    //import .ts file
    if(ui->tsPathEdit->text().isEmpty()) {
        on_tsLookBtn_clicked();
    }

    QFileInfo info(ui->tsPathEdit->text());
    if (!info.isFile() || "ts" != info.suffix()){
        onReceiveMsg("导入失败, 请确认文件是否存在且是Ts文件");
        return;
    }

    m_transList.clear();
    re = m_pXmlWorker->ImportFromTS(m_transList, ui->tsPathEdit->text());

    if(re) {
        onReceiveMsg("Ts文件导入成功");
    } else {
        onReceiveMsg("Ts文件导入失败");
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
            "  border: 1px solid #0099FF;"
            "  border-radius: 4px;"
            "  padding: 8px 16px;"
            "  background: #E6F4FF;"
            "  color: #0099FF;"
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
    bool re;

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
    qDebug() << excelinfo.filePath() << excelinfo.absoluteDir().path();

    QStringList filters;
    filters << QString("*.ts");
    QDir tsdir(ui->tsDirEdit->text());
    tsdir.setFilter(QDir::Files | QDir::NoSymLinks);
    tsdir.setNameFilters(filters);

    if (tsdir.count() <= 0) {
        onReceiveMsg("Ts目录下未发现Ts文件");
        return;
    }

    for (QFileInfo info : tsdir.entryInfoList()) {
        //import ts file
        m_transList.clear();
        re = m_pXmlWorker->ImportFromTS(m_transList, info.absoluteFilePath());

        if(re) {
            onReceiveMsg("文件导入成功 " + info.fileName());
        } else {
            onReceiveMsg("文件导入失败 " + info.fileName());
        }

        //generate excel file
        m_pExcelWorker->SetTransColumn(ui->transSpinBox->value());
        QString excelFileName = excelinfo.absoluteDir().path() + "/" + info.baseName() + ".xlsx";
        re = m_pExcelWorker->ExportToXlsx(m_transList, excelFileName);
        if(re) {
            onReceiveMsg("文件生成成功 " + excelFileName);
            ui->youdaoTipLabel->setVisible(false);
        } else {
            onReceiveMsg("文件生成失败  " + excelFileName);
        }
    }
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

    for (QFileInfo info : tsdir.entryInfoList()) {
        qDebug() << "1111111" << info.fileName() << m_tsColumnMap[info.fileName()];
        QString path = info.fileName();
        if (!m_tsColumnMap.contains(info.fileName())) {
            continue;
        }

        //import ts file
        m_transList.clear();
        re = m_pXmlWorker->ImportFromTS(m_transList, info.absoluteFilePath());

        if(!re) {
            continue;
        }

        m_pExcelWorker->SetTransColumn(m_tsColumnMap[info.fileName()]);
        re = m_pExcelWorker->ImportFromXlsx(m_transList, ui->excelDirEdit->text());
        if(!re) {
            continue;
        }

        re = m_pXmlWorker->ExportToTS(m_transList, info.absoluteFilePath());

        if(!re) {
            continue;
        }
    }

    onReceiveMsg("所有Ts文件已翻译完成");
}

void MainWindow::readConfig()
{
    QString configPath = QApplication::applicationDirPath();
#if __DEBUG
    configPath.append("/../Config");
#endif
    QSettings settings(configPath + "/config.ini", QSettings::IniFormat);
    settings.beginGroup("path");
    ui->tsPathEdit->setText(settings.value("tsPath").toString());
    ui->tsDirEdit->setText(settings.value("tsDir").toString());

    ui->excelPathEdit->setText(settings.value("excelPath").toString());
    ui->excelDirEdit->setText(settings.value("excelPath2").toString());
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
#if __DEBUG
    configPath.append("/../Config");
#endif
    QSettings settings(configPath + "/config.ini", QSettings::IniFormat);
    settings.beginGroup("path");
    settings.setValue("tsPath", ui->tsPathEdit->text());
    settings.setValue("tsDir", ui->tsDirEdit->text());
    settings.setValue("excelPath", ui->excelPathEdit->text());
    settings.setValue("excelPath2", ui->excelDirEdit->text());
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
void MainWindow::applyStyles()
{
    // ---- 标题栏按钮样式 ----
    // 最小化按钮：透明背景，灰色文字，悬浮灰色背景
    QPushButton *minimizeBtn = findChild<QPushButton*>(); // 通过布局获取
    // 由于标题栏按钮是动态创建的，需要通过遍历标题栏获取
    QWidget *titleBarWidget = nullptr;
    for (int i = 0; i < ui->centralWidget->layout()->count(); ++i) {
        QWidget *w = ui->centralWidget->layout()->itemAt(i)->widget();
        if (w && w->height() == 48) { // titleBarWidget高度为48
            titleBarWidget = w;
            break;
        }
    }

    if (titleBarWidget) {
        // 标题栏背景样式：浅蓝色背景，顶部圆角12px
        titleBarWidget->setStyleSheet(
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
        QLabel *titleLabel = titleBarWidget->findChild<QLabel*>();
        if (titleLabel) {
            titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #333333; background: transparent;");
        }

        // 最小化按钮样式：透明背景，灰色文字，悬浮灰色背景
        QList<QPushButton*> btns = titleBarWidget->findChildren<QPushButton*>();
        if (btns.size() >= 2) {
            btns[0]->setStyleSheet(
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

            // 关闭按钮样式：透明背景，灰色文字，悬浮红色背景白色文字
            btns[1]->setStyleSheet(
                "QPushButton {"
                "  border: none;"
                "  border-radius: 4px;"
                "  background: transparent;"
                "  color: #666666;"
                "  font-size: 24px;"
                "  padding: 4px 8px 4px 8px;"
                "}"
                "QPushButton:hover {"
                "  background: #e81123;"
                "  color: #ffffff;"
                "}"
            );
        }
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
    ui->tsImportBtn->setFixedWidth(150);
    ui->generateBtn->setFixedWidth(150);
    ui->tsUpdateBtn->setFixedWidth(150);
    ui->tsLookBtn->setFixedWidth(150);
    ui->excelLookBtn->setFixedWidth(150);
    ui->tsDirLookBtn->setFixedWidth(150);
    ui->excelDirBtn->setFixedWidth(150);

    // 批量模块按钮：继承单文件按钮样式，宽度225px，高度36px
    ui->generateBtn_2->setStyleSheet(ui->generateBtn->styleSheet());
    ui->tsUpdateBtn_2->setStyleSheet(ui->tsUpdateBtn->styleSheet());
    ui->generateBtn_2->setFixedWidth(225);
    ui->generateBtn_2->setFixedHeight(36);
    ui->tsUpdateBtn_2->setFixedWidth(225);
    ui->tsUpdateBtn_2->setFixedHeight(36);

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

    // ---- ComboBox样式 ----
    // 语言选择下拉框：可编辑，居中显示，无边框输入框
    ui->comboBox->setEditable(true);
    QListView *comboBoxView = new QListView();
    ui->comboBox->setView(comboBoxView);
    ui->comboBox->lineEdit()->setReadOnly(true);
    ui->comboBox->lineEdit()->setAlignment(Qt::AlignCenter);
    ui->comboBox->lineEdit()->setStyleSheet("QLineEdit { border: none; background: transparent; }");
}
