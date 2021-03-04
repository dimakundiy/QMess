#include "qmess.h"
#include "ui_qmess.h"

#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>

//-----------------------------------------------------

Qmess::Qmess(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Qmess)
{
    ui->setupUi(this);

    ui->ProgressBar->hide();

    ui->labIPAdress->setText(Tools::getLocalIP());
    ui->edtFileIP->setText("127.0.0.2");
    ui->edtFilePort->setText(QString::number(FILE_PORT));

    ui->btnSendFile->setEnabled(false);
    ui->edtMessage->setEnabled(false);
    ui->btnSendMessage->setEnabled(false);

    state = NoState;
    sendTimes = 0;
    QFont font;
    font.setPixelSize(14);
    ui->browserMessage->setFont(font);

    messageReader = new QUdpSocket();
    messageSender = new QUdpSocket();

    messageReader->bind(DEFAULT_MESSAGE_PORT, QUdpSocket::ShareAddress| QUdpSocket::ReuseAddressHint);
    connect(messageReader,SIGNAL(readyRead()),this, SLOT(readAllMessage()));

    connect(ui->btnLogin,SIGNAL(clicked()),this, SLOT(sendLoginMessage()));
    connect(ui->btnLogout,SIGNAL(clicked(bool)),this,SLOT(sendLogoutMessage()));
    connect(ui->btnSendMessage,SIGNAL(clicked()),this,SLOT(sendChatMessage()));
    connect(ui->btnChooseFile,SIGNAL(clicked(bool)),this,SLOT(chooseSendFile()));

    fileServer = new QTcpServer();
    connect(fileServer, SIGNAL(newConnection()), this, SLOT(acceptConnection()));

    sendSocket = new QTcpSocket();
    connect(sendSocket, SIGNAL(connected()), this, SLOT(sendFileInfo()));

    connect(ui->btnListen,SIGNAL(clicked(bool)),this,SLOT(listen()));
    connect(ui->btnSendFile,SIGNAL(clicked(bool)),this,SLOT(sendConnection()));
}

//-----------------------------------------------------

Qmess::~Qmess()
{
    sendJson(Logout,ui->edtName->text());
    delete ui;
}

//-----------------------------------------------------

void Qmess::showMessage(MessageType type, QString hint, QString content)
{
    QDateTime now = QDateTime::currentDateTime();

    switch (type) {
        case Login:
        case Logout:
        case System:
        {
            ui->browserMessage->setTextColor(QColor(190,190,190));
            ui->browserMessage->append(hint+ now.toString("  hh:mm::ss"));
            ui->browserMessage->append(content);
        } break;
        case Chat:
        {
            ui->browserMessage->setTextColor(QColor(70,130,180));
            ui->browserMessage->append(hint + now.toString("  hh:mm:ss"));
            ui->browserMessage->setTextColor(QColor(0,0,0));
            ui->browserMessage->append(content);
        } break;

        default: break;
    }
}

//-----------------------------------------------------

void Qmess::sendJson(MessageType type, QString nick_name, QString content)
{
    QJsonObject obj;

    if(nick_name.isEmpty())
        return;

    switch (type) {
        case Chat : obj.insert("type", "chat"); break;
        case Login: obj.insert("type", "login"); break;
        case Logout: obj.insert("type", "logout"); break;
        case Online: obj.insert("type", "online"); break;

        default: break;
    }

    if(content != "")
        obj.insert("content", content);

    obj.insert("nick-name", nick_name);

    QJsonDocument doc;
    doc.setObject(obj);

    QByteArray data = doc.toJson();

    messageSender->writeDatagram(data.data(), data.size(),QHostAddress(ui->boxMask->currentText()),DEFAULT_MESSAGE_PORT);

}

//-----------------------------------------------------

