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
    ui->edtFileIP->setText(DEFAULT_FILE_IP);
    ui->edtFilePort->setText(QString::number(DEFAULT_FILE_PORT));

    setLocalUserStatus(false);
    setLocalFileStatus(false);

    sendTimes = 0;
    QFont font;
    font.setPixelSize(DEFAULT_MESSAGE_FONT_SIZE);
    ui->browserMessage->setFont(font);

    messageReader = new QUdpSocket();
    messageSender = new QUdpSocket();

    messageReader->bind(DEFAULT_MESSAGE_PORT, QUdpSocket::ShareAddress| QUdpSocket::ReuseAddressHint);
    connect(messageReader,SIGNAL(readyRead()),this, SLOT(readAllMessage()));

    connect(ui->btnLogin,SIGNAL(clicked()),this, SLOT(sendLoginMessage()));
    connect(ui->btnLogout,SIGNAL(clicked(bool)),this,SLOT(sendLogoutMessage()));
    connect(ui->btnSendMessage,SIGNAL(clicked()),this,SLOT(sendChatMessage()));
    connect(ui->btnChooseFile,SIGNAL(clicked(bool)),this,SLOT(chooseSendFile()));

    connect(ui->btnListen,SIGNAL(clicked(bool)),this,SLOT(listen()));
    connect(ui->btnSendFile,SIGNAL(clicked(bool)),this,SLOT(sendConnection()));

    fileServer = new QTcpServer();
    connect(fileServer, SIGNAL(newConnection()), this, SLOT(acceptConnection()));

    sendSocket = new QTcpSocket();
    connect(sendSocket, SIGNAL(connected()), this, SLOT(sendFileInfo()));
    connect(sendSocket, SIGNAL(bytesWritten(qint64)),this,SLOT(continueToSend(qint64)));

}

//-----------------------------------------------------

Qmess::~Qmess()
{
    sendJson(Logout,ui->edtName->text());
    delete ui;
}

//-----------------------------------------------------

bool Qmess::localUserStatus()
{
    return (ui->edtName->isEnabled()) ? false : true;
}

//-----------------------------------------------------

void Qmess::setLocalUserStatus(bool status)
{
    ui->edtName->setEnabled(!status);
    ui->edtMessage->setEnabled(status);
    ui->btnSendMessage->setEnabled(status);
    ui->btnLogin->setEnabled(!status);
    ui->btnLogout->setEnabled(status);
    ui->boxMask->setEnabled(!status);
}

//-----------------------------------------------------

void Qmess::setLocalFileStatus( bool status )
{
    ui->edtFileIP->setEnabled(status);
    ui->edtFilePort->setEnabled(status);
    ui->btnChooseFile->setEnabled(status);
    ui->btnListen->setEnabled(status);
    listenType=Unlisten;
    ui->btnListen->setText(tr("Listen"));
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
                    if(localUserStatus())
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
    if(!Tools::validNickName(ui->edtName->text()))
        QMessageBox::information(this,tr("Invaild Nickname!"),tr("Invaild Nickname!"),QMessageBox::Yes);
    else
    {
        setLocalUserStatus(true);
        setLocalFileStatus(true);
        ui->btnListen->setEnabled(true);
        sendJson(Login,ui->edtName->text());
    }
}

//-----------------------------------------------------

void Qmess::sendLogoutMessage()
{
    setLocalUserStatus(false);
    setLocalFileStatus(false);
    sendJson(Logout,ui->edtName->text());
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
        sendTimes = 0;
        sendFileBlock.clear();
    }
}

//-----------------------------------------------------

void Qmess::listen()
{
    if(listenType == Unlisten)
    {
        ui->btnListen->setText(tr("UnListen"));
        listenType = Listen;
        fileServer->listen(QHostAddress(ui->edtFileIP->text()),ui->edtFilePort->text().toInt());
        showMessage(System,tr("System"),tr(" -- File Port Listening"));
    }
    else if(listenType == Unlisten)
    {
        ui->btnListen->setText(tr("UnListen"));
        listenType = Listen;

        fileServer->close();
        showMessage(System,tr("System"),tr(" -- File Port Listening Closed"));
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

        QString name = receiveFileName.mid(0,receiveFileName.lastIndexOf("."));
        QString suffix = receiveFileName.mid(receiveFileName.lastIndexOf(".")+1,receiveFileName.size());

        if( QSysInfo::kernelType() == "linux" )
        {
            if(QFile::exists(receiveFileName))
            {
                int id = 1;
                while( QFile::exists( name + "(" + QString::number(id) + ")." + suffix ) )
                    id++;
                recieveFile = new QFile( name + "(" + QString::number(id) + ")." + suffix );
            }
            else
                recieveFile = new QFile(receiveFileName);
        }
        else
        {
            if(QFile::exists(DEFAULT_FILE_STORE+receiveFileName))
             {
                 int id = 1;
                 while( QFile::exists(DEFAULT_FILE_STORE + name + "(" + QString::number(id) + ")." + suffix) )
                     id++;
                 recieveFile = new QFile(DEFAULT_FILE_STORE + name + "(" + QString::number(id) + ")." + suffix);
             }
             else
             {
                 recieveFile = new QFile(DEFAULT_FILE_STORE+receiveFileName);
             }
        }

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
            if(QSysInfo::kernelType() == "linux")
            {
                QDir dir("");
                QDesktopServices::openUrl(QUrl(dir.absolutePath() , QUrl::TolerantMode)); // 打开文件夹
            }
            else
            {
                QDir dir(DEFAULT_FILE_STORE);
                QDesktopServices::openUrl(QUrl(dir.absolutePath() , QUrl::TolerantMode)); // 打开文件夹
            }
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
    sendFileEachSize = 40 * 1024;

    QDataStream out(&sendFileBlock,QIODevice::WriteOnly);
    out<<qint64(0)<<qint64(0)<<sendFileName;

    sendFileTotalSize = sendFile->size() + sendFileBlock.size();
    sendFileLeftSize = sendFile->size() + sendFileBlock.size();

    out.device()->seek(0);
    out<<sendFileTotalSize<<qint64(sendFileBlock.size());

    sendSocket->write(sendFileBlock);

    ui->ProgressBar->setMaximum(sendFileTotalSize);
    ui->ProgressBar->setValue(sendFileTotalSize - sendFileLeftSize);
    ui->ProgressBar->show();

    showMessage(System,tr("System"),tr(" -- File Name: %1 File Size: %2").arg(sendFileName,QString::number(sendFileTotalSize)));
}

//-----------------------------------------------------

void Qmess::sendConnection()
{

    if(sendTimes == 0)
    {
        sendSocket->connectToHost(QHostAddress(ui->edtFileIP->text()),ui->edtFilePort->text().toInt());
        if(!sendSocket->waitForConnected(2000))
            QMessageBox::information(this,tr("ERROR"),tr("Network error"),QMessageBox::Yes);
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

    if(sendFileLeftSize == 0)
    {
        showMessage(System,tr("System"),tr(" -- File Transmission Complete"));

        sendSocket->disconnectFromHost();

        ui->ProgressBar->hide();
    }
    else
    {
        sendFileBlock = sendFile->read( qMin(sendFileLeftSize,sendFileEachSize) );

        sendSocket->write(sendFileBlock);

        ui->ProgressBar->setValue(sendFileTotalSize - sendFileLeftSize);
    }

}

//-----------------------------------------------------
