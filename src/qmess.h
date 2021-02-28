#ifndef QMESS_H
#define QMESS_H

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

    /* --- Program State for fileServer --- */
    enum CurrentState {
        SendFile,
        ReceiveFile,
        NoState
    };

public:
   explicit Qmess(QWidget *parent = nullptr);
    ~Qmess();

    /* --- Tools --- */
    QString toIPv4(qint32 arg);
    QString getLocalIP();

    /* --- Chat helpers --- */
    bool validNickName(QString name);
    void showMessage(MessageType type, QString hint, QString content);

    /* --- UDP --- */
    void sendJson(MessageType type,QString nick_name,QString content = "");

private:
    Ui::Qmess *ui;

    /* --- UDP --- */
    const qint16 DEFAULT_MESSAGE_PORT = 6108;
    QUdpSocket * messageSender,* messageReader;

    /* --- TCP --- */
    quint16 FILE_PORT = 6109;

    QTcpServer *fileServer;
    CurrentState state;

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
    void stopToRecvFile();
    void readConnection();
    void sendConnection();
    void sendFileInfo();
    void continueToSend(qint64 size);

};
#endif // QMESS_H
