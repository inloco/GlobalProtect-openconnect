#ifndef GPCLIENT_H
#define GPCLIENT_H

#include "gpservice_interface.h"
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>

QT_BEGIN_NAMESPACE
namespace Ui { class GPClient; }
QT_END_NAMESPACE

class GPClient : public QMainWindow
{
    Q_OBJECT

public:
    GPClient(QWidget *parent = nullptr);
    ~GPClient();

signals:
    void connectFailed();

private slots:
    void on_connectButton_clicked();
    void preloginResultFinished();
    void getconfigResultFinished();

    void onLoginSuccess(QJsonObject loginResult);

    void onVPNConnected();
    void onVPNDisconnected();
    void onVPNLogAvailable(QString log);

    void on_actionLogout_triggered();

    void on_actionClear_data_triggered();

private:
    Ui::GPClient *ui;
    QNetworkAccessManager *networkManager;
    QNetworkReply *reply;
    com::yuezk::qt::GPService *vpn;
    QSettings *settings;
    QString user;
    QString phpsessid;
    QString preloginCookie;

    void initVpnStatus();
    void moveCenter();
    void updateConnectionStatus(QString status);
    void doAuth(const QString portal);
    void samlLogin(const QString loginUrl, const QString html = "");
    const QString getBestAvaialble(const QStringList gatewaysList);
};
#endif // GPCLIENT_H
