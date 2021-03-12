#ifndef QMESS_H
#define QMESS_H

#include "tools/tools.h"

#include <QMainWindow>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>
#include <QFile>

QT_BEGIN_NAMESPACE
namespace Ui {
class Qmess;
}
QT_END_NAMESPACE

class Qmess : public QMainWindow
{
    Q_OBJECT

    /* --- Message type for showMessage() --- */
    enum MessageType {
        Chat,
        Login,
        Logout,
        Online,
        System
    };

    /* --- Local Listen Type --- */
    enum ListenType{Listen,Unlisten};

public:
   explicit Qmess(QWidget *parent = nullptr);
    ~Qmess();

private:
    Ui::Qmess *ui;

    /* --- UDP --- */
    const qint16 DEFAULT_MESSAGE_PORT = 6108;
    const qint8 DEFAULT_MESSAGE_FONT_SIZE = 14;
    QUdpSocket * messageSender,* messageReader;

    /* --- TCP --- */
    const QString DEFAULT_FILE_IP = "127.0.0.1";
    const quint16 DEFAULT_FILE_PORT = 6109;
    const QString DEFAULT_FILE_STORE = "D:\\";

    /* --- Tools ---*/
    void sendJson(MessageType type,QString nick_name,QString content = "");
    void showMessage(MessageType type,QString hint,QString content);
    bool localUserStatus();
    void setLocalUserStatus(bool status);
    void setLocalFileStatus(bool status);

    QTcpServer *fileServer;

    /* --- File Send --- */
    qint8 sendTimes;
    QTcpSocket *sendSocket;
    QFile *sendFile;
    QString sendFileName;
    qint64 sendFileTotalSize, sendFileLeftSize, sendFileEachSize;
    QByteArray sendFileBlock;

    /* --- File Recieve --- */
    QTcpSocket * receiveSocket;
    QFile *recieveFile, *finalFile;
    QString receiveFileName;
    qint64 receiveFileTotalSize, receiveFileTransSize;
    QByteArray receiveFileBlock;
    ListenType listenType;

private slots:

    /* --- UDP --- */
    void readAllMessage();
    void sendChatMessage();
    void sendLoginMessage();
    void sendLogoutMessage();

    /* --- TCP --- */
    void chooseSendFile();
    void listen();
    void acceptConnection();
    void readConnection();
    void sendConnection();
    void sendFileInfo();
    void continueToSend(qint64 size);

};
#endif // QMESS_H
