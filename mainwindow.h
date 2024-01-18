#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>
#include "QJsonDocument"

#include "serverconnectiondlg.h"
#include "tcp_socket.hpp"
#include "fw_loader.hpp"

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

    void LoadFromJson();
    void SignalConnections();
    void EventServerConnectionHandler(const QString &eventStr, bool isError = true);
    void SetViewIcons();
    void SaveToJson();
    void ServerBtnClicked();
    void StatusBtnClicked();

    void AddDevice();

    void ackInBootReceived();

    void finishedOk(uint uid, int msecs);

    void getError(const QString &error, uint uid);

    void updateStatus(uint delta, uint uid, uint addr);
};
#endif // MAINWINDOW_H
