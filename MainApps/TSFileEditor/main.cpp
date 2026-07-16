#include "MainWindow.h"
#include <QApplication>
#include <QDebug>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>

static QFile *logFile = nullptr;

void logMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (!logFile || !logFile->isOpen()) return;

    QTextStream stream(logFile);
    stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " ";

    switch (type) {
    case QtDebugMsg:
        stream << "[DEBUG] ";
        break;
    case QtWarningMsg:
        stream << "[WARNING] ";
        break;
    case QtCriticalMsg:
        stream << "[CRITICAL] ";
        break;
    case QtFatalMsg:
        stream << "[FATAL] ";
        break;
    case QtInfoMsg:
        stream << "[INFO] ";
        break;
    }

    stream << msg;
    if (context.file && context.line > 0)
        stream << " (" << context.file << ":" << context.line << ")";
    stream << "\n";
    stream.flush();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Setup log file
    QString logPath = QApplication::applicationDirPath() + "/log.txt";
    logFile = new QFile(logPath);
    if (!logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open log file:" << logPath;
        delete logFile;
        logFile = nullptr;
    } else {
        qInstallMessageHandler(logMessageHandler);
    }

    MainWindow w;
    w.show();
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    QSslConfiguration::setDefaultConfiguration(sslConfig);

    int ret = a.exec();

    if (logFile) {
        logFile->close();
        delete logFile;
        logFile = nullptr;
    }

    return ret;
}
