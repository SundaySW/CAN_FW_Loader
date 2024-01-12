#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>
#include "QJsonDocument"

#include "serverconnectiondlg.h"
#include "tcp_socket.hpp"

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
    void LoadFromJson();
    void SignalConnections();
    void EventServerConnectionHandler(const QString &eventStr, bool isError = true);
    void SetViewIcons();
    void SaveToJson();
    void ServerBtnClicked();
    void StatusBtnClicked();
};
#endif // MAINWINDOW_H
