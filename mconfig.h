//
//   Copyright (C) 2003-2010 by Warren Woodford
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//

#ifndef MCONFIG_H
#define MCONFIG_H

#include "ui_meconfig.h"
#include <qlistview.h>
#include <qcheckbox.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qcursor.h>
#include <qdir.h>
#include <qprocess.h>
#include <qtimer.h>

class MConfig : public QDialog, public Ui::MEConfig {
  Q_OBJECT
  protected:
    QProcess *proc;
    QTimer *timer;

public:
  MConfig(QWidget* parent = 0);
  ~MConfig();
    // helpers
    static QString getCmdOut(QString cmd);
    static QStringList getCmdOuts(QString cmd);
    static QString getCmdValue(QString cmd, QString key, QString keydel, QString valdel);
    static QStringList getCmdValues(QString cmd, QString key, QString keydel, QString valdel);
    static bool replaceStringInFile(QString oldtext, QString newtext, QString filepath);
    static bool mountPartition(QString dev, QString point);
    // common
    void refresh();
    // special
    void refreshRestore();
    void diskComboChanged();
    void refreshGeneral();
    void refreshMouse();
    void refreshMonitor();
    void brandComboChanged();
    void refreshNvidia();
    void brand2ComboChanged();

    void applyRestore();
    void applyGeneral();
    void applyMouse();
    void applyMonitor();
    void applyNvidia();
    void nvDriverEnable();
    void nvidiaDriverEnable();

public slots:
    void nvStart();
    void nvTime();
    void nvidiaUpdateDone(int exitCode, QProcess::ExitStatus exitStatus);
    void nvidiaDriverDone(int exitCode, QProcess::ExitStatus exitStatus);
    void legacyUpdateDone(int exitCode, QProcess::ExitStatus exitStatus);
    void legacyDriverDone(int exitCode, QProcess::ExitStatus exitStatus);
    void nvDriverDone(int exitCode, QProcess::ExitStatus exitStatus);

    virtual void show();
    virtual void on_diskCombo_activated();
    virtual void on_brandCombo_activated();
    virtual void on_modelCombo_activated();
    virtual void on_horizEdit_textEdited();
    virtual void on_vertEdit_textEdited();
    virtual void on_brand2Combo_activated();
    virtual void on_model2Combo_activated();
    virtual void on_horiz2Edit_textEdited();
    virtual void on_vert2Edit_textEdited();
    virtual void on_tabWidget_currentChanged();
    virtual void on_nvidiaButton_clicked();
    virtual void on_legacyButton_clicked();
    virtual void on_nvButton_clicked();
    virtual void on_nvVesaButton_clicked();
    virtual void on_ps2CheckBox_clicked();
    virtual void on_usbCheckBox_clicked();
    virtual void on_serialCheckBox_clicked();
    virtual void on_synCheckBox_clicked();
    virtual void on_appleCheckBox_clicked();
    virtual void on_alpsCheckBox_clicked();
    virtual void on_wacomCheckBox_clicked();
    virtual void on_smallerCheck_clicked();
    virtual void on_mediumCheck_clicked();
    virtual void on_largerCheck_clicked();
    virtual void on_buttonApply_clicked();
    virtual void on_buttonCancel_clicked();
    virtual void on_buttonOk_clicked();
    virtual void on_buttonAbout_clicked();
    virtual void on_generalHelpPushButton_clicked();

protected:

private:
    static bool hasInternetConnection();
    static void executeChild(const char* cmd, const char* param);

protected slots:
  /*$PROTECTED_SLOTS$*/

};

#endif

