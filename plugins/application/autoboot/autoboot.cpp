/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2019 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "autoboot.h"
#include "SwitchButton/switchbutton.h"
#include "HoverWidget/hoverwidget.h"
#include "ImageUtil/imageutil.h"
#include "autobootworker.h"

#include <QThread>
#include <QSignalMapper>
#include <QDebug>
#include <QFont>
#include <QMouseEvent>
#include <QPushButton>
#include <QGSettings>
#include <QToolButton>
#include <QMenu>

/* qt会将glib里的signals成员识别为宏，所以取消该宏
 * 后面如果用到signals时，使用Q_SIGNALS代替即可
 **/
#ifdef signals
#undef signals
#endif

extern "C" {
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
}

#define ITEMWIDTH 522
#define ITEMHEIGHT 62
#define HEADHEIGHT 38

#define THEME_QT_SCHEMA  "org.ukui.style"
#define THEME_GTK_SCHEMA "org.mate.interface"

#define ICON_QT_KEY      "icon-theme-name"
#define ICON_GTK_KEY     "icon-theme"

#define LOCAL_CONFIG_DIR           "/.config/autostart/"
#define SYSTEM_CONFIG_DIR          "/etc/xdg/autostart/"

AutoBoot::AutoBoot() : mFirstLoad(true)
{
    pluginName = tr("Auto Boot");
    pluginType = APPLICATION;
}

AutoBoot::~AutoBoot()
{
    if (!mFirstLoad) {
    }
}

QString AutoBoot::get_plugin_name()
{
    return pluginName;
}

int AutoBoot::get_plugin_type()
{
    return pluginType;
}

QWidget *AutoBoot::get_plugin_ui()
{
    if (mFirstLoad) {
        mFirstLoad = false;

        pluginWidget = new QWidget;
        pluginWidget->setAttribute(Qt::WA_DeleteOnClose);

        initConfig();
        connectToServer();
        initUI(pluginWidget);
        initStyle();
        setupGSettings();
        initConnection();
    }
    return pluginWidget;
}

void AutoBoot::plugin_delay_control()
{
}

const QString AutoBoot::name() const
{
    return QStringLiteral("autoboot");
}

void AutoBoot::initUI(QWidget *widget)
{
    QVBoxLayout *mverticalLayout = new QVBoxLayout(widget);
    mverticalLayout->setSpacing(0);
    mverticalLayout->setContentsMargins(0, 0, 32, 100);

    QWidget *AutobootWidget = new QWidget(widget);
    AutobootWidget->setMinimumSize(QSize(550, 0));
    AutobootWidget->setMaximumSize(QSize(960, 16777215));

    QVBoxLayout *AutobootLayout = new QVBoxLayout(AutobootWidget);
    AutobootLayout->setContentsMargins(0, 0, 0, 0);
    AutobootLayout->setSpacing(8);

    mTitleLabel = new TitleLabel(AutobootWidget);

    mAutoBootFrame = new QFrame(AutobootWidget);
    mAutoBootFrame->setMinimumSize(QSize(550, 0));
    mAutoBootFrame->setMaximumSize(QSize(960, 16777215));
    mAutoBootFrame->setFrameShape(QFrame::NoFrame);

    mAutoBootLayout = new QVBoxLayout(mAutoBootFrame);
    mAutoBootLayout->setContentsMargins(0, 0, 0, 0);
    mAutoBootLayout->setSpacing(2);

    initAddBtn();

    AutobootLayout->addWidget(mTitleLabel);
    AutobootLayout->addWidget(mAutoBootFrame);
    AutobootLayout->addWidget(addWgt);

    mverticalLayout->addWidget(AutobootWidget);
    mverticalLayout->addStretch();

    initAutoUI();

}

