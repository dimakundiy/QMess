#include "tools.h"

#include <QNetworkInterface>
#include <QRegExp>

//-----------------------------------------------------

Tools::Tools()
{

}

//-----------------------------------------------------

QString Tools::toIPv4(qint32 arg)
{
    QString res;
    int bits[4], cnt=0;

    while(arg)
    {
        bits[cnt] = arg % 256;
        arg /= 256;
        cnt++;
    }

    for(int i=3;i>=0;i--)
        res.append(QString::number(bits[i]).append(i==0?"":"."));
    return res;
}

//-----------------------------------------------------

QString Tools::getLocalIP()
{
    //const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    //for (const QHostAddress &address: QNetworkInterface::allAddresses()) {
    //    if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
    //         qDebug() << address.toString();
    //}

    QList<QHostAddress> list = QNetworkInterface::allAddresses();

    foreach(QHostAddress in, list)
    {
        if(in.protocol() == QAbstractSocket::IPv4Protocol && in.toString().contains("192.168"))
            return in.toString();
    }

    return QString();
}

//-----------------------------------------------------

bool Tools::validNickName(QString name)
{
    QRegExp reg1("[A-Za-z0-9_]{1,}");

    return (reg1.exactMatch(name)) ? true : false ;
}

//-----------------------------------------------------
