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

#include "mconfig.h"

MConfig::MConfig(QWidget* parent) : QDialog(parent) {
  setupUi(this);
  setWindowIcon(QApplication::windowIcon());

  proc = new QProcess(this);
  timer = new QTimer(this);

  tabWidget->setCurrentIndex(0);
}

MConfig::~MConfig(){
}

/////////////////////////////////////////////////////////////////////////
// util functions

QString MConfig::getCmdOut(QString cmd) {
  char line[260];
  const char* ret = "";
  FILE* fp = popen(cmd.toAscii(), "r");
  if (fp == NULL) {
    return QString (ret);
  }
  int i;
  if (fgets(line, sizeof line, fp) != NULL) {
    i = strlen(line);
    line[--i] = '\0';
    ret = line;
  }
  pclose(fp);
  return QString (ret);
}

QStringList MConfig::getCmdOuts(QString cmd) {
  char line[260];
  FILE* fp = popen(cmd.toAscii(), "r");
  QStringList results;
  if (fp == NULL) {
    return results;
  }
  int i;
  while (fgets(line, sizeof line, fp) != NULL) {
    i = strlen(line);
    line[--i] = '\0';
    results.append(line);
  }
  pclose(fp);
  return results;
}

QString MConfig::getCmdValue(QString cmd, QString key, QString keydel, QString valdel) {
  const char *ret = "";
  char line[260];

  QStringList strings = getCmdOuts(cmd);
  for (QStringList::Iterator it = strings.begin(); it != strings.end(); ++it) {
    strcpy(line, ((QString)*it).toAscii());
    char* keyptr = strstr(line, key.toAscii());
    if (keyptr != NULL) {
      // key found
      strtok(keyptr, keydel.toAscii());
      const char* val = strtok(NULL, valdel.toAscii());
      if (val != NULL) {
        ret = val;
      }
      break;
    }
  }
  return QString (ret);
}

QStringList MConfig::getCmdValues(QString cmd, QString key, QString keydel, QString valdel) {
  char line[130];
  FILE* fp = popen(cmd.toAscii(), "r");
  QStringList results;
  if (fp == NULL) {
    return results;
  }
  int i;
  while (fgets(line, sizeof line, fp) != NULL) {
    i = strlen(line);
    line[--i] = '\0';
    char* keyptr = strstr(line, key.toAscii());
    if (keyptr != NULL) {
      // key found
      strtok(keyptr, keydel.toAscii());
      const char* val = strtok(NULL, valdel.toAscii());
      if (val != NULL) {
        results.append(val);
      }
    }
  }
  pclose(fp);
  return results;
}

bool MConfig::replaceStringInFile(QString oldtext, QString newtext, QString filepath) {

  QString cmd = QString("sed -i 's/%1/%2/g' %3").arg(oldtext).arg(newtext).arg(filepath);
  if (system(cmd.toAscii()) != 0) {
    return false;
  }
  return true;
}

bool MConfig::mountPartition(QString dev, QString point) {
  QDir("").mkdir(point);
  QString cmd = QString("mount %1 %2").arg(dev).arg(point);
  if (system(cmd.toAscii()) != 0) {
    return false;
  }
  return true;
}

/////////////////////////////////////////////////////////////////////////
// common

void MConfig::refresh() {
  int i = tabWidget->currentIndex();
  switch (i) {
    case 1:
      refreshGeneral();
      buttonApply->setEnabled(false);
      buttonOk->setEnabled(true);
      break;

    case 2:
      refreshMouse();
      buttonApply->setEnabled(false);
      buttonOk->setEnabled(true);
      break;

    case 3:
      refreshMonitor();
      buttonApply->setEnabled(false);
      buttonOk->setEnabled(true);
      break;

    case 4:
      refreshNvidia();
      buttonApply->setEnabled(true);
      buttonOk->setEnabled(false);
      break;

    default:
      refreshRestore();
      buttonApply->setEnabled(false);
      buttonOk->setEnabled(true);
      break;
  }
}

/////////////////////////////////////////////////////////////////////////
// special

void MConfig::refreshRestore() {
  char line[130];

//  system("umount -a 2>/dev/null");
  FILE *fp = popen("cat /proc/partitions | grep '[h,s].[a-z]$' | sort --key=4 2>/dev/null", "r");
  if (fp == NULL) {
    return;
  }
  diskCombo->clear();
  diskCombo->addItem("none");
  char *ndev, *nsz;
  int i, nsize;
  while (fgets(line, sizeof line, fp) != NULL) {
    i = strlen(line);
    line[--i] = '\0';
    strtok(line, " \t");
    strtok(NULL, " \t");
    nsz = strtok(NULL, " \t");
    ndev = strtok(NULL, " \t");
    if (ndev != NULL && strlen(ndev) == 3) {
      nsize = atoi(nsz) / 1024;
      if (nsize > 1000) {
        sprintf(line, "%s", ndev);
        diskCombo->addItem(line);
      }
    }
  }
  pclose(fp);
  diskCombo->setCurrentIndex(0);
  diskComboChanged();
}