void AutoBoot::initAutoUI()
{
    initStatus();
    appgroupMultiMaps.clear();
    QSignalMapper *checkSignalMapper = new QSignalMapper(this);
    // 构建每个启动项
    QMap<QString, AutoApp>::iterator it = statusMaps.begin();
    for (int index = 0; it != statusMaps.end(); it++, index++) {
        QString bname = it.value().bname;
        QString appName = it.value().name;

        QFrame *baseWidget = new QFrame(pluginWidget);
        baseWidget->setMinimumWidth(550);
        baseWidget->setMaximumWidth(960);
        baseWidget->setFrameShape(QFrame::Box);
        baseWidget->setAttribute(Qt::WA_DeleteOnClose);

        QVBoxLayout *baseVerLayout = new QVBoxLayout(baseWidget);
        baseVerLayout->setSpacing(0);
        baseVerLayout->setContentsMargins(0, 0, 0, 2);

        HoverWidget *widget = new HoverWidget(bname);
        widget->setMinimumWidth(550);
        widget->setMaximumWidth(960);

        widget->setMinimumHeight(60);
        widget->setMaximumHeight(60);

        widget->setAttribute(Qt::WA_DeleteOnClose);

        QHBoxLayout *mainHLayout = new QHBoxLayout(widget);
        mainHLayout->setContentsMargins(16, 0, 16, 0);
        mainHLayout->setSpacing(16);

        QLabel *iconLabel = new QLabel(widget);
        iconLabel->setFixedSize(32, 32);
        iconLabel->setPixmap(it.value().pixmap);

        QLabel *textLabel = new QLabel(widget);
        textLabel->setFixedWidth(250);
        textLabel->setText(appName);

        SwitchButton *button = new SwitchButton(widget);
        button->setAttribute(Qt::WA_DeleteOnClose);
        button->setChecked(!it.value().hidden);
        checkSignalMapper->setMapping(button, it.key());
        connect(button, SIGNAL(checkedChanged(bool)), checkSignalMapper, SLOT(map()));
        appgroupMultiMaps.insert(it.key(), button);

        QToolButton *deBtn = new QToolButton(widget);
        deBtn->setPopupMode(QToolButton::InstantPopup);
        deBtn->setFixedSize(QSize(36, 36));
        deBtn->setIcon(QIcon(":/more.svg"));
        deBtn->setProperty("isWindowButton", 0x01);

        QMenu *pMenu = new QMenu(deBtn);
        pMenu->installEventFilter(this);

        deBtn->setMenu(pMenu);
        QAction* mDel = new QAction(tr("Delete"),this);
        pMenu->addAction(mDel);
        connect(mDel, &QAction::triggered, this, [=](){
               delAutoApp(bname);
        });

        mainHLayout->addWidget(iconLabel);
        mainHLayout->addWidget(textLabel);
        mainHLayout->addStretch();
        if (it.value().xdg_position == LOCALPOS) {
            mainHLayout->addWidget(deBtn);
        } else {
            deBtn->hide();
        }

        mainHLayout->addWidget(button);
        widget->setLayout(mainHLayout);

        baseVerLayout->addWidget(widget);
        baseVerLayout->addStretch();

        baseWidget->setLayout(baseVerLayout);

        mAutoBootLayout->addWidget(baseWidget);
    }
    connect(checkSignalMapper, SIGNAL(mapped(QString)), this, SLOT(checkbox_changed_cb(QString)));
}

void AutoBoot::setupGSettings()
{
    const QByteArray id(THEME_QT_SCHEMA);
    mQtSettings = new QGSettings(id, QByteArray(), this);
}

