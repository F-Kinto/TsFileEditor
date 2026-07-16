#pragma execution_character_set("utf-8")
#include "HelpDialog.h"
#include "ScriptErrorDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QPaintEvent>
#include <QScrollBar>

HelpDialog::HelpDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("帮助"));
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    resize(600, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(0);

    // 自定义标题栏
    QWidget *titleBar = new QWidget(this);
    titleBar->setFixedHeight(48);
    titleBar->setStyleSheet(
        "QWidget {"
        "  background: #E5F5FF;"
        "  border: none;"
        "  border-radius: 4px"
        "}"
    );
    QHBoxLayout *titleBarLayout = new QHBoxLayout(titleBar);
    titleBarLayout->setContentsMargins(24, 0, 24, 0);

    QLabel *titleLabel = new QLabel(tr("帮助"), titleBar);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #333333; background: transparent;");
    titleBarLayout->addWidget(titleLabel, 0, Qt::AlignVCenter);
    titleBarLayout->addStretch();

    QPushButton *dlgCloseBtn = new QPushButton("X", titleBar);
    dlgCloseBtn->setFixedSize(32, 32);
    dlgCloseBtn->setStyleSheet(
        "QPushButton {"
        "  border: none;"
        "  border-radius: 4px;"
        "  background: transparent;"
        "  color: #666666;"
        "  font-size: 24px;"
        "  padding: 4px 8px;"
        "}"
        "QPushButton:hover {"
        "  background: #e81123;"
        "  color: #ffffff;"
        "}"
    );
    connect(dlgCloseBtn, &QPushButton::clicked, this, &QDialog::close);
    titleBarLayout->addWidget(dlgCloseBtn, 0, Qt::AlignVCenter);

    mainLayout->addWidget(titleBar);

    // 内容区域
    QWidget *contentWidget = new QWidget(this);
    contentWidget->setStyleSheet("QWidget { background: transparent; }");
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(12);

    QTextEdit *textEdit = new QTextEdit(contentWidget);
    textEdit->setReadOnly(true);
    textEdit->setLineWrapMode(QTextEdit::WidgetWidth);
    textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textEdit->setStyleSheet(
        "QTextEdit {"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 14px;"
        "  border: 1px solid #d0d0d0;"
        "  border-radius: 4px;"
        "  padding: 12px;"
        "  background: #ffffff;"
        "  color: #333333;"
        "  line-height: 1.8;"
        "}"
    );
    textEdit->verticalScrollBar()->setStyleSheet(
        "QScrollBar:vertical {"
        "  background: transparent;"
        "  width: 6px;"
        "  margin: 2px 0;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #c0c0c0;"
        "  border-radius: 3px;"
        "  min-height: 30px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #a0a0a0;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "  background: transparent;"
        "}"
    );

    textEdit->setHtml(
        tr("<h3>使用说明</h3>"
            "<p><b>一、批量模式(VMSTools适用此步骤)</b></p>"
           "<p>1. 点击右侧「扫描Ts文件」按钮, 更新 Ts 字段内容(项目没有新增/删除字段可忽略)；</p>"
           "<p>2. 设置 Ts 文件目录和 Excel 文件目录；</p>"
           "<p>3. 点击「Ts批量导入至Excel」将目录下所有 Ts 文件导出为 Excel；(此操作会将新增的字段写入到Excel表格最末端)</p>"
           "<p>4. 在Excel表格中完成指定语言的翻译</p>"
           "<p>5. 翻译完成后，点击「批量译文写入Ts文件」将翻译结果批量写回。</p>"
           "<p>6. 点击右侧 '开始生成Qm翻译文件' 按钮。</p>"
           "<br>"
            "<p><b>二、单文件模式</b></p>"
           "<p>1. 点击右侧「扫描Ts文件」按钮, 更新 Ts 字段内容(项目没有新增/删除字段可忽略)；</p>"
           "<p>2. 点击「导入Ts文件」按钮，选择需要翻译的 .ts 文件；</p>"
           "<p>3. 点击「生成Excel表格」按钮，将 Ts 文件内容导出为 Excel 表格(项目没有新增/删除字段可忽略)；</p>"
           "<p>4. 在 Excel 中完成翻译后，点击「译文写入Ts文件」按钮，将翻译结果写回 Ts 文件；</p>"
           "<p>5. 如需使用有道AI翻译，请先点击「使用有道AI翻译」展开设置面板，填入应用ID和密钥后点击「有道翻译」。</p>"
           "<br>"
           "<p><b>三、脚本工具</b></p>"
           "<p>1. 右侧「扫描Ts文件」：导入扫描脚本路径后，点击按钮扫描项目文件并；</p>"
           "<p>2. 右侧「生成Qm文件」：导入生成脚本路径后，点击按钮将 Ts 文件编译为 Qm 翻译文件。</p>"
           "<br>"
           "<p><b>四、注意事项</b></p>"
           "<p>· 执行脚本操作前请确保脚本路径正确；</p>"
           "<p>· 批量操作前请确认目录路径有效；</p>"
           "<p>· 有道翻译需要有效的应用ID和密钥；</p>"
           "<p>· 建议按步骤顺序操作，避免数据丢失。</p>")
    );

    contentLayout->addWidget(textEdit);

    // 底部关闭按钮
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();

    QPushButton *closeBtn = new QPushButton(tr("关闭"), contentWidget);
    closeBtn->setFixedSize(100, 36);
    closeBtn->setStyleSheet(
        "QPushButton {"
        "  border: 1px solid #0099FF;"
        "  border-radius: 4px;"
        "  padding: 5px 16px;"
        "  background: #E6F4FF;"
        "  color: #0099FF;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #0099FF;"
        "  color: #ffffff;"
        "}"
    );
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    bottomLayout->addWidget(closeBtn);
    contentLayout->addLayout(bottomLayout);

    mainLayout->addWidget(contentWidget, 1);

    // 标题栏拖拽移动窗口
    titleBar->installEventFilter(new DragEventFilter(titleBar, this));

    setAttribute(Qt::WA_DeleteOnClose);
}

void HelpDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 白色圆角背景 + 边框
    painter.setBrush(QColor("#ffffff"));
    painter.setPen(QPen(QColor("#b0b0b0"), 1));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 12, 12);

    // 标题栏蓝色区域（顶部圆角）
    QPainterPath titlePath;
    titlePath.addRoundedRect(rect().adjusted(1, 1, -1, -1), 12, 12);
    QRect titleRect(rect().adjusted(1, 1, -1, -1));
    titleRect.setBottom(titleRect.top() + 50);
    painter.setClipRect(titleRect);
    painter.setBrush(QColor("#E5F5FF"));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 12, 12);
    painter.setClipping(false);
}