// disk selection changed, rebuild root
void MConfig::diskComboChanged() {
  char line[130];
  QString drv = QString("/dev/%1").arg(diskCombo->currentText());

  rootCombo->clear();
  QString cmd = QString("/sbin/fdisk -l %1 | /bin/grep \"^/dev\"").arg(drv);
  FILE *fp = popen(cmd.toAscii(), "r");
  int rcount = 0;
  if (fp != NULL) {
    char *ndev, *nsz, *nsys, *nsys2;
    int nsize;
    int i;
    while (fgets(line, sizeof line, fp) != NULL) {
      i = strlen(line);
      line[--i] = '\0';
      strtok(line, " /*+\t");
      ndev = strtok(NULL, " /*+\t");
      strtok(NULL, " *+\t");
      strtok(NULL, " *+\t");
      nsz = strtok(NULL, " *+\t");
      strtok(NULL, " *+\t");
      nsys = strtok(NULL, " *+\t");
      nsys2 = strtok(NULL, " *+\t");
      nsize = atoi(nsz);
      nsize = nsize / 1024;

      if ((nsize >= 1800) && (strncmp(nsys, "Linux", 5) == 0)) {;
        sprintf(line, "%s - %dMB - %s", ndev, nsize, nsys);
        rootCombo->addItem(line);
        rcount++;
      }
    }
    pclose(fp);
  }
  if (rcount == 0) {
    rootCombo->addItem("none");
    buttonApply->setEnabled(false);
  } else {
    buttonApply->setEnabled(true);
  }
}

void MConfig::refreshGeneral() {
  QString val = getCmdOut("grep 'dpi 75' /etc/kde4/kdm/kdmrc");
  smallerCheck->setChecked(!val.isEmpty());
  if (val.isEmpty()) {
    // not small
    val = getCmdOut("grep 'dpi 120' /etc/kde4/kdm/kdmrc");
    largerCheck->setChecked(!val.isEmpty());
    if (val.isEmpty()) {
      // must be medium
      mediumCheck->setChecked(true);
    }
  }
}

void MConfig::refreshMouse() {
  QString val = getCmdOut("grep '#InputDevice \"PS/2' /etc/X11/xorg.conf");
  ps2CheckBox->setChecked(val.isEmpty());
  val = getCmdOut("grep '#InputDevice \"USB' /etc/X11/xorg.conf");
  usbCheckBox->setChecked(val.isEmpty());
  val = getCmdOut("grep '#InputDevice \"Ser' /etc/X11/xorg.conf");
  serialCheckBox->setChecked(val.isEmpty());
  val = getCmdOut("grep '#InputDevice \"Tou' /etc/X11/xorg.conf");
  synCheckBox->setChecked(val.isEmpty());
  val = getCmdOut("grep '#InputDevice \"ALP' /etc/X11/xorg.conf");
  alpsCheckBox->setChecked(val.isEmpty());
  val = getCmdOut("grep '#InputDevice \"Sty' /etc/X11/xorg.conf");
  wacomCheckBox->setChecked(val.isEmpty());
  val = getCmdOut("grep '#InputDevice \"App' /etc/X11/xorg.conf");
  appleCheckBox->setChecked(val.isEmpty());
}

void MConfig::refreshMonitor() {
  //monitorCombo
  QString val = getCmdValue("grep 'VendorName' /etc/X11/xorg.conf", "VendorName", " ", "\"");
  if (val.isEmpty()) {
    brandCombo->setCurrentIndex(brandCombo->findText("unknown"));
    brandComboChanged();
    modelCombo->setCurrentIndex(modelCombo->findText("unknown"));
  } else {
    brandCombo->setCurrentIndex(brandCombo->findText(val.trimmed()));
    if (brandCombo->currentText().isEmpty()) {
      brandCombo->setCurrentIndex(brandCombo->findText("unknown"));
      brandComboChanged();
      modelCombo->setCurrentIndex(modelCombo->findText("unknown"));
    } else {
      brandComboChanged();
      val = getCmdValue("grep 'ModelName' /etc/X11/xorg.conf", "ModelName", " ", "\"");
      modelCombo->setCurrentIndex(modelCombo->findText(val.trimmed()));
    }
  }
  val = getCmdValue("grep 'HorizSync' /etc/X11/xorg.conf", "HorizSync", " ", "#");
  horizEdit->setText(val.trimmed());
  val = getCmdValue("grep 'VertRefresh' /etc/X11/xorg.conf", "VertRefresh", " ", "#");
  vertEdit->setText(val.trimmed());
}

void MConfig::brandComboChanged() {
  QString model, cmd;

  modelCombo->clear();
  if (brandCombo->currentText().contains("unknown") > 0) {
    modelCombo->addItem("unknown");
    return;
  }

  if (brandCombo->currentText().contains("KDS") > 0) {
    cmd = QString("grep '^%1' /usr/share/hwdata/MonitorsDB").arg("Korea Data Systems");
  } else {
    cmd = QString("grep '^%1' /usr/share/hwdata/MonitorsDB").arg(brandCombo->currentText());
  }
  QStringList strings = getCmdOuts(cmd);
  if (strings.isEmpty()) {
    modelCombo->addItem("unknown");
    return;
  }

  for (QStringList::Iterator it = strings.begin(); it != strings.end(); ++it) {
    model = *it;
    model = model.section(";", 1, 1).remove(brandCombo->currentText()).trimmed();
    modelCombo->addItem(model);
  }
  on_modelCombo_activated();
}

