#include <QFile>
#include <QListWidget>

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "config.h"
#include "QLineEdit"
#include "QDialogButtonBox"
#include "QFormLayout"
#include "QFileDialog"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket_(new Tcp_socket())
    , serverConnectionDlg_(new ServerConnectionDlg(socket_, this))
    , fwLoader_(new FWLoader(socket_))
{
    ui->setupUi(this);
    SetViewIcons();
    SignalConnections();
    LoadFromJson();
    socket_->AddErrorHandler([this](auto& str){ LogEvent(str); });
    socket_->AddRxMsgHandler([this](const ProtosMessage& rxMsg) { fwLoader_->ParseBootMsg(rxMsg); });
}

void MainWindow::AddDevice(){
    QDialog dlg(this);
    dlg.setWindowTitle(tr("AddDevice"));
    auto *uid = new QLineEdit(lastUID, &dlg);
    auto *addr = new QLineEdit(lasrADDR, &dlg);
    auto *sw_ver = new QLineEdit(SWVer, &dlg);

    auto *btn_box = new QDialogButtonBox(&dlg);
    btn_box->setStandardButtons(QDialogButtonBox::Ok);
    connect(btn_box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);

    auto *layout = new QFormLayout();
    layout->addRow(tr("uid_(hex) 0x: "), uid);
    layout->addRow(tr("ADDR(hex) 0x: "), addr);
    layout->addRow(tr("SW Ver: "), sw_ver);
    layout->addWidget(btn_box);

    dlg.setLayout(layout);
    if(dlg.exec() == QDialog::Accepted) {
        lasrADDR = addr->text();
        lastUID = uid->text();
        SWVer = sw_ver->text();
        uint32_t UID = uid->text().toUInt(nullptr, 16);
        uint8_t addr8 = addr->text().toShort(nullptr, 16);
        uchar version = sw_ver->text().toShort(nullptr, 10);
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("Open Bin"), "/home", tr("Bin Files (*.bin)"));
        QFile file(fileName);
        if (file.open(QIODevice::ReadWrite))
        {
            devices_.insert(UID, new DeviceItem());
            auto new_device = devices_[UID];
            new_device->SetReqData(fileName, addr8, version);
            AddDeviceToGrid(new_device);
            connect(new_device, &DeviceItem::RefreshDevice, this, [this, UID] { fwLoader_->RefreshDevice(UID);});
            connect(new_device, &DeviceItem::CancelDevice, this, [this, UID] { fwLoader_->CancelFWLoad(UID);});
            fwLoader_->addDevice(fileName, addr8, UID, 0x1, version);
        }
        else
            LogEvent(tr("Failed to open file"));
    }
}

void MainWindow::AddDeviceToGrid(DeviceItem* item) {
    ui->gridLayout->addWidget(item, row_, column_);
    if(column_ < k_column_n_)
        column_++;
    else{
        column_ = 0;
        row_++;
    }
}

void MainWindow::SignalConnections(){
    connect(ui->status_button, &QPushButton::clicked, [this](){ StatusBtnClicked(); });
    connect(ui->server_button, &QPushButton::clicked, [this](){ ServerBtnClicked(); });
    connect(serverConnectionDlg_, &ServerConnectionDlg::eventInServerConnection,
            [this](const QString& s, bool b){ LogEvent(s, b);});
    connect(serverConnectionDlg_, &ServerConnectionDlg::SaveMe, [this](){ SaveToJson();});
    connect(ui->status_listWidget, &QListWidget::itemDoubleClicked, [this](){ ui->status_listWidget->clear(); });
    connect(ui->add_device_btn, &QPushButton::clicked, this, [this]{
        AddDevice();
    });
    connect(fwLoader_.get(), &FWLoader::signalNextBlock, this, [this](uint d, uint u, uint a){
        UpdateDeviceStatus(d, u, a);
    });
    connect(fwLoader_.get(), &FWLoader::signalAckReceived, this, [this](uint uid, uchar addr, uchar hw, uchar fw) {
        AckInBootReceived(uid, addr, hw, fw);
    });
    connect(fwLoader_.get(), &FWLoader::signalFinishedOK, this, [this](uint uid, qint64 msecs){
        FinishedOk(uid, msecs);
    });
    connect(fwLoader_.get(), &FWLoader::signalError, this, [this](const QString& e, uint uid){
        GetError(e, uid);
    });
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

void MainWindow::LogEvent(const QString& eventStr, bool isError){
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

void MainWindow::UpdateDeviceStatus(uint delta, uint uid, uint addr) {
    if(!devices_.contains(uid)) return;
    auto device_item = devices_[uid];
    device_item->SetStatusValue(delta);
}

void MainWindow::AckInBootReceived(uint uid, uchar addr, uchar hw, uchar fw) {
    if(!devices_.contains(uid))
        return;
    auto device_item = devices_[uid];
    device_item->ProcessStarted();
    device_item->PostDeviceData(uid, addr, hw, fw);
    LogEvent(tr("Device loaded with UID: 0x%1").arg(uid,1,16), false);
}

void MainWindow::FinishedOk(uint uid, qint64 msecs) {
    if(!devices_.contains(uid))
        return;
    auto device_item = devices_[uid];
    device_item->ProcessFinished(msecs);
}

void MainWindow::GetError(const QString &error, uint uid) {
    QDialog dlg(this);
    dlg.setWindowTitle(error);
    auto *layout = new QFormLayout();
    auto errorLabel = new QLabel(error, &dlg);
    auto questionLabel = new QLabel((QStringLiteral("Finish device FW with UID: 0x%1").arg(uid,1,16)), &dlg);
    auto *btn_box = new QDialogButtonBox(&dlg);
    btn_box->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btn_box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btn_box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    errorLabel->setAlignment(Qt::AlignHCenter);
    layout->addRow(errorLabel);
    layout->addRow(questionLabel);
    layout->addWidget(btn_box);

    dlg.setLayout(layout);

    if(dlg.exec() == QDialog::Accepted)
        fwLoader_->CancelFWLoad(uid);
}