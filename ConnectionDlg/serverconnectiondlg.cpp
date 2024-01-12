#include <QRegExpValidator>
#include "QJsonObject"

#include "serverconnectiondlg.h"
#include "ui_serverconnectiondlg.h"
#include "config.h"

ServerConnectionDlg::~ServerConnectionDlg()
{
    delete ui;
}

ServerConnectionDlg::ServerConnectionDlg(const QSharedPointer<Tcp_socket>& socket, QWidget *parent):
        QDialog(parent),
        ui(new Ui::ServerConnectionDlg),
        socket_(socket),
        autoConnect_(false),
        serverReconnectionTimer_(new QTimer(this))
{
    ui->setupUi(this);
    ui->ip_lineEdit->setValidator(new QRegExpValidator(QRegExp(R"([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3})")));
    ui->port_lineEdit->setValidator(new QRegExpValidator(QRegExp("[0-9]{4}")));
    connect(ui->connect_pushButton, &QPushButton::clicked, [this](){ connectBtnClicked();});
    connect(ui->autoConnect_pushButton, &QPushButton::clicked, [this](){ autoConnectBtnClicked();});
    ui->connect_pushButton->setCheckable(true);

    connect(serverReconnectionTimer_, &QTimer::timeout, [this]() { ServerReconnectionTimerHandler(); });
    serverReconnectionTimer_->start(kServerReconnectionTime);
 }

void ServerConnectionDlg::connectBtnClicked(){
    if(socket_->IsConnected()){
        socket_->Disconnect();
        emit eventInServerConnection(
                QString("Event is server connection: "
                        "disconnected from server"), false);
    }
    else
        connectToServer();
    emit SaveMe();
}

void ServerConnectionDlg::connectToServer(){
    QString ip, port;
    ip = ui->ip_lineEdit->text();
    port = ui->port_lineEdit->text();
    bool isConnected = socket_->Connect(ip, port.toInt());
    if(isConnected){
        emit eventInServerConnection(
                QString("Event is server connection: "
                        "Connected to sever ip: %1 pot: %2").arg(ip, port), false);
    }
    else{
        emit eventInServerConnection(
                QString("Event is server connection: "
                        "Cant connect to sever ip: %1 pot: %2").arg(ip, port), true);
    }
    ui->connect_pushButton->setChecked(isConnected);
}

void ServerConnectionDlg::autoConnectBtnClicked(){
    autoConnect_ = !autoConnect_;
    if(autoConnect_)
        ui->autoConnect_pushButton->setChecked(autoConnect_);
    emit autoConnectStateChanged(autoConnect_);
}

QJsonObject ServerConnectionDlg::SaveDataToJson(){
    QJsonObject retVal;
    retVal["IP"] = ui->ip_lineEdit->text();
    retVal["Port"] = ui->port_lineEdit->text();
    retVal["autoConnect_"] = autoConnect_;
    return retVal;
}

void ServerConnectionDlg::loadDataFromJson(const QJsonObject& jsonObject){
    if(jsonObject.empty())
        return;
    ui->ip_lineEdit->setText(jsonObject["IP"].toString());
    ui->port_lineEdit->setText(jsonObject["Port"].toString());
    ui->autoConnect_pushButton->setChecked(jsonObject["autoConnect_"].toBool());
    autoConnect_ = jsonObject["autoConnect_"].toBool();
}

bool ServerConnectionDlg::reconnectIsOn() const {
    return autoConnect_;
}

void ServerConnectionDlg::ServerReconnectionTimerHandler(){
    if(reconnectIsOn() && !socket_->IsConnected())
        connectToServer();
}