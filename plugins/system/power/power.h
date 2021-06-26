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
#ifndef POWER_H
#define POWER_H

#include <QObject>
#include <QtPlugin>
#include <QStyledItemDelegate>
#include <QGSettings>
#include <QDBusInterface>
#include <QDBusReply>

#include "shell/interface.h"
#include "Label/titlelabel.h"

#include "commonComponent/ComboxFrame/comboxframe.h"
#include "SwitchButton/switchbutton.h"

namespace Ui {
class Power;
}

class Power : public QObject, CommonInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kycc.CommonInterface")
    Q_INTERFACES(CommonInterface)

public:
    explicit Power();
    ~Power();

public:
    QString get_plugin_name()   Q_DECL_OVERRIDE;
    int get_plugin_type()       Q_DECL_OVERRIDE;
    QWidget *get_plugin_ui()   Q_DECL_OVERRIDE;
    void plugin_delay_control() Q_DECL_OVERRIDE;
    const QString name() const Q_DECL_OVERRIDE;

public:
    void InitUI(QWidget *widget);
    void retranslateUi();

    void initSearText();
    void setupComponent();
    void setupConnect();
    void initModeStatus();
    void initCustomPlanStatus();
    void resetCustomPlanStatus();
    void initPowerOtherStatus();
    void isLidPresent();
    void isHibernateSupply();
    void refreshUI();
    int  getIdleTime();
    bool QLabelSetText(QLabel *label, QString string);

private:

    QWidget *pluginWidget;

    QGSettings *settings;
    QGSettings *sessionSetting;
    QGSettings *stylesettings;
    QGSettings *sessionsettings;
    QGSettings *mUkccpersonpersonalize;

    QString pluginName;
    int pluginType;

    TitleLabel *CustomTitleLabel;
    TitleLabel *PowerPlanTitleLabel;
    TitleLabel *BatteryPlanTitleLabel;
    QLabel *mSleepPwdLabel;
    QLabel *mWakenPwdLabel;
    QLabel *mPowerKeyLabel;
    QLabel *mCloseLabel;
    QLabel *mSleepLabel;
    QLabel *mCloseLidLabel;
    QLabel *mPowerLabel;
    QLabel *mBatteryLabel;
    QLabel *mLowpowerLabel1;
    QLabel *mLowpowerLabel2;
    QLabel *mNoticeLabel;
    QLabel *mLowSaveLabel;
    QLabel *mBatterySaveLabel;
    QLabel *mDisplayTimeLabel;

    QFrame *mSleepPwdFrame;
    QFrame *mWakenPwdFrame;
    QFrame *mPowerKeyFrame;
    QFrame *mCloseFrame;
    QFrame *mSleepFrame;
    QFrame *mCloseLidFrame;
    QFrame *mPowerFrame;
    QFrame *mBatteryFrame;
    QFrame *mLowpowerFrame;
    QFrame *mNoticeLFrame;
    QFrame *mLowSaveFrame;
    QFrame *mBatterySaveFrame;
    QFrame *mDisplayTimeFrame;

    QComboBox *mPowerKeyComboBox;
    QComboBox *mCloseComboBox;
    QComboBox *mSleepComboBox;
    QComboBox *mCloseLidComboBox;
    QComboBox *mPowerComboBox;
    QComboBox *mBatteryComboBox;
    QComboBox *mLowpowerComboBox1;
    QComboBox *mLowpowerComboBox2;
    QComboBox *mNoticeComboBox;

    SwitchButton *mSleepPwdBtn;
    SwitchButton *mWakenPwdBtn;
    SwitchButton *mLowSaveBtn;
    SwitchButton *mBatterySaveBtn;
    SwitchButton *mDisplayTimeBtn;

    QStringList sleepStringList;
    QStringList closeStringList;
    QStringList closeLidStringList;
    QStringList PowerplanStringList;
    QStringList BatteryplanStringList;
    QStringList LowpowerStringList;
    QStringList LowNoticeStringList;
    QStringList darkenStringList;
    QStringList buttonStringList;

    QStringList mPowerKeys;

    bool settingsCreate;
    bool isExitsPower;
    bool mFirstLoad;
    bool isExitsLid;
    bool isExitHibernate;

    bool hasBat;

    QDBusInterface *mUkccInterface;

private:
    void initGeneralSet();
    bool getHibernateStatus();
    QString  getHibernateTime();
    void initDbus();
    void initDeviceStatus();

private slots:
    void setIdleTime(int idleTime);
    void setHibernateTime(QString hibernate);
};

#endif // POWER_H
