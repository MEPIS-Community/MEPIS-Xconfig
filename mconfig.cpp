//
//   Copyright (C) 2003-2011 by Warren Woodford
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
  char line[130];
  int i;

  FILE *fp = popen("lspci | grep VGA", "r");
  if (fp == NULL) {
    return;
  }
  fgets(line, sizeof line, fp);
  i = strlen(line);
  line[i-1] = '\0';
  pclose(fp);
  QString vga = QString("%1").arg(line);
  gfxAdapterEdit->setText(vga);

  buttonApply->setEnabled(false);
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
  refresh();
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

void MConfig::on_tabWidget_currentChanged() {
  refresh();
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
      "<p>Copyright (C) 2003-11 by MEPIS LLC.  All rights reserved.</p>"));
}

