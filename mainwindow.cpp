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
    socket_->AddErrorHandler([this](auto& str){ EventServerConnectionHandler(str); });
    socket_->AddRxMsgHandler([this](const ProtosMessage& rxMsg) { fwLoader_->ParseBootMsg(rxMsg); });
    connect(fwLoader_.get(), &FWLoader::signalNextBlock, this, [this](uint d, uint u, uint a){ updateStatus(d, u, a); });
    connect(fwLoader_.get(), &FWLoader::signalAckReceived, this, [this]{ ackInBootReceived(); });
    connect(fwLoader_.get(), &FWLoader::signalFinishedOK, this, [this](uint uid, int msecs){ finishedOk(uid, msecs); });
    connect(fwLoader_.get(), &FWLoader::signalError, this, [this](const QString& e, uint uid){ getError(e, uid); });
}

void MainWindow::AddDevice(){
    QDialog dlg(this);
    dlg.setWindowTitle(tr("AddDevice"));
    auto *uid = new QLineEdit(lastUID, &dlg);
    auto *addr = new QLineEdit(lasrADDR, &dlg);
    auto *ver = new QLineEdit(SWVer, &dlg);

    auto *btn_box = new QDialogButtonBox(&dlg);
    btn_box->setStandardButtons(QDialogButtonBox::Ok);
    connect(btn_box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);

    auto *layout = new QFormLayout();
    layout->addRow(tr("UID: "), uid);
    layout->addRow(tr("ADDR: "), addr);
    layout->addRow(tr("SW Ver: "), ver);
    layout->addWidget(btn_box);

    dlg.setLayout(layout);
    if(dlg.exec() == QDialog::Accepted) {
        lasrADDR = addr->text();
        lastUID = uid->text();
        SWVer = ver->text();
        uint32_t uid24 = uid->text().toUInt(nullptr, 16);
        uint8_t addr8 = addr->text().toShort(nullptr, 16);
        uchar version = ver->text().toShort(nullptr, 16);
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        tr("Open Bin"), "/home", tr("Bin Files (*.bin)"));
//            QString fileName = "D:\\u\\sa_pico_can.bin";
        QFile file(fileName);
        if (file.open(QIODevice::ReadWrite))
        {
            fwLoader_->addDevice(fileName, addr8, uid24, 0x1, version);
            qDebug() << (tr("UID: %1 ADDR: %2 ").arg(uid24).arg(addr8));
//            statusLabel->setText(tr("Device loaded UID: %1 ADDR: %2 ").arg(uid24, 8, 16).arg(addr8, 2,16));
//            logView->AddMsg(tr("Device loaded UID: %1 ADDR: %2 ").arg(uid24, 8, 16).arg(addr8, 2,16));
            ui-
        }
        else{
//            logView->AddMsg(tr("Failed to open file"));
        }
    } else{
//        statusLabel->setText(tr("Aborted adding device"));
//        logView->AddMsg(tr("Aborted adding device"));
    }
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

void MainWindow::updateStatus(uint delta, uint uid, uint addr) {
    if(delta >= 100) statusLabel->setText("Status: all packets sent and received by target");
    else {
        statusLabel->setText(QStringLiteral("Status: %1%").arg(delta));
        logView->AddMsg(tr("Device UID: %1, Boot Addr: %2 Status: %3%").arg(uid,8,16).arg(addr, 3,16).arg(delta));
    }
}

void MainWindow::ackInBootReceived() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("ackRecieved"));
    QDialogButtonBox *btn_box = new QDialogButtonBox(&dlg);
    btn_box->setStandardButtons(QDialogButtonBox::Ok);
    connect(btn_box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QFormLayout *layout = new QFormLayout();
    layout->addWidget(btn_box);
    dlg.setLayout(layout);
    if(dlg.exec() == QDialog::Accepted) {
    }
}

void MainWindow::finishedOk(uint uid, int msecs) {
    QString msg = QString(tr("On device UID:%1 successfully loaded new FW. Time spent(msecs): %2").arg(uid, 8, 16).arg(msecs));
    logView->AddMsg(msg);
    statusLabel->setText(msg);
}

void MainWindow::getError(const QString &error, uint uid) {
    QDialog dlg(this);
    dlg.setWindowTitle(error);
    auto *layout = new QFormLayout();
    auto errorLabel = new QLabel(error, &dlg);
    auto questionLabel = new QLabel((QStringLiteral("Finish device FW with UID: %1").arg(uid,8,16)), &dlg);
    auto *btn_box = new QDialogButtonBox(&dlg);
    btn_box->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btn_box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btn_box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    errorLabel->setAlignment(Qt::AlignHCenter);
    errorLabel->setMinimumSize(statusLabel->sizeHint());
    layout->addRow(errorLabel);
    layout->addRow(questionLabel);
    layout->addWidget(btn_box);

    dlg.setLayout(layout);

    if(dlg.exec() == QDialog::Accepted) {
        fwLoader->cancelFWLoad(uid);
    }
}