void MConfig::refreshNvidia() {
  // default to vesa
  nvVesaButton->setChecked(true);
  nvGroupBox->setEnabled(false);
  QString val = getCmdOut("grep 'Driver \"nvidia\"' /etc/X11/xorg.conf");
  if (val.isEmpty()) {
    // not nvidia
    val = getCmdOut("grep 'Driver \"nv\"' /etc/X11/xorg.conf");
    if (!val.isEmpty()) {
      // it's nv
      nvButton->setChecked(true);
    }
  } else {
    // kind of nvidia
    nvidiaButton->setChecked(true);
    nvGroupBox->setEnabled(true);
    val = getCmdValue("dpkg -s nvidia-glx-legacy-96xx | grep '^Status'", "ok", " ", " ");
    if (val.compare("installed") == 0) {
      // nvidia-glx-legacy is installed
      legacyButton->setChecked(true);
    }
  }
  nvProgressBar->setValue(0);
  val = getCmdOut("grep 'CursorShadow\" \"1\"' /etc/X11/xorg.conf");
  shadowCheck->setChecked(!val.isEmpty());

  val = getCmdOut("grep 'TwinView\" \"true\"' /etc/X11/xorg.conf");
  twinviewCheck->setChecked(!val.isEmpty());
  val = getCmdOut("grep 'ConnectedMonitor\" \"dfp,tv\"' /etc/X11/xorg.conf");
  if (!val.isEmpty()) {
    tvButton->setChecked(true);
  } else {
    val = getCmdOut("grep 'TwinViewOrientation\" \"RightOf\"' /etc/X11/xorg.conf");
    if (!val.isEmpty()) {
      rightButton->setChecked(true);
    } else {
      val = getCmdOut("grep 'TwinViewOrientation\" \"LeftOf\"' /etc/X11/xorg.conf");
      if (!val.isEmpty()) {
        leftButton->setChecked(true);
      } else {
        cloneButton->setChecked(true);
      }
    }
  }

  //monitor2Combo
  val = getCmdValue("grep 'SecondMonitorVendorName' /etc/X11/xorg.conf", "VendorName", " ", "\"");
  if (val.isEmpty()) {
    brand2Combo->setCurrentIndex(brand2Combo->findText("unknown"));
    brand2ComboChanged();
    model2Combo->setCurrentIndex(model2Combo->findText("unknown"));
  } else {
    brand2Combo->setCurrentIndex(brand2Combo->findText(val.trimmed()));
    if (brand2Combo->currentText().isEmpty()) {
      brand2Combo->setCurrentIndex(brand2Combo->findText("unknown"));
      brand2ComboChanged();
      model2Combo->setCurrentIndex(model2Combo->findText("unknown"));
    } else {
      brand2ComboChanged();
      val = getCmdValue("grep 'SecondMonitorModelName' /etc/X11/xorg.conf", "ModelName", " ", "\"");
      model2Combo->setCurrentIndex(model2Combo->findText(val.trimmed()));
    }
  }

  val = getCmdValue("grep 'SecondMonitorHorizSync' /etc/X11/xorg.conf", "HorizSync", " ", "#");
  horiz2Edit->setText(val.trimmed().remove('"'));
  val = getCmdValue("grep 'SecondMonitorVertRefresh' /etc/X11/xorg.conf", "VertRefresh", " ", "#");
  vert2Edit->setText(val.trimmed().remove('"'));
}

void MConfig::brand2ComboChanged() {
  QString model, cmd;

  model2Combo->clear();
  if (brand2Combo->currentText().contains("unknown") > 0) {
    model2Combo->addItem("unknown");
    return;
  }
  if (brand2Combo->currentText().contains("KDS") > 0) {
    cmd = QString("grep '^%1' /usr/share/hwdata/MonitorsDB").arg("Korea Data Systems");
  } else {
    cmd = QString("grep '^%1' /usr/share/hwdata/MonitorsDB").arg(brand2Combo->currentText());
  }
  QStringList strings = getCmdOuts(cmd);
  if (strings.isEmpty()) {
    model2Combo->addItem("unknown");
    return;
  }

  for (QStringList::Iterator it = strings.begin(); it != strings.end(); ++it) {
    model = *it;
    model = model.section(";", 1, 1).remove(brand2Combo->currentText()).trimmed();
    model2Combo->addItem(model);
  }
  on_model2Combo_activated();
}

// apply but do not close
void MConfig::applyRestore() {
  char line[130];
  system("umount -l /mnt/mxconfig 2>/dev/null");
  strcpy(line, rootCombo->currentText().toAscii());
  char *tok = strtok(line, " -");
  QString rootdev = QString("/dev/%1").arg(tok);
  mountPartition(rootdev, "/mnt/mxconfig");
  if (system("/bin/cp /etc/X11/xorg.conf /mnt/mxconfig/etc/X11") != 0) {
    QMessageBox::critical(0, QString::null,
      tr("Copying X config appears to have failed.  There may be something wrong with the destination root partition."));
    return;
  }

  QMessageBox::information(0, QString::null,
      tr("Your current X configuration has been installed on the destination partition."));

  system("umount -l /mnt/mxconfig 2>/dev/null");
  system("rmdir /mnt/mxconfig");
  refresh();
}

void MConfig::applyGeneral() {
  if (smallerCheck->isChecked()) {
    replaceStringInFile("dpi 100", "dpi 75", "/etc/kde4/kdm/kdmrc");
    replaceStringInFile("dpi 96", "dpi 75", "/etc/kde4/kdm/kdmrc");
    replaceStringInFile("dpi 120", "dpi 75", "/etc/kde4/kdm/kdmrc");
  } else if (mediumCheck->isChecked()) {
    replaceStringInFile("dpi 75", "dpi 96", "/etc/kde4/kdm/kdmrc");
    replaceStringInFile("dpi 100", "dpi 96", "/etc/kde4/kdm/kdmrc");
    replaceStringInFile("dpi 120", "dpi 96", "/etc/kde4/kdm/kdmrc");
  } else {
    // must be larger
    replaceStringInFile("dpi 75", "dpi 120", "/etc/kde4/kdm/kdmrc");
    replaceStringInFile("dpi 100", "dpi 120", "/etc/kde4/kdm/kdmrc");
    replaceStringInFile("dpi 96", "dpi 120", "/etc/kde4/kdm/kdmrc");
  }
  QMessageBox::information(0, QString::null,
    tr("The display text size (dpi) has been updated. The change will take effect when you restart X or reboot"));
  refresh();
}