AutoApp AutoBoot::setInformation(QString filepath)
{
    AutoApp app;
    QSettings* desktopFile = new QSettings(filepath, QSettings::IniFormat);
    QString icon, only_showin, not_show_in;
    if (desktopFile) {
       desktopFile->setIniCodec("utf-8");

       QFileInfo file = QFileInfo(filepath);
       app.bname = file.fileName();
       app.path = filepath;
       app.exec = desktopFile->value(QString("Desktop Entry/Exec")).toString();
       icon = desktopFile->value(QString("Desktop Entry/Icon")).toString();
       app.hidden = desktopFile->value(QString("Desktop Entry/Hidden")).toBool();
       app.no_display = desktopFile->value(QString("Desktop Entry/NoDisplay")).toBool();
       only_showin = desktopFile->value(QString("Desktop Entry/OnlyShowIn")).toString();
       not_show_in = desktopFile->value(QString("Desktop Entry/NotShowIn")).toString();
       bool mshow = true;
       if (only_showin != nullptr) {
           if (!only_showin.contains("UKUI")) {
               mshow = false;
           }
       }
       if (not_show_in != nullptr) {
           if (not_show_in.contains("UKUI")) {
               mshow = false;
           }
       }
       app.shown = mshow;

       QFileInfo iconfile(icon);

       if (!QString(icon).isEmpty()) {
           QIcon currenticon
               = QIcon::fromTheme(icon,
                                  QIcon(QString("/usr/share/pixmaps/"+icon
                                                +".png")));
           app.pixmap = currenticon.pixmap(QSize(32, 32));
       } else if (iconfile.exists()) {
           app.pixmap = QPixmap(iconfile.filePath()).scaled(32, 32);
       } else {
           app.pixmap = QPixmap(QString(":/img/plugins/autoboot/desktop.png"));
       }

       delete desktopFile;
       desktopFile = nullptr;
    }
    //通过glib库函数获取Name字段，防止特殊情况（含有字段X-Ubuntu-Gettext-Domain）
    GKeyFile *keyfile = g_key_file_new();
    if (!g_key_file_load_from_file(keyfile, filepath.toLatin1().data(), G_KEY_FILE_NONE, NULL)) {
        g_key_file_free(keyfile);
        return app;
    }
    app.name = g_key_file_get_locale_string(keyfile, G_KEY_FILE_DESKTOP_GROUP,
                                        G_KEY_FILE_DESKTOP_KEY_NAME, NULL, NULL);
    g_key_file_free(keyfile);

    return app;

}

bool AutoBoot::copyFileToLocal(QString bname)
{
    QString srcPath;
    QString dstPath;

    QMap<QString, AutoApp>::iterator it = appMaps.find(bname);

    srcPath = it.value().path;
    dstPath = QDir::homePath()+LOCAL_CONFIG_DIR+bname;

    if (!QFile::copy(srcPath, dstPath))
        return false;

    //将复制的文件权限改为可读写
    QFile(dstPath).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);

    // 更新数据，将新加入该目录下的应用信息读取，加入到localappMaps，更新statusMaps对应应用的信息
    AutoApp addapp;
    addapp = setInformation(dstPath);
    addapp.xdg_position = ALLPOS;

    localappMaps.insert(addapp.bname, addapp);

    QMap<QString, AutoApp>::iterator updateit = statusMaps.find(bname);
    updateit.value().xdg_position = ALLPOS;
    updateit.value().path = dstPath;

    return true;
}

bool AutoBoot::deleteLocalAutoapp(QString bname)
{
    QString dstpath = QDir::homePath()+LOCAL_CONFIG_DIR+bname;

    if (dstpath.isEmpty() || !QDir().exists(dstpath))//是否传入了空的路径||路径是否存在
            return false;

    // 更新数据
    localappMaps.remove(bname);
    QMap<QString, AutoApp>::iterator updateit = statusMaps.find(bname);
    if (updateit == statusMaps.end())
        qDebug() << "statusMaps Data Error when delete local file";
    else {
        if (updateit.value().xdg_position == LOCALPOS) {
            statusMaps.remove(bname);
        } else if (updateit.value().xdg_position == ALLPOS) {
            //当系统应用的自启动开关打开时，将其属性变为SYSTEMPOS，将其从~/.config/autostart目录下删除
            QMap<QString, AutoApp>::iterator appit = appMaps.find(bname);
            if (appit == appMaps.end())
                qDebug() << "appMaps Data Error when delete local file";
            else {
                updateit.value().hidden = appit.value().hidden;
                updateit.value().path = appit.value().path;
            }
            updateit.value().xdg_position = SYSTEMPOS;
        }
    }

    QFile::remove(dstpath);
    return true;
}

bool AutoBoot::setAutoAppStatus(QString bname, bool status)
{
    QString dstpath = QDir::homePath()+LOCAL_CONFIG_DIR+bname;

    //修改hidden字段
    QSettings* desktopFile = new QSettings(dstpath, QSettings::IniFormat);
    if (desktopFile) {
       desktopFile->setIniCodec("utf-8");
       desktopFile->beginGroup("Desktop Entry");
       desktopFile->setValue("Hidden", !status);
       desktopFile->endGroup();
       delete desktopFile;
       desktopFile = nullptr;
       // 更新数据
       QMap<QString, AutoApp>::iterator updateit = statusMaps.find(bname);
       if (updateit == statusMaps.end())
           qDebug() << "Start autoboot failed because autoBoot Data Error";
       else {
           updateit.value().hidden = !status;
       }
       return true;
    }
    return false;
}

