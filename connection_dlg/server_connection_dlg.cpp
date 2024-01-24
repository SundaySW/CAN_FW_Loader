#include <QRegExpValidator>
#include "QJsonObject"

#include "server_connection_dlg.h"
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
    connect(socket_.get(), &Tcp_socket::connected, this, [this](const QString& m){
        ui->connect_pushButton->setChecked(true);
        emit eventInServerConnection(m, false);
    });
    connect(socket_.get(), &Tcp_socket::error, this, [this](const QString& m){
        ui->connect_pushButton->setChecked(false);
        emit eventInServerConnection(m, true);
    });
    connect(socket_.get(), &Tcp_socket::disconnected, this, [this](const QString& m){
        ui->connect_pushButton->setChecked(false);
        emit eventInServerConnection(m, true);
    });
 }

void ServerConnectionDlg::connectBtnClicked(){
    connectToServer();
    emit SaveMe();
}

void ServerConnectionDlg::connectToServer(){
    if(socket_->IsConnected()){
        socket_->Disconnect();
    }else{
        QString ip = ui->ip_lineEdit->text();
        QString port = ui->port_lineEdit->text();
        socket_->Connect(ip, port.toInt());
    }
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