void MConfig::applyMouse() {
  if (ps2CheckBox->isChecked()) {
    replaceStringInFile("#InputDevice \"PS", " InputDevice \"PS", "/etc/X11/xorg.conf");
  } else {
    replaceStringInFile(" InputDevice \"PS", "#InputDevice \"PS", "/etc/X11/xorg.conf");
  }
  if (usbCheckBox->isChecked()) {
    replaceStringInFile("#InputDevice \"USB", " InputDevice \"USB", "/etc/X11/xorg.conf");
  } else {
    replaceStringInFile(" InputDevice \"USB", "#InputDevice \"USB", "/etc/X11/xorg.conf");
  }
  if (serialCheckBox->isChecked()) {
    replaceStringInFile("#InputDevice \"Ser", " InputDevice \"Ser", "/etc/X11/xorg.conf");
  } else {
    replaceStringInFile(" InputDevice \"Ser", "#InputDevice \"Ser", "/etc/X11/xorg.conf");
  }
  if (synCheckBox->isChecked()) {
    replaceStringInFile("#InputDevice \"Tou", " InputDevice \"Tou", "/etc/X11/xorg.conf");
  } else {
    replaceStringInFile(" InputDevice \"Tou", "#InputDevice \"Tou", "/etc/X11/xorg.conf");
  }
  if (alpsCheckBox->isChecked()) {
    replaceStringInFile("#InputDevice \"ALP", " InputDevice \"ALP", "/etc/X11/xorg.conf");
  } else {
    replaceStringInFile(" InputDevice \"ALP", "#InputDevice \"ALP", "/etc/X11/xorg.conf");
  }
  if (appleCheckBox->isChecked()) {
    replaceStringInFile("#InputDevice \"App", " InputDevice \"App", "/etc/X11/xorg.conf");
  } else {
    replaceStringInFile(" InputDevice \"App", "#InputDevice \"App", "/etc/X11/xorg.conf");
  }
  if (wacomCheckBox->isChecked()) {
    replaceStringInFile("#InputDevice \"Sty", " InputDevice \"Sty", "/etc/X11/xorg.conf");
    replaceStringInFile("#InputDevice \"Era", " InputDevice \"Era", "/etc/X11/xorg.conf");
    replaceStringInFile("#InputDevice \"Cur", " InputDevice \"Cur", "/etc/X11/xorg.conf");
  } else {
    replaceStringInFile(" InputDevice \"Sty", "#InputDevice \"Sty", "/etc/X11/xorg.conf");
    replaceStringInFile(" InputDevice \"Era", "#InputDevice \"Era", "/etc/X11/xorg.conf");
    replaceStringInFile(" InputDevice \"Cur", "#InputDevice \"Cur", "/etc/X11/xorg.conf");
  }

  QMessageBox::information(0, QString::null,
    tr("The enabled mouse types have been updated. The new config will take effect when you restart X or reboot.  If a type is not checked, it still may be configured automatically by the X server."));
  refresh();
}

void MConfig::applyMonitor() {
  // brand
  QString cmd = QString(" VendorName \"%1\"").arg(brandCombo->currentText());
  replaceStringInFile(" VendorName.*", cmd, "/etc/X11/xorg.conf");

  // brand
  cmd = QString(" ModelName \"%1\"").arg(modelCombo->currentText());
  replaceStringInFile(" ModelName.*", cmd, "/etc/X11/xorg.conf");

  // vert
  cmd = QString(" VertRefresh  %1").arg(vertEdit->text());
  replaceStringInFile(" VertRefresh.*", cmd, "/etc/X11/xorg.conf");

  // horiz
  cmd = QString(" HorizSync    %1").arg(horizEdit->text());
  replaceStringInFile(" HorizSync.*", cmd, "/etc/X11/xorg.conf");

  QMessageBox::information(0, QString::null,
    tr("The monitor specs have been updated. The new config will take effect when you restart X or reboot"));
  refresh();
}

