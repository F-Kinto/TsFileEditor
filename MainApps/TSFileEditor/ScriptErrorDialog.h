#ifndef SCRIPTERRORDIALOG_H
#define SCRIPTERRORDIALOG_H

#include <QDialog>
#include <QObject>
#include <QPainter>
#include <QPainterPath>

class DragEventFilter : public QObject
{
    Q_OBJECT
public:
    DragEventFilter(QObject *watched, QWidget *window);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWidget *m_window;
    bool m_dragging;
    QPoint m_dragPos;
};

class ScriptErrorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ScriptErrorDialog(const QString &title, const QString &output, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // SCRIPTERRORDIALOG_H