void AutoBoot::clearAutoItem()
{
    if (mAutoBootLayout->layout() != NULL) {
        QLayoutItem *item;
        while ((item = mAutoBootLayout->layout()->takeAt(0)) != NULL)
        {
            delete item->widget();
            delete item;
            item = nullptr;
        }
    }
}

bool AutoBoot::eventFilter(QObject *obj, QEvent *event)
{
    QString strObjName(obj->metaObject()->className());
    if (strObjName == "QMenu") {
        if (event->type() == QEvent::Show) {
            QMenu *mMenu = static_cast<QMenu *>(obj);
            if (mMenu) {
                int menuXPos = mMenu->pos().x();
                int menuWidth = mMenu->size().width()-4;
                int buttonWidth = 36;
                QPoint pos = QPoint(menuXPos - menuWidth + buttonWidth,
                                    mMenu->pos().y());
                mMenu->move(pos);
                return true;
            }
        }
        return false;
    }
}

void AutoBoot::checkbox_changed_cb(QString bname)
{
    foreach (QString key, appgroupMultiMaps.keys()) {
        if (key == bname) {
            QMap<QString, AutoApp>::iterator it = statusMaps.find(bname);
            if (it == statusMaps.end()) {
                qDebug() << "AutoBoot Data Error";
                return;
            }

            if (((SwitchButton *)appgroupMultiMaps.value(key))->isChecked()) { // 开启开机启动
                if (it.value().xdg_position == SYSTEMPOS) { //
                } else if (it.value().xdg_position == ALLPOS) { // 从~/.config/autostart目录下删除
                    QMap<QString, AutoApp>::iterator appit = appMaps.find(bname);
                    if (!appit.value().hidden) { // 直接删除
                        deleteLocalAutoapp(bname);
                        // 更新状态
                        QMap<QString, AutoApp>::iterator updateit = statusMaps.find(bname);
                        if (updateit != statusMaps.end()) {
                            updateit.value().hidden = false;
                            updateit.value().xdg_position = SYSTEMPOS;
                            //路径改为/etc/xdg/autostart
                            updateit.value().path = appit.value().path;
                        } else
                            qDebug() << "Update status failed when start autoboot";
                    }
                } else if (it.value().xdg_position == LOCALPOS) {// 改值("hidden"字段->false)
                    setAutoAppStatus(bname, true);
                }
            } else { // 关闭
                if (it.value().xdg_position == SYSTEMPOS) { // 复制后改值，将对应应用的desktop文件从/etc/xdg/autostart目录下复制到~/.config/autostart目录下
                    if (copyFileToLocal(bname)) {
                        //更新数据（"hidden"字段->true）
                         setAutoAppStatus(bname, false);
                    }
                } else if (it.value().xdg_position == ALLPOS) {// 正常逻辑不应存在该情况,预防处理
                    QMap<QString, AutoApp>::iterator appit = appMaps.find(bname);
                    if (appit.value().hidden)
                        deleteLocalAutoapp(bname);
                } else if (it.value().xdg_position == LOCALPOS) {// 改值
                    //更新数据（"hidden"字段->true）
                   setAutoAppStatus(bname, false);
                }
            }
        }
    }
}

void AutoBoot::keyChangedSlot(const QString &key)
{
    if (key == "boot") {
        clearAutoItem();
        initAutoUI();
    }
}

void AutoBoot::add_autoboot_realize_slot(QString path, QString name, QString exec, QString comment, QString icon)
{
    if (exec.contains("kylin-screenshot")) {
        QStringList screenshotExec = exec.split(" ");
        exec = screenshotExec.at(0);
    }
    if (path.isEmpty())
        return;

    QFileInfo file = QFileInfo(path);
    QString mFileName = file.fileName();
    QString filepath = QDir::homePath()+LOCAL_CONFIG_DIR+mFileName;
    if(!QFile::copy(path,filepath))
        return;
    clearAutoItem();
    initAutoUI();
}

