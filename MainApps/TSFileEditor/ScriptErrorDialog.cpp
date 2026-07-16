#pragma execution_character_set("utf-8")
#include "ScriptErrorDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QShortcut>
#include <QTextDocument>
#include <QEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainterPath>
#include <QScrollBar>

DragEventFilter::DragEventFilter(QObject *watched, QWidget *window)
    : QObject(watched), m_window(window), m_dragging(false)
{
}

bool DragEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragPos = me->globalPos() - m_window->frameGeometry().topLeft();
        }
    } else if (event->type() == QEvent::MouseMove) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (m_dragging && (me->buttons() & Qt::LeftButton)) {
            m_window->move(me->globalPos() - m_dragPos);
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        m_dragging = false;
    }
    return QObject::eventFilter(watched, event);
}

void ScriptErrorDialog::paintEvent(QPaintEvent *event)
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
    // 裁剪为上半部分（标题栏高度48+2margin=50）
    QRect titleRect(rect().adjusted(1, 1, -1, -1));
    titleRect.setBottom(titleRect.top() + 50);
    painter.setClipRect(titleRect);
    painter.setBrush(QColor("#FFF0F0"));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 12, 12);
    painter.setClipping(false);
}

ScriptErrorDialog::ScriptErrorDialog(const QString &title, const QString &output, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    resize(800, 600);
    

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setSpacing(0);

    // 自定义标题栏
    QWidget *titleBar = new QWidget(this);
    titleBar->setFixedHeight(48);
    titleBar->setStyleSheet(
        "QWidget {"
        "  background: #FFF0F0;"
        "  border: none;"
        "}"
    );
    QHBoxLayout *titleBarLayout = new QHBoxLayout(titleBar);
    titleBarLayout->setContentsMargins(24, 0, 24, 0);

    QLabel *titleLabel = new QLabel(title, titleBar);
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
    textEdit->setPlainText(output);
    textEdit->setReadOnly(true);
    textEdit->setLineWrapMode(QTextEdit::WidgetWidth);
    textEdit->setStyleSheet(
        "QTextEdit {"
        "  font-family: 'Consolas', 'Microsoft YaHei';"
        "  font-size: 13px;"
        "  border: 1px solid #d0d0d0;"
        "  border-radius: 4px;"
        "  padding: 8px;"
        "  background: #ffffff;"
        "  color: #333333;"
        "}"
    );
    textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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
    contentLayout->addWidget(textEdit);

    // 搜索栏
    QHBoxLayout *searchLayout = new QHBoxLayout();
    QLineEdit *searchEdit = new QLineEdit(contentWidget);
    searchEdit->setPlaceholderText(tr("输入关键字搜索 (Ctrl+F)"));
    searchEdit->setFixedHeight(30);
    searchEdit->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #c0c0c0;"
        "  border-radius: 4px;"
        "  padding: 2px 8px;"
        "  background: #fafafa;"
        "}"
    );

    QPushButton *findPrevBtn = new QPushButton(tr("上一个"), contentWidget);
    findPrevBtn->setFixedSize(96, 36);
    findPrevBtn->setStyleSheet(
        "QPushButton { border: 1px solid #0099FF; border-radius: 4px; background: #E6F4FF; color: #0099FF; font-weight: bold; }"
        "QPushButton:hover { background: #0099FF; color: #ffffff; }"
    );
    QPushButton *findNextBtn = new QPushButton(tr("下一个"), contentWidget);
    findNextBtn->setFixedSize(96, 36);
    findNextBtn->setStyleSheet(
        "QPushButton { border: 1px solid #0099FF; border-radius: 4px; background: #E6F4FF; color: #0099FF; font-weight: bold; }"
        "QPushButton:hover { background: #0099FF; color: #ffffff; }"
    );
    QLabel *resultLabel = new QLabel(contentWidget);
    resultLabel->setFixedWidth(80);
    resultLabel->setStyleSheet("color: #6D7682; font-size: 13px; background: transparent;");

    searchLayout->addWidget(searchEdit, 0, Qt::AlignVCenter);
    searchLayout->addWidget(findPrevBtn, 0, Qt::AlignVCenter);
    searchLayout->addWidget(findNextBtn, 0, Qt::AlignVCenter);
    searchLayout->addWidget(resultLabel, 0, Qt::AlignVCenter);
    searchLayout->addStretch();
    contentLayout->addLayout(searchLayout);

    // 搜索逻辑
    QList<QTextCursor> *searchResults = new QList<QTextCursor>();
    int *currentResultIndex = new int(-1);

    auto doSearch = [textEdit, searchEdit, resultLabel, searchResults, currentResultIndex]() {
        searchResults->clear();
        *currentResultIndex = -1;
        QString keyword = searchEdit->text();
        if (keyword.isEmpty()) {
            resultLabel->clear();
            textEdit->setTextCursor(QTextCursor(textEdit->document()));
            return;
        }
        QTextDocument *doc = textEdit->document();
        QTextCursor cursor(doc);
        while (!cursor.isNull() && !cursor.atEnd()) {
            cursor = doc->find(keyword, cursor);
            if (!cursor.isNull()) {
                searchResults->append(cursor);
            }
        }
        if (searchResults->isEmpty()) {
            resultLabel->setText(ScriptErrorDialog::tr("无匹配"));
        } else {
            *currentResultIndex = 0;
            textEdit->setTextCursor(searchResults->at(0));
            textEdit->ensureCursorVisible();
            resultLabel->setText(QString("1/%1").arg(searchResults->size()));
        }
    };

    auto goToResult = [textEdit, resultLabel, searchResults, currentResultIndex, doSearch](int offset) {
        if (searchResults->isEmpty()) {
            doSearch();
            return;
        }
        *currentResultIndex += offset;
        if (*currentResultIndex < 0) *currentResultIndex = searchResults->size() - 1;
        if (*currentResultIndex >= searchResults->size()) *currentResultIndex = 0;
        textEdit->setTextCursor(searchResults->at(*currentResultIndex));
        textEdit->ensureCursorVisible();
        resultLabel->setText(QString("%1/%2").arg(*currentResultIndex + 1).arg(searchResults->size()));
    };

    connect(searchEdit, &QLineEdit::returnPressed, this, [goToResult]() { goToResult(1); });
    connect(searchEdit, &QLineEdit::textChanged, this, doSearch);
    connect(findNextBtn, &QPushButton::clicked, this, [goToResult]() { goToResult(1); });
    connect(findPrevBtn, &QPushButton::clicked, this, [goToResult]() { goToResult(-1); });

    QShortcut *shortcut = new QShortcut(QKeySequence::Find, this);
    connect(shortcut, &QShortcut::activated, this, [searchEdit]() { searchEdit->setFocus(); searchEdit->selectAll(); });

    // 底部按钮栏
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();

    QPushButton *closeBtn = new QPushButton(tr("关闭"), contentWidget);
    closeBtn->setFixedSize(100, 36);
    closeBtn->setStyleSheet(
        "QPushButton {"
        "  border: 1px solid #FF4A4A;"
        "  border-radius: 4px;"
        "  padding: 5px 16px;"
        "  background: #FFF0F0;"
        "  color: #FF4A4A;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #FF4A4A;"
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
