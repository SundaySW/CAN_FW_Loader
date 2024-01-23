#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>
#include "QJsonDocument"

#include "serverconnectiondlg.h"
#include "tcp_socket.hpp"
#include "fw_loader.hpp"
#include "DeviceItemView/deviceitem.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    Ui::MainWindow *ui;
    QSharedPointer<Tcp_socket> socket_;
    ServerConnectionDlg* serverConnectionDlg_;
    QJsonObject jsonSaved_;
    QIcon serverConnectedIcon, serverDisconnectedIcon;
    QIcon statusOkIcon, statusErrorIcon;
    bool hasError_;
    QTimer* serverReconnectionTimer_;
    QString lastUID = "0", lasrADDR = "0", SWVer = "0";
    QSharedPointer<FWLoader> fwLoader_;
    QMap<uint, DeviceItem*> devices_;
    const int k_column_n_ = 2;
    int column_ = 0;
    int row_ = 0;

    void LoadFromJson();
    void SignalConnections();
    void LogEvent(const QString &eventStr, bool isError = true);
    void SetViewIcons();
    void SaveToJson();
    void ServerBtnClicked();
    void StatusBtnClicked();
    void AddDevice();
    void FinishedOk(uint uid, qint64 msecs);
    void GetError(const QString &error, uint uid);
    void UpdateDeviceStatus(uint delta, uint uid, uint addr);
    void AddDeviceToGrid(DeviceItem *item);
    void AckInBootReceived(uint uid, uchar addr, uchar hw, uchar fw);
};
#endif // MAINWINDOW_H
