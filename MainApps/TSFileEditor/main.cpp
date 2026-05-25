#include "MainWindow.h"
#include <QApplication>
#include <QDebug>
#include <QSslSocket>
#include <QSslConfiguration>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone); // ??????
    QSslConfiguration::setDefaultConfiguration(sslConfig);
    return a.exec();
}