void MConfig::applyNvidia() {
  // nvidia
  if (shadowCheck->isChecked()) {
    replaceStringInFile("CursorShadow\" \"0", "CursorShadow\" \"1", "/etc/X11/xorg.conf");
  } else {
    replaceStringInFile("CursorShadow\" \"1", "CursorShadow\" \"0", "/etc/X11/xorg.conf");
  }

  if (twinviewCheck->isChecked()) {
    replaceStringInFile("TwinView\" \"false", "TwinView\" \"true", "/etc/X11/xorg.conf");
    replaceStringInFile("#Option \"MetaModes", " Option \"MetaModes", "/etc/X11/xorg.conf");
    if (leftButton->isChecked()) {
     replaceStringInFile("TwinViewOrientation.*", "TwinViewOrientation\" \"LeftOf\"", "/etc/X11/xorg.conf");
      replaceStringInFile("ConnectedMonitor.*", "ConnectedMonitor\" \"dfp,dfp\"", "/etc/X11/xorg.conf");
    } else if (rightButton->isChecked()) {
      replaceStringInFile("TwinViewOrientation.*", "TwinViewOrientation\" \"RightOf\"", "/etc/X11/xorg.conf");
      replaceStringInFile("ConnectedMonitor.*", "ConnectedMonitor\" \"dfp,dfp\"", "/etc/X11/xorg.conf");
    } else if (cloneButton->isChecked()) {
      replaceStringInFile("TwinViewOrientation.*", "TwinViewOrientation\" \"Clone\"", "/etc/X11/xorg.conf");
      replaceStringInFile("ConnectedMonitor.*", "ConnectedMonitor\" \"dfp,dfp\"", "/etc/X11/xorg.conf");
    } else {
      replaceStringInFile("TwinViewOrientation.*", "TwinViewOrientation\" \"Clone\"", "/etc/X11/xorg.conf");
      replaceStringInFile("ConnectedMonitor.*", "ConnectedMonitor\" \"dfp,tv\"", "/etc/X11/xorg.conf");
    }
    // monitor
    // brand
    QString cmd = QString("torVendorName\" \"%1\"").arg(brand2Combo->currentText());
    replaceStringInFile("torVendorName.*", cmd, "/etc/X11/xorg.conf");

    // brand
    cmd = QString("torModelName\" \"%1\"").arg(model2Combo->currentText());
    replaceStringInFile("torModelName.*", cmd, "/etc/X11/xorg.conf");

        // vert
    cmd = QString("torVertRefresh\" \"%1\"").arg(vert2Edit->text());
    replaceStringInFile("torVertRefresh.*", cmd, "/etc/X11/xorg.conf");

        // horiz
    cmd = QString("torHorizSync\" \"%1\"").arg(horiz2Edit->text());
    replaceStringInFile("torHorizSync.*", cmd, "/etc/X11/xorg.conf");
  } else {
    replaceStringInFile("TwinView\" \"true", "TwinView\" \"false", "/etc/X11/xorg.conf");
    replaceStringInFile(" Option \"MetaModes", "#Option \"MetaModes", "/etc/X11/xorg.conf");
  }

  if (nvidiaButton->isChecked()) {
    // use the nvidia new driver
    QString val = getCmdValue("dpkg -s nvidia-glx | grep '^Status'", "ok", " ", " ");
    if (val.compare("installed") != 0) {
      int ans = QMessageBox::warning(0, QString::null,
        tr("The nvidia (new) driver is designed for NVIDIA graphics chips newer than Quatro4 700. The nvidia (new) driver requires that the nvidia-glx package be installed. You must be running from hard drive, and you must be connected to the internet. This will take a while!  Are you sure you want to do this now?"),
        tr("Yes"), tr("No"));
      if (ans != 0) {
        refresh();
        return;
      }
      // update apt db
      setCursor(QCursor(Qt::WaitCursor));
      nvStatusEdit->setText(tr("Update package list (apt)..."));
      disconnect(timer, SIGNAL(timeout()), 0, 0);
      connect(timer, SIGNAL(timeout()), this, SLOT(nvTime()));
      disconnect(proc, SIGNAL(started()), 0, 0);
      connect(proc, SIGNAL(started()), this, SLOT(nvStart()));
      disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
      connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(nvidiaUpdateDone(int, QProcess::ExitStatus)));
      val = QString("apt-get update");
      proc->start(val);
    } else {
      // nvidia-glx already installed
      nvStatusEdit->setText(tr("Enable driver..."));
//      replaceStringInFile("blacklist nv.*", "blacklist nvidia-legacy", "/etc/modprobe.d/nvidia");
//      replaceStringInFile("^alias nvidia", "#alias nvidia", "/etc/modprobe.d/nvidia");
      nvidiaDriverEnable();
    }
  } else if (legacyButton->isChecked()) {
    QString val = getCmdValue("dpkg -s nvidia-glx-legacy-96xx | grep '^Status'", "ok", " ", " ");
    if (val.compare("installed") != 0) {
      int ans = QMessageBox::warning(0, QString::null,
        tr("The nvidia (legacy) driver is designed for NVIDIA graphics chips as old as GeForce2 MX and as new as Quatro4 700. The legacy driver requires that the nvidia-glx-legacy-96xx package be installed. You must be running from hard drive, and you must be connected to the internet. This will take a while!  Are you sure you want to do this now?"),
        tr("Yes"), tr("No"));
      if (ans != 0) {
        refresh();
        return;
      }
      nvStatusEdit->setText(tr("Update package list (apt)..."));
      setCursor(QCursor(Qt::WaitCursor));	  
      disconnect(timer, SIGNAL(timeout()), 0, 0);
      connect(timer, SIGNAL(timeout()), this, SLOT(nvTime()));
      disconnect(proc, SIGNAL(started()), 0, 0);
      connect(proc, SIGNAL(started()), this, SLOT(nvStart()));
      disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
      connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(legacyUpdateDone(int, QProcess::ExitStatus)));
      val = QString("apt-get update");
      proc->start(val);
    } else {
      // nvidia-glx-legacy already installed
      nvStatusEdit->setText(tr("Enable driver..."));
//      replaceStringInFile("blacklist nv.*", "blacklist nvidia", "/etc/modprobe.d/nvidia");
//      replaceStringInFile("#alias nvidia", "alias nvidia", "/etc/modprobe.d/nvidia");
      nvidiaDriverEnable();
    }
  } else {
    // use the nv or the vesa driver
    QString val = getCmdValue("dpkg -s nvidia-glx | grep '^Status'", "ok", " ", " ");
    if (val.compare("installed") == 0) {
      // remove nvidia-glx
      int ans = QMessageBox::warning(0, QString::null,
        tr("The nvidia-glx package is installed.  It must be removed before using a different driver. Are you sure you want to do this now?"),
        tr("Yes"), tr("No"));
      if (ans != 0) {
        refresh();
        return;
      }
      nvStatusEdit->setText(tr("Remove nvidia driver..."));
      setCursor(QCursor(Qt::WaitCursor));	  
      disconnect(timer, SIGNAL(timeout()), 0, 0);
      connect(timer, SIGNAL(timeout()), this, SLOT(nvTime()));
      disconnect(proc, SIGNAL(started()), 0, 0);
      connect(proc, SIGNAL(started()), this, SLOT(nvStart()));
      disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
      connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(nvDriverDone(int, QProcess::ExitStatus)));
      val = QString("apt-get -qq remove --purge nvidia-glx");
      proc->start(val);
    } else {
      QString val = getCmdValue("dpkg -s nvidia-glx-legacy-96xx | grep '^Status'", "ok", " ", " ");
      if (val.compare("installed") == 0) {
        // remove nvidia-glx-legacy
        int ans = QMessageBox::warning(0, QString::null,
          tr("The nvidia-glx-legacy-96xx package is installed. It must be removed before using a different driver. Are you sure you want to do this now?"),
          tr("Yes"), tr("No"));
        if (ans != 0) {
          refresh();
          return;
        }
        nvStatusEdit->setText(tr("Remove nvidia driver..."));
        setCursor(QCursor(Qt::WaitCursor));	  
        disconnect(timer, SIGNAL(timeout()), 0, 0);
        connect(timer, SIGNAL(timeout()), this, SLOT(nvTime()));
        disconnect(proc, SIGNAL(started()), 0, 0);
        connect(proc, SIGNAL(started()), this, SLOT(nvStart()));
        disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
        connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(nvDriverDone(int, QProcess::ExitStatus)));
        val = QString("apt-get -qq remove --purge nvidia-glx-legacy-96xx");
        proc->start(val);
      } else {
        // just enable nv or vesa
        nvStatusEdit->setText(tr("Enable driver..."));
        nvDriverEnable();
      }
    }
  }
}

