#include "gpclient.h"
#include "ui_gpclient.h"
#include "samlloginwindow.h"

#include <QDesktopWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QStyle>
#include <QMessageBox>
#include <QHostInfo>
#include <QtXml/QDomDocument>


GPClient::GPClient(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GPClient)
{
    ui->setupUi(this);
    setFixedSize(width(), height());
    moveCenter();

    // Restore portal from the previous settings
    settings = new QSettings("com.yuezk.qt", "GPClient");
    ui->portalInput->setText(settings->value("portal", "").toString());
    ui->gatewaysComboBox->clear();
    ui->gatewaysComboBox->addItems(settings->value("gatewaynames", QStringList()).toStringList());
    QObject::connect(this, &GPClient::connectFailed, [this]() {
        updateConnectionStatus("not_connected");
    });

    // QNetworkAccessManager setup
    networkManager = new QNetworkAccessManager(this);

    // DBus service setup
    vpn = new com::yuezk::qt::GPService("com.yuezk.qt.GPService", "/", QDBusConnection::systemBus(), this);
    QObject::connect(vpn, &com::yuezk::qt::GPService::connected, this, &GPClient::onVPNConnected);
    QObject::connect(vpn, &com::yuezk::qt::GPService::disconnected, this, &GPClient::onVPNDisconnected);
    QObject::connect(vpn, &com::yuezk::qt::GPService::logAvailable, this, &GPClient::onVPNLogAvailable);

    initVpnStatus();
}

GPClient::~GPClient()
{
    delete ui;
    delete networkManager;
    delete reply;
    delete vpn;
    delete settings;
}

void GPClient::on_connectButton_clicked()
{
    QString btnText = ui->connectButton->text();
    QString usertoken = settings->value("userauthcookie", "").toString();
    QString user = settings->value("user", "").toString();

    if (btnText.endsWith("Login")) {
        QString portal = ui->portalInput->text();
        settings->setValue("portal", portal);
        ui->statusLabel->setText("Authenticating...");
        updateConnectionStatus("pending");
        doAuth(portal);
    } else if (btnText.endsWith("Cancel")) {
        ui->statusLabel->setText("Canceling...");
        updateConnectionStatus("pending");

        if (reply->isRunning()) {
            reply->abort();
        }
        vpn->disconnect();
    } else if (btnText.endsWith("Connect")) {
        QString portal = ui->portalInput->text();
        settings->setValue("portal", portal);
        ui->statusLabel->setText("Connecting...");
        updateConnectionStatus("pending");

        QString host = QString("https://%1/%2:%3").arg(portal, "portal", "portal-userauthcookie");
        qInfo("Host: %s User: %s Token: %s", host.toStdString().c_str(), user.toStdString().c_str(), usertoken.toStdString().c_str());
        vpn->connect_gw(host, user, usertoken, ui->gatewaysComboBox->currentText());

    } else {
        ui->statusLabel->setText("Disconnecting...");
        updateConnectionStatus("pending");
        vpn->disconnect();
    }
}

void GPClient::preloginResultFinished()
{
    QNetworkReply::NetworkError err = reply->error();
    if (err) {
        qWarning() << "Prelogin request error: " << err;
        emit connectFailed();
        return;
    }

    QByteArray xmlBytes = reply->readAll();
    const QString tagMethod = "saml-auth-method";
    const QString tagRequest = "saml-request";
    QString samlMethod;
    QString samlRequest;

    // Save phpsessid
    if(reply->hasRawHeader("Set-Cookie")){
        phpsessid = reply->rawHeader("Set-Cookie");
        phpsessid.truncate(phpsessid.indexOf(';'));
    }

    QXmlStreamReader xml(xmlBytes);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.tokenType() == xml.StartElement) {
            if (xml.name() == tagMethod) {
                samlMethod = xml.readElementText();
            } else if (xml.name() == tagRequest) {
                samlRequest = QByteArray::fromBase64(QByteArray::fromStdString(xml.readElementText().toStdString()));
            }
        }
    }

    if (samlMethod == nullptr || samlRequest == nullptr) {
        qWarning("This does not appear to be a SAML prelogin response (<saml-auth-method> or <saml-request> tags missing)");
        emit connectFailed();
        return;
    }

    if (samlMethod == "POST") {
        samlLogin(reply->url().toString(), samlRequest);
    } else if (samlMethod == "REDIRECT") {
        samlLogin(samlRequest);
    }
}

void GPClient::onLoginSuccess(QJsonObject loginResult)
{
    QString portal = loginResult.value("server").toString();
    QString preloginUrl = "https://" + portal + "/global-protect/getconfig.esp";
    QUrl url(preloginUrl);
    QUrlQuery query;

    user = loginResult.value("saml-username").toString();
    settings->setValue("user", user);
    preloginCookie = loginResult.value("prelogin-cookie").toString();

    query.addQueryItem("jnlpReady", "jnlpReady");
    query.addQueryItem("ok", "Login");
    query.addQueryItem("direct", "yes");
    query.addQueryItem("clientVer", "4100");
    query.addQueryItem("prot", "https:");
    query.addQueryItem("ipv6-support", "no");
    query.addQueryItem("clientos", "Linux");
    query.addQueryItem("os-version", "linux-64");
    query.addQueryItem("server", portal);
    query.addQueryItem("computer", QHostInfo::localHostName());
    query.addQueryItem("user", user);
    query.addQueryItem("prelogin-cookie", preloginCookie);


    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "PAN GlobalProtect");
    request.setRawHeader("Cookie", phpsessid.toUtf8());
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setRawHeader("Connection", "close");
    request.setRawHeader("Host", portal.toStdString().c_str());

    reply = networkManager->post(request, query.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, &GPClient::getconfigResultFinished);
}