void Qmess::readAllMessage()
{
    while (messageReader->hasPendingDatagrams())
    {
        QByteArray data;
        data.resize(messageReader->pendingDatagramSize());
        QHostAddress source;
        messageReader->readDatagram(data.data(),data.size(),&source);

        QJsonParseError jsonError;
        QJsonDocument doc = QJsonDocument::fromJson(data,&jsonError);
        if(jsonError.error == QJsonParseError::NoError && doc.isObject())
        {
            QJsonObject obj = doc.object();
            if(obj.contains("type") && obj.contains("nick-name"))
            {
                QJsonValue type = obj.take("type");
                QString info = obj.take("nick-name").toString() + "(" + Tools::toIPv4(source.toIPv4Address()) + ")" ;
                if(type.toString() == "chat" && obj.contains("content"))
                {
                    showMessage(Chat,info,obj.take("content").toString());
                }
                else if(type.toString() == "login")
                {
                    QList<QListWidgetItem *> user = ui->listOnlineUser->findItems(info, Qt::MatchExactly | Qt::MatchCaseSensitive );
                    if(user.size() == 0)
                        ui->listOnlineUser->insertItem(ui->listOnlineUser->count()+1,info);
                    showMessage(Login,info,tr(" -- enter the chat room"));
                    if(!ui->edtName->isEnabled())
                        sendJson(Online,ui->edtName->text());
                }
                else if(type.toString() == "logout")
                {
                    QList<QListWidgetItem *> user = ui->listOnlineUser->findItems(info, Qt::MatchExactly | Qt::MatchCaseSensitive );
                    for(auto it = user.begin();it!=user.end();it++)
                    {
                        ui->listOnlineUser->removeItemWidget((*it));
                        delete (*it);
                    }
                    showMessage(Logout,info,tr(" -- quit the chat room"));
                }
                else if(type.toString() == "online")
                {
                    QList<QListWidgetItem *> user = ui->listOnlineUser->findItems(info, Qt::MatchExactly | Qt::MatchCaseSensitive );
                    if(user.size() == 0)
                        ui->listOnlineUser->insertItem(ui->listOnlineUser->count()+1,info);
                }
            }
        }
    }
}

//-----------------------------------------------------

void Qmess::sendChatMessage()
{
    if(ui->edtMessage->toPlainText().isEmpty())
        QMessageBox::information(this,tr("No message"),tr("No message"),QMessageBox::Yes);
    else
    {
        sendJson(Chat,ui->edtName->text(),ui->edtMessage->toPlainText());
        ui->edtMessage->clear();
    }
}

//-----------------------------------------------------

void Qmess::sendLoginMessage()
{
    if(!ui->edtName->isEnabled())
        QMessageBox::information(this,tr("Have logined!"),tr("Have logined!"),QMessageBox::Yes);
    else if(!Tools::validNickName(ui->edtName->text()))
        QMessageBox::information(this,tr("Invaild Nickname!"),tr("Invaild Nickname!"),QMessageBox::Yes);
    else
    {
        ui->edtName->setEnabled(false);
        ui->edtMessage->setEnabled(true);
        ui->btnSendMessage->setEnabled(true);
        sendJson(Login,ui->edtName->text());
    }
}

//-----------------------------------------------------

void Qmess::sendLogoutMessage()
{
    if(ui->edtName->isEnabled())
        QMessageBox::information(this,tr("Have not logined!"),tr("Have not logined!"),QMessageBox::Yes);
    else
    {
        ui->edtName->setEnabled(true);
        ui->edtMessage->setEnabled(false);
        ui->btnSendMessage->setEnabled(false);
        sendJson(Logout,ui->edtName->text());
    }
}

//-----------------------------------------------------

void Qmess::chooseSendFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Choose File"), ".", tr("All File(*.*)"));

    if (!fileName.isEmpty())
    {
        ui->btnSendFile->setEnabled(true);
        sendFile = new QFile(fileName);
        sendFile->open(QIODevice::ReadOnly);
        sendFileName = fileName.right(fileName.size()-fileName.lastIndexOf('/')-1);
        showMessage(System, tr("System"), tr(" -- File Selected: %1").arg(sendFileName));
        sendFileTotalSize = sendFileLeftSize = 0;
        sendFileBlock.clear();
    }
}

//-----------------------------------------------------

void Qmess::listen()
{
    if(ui->edtName->isEnabled())
        QMessageBox::information(this, tr("Have logined!"), tr("Login to continue"), QMessageBox::Yes);

    switch (state) {

        case NoState: {
            ui->edtFileIP->setEnabled(false);
            ui->edtFilePort->setEnabled(false);
            ui->btnSendFile->setEnabled(false);
            ui->btnListen->setText(tr("UnListen"));
            fileServer->listen(QHostAddress(ui->edtFileIP->text()),ui->edtFilePort->text().toInt());
            showMessage(System,tr("System"),tr(" -- File Port Listening"));
            state = ReceiveFile;
        } break;

        case ReceiveFile: {
            ui->edtFileIP->setEnabled(true);
            ui->edtFilePort->setEnabled(true);
            ui->btnListen->setText(tr("Listen"));
            fileServer->close();
            showMessage(System,tr("System"),tr(" -- File Port Listening Closed"));
            state = NoState;
        } break;

        default: break;
    }
}

