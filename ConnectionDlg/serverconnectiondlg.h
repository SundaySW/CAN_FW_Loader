#ifndef SERVERCONNECTIONDLG_H
#define SERVERCONNECTIONDLG_H

#include <QDialog>
#include "QIcon"
#include "QTimer"

#include "tcp_socket.hpp"

namespace Ui {
class ServerConnectionDlg;
}

class ServerConnectionDlg : public QDialog
{
    Q_OBJECT
public:
    explicit ServerConnectionDlg(const QSharedPointer<Tcp_socket>& socket, QWidget *parent = nullptr);
    ~ServerConnectionDlg();
    void loadDataFromJson(const QJsonObject&);
    void connectToServer();
    QJsonObject SaveDataToJson();
    bool reconnectIsOn() const;
signals:
    void autoConnectStateChanged(bool);
    void eventInServerConnection(const QString&, bool);
    void SaveMe();
private:
    Ui::ServerConnectionDlg *ui;
    QSharedPointer<Tcp_socket> socket_;
    QTimer* serverReconnectionTimer_;
    bool autoConnect_;
    void connectBtnClicked();
    void autoConnectBtnClicked();
    void ServerReconnectionTimerHandler();
};

#endif // SERVERCONNECTIONDLG_H