void AutoBoot::initAddBtn()
{
    addWgt = new HoverWidget("", pluginWidget);
    addWgt->setObjectName("addwgt");
    addWgt->setMinimumSize(QSize(580, 60));
    addWgt->setMaximumSize(QSize(960, 60));
    QPalette pal;
    QBrush brush = pal.highlight();  //获取window的色值
    QColor highLightColor = brush.color();
    QString stringColor = QString("rgba(%1,%2,%3)") //叠加20%白色
           .arg(highLightColor.red()*0.8 + 255*0.2)
           .arg(highLightColor.green()*0.8 + 255*0.2)
           .arg(highLightColor.blue()*0.8 + 255*0.2);

    addWgt->setStyleSheet(QString("HoverWidget#addwgt{background: palette(button);\
                                   border-radius: 4px;}\
                                   HoverWidget:hover:!pressed#addwgt{background: %1;  \
                                   border-radius: 4px;}").arg(stringColor));
    QHBoxLayout *addLyt = new QHBoxLayout(addWgt);

    QLabel *iconLabel = new QLabel(pluginWidget);
    QLabel *textLabel = new QLabel(tr("Add autoboot app "), pluginWidget);
    QPixmap pixgray = ImageUtil::loadSvg(":/img/titlebar/add.svg", "black", 12);
    iconLabel->setPixmap(pixgray);
    iconLabel->setProperty("useIconHighlightEffect", true);
    iconLabel->setProperty("iconHighlightEffectMode", 1);
    addLyt->addStretch();
    addLyt->addWidget(iconLabel);
    addLyt->addWidget(textLabel);
    addLyt->addStretch();
    addWgt->setLayout(addLyt);

    // 悬浮改变Widget状态
    connect(addWgt, &HoverWidget::enterWidget, this, [=](){

        iconLabel->setProperty("useIconHighlightEffect", false);
        iconLabel->setProperty("iconHighlightEffectMode", 0);
        QPixmap pixgray = ImageUtil::loadSvg(":/img/titlebar/add.svg", "white", 12);
        iconLabel->setPixmap(pixgray);
        textLabel->setStyleSheet("color: white;");
    });

    // 还原状态
    connect(addWgt, &HoverWidget::leaveWidget, this, [=](){

        iconLabel->setProperty("useIconHighlightEffect", true);
        iconLabel->setProperty("iconHighlightEffectMode", 1);
        QPixmap pixgray = ImageUtil::loadSvg(":/img/titlebar/add.svg", "black", 12);
        iconLabel->setPixmap(pixgray);
        textLabel->setStyleSheet("color: palette(windowText);");
    });
    dialog = new AddAutoBoot(pluginWidget);
    connect(addWgt, &HoverWidget::widgetClicked, this, [=](QString mname){
        Q_UNUSED(mname);
        dialog->exec();
    });
}

void AutoBoot::initStyle()
{
    //~ contents_path /autoboot/Autoboot Settings
    mTitleLabel->setText(tr("Autoboot Settings"));
}