//-----------------------------------------------------

void Qmess::acceptConnection()
{
    receiveFileTotalSize = receiveFileTransSize = 0;
    showMessage(System,tr("System"),tr(" -- New File Arriving"));

    receiveSocket = fileServer->nextPendingConnection();
    connect(receiveSocket,SIGNAL(readyRead()),this,SLOT(readConnection()));

    ui->ProgressBar->show();
}

//-----------------------------------------------------


void Qmess::readConnection()
{
    if(receiveFileTotalSize == 0)
    {
        QDataStream in(receiveSocket);
        in>>receiveFileTotalSize>>receiveFileTransSize>>receiveFileName;

        ui->ProgressBar->setMaximum(receiveFileTotalSize);

        recieveFile = new QFile("D:\\"+receiveFileName);
        recieveFile->open(QFile::ReadWrite);

        showMessage(System,tr("System"),tr(" -- File Name: %1 File Size: %2").arg(receiveFileName,QString::number(receiveFileTotalSize)));
    }
    else
    {
        receiveFileBlock = receiveSocket->readAll();
        receiveFileTransSize += receiveFileBlock.size();

        ui->ProgressBar->setValue(receiveFileTransSize);

        recieveFile->write(receiveFileBlock);
        recieveFile->flush();
    }

    if(receiveFileTransSize == receiveFileTotalSize)
    {
        showMessage(System,tr("System"),tr(" -- File Transmission Complete"));

        QMessageBox::StandardButton choice;
        choice = QMessageBox::information(this,tr("Open File Folder?"),tr("Open File Folder?"),QMessageBox::Yes,QMessageBox::No);
        if(choice == QMessageBox::Yes)
        {
            QDir dir("D://");
            QDesktopServices::openUrl(QUrl(dir.absolutePath() , QUrl::TolerantMode));
        }
        receiveFileTotalSize = receiveFileTransSize = 0;
        receiveFileName = QString();
        ui->ProgressBar->hide();
        recieveFile->close();
    }
}

//-----------------------------------------------------

void Qmess::sendFileInfo()
{
    sendFileEachSize = 4 * 1024;

    QDataStream out(&sendFileBlock,QIODevice::WriteOnly);
    out<<qint64(0)<<qint64(0)<<sendFileName;

    sendFileTotalSize += sendFile->size() + sendFileBlock.size();
    sendFileLeftSize += sendFile->size() + sendFileBlock.size();

    out.device()->seek(0);
    out<<sendFileTotalSize<<qint64(sendFileBlock.size());

    sendSocket->write(sendFileBlock);

    ui->ProgressBar->setMaximum(sendFileTotalSize);
    ui->ProgressBar->setValue(sendFileTotalSize - sendFileLeftSize);
    ui->ProgressBar->show();

    showMessage(System,tr("System"),tr(" -- File Name: %1 File Size: %2").arg(sendFileName,QString::number(sendFileTotalSize)));

    connect(sendSocket,SIGNAL(bytesWritten(qint64)),this,SLOT(continueToSend(qint64)));
}

//-----------------------------------------------------

void Qmess::sendConnection()
{

    if(sendTimes == 0)
    {
        sendSocket->connectToHost(QHostAddress(ui->edtFileIP->text()),ui->edtFilePort->text().toInt());
        if(!sendSocket->waitForConnected(100))
            QMessageBox::information(this,tr("ERROR"),tr("network error"),QMessageBox::Yes);
        else
            sendTimes = 1;
    }
    else
        sendFileInfo();
}

//-----------------------------------------------------

void Qmess::continueToSend(qint64 size)
{
    if(sendSocket->state() != QAbstractSocket::ConnectedState)
    {
        QMessageBox::information(this,tr("Error"),tr("Error"),QMessageBox::Yes);
        ui->ProgressBar->hide();
        return;
    }

    sendFileLeftSize -= size;

    sendFileBlock = sendFile->read( qMin(sendFileLeftSize,sendFileEachSize) );

    sendSocket->write(sendFileBlock);

    ui->ProgressBar->setValue(sendFileTotalSize - sendFileLeftSize);

    if(sendFileLeftSize == 0)
    {
        showMessage(System,tr("System"),tr(" -- File Transmission Complete"));



        sendSocket->disconnectFromHost();
        sendSocket->close();
        sendTimes = 0;

        ui->ProgressBar->hide();
        sendFileLeftSize  = sendFileTotalSize ;

    }

}

//-----------------------------------------------------
