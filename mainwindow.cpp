#include <QFile>
#include <QListWidget>

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "config.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket_(new Tcp_socket())
    , serverConnectionDlg_(new ServerConnectionDlg(socket_, this))
{
    ui->setupUi(this);
    SetViewIcons();
    SignalConnections();
    LoadFromJson();
    socket_->AddErrorHandler([this](auto& str){ EventServerConnectionHandler(str); });
}

void MainWindow::SignalConnections(){
    connect(ui->status_button, &QPushButton::clicked, [this](){ StatusBtnClicked(); });
    connect(ui->server_button, &QPushButton::clicked, [this](){ ServerBtnClicked(); });
    connect(serverConnectionDlg_, &ServerConnectionDlg::eventInServerConnection,
            [this](const QString& s, bool b){ EventServerConnectionHandler(s, b);});
    connect(serverConnectionDlg_, &ServerConnectionDlg::SaveMe, [this](){ SaveToJson();});
    connect(ui->status_listWidget, &QListWidget::itemDoubleClicked, [this](){ ui->status_listWidget->clear(); });
}

void MainWindow::ServerBtnClicked(){
    serverConnectionDlg_->show();
    serverConnectionDlg_->raise();
}

void MainWindow::StatusBtnClicked(){
    for(QListWidgetItem* item : ui->status_listWidget->selectedItems()){
        ui->status_listWidget->removeItemWidget(item);
        delete item;
    }
    hasError_ = false;
    ui->status_listWidget->clear();
    if(socket_->IsConnected())
        ui->status_button->setIcon(statusOkIcon);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::LoadFromJson(){
    auto pathToFile = QString(kBUILD_TYPE) == "Debug" ? "/../" : "/";
    auto configFile = new QFile(QCoreApplication::applicationDirPath() + QString("%1/saved.json").arg(pathToFile));
    configFile->open(QIODevice::ReadWrite);
    QByteArray saveData = configFile->readAll();
    configFile->close();
    QJsonDocument jsonDocument(QJsonDocument::fromJson(saveData));
    jsonSaved_ = jsonDocument.object();
    serverConnectionDlg_->loadDataFromJson(jsonSaved_["serverConnection"].toObject());
}


void MainWindow::SaveToJson() {
    jsonSaved_["serverConnection"] = serverConnectionDlg_->SaveDataToJson();

    QJsonDocument doc;
    doc.setObject(jsonSaved_);
    auto pathToFile = QString(kBUILD_TYPE) == "Debug" ? "/../" : "/";
    auto configFile = new QFile(QCoreApplication::applicationDirPath() + QString("%1/saved.json").arg(pathToFile));
    configFile->open(QIODevice::ReadWrite);
    configFile->resize(0);
    configFile->write(doc.toJson(QJsonDocument::Indented));
    configFile->close();
}

void MainWindow::EventServerConnectionHandler(const QString& eventStr, bool isError){
    if(isError){
        ui->status_listWidget->addItem(eventStr);
        ui->status_listWidget->item(ui->status_listWidget->count()-1)->setIcon(statusErrorIcon);
    } else{
        ui->status_listWidget->addItem(eventStr);
        ui->status_listWidget->item(ui->status_listWidget->count()-1)->setIcon(statusOkIcon);
    }
    ui->server_button->setIcon(socket_->IsConnected() ? serverConnectedIcon : serverDisconnectedIcon);
    if(!hasError_)
        ui->status_button->setIcon(socket_->IsConnected() ? statusOkIcon : statusErrorIcon);
}

void MainWindow::SetViewIcons(){
    statusOkIcon = QIcon(":/icons/icons/ok_circle.svg");
    statusErrorIcon = QIcon(":/icons/icons/error_circle.svg");
    serverConnectedIcon = QIcon(":/icons/icons/server_connected.svg");
    serverDisconnectedIcon = QIcon(":/icons/icons/server_disconnected.svg");
}