void MConfig:: nvDriverEnable() {
  replaceStringInFile("Composite\" \"Disable", "Composite\" \"Enable", "/etc/X11/xorg.conf");
//  replaceStringInFile("blacklist nv.*", "blacklist nvidia-legacy", "/etc/modprobe.d/nvidia");
//  replaceStringInFile("^alias nvidia", "#alias nvidia", "/etc/modprobe.d/nvidia");
  replaceStringInFile("DefaultColorDepth .*", "DefaultColorDepth 16", "/etc/X11/xorg.conf");
  if (nvButton->isChecked()) {
    // use nv
    replaceStringInFile("Driver \"vesa\"", "Driver \"nv\"", "/etc/X11/xorg.conf");
    replaceStringInFile("Driver \"nvidia\"", "Driver \"nv\"", "/etc/X11/xorg.conf");
    nvStatusEdit->setText(tr("nv driver...enabled"));
  } else {
    // use vesa
    replaceStringInFile("Driver \"nvidia\"", "Driver \"vesa\"", "/etc/X11/xorg.conf");
    replaceStringInFile("Driver \"nv\"", "Driver \"vesa\"", "/etc/X11/xorg.conf");
    nvStatusEdit->setText(tr("vesa driver...enabled"));
  }
  replaceStringInFile("#Load \"glx\"", " Load \"glx\"", "/etc/X11/xorg.conf");
  replaceStringInFile("#Load \"dri\"", " Load \"dri\"", "/etc/X11/xorg.conf");
  replaceStringInFile("#Load \"GLcore\"", " Load \"GLcore\"", "/etc/X11/xorg.conf");
  replaceStringInFile(" Screen 0 \"ATIScreen\"", "#Screen 0 \"ATIScreen\"", "/etc/X11/xorg.conf");
  replaceStringInFile("#Screen 0 \"Screen0\"", " Screen 0 \"Screen0\"", "/etc/X11/xorg.conf");
  refresh();
  QMessageBox::information(0, QString::null,
    tr("The driver has been changed. You must reboot so the change will take effect."));
}

void MConfig:: nvidiaDriverEnable() {
  replaceStringInFile("Composite\" \"Disable", "Composite\" \"Enable", "/etc/X11/xorg.conf");
  replaceStringInFile("DefaultColorDepth .*", "DefaultColorDepth 24", "/etc/X11/xorg.conf");
  replaceStringInFile("Driver \"vesa\"", "Driver \"nvidia\"", "/etc/X11/xorg.conf");
  replaceStringInFile("Driver \"nv\"", "Driver \"nvidia\"", "/etc/X11/xorg.conf");
  replaceStringInFile("#Load \"glx\"", " Load \"glx\"", "/etc/X11/xorg.conf");
  replaceStringInFile(" Load \"dri\"", "#Load \"dri\"", "/etc/X11/xorg.conf");
  replaceStringInFile(" Load \"GLcore\"", "#Load \"GLcore\"", "/etc/X11/xorg.conf");
  replaceStringInFile(" Screen 0 \"ATIScreen\"", "#Screen 0 \"ATIScreen\"", "/etc/X11/xorg.conf");
  replaceStringInFile("#Screen 0 \"Screen0\"", " Screen 0 \"Screen0\"", "/etc/X11/xorg.conf");
  refresh();
  nvStatusEdit->setText(tr("nvidia driver...enabled"));
  QMessageBox::information(0, QString::null,
    tr("The driver has been changed. You must reboot so the change will take effect."));
}

/////////////////////////////////////////////////////////////////////////
// nvidia process events

void MConfig::nvStart() {
  timer->start(100);
}

void MConfig::nvTime() {
  int i = nvProgressBar->value() + 1;
  if (i > 100) {
    i = 0;
  }
  nvProgressBar->setValue(i);
}

void MConfig::nvidiaUpdateDone(int exitCode, QProcess::ExitStatus exitStatus) {
  if (exitStatus == QProcess::NormalExit) {
    // now install the driver
    nvStatusEdit->setText(tr("Install nvidia-glx driver..."));
    disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
    connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(nvidiaDriverDone(int, QProcess::ExitStatus)));
    QString ker = getCmdOut("uname -r");
    system("apt-get -qq remove --purge nvidia-kernel-legacy-71xx-2.*");
    system("apt-get -qq remove --purge nvidia-kernel-legacy-96xx-2.*");
    QString val = QString("apt-get -qq install nvidia-glx");
    val = QString("apt-get -qq install nvidia-kernel-%1 nvidia-glx").arg(ker);
    proc->start(val);
  } else {
    timer->stop();
    nvProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
    nvStatusEdit->setText(tr("Update package list (apt)...failed"));
    QMessageBox::information(0, QString::null,
      tr("Updating the apt package list failed. If you are sure you are on-line you can try again."));
  }
}

void MConfig::nvidiaDriverDone(int exitCode, QProcess::ExitStatus exitStatus) {
  if (exitStatus == QProcess::NormalExit) {
    nvStatusEdit->setText(tr("Install nvidia-glx driver...finalizing"));
    system("depmod -a");
    timer->stop();
    nvProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
//    replaceStringInFile("blacklist nv.*", "blacklist nvidia-legacy", "/etc/modprobe.d/nvidia");
//    replaceStringInFile("^alias nvidia", "#alias nvidia", "/etc/modprobe.d/nvidia");
    nvidiaDriverEnable();
  } else {
    timer->stop();
    nvProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
    nvStatusEdit->setText(tr("Install nvidia-glx driver...failed"));
    QMessageBox::information(0, QString::null,
      tr("Installing the nvidia-glx driver failed. It is highly advised that you select nv or vesa and click Apply immediately."));
  }
}

void MConfig::legacyUpdateDone(int exitCode, QProcess::ExitStatus exitStatus) {
  if (exitStatus == QProcess::NormalExit) {
    // now install the driver
    nvStatusEdit->setText(tr("Install nvidia-glx-legacy driver..."));
    disconnect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), 0, 0);
    connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(legacyDriverDone(int, QProcess::ExitStatus)));
    QString ker = getCmdOut("uname -r");
    system("apt-get -qq remove --purge nvidia-kernel-legacy-71*");
    system("apt-get -qq remove --purge nvidia-kernel-2.*");
    QString val = QString("apt-get -qq install nvidia-kernel-legacy-96xx-%1 nvidia-glx-legacy-96xx").arg(ker);
    proc->start(val);
  } else {
    timer->stop();
    nvProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
    nvStatusEdit->setText(tr("Update package list (apt)...failed"));
    QMessageBox::information(0, QString::null,
      tr("Updating the apt package list failed. If you are sure you are on-line you can try again."));
  }
}

void MConfig::legacyDriverDone(int exitCode, QProcess::ExitStatus exitStatus) {
  if (exitStatus == QProcess::NormalExit) {
    nvStatusEdit->setText(tr("Install nvidia-glx-legacy driver...finalizing"));
    system("depmod -a");
    timer->stop();
    nvProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
//    replaceStringInFile("blacklist nv.*", "blacklist nvidia", "/etc/modprobe.d/nvidia");
//    replaceStringInFile("#alias nvidia", "alias nvidia", "/etc/modprobe.d/nvidia");
    nvidiaDriverEnable();
  } else {
    timer->stop();
    nvProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
    nvStatusEdit->setText(tr("Install nvidia-glx-legacy driver...failed"));
    QMessageBox::information(0, QString::null,
      tr("Installing the nvidia-glx-legacy driver failed. It is highly advised that you select nv or vesa and click Apply immediately."));
  }
}

void MConfig::nvDriverDone(int exitCode, QProcess::ExitStatus exitStatus) {
  if (exitStatus == QProcess::NormalExit) {
    timer->stop();
    nvProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
    nvDriverEnable();
  } else {
    timer->stop();
    nvProgressBar->setValue(0);
    setCursor(QCursor(Qt::ArrowCursor));
    nvStatusEdit->setText(tr("Remove nvidia driver...failed"));
  }
}

/////////////////////////////////////////////////////////////////////////
// slots

void MConfig::show() {
  QDialog::show();
  refresh();
}

// disk selection changed, rebuild root
void MConfig::on_diskCombo_activated() {
  diskComboChanged();
}

void MConfig::on_brandCombo_activated() {
  brandComboChanged();
}

void MConfig::on_modelCombo_activated() {
  QString cmd = QString("grep '.*%1.*%2' /usr/share/hwdata/MonitorsDB").arg(brandCombo->currentText()).arg( modelCombo->currentText());
  QString model = getCmdOut(cmd);
  QString part = model.section(";", 3, 3).trimmed();
  horizEdit->setText(part);
  part = model.section(";", 4, 4).trimmed();
  vertEdit->setText(part);
  buttonApply->setEnabled(true);
}

void MConfig::on_brand2Combo_activated() {
  brand2ComboChanged();
}

void MConfig::on_model2Combo_activated() {
  QString cmd = QString("grep '.*%1.*%2' /usr/share/hwdata/MonitorsDB").arg(brand2Combo->currentText()).arg( model2Combo->currentText());
  QString model = getCmdOut(cmd);
  QString part = model.section(";", 3, 3).trimmed();
  horiz2Edit->setText(part);
  part = model.section(";", 4, 4).trimmed();
  vert2Edit->setText(part);
  buttonApply->setEnabled(true);
}

void MConfig::on_horizEdit_textEdited() {
  buttonApply->setEnabled(true);
}

void MConfig::on_vertEdit_textEdited() {
  buttonApply->setEnabled(true);
}

void MConfig::on_horiz2Edit_textEdited() {
  buttonApply->setEnabled(true);
}

void MConfig::on_vert2Edit_textEdited() {
  buttonApply->setEnabled(true);
}

void MConfig::on_tabWidget_currentChanged() {
  refresh();
}

void MConfig::on_nvidiaButton_clicked() {
  nvGroupBox->setEnabled(true);
  buttonApply->setEnabled(true);
}

void MConfig::on_legacyButton_clicked() {
  nvGroupBox->setEnabled(true);
  buttonApply->setEnabled(true);
}

void MConfig::on_nvButton_clicked() {
  nvGroupBox->setEnabled(false);
  buttonApply->setEnabled(true);
}

void MConfig::on_nvVesaButton_clicked() {
  nvGroupBox->setEnabled(false);
  buttonApply->setEnabled(true);
}

void MConfig::on_ps2CheckBox_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_usbCheckBox_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_serialCheckBox_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_synCheckBox_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_appleCheckBox_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_alpsCheckBox_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_wacomCheckBox_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_smallerCheck_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_mediumCheck_clicked() {
  buttonApply->setEnabled(true);
}

void MConfig::on_largerCheck_clicked() {
  buttonApply->setEnabled(true);
}

// apply but do not close
void MConfig::on_buttonApply_clicked() {
  if (!buttonApply->isEnabled()) {
    return;
  }

  int i = tabWidget->currentIndex();
  switch (i) {
    case 1:
      setCursor(QCursor(Qt::WaitCursor));
      applyGeneral();
      setCursor(QCursor(Qt::ArrowCursor));
      break;

    case 2:
      setCursor(QCursor(Qt::WaitCursor));
      applyMouse();
      setCursor(QCursor(Qt::ArrowCursor));
      break;

    case 3:
      setCursor(QCursor(Qt::WaitCursor));
      applyMonitor();
      setCursor(QCursor(Qt::ArrowCursor));
      break;

    case 4:
      applyNvidia();
      break;

    default:
      setCursor(QCursor(Qt::WaitCursor));
      applyRestore();
      setCursor(QCursor(Qt::ArrowCursor));
      break;
  }


  // disable button
  buttonApply->setEnabled(false);
}

// close but do not apply
void MConfig::on_buttonCancel_clicked() {
  close();
}

// apply then close
void MConfig::on_buttonOk_clicked() {
  on_buttonApply_clicked();
  close();
}

bool MConfig::hasInternetConnection() 
{
   bool internetConnection  = false;
   // Query network interface status
   QStringList interfaceList  = getCmdOuts("ifconfig -a -s");
   int i=1;
   while (i<interfaceList.size()) {
      QString interface = interfaceList.at(i);
      interface = interface.left(interface.indexOf(" "));
      if ((interface != "lo") && (interface != "wmaster0") && (interface != "wifi0")) {
         QStringList ifStatus  = getCmdOuts(QString("ifconfig %1").arg(interface));
         QString unwrappedList = ifStatus.join(" ");
         if (unwrappedList.indexOf("UP ") != -1) {
            if (unwrappedList.indexOf(" RUNNING ") != -1) {
               internetConnection  = true;
            }
         }
      }
      ++i;
   }
   return internetConnection;
}

void MConfig::executeChild(const char* cmd, const char* param)
{
   pid_t childId;
   childId = fork();
   if (childId >= 0)
      {
      if (childId == 0)
         {
         execl(cmd, cmd, param, (char *) 0);

         //system(cmd);
         }
      }
}

void MConfig::on_generalHelpPushButton_clicked() 
{
   QString manualPackage = tr("mepis-manual");
   QString statusl = getCmdValue(QString("dpkg -s %1 | grep '^Status'").arg(manualPackage).toAscii(), "ok", " ", " ");
   if (statusl.compare("installed") != 0) {
      if (this->hasInternetConnection()) {
         setCursor(QCursor(Qt::WaitCursor));
         int ret = QMessageBox::information(0, tr("The Mepis manual is not installed"),
                      tr("The Mepis manual is not installed, do you want to install it now?"),
                      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
         if (ret == QMessageBox::Yes) {
            system(QString("apt-get install -qq %1").arg(manualPackage).toAscii());
            setCursor(QCursor(Qt::ArrowCursor));
            statusl = getCmdValue(QString("dpkg -s %1 | grep '^Status'").arg(manualPackage).toAscii(), "ok", " ", " ");
            if (statusl.compare("installed") != 0) {
               QMessageBox::information(0, tr("The Mepis manual hasn't been installed"),
               tr("The Mepis manual cannot be installed. This may mean you are using the LiveCD or that there are some kind of transitory problem with the repositories,"),
               QMessageBox::Ok);

            }
         }
         else {
            setCursor(QCursor(Qt::ArrowCursor));               
            return;
            }
      }
      else {
         QMessageBox::information(0, tr("The MEPIS manual is not installed"),
            tr("The MEPIS manual is not installed and no Internet connection could be detected so it cannot be installed"),
            QMessageBox::Ok);
         return;
      }
   }
   QString page;
   page = tr("file:///usr/share/mepis-manual/en/index.html#section05-3-4");

   //executeChild("/usr/bin/konqueror", page.toAscii());
   executeChild("/etc/alternatives/x-www-browser", page.toAscii());
}

// show about
void MConfig::on_buttonAbout_clicked() {
  QMessageBox::about(0, tr("About"),
    tr("<p><b>MEPIS XConfig</b></p>"
      "<p>Copyright (C) 2003-10 by MEPIS LLC.  All rights reserved.</p>"));
}