void AutoBoot::initStatus()
{
    QDir localdir(QString(QDir::homePath()+LOCAL_CONFIG_DIR).toUtf8());
    QDir systemdir(QString(SYSTEM_CONFIG_DIR).toUtf8());

    QStringList filters;
    filters<<QString("*.desktop");
    localdir.setFilter(QDir::Files | QDir::NoSymLinks); // 设置类型过滤器，只为文件格式
    systemdir.setFilter(QDir::Files | QDir::NoSymLinks);

    localdir.setNameFilters(filters);  // 设置文件名称过滤器，只为filters格式
    systemdir.setNameFilters(filters);

    //将系统目录下的应用加入appMaps
    appMaps.clear();
    for (int i = 0; i < systemdir.count(); i++) {
        QString file_name = systemdir[i];  // 文件名称
        AutoApp app;
        app = setInformation(SYSTEM_CONFIG_DIR+file_name);
        app.xdg_position = SYSTEMPOS;
        appMaps.insert(app.bname, app);
    }

    //将本地配置目录下的应用加入localappMaps
    localappMaps.clear();
    for (int i = 0; i < localdir.count(); i++) {
        QString file_name = localdir[i];  // 文件名称
        AutoApp app;
        app = setInformation(QDir::homePath()+LOCAL_CONFIG_DIR+file_name);
        app.xdg_position = LOCALPOS;
        localappMaps.insert(app.bname, app);
    }

    //将localappMaps和appMaps中的应用加入statusMaps
    statusMaps.clear();

    QMap<QString, AutoApp>::iterator it = appMaps.begin();
    for (; it != appMaps.end(); it++) {
        if (/*it.value().hidden || */ it.value().no_display || !it.value().shown
            || it.value().bname == "browser360-cn_preheat.desktop"
            || it.value().bname == "vmware-user.desktop"

            || it.value().exec == "/usr/bin/ukui-settings-daemon")  // gtk控制面板屏蔽ukui-settings-daemon,猜测禁止用户关闭
            continue;
        statusMaps.insert(it.key(), it.value());
    }

    QMap<QString, AutoApp>::iterator localit = localappMaps.begin();
    for (; localit != localappMaps.end(); localit++) {
        if (/*localit.value().hidden || */ localit.value().no_display || !localit.value().shown) {
            statusMaps.remove(localit.key());
            continue;
        }

        if (statusMaps.contains(localit.key())) {
            // 整合状态
            QMap<QString, AutoApp>::iterator updateit = statusMaps.find(localit.key());

            if (localit.value().hidden != updateit.value().hidden) {
                //将statusMaps中的应用状态与localappMaps中对应的应用状态同步
                updateit.value().hidden = localit.value().hidden;
                updateit.value().path = localit.value().path;
                //appMaps中hidden属性为false的应用进入下面这个if语句
                if (appMaps.contains(localit.key())) {
                    //ALLPOS代表系统目录下没被过滤掉的应用，自启动按钮关闭（hidden = false）
                    updateit.value().xdg_position = ALLPOS;
                }
            }
        } else {
            statusMaps.insert(localit.key(), localit.value());
        }
    }
}

void AutoBoot::initConnection()
{
    connect(mQtSettings, &QGSettings::changed, this, [=] {
        clearAutoItem();
        initAutoUI();
    });

    connect(dialog, SIGNAL(autoboot_adding_signals(QString,QString,QString,QString,QString)),
            this, SLOT(add_autoboot_realize_slot(QString,QString,QString,QString,QString)));
}

void AutoBoot::delAutoApp(QString bname)
{
    QMap<QString, AutoApp>::iterator it = statusMaps.find(bname);
    if (it == statusMaps.end()) {
        qDebug() << "AutoBoot Data Error";
        return;
    }

    deleteLocalAutoapp(bname);
    QTimer::singleShot(100, this, [&,this](){
        clearAutoItem();
        initAutoUI();
    });
}

void AutoBoot::connectToServer()
{
    m_cloudInterface = new QDBusInterface("org.kylinssoclient.dbus",
                                          "/org/kylinssoclient/path",
                                          "org.freedesktop.kylinssoclient.interface",
                                          QDBusConnection::sessionBus());
    if (!m_cloudInterface->isValid()) {
        qDebug() << "fail to connect to service";
        qDebug() << qPrintable(QDBusConnection::systemBus().lastError().message());
        return;
    }
    QDBusConnection::sessionBus().connect(QString(), QString("/org/kylinssoclient/path"),
                                          QString(
                                              "org.freedesktop.kylinssoclient.interface"), "keyChanged", this,
                                          SLOT(keyChangedSlot(QString)));
    // 将以后所有DBus调用的超时设置为 milliseconds
    m_cloudInterface->setTimeout(2147483647); // -1 为默认的25s超时
}

bool AutoBoot::initConfig()
{
    QDir localdir(QString(QDir::homePath()+LOCAL_CONFIG_DIR).toUtf8());
    if(localdir.exists()) {
      return true;
    } else {
       return localdir.mkdir(QDir::homePath()+LOCAL_CONFIG_DIR);
    }
}