void GPClient::getconfigResultFinished()
{
    QNetworkReply::NetworkError err = reply->error();
    if (err) {
        qWarning() << "Prelogin request error: " << err;
        emit connectFailed();
        return;
    }

    QByteArray xmlBytes = reply->readAll();
    //qInfo(xmlBytes);
    QDomDocument doc;
    doc.setContent(xmlBytes);

    QDomElement docElem = doc.documentElement();
    QDomNodeList usercookie = docElem.elementsByTagName("portal-userauthcookie");
    settings->setValue("userauthcookie", usercookie.item(0).toElement().text());

    QDomNodeList gateways = docElem.elementsByTagName("gateways")
            .item(0).toElement().elementsByTagName("list")
            .item(0).toElement().childNodes();
    QStringList gatewaynames;
    for(int i = 0; i < gateways.length(); i++){
        gatewaynames.append(gateways.item(i).toElement().attribute("name"));
    }

    settings->setValue("gatewaynames", gatewaynames);
    ui->gatewaysComboBox->clear();
    ui->gatewaysComboBox->addItems(gatewaynames);
    updateConnectionStatus("not_connected");
}

void GPClient::updateConnectionStatus(QString status)
{
    if (status == "not_connected") {
        if(settings->value("userauthcookie").toString().length() == 0
                || settings->value("gatewaynames", QStringList()).toStringList().length() ==0){
            ui->statusLabel->setText("Not logged in");
            ui->statusImage->setStyleSheet("image: url(:/images/not_connected.png); padding: 15;");
            ui->connectButton->setText("Login");
            ui->connectButton->setDisabled(false);
        } else {
            ui->statusLabel->setText("Not Connected");
            ui->statusImage->setStyleSheet("image: url(:/images/not_connected.png); padding: 15;");
            ui->connectButton->setText("Connect");
            ui->connectButton->setDisabled(false);
        }
    } else if (status == "pending") {
        ui->statusImage->setStyleSheet("image: url(:/images/pending.png); padding: 15;");
        ui->connectButton->setText("Cancel");
        ui->connectButton->setDisabled(false);
    } else if (status == "connected") {
        ui->statusLabel->setText("Connected");
        ui->statusImage->setStyleSheet("image: url(:/images/connected.png); padding: 15;");
        ui->connectButton->setText("Disconnect");
        ui->connectButton->setDisabled(false);
    }
}

void GPClient::onVPNConnected()
{
    updateConnectionStatus("connected");
}

void GPClient::onVPNDisconnected()
{
    updateConnectionStatus("not_connected");
}

void GPClient::onVPNLogAvailable(QString log)
{
    qInfo() << log;
}

void GPClient::initVpnStatus() {
    int status = vpn->status();
    if (status == 0) {
        updateConnectionStatus("not_connected");
    } else if (status == 1) {
        ui->statusLabel->setText("Connecting...");
        updateConnectionStatus("pending");
    } else if (status == 2) {
        updateConnectionStatus("connected");
    } else if (status == 3) {
        ui->statusLabel->setText("Disconnecting...");
        updateConnectionStatus("pending");
    }
}

void GPClient::moveCenter()
{
    QDesktopWidget *desktop = QApplication::desktop();

    int screenWidth, width;
    int screenHeight, height;
    int x, y;
    QSize windowSize;

    screenWidth = desktop->width();
    screenHeight = desktop->height();

    windowSize = size();
    width = windowSize.width();
    height = windowSize.height();

    x = (screenWidth - width) / 2;
    y = (screenHeight - height) / 2;
    y -= 50;
    move(x, y);
}

void GPClient::doAuth(const QString portal)
{
    const QString preloginUrl = "https://" + portal + "/global-protect/prelogin.esp";
    QUrl url(preloginUrl);
    QUrlQuery query;

    query.addQueryItem("tmp", "tmp");
    query.addQueryItem("clientVer", "4100");
    query.addQueryItem("clientos", "Linux");

    url.setQuery(query.query());

    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "PAN GlobalProtect");
    request.setRawHeader("Connection", "close");
    request.setRawHeader("Content-Length", "0");
    request.setRawHeader("Host", portal.toStdString().c_str());

    reply = networkManager->post(request, (QByteArray) nullptr);
    connect(reply, &QNetworkReply::finished, this, &GPClient::preloginResultFinished);
}

void GPClient::samlLogin(const QString loginUrl, const QString html)
{
    SAMLLoginWindow *loginWindow = new SAMLLoginWindow(this);

    QObject::connect(loginWindow, &SAMLLoginWindow::success, this, &GPClient::onLoginSuccess);
    QObject::connect(loginWindow, &SAMLLoginWindow::rejected, this, &GPClient::connectFailed);

    loginWindow->login(loginUrl, html);
    loginWindow->exec();
    delete loginWindow;
}

void GPClient::on_actionLogout_triggered()
{
    settings->setValue("userauthcookie", "");
    vpn->disconnect();
    updateConnectionStatus("not_connected");
}

void GPClient::on_actionClear_data_triggered()
{
    settings->setValue("portal", "");
    settings->setValue("gatewaynames", QStringList());
    ui->portalInput->setText("");
    ui->gatewaysComboBox->clear();
    vpn->disconnect();
    updateConnectionStatus("not_connected");
}
