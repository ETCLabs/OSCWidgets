// Copyright (c) 2018 Electronic Theatre Controls, Inc., http://www.etcconnect.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#ifndef QT_INCLUDE_H
#include "QtInclude.h"
#endif

#ifndef EOS_LOG_H
#include "EosLog.h"
#endif

#ifndef TOYS_H
#include "Toys.h"
#endif

#ifndef NETWORK_THREADS_H
#include "NetworkThreads.h"
#endif

#ifndef LOG_FILE_H
#include "LogFile.h"
#endif

class LogWidget;
class EosPlatform;
class SettingsPanel;
class AdvancedPanel;

////////////////////////////////////////////////////////////////////////////////

class Logo : public QWidget
{
public:
  Logo(const QString &path, QWidget *parent);

protected:
  QImage m_Original;
  QPixmap m_Scaled;

  virtual void resizeEvent(QResizeEvent *event);
  virtual void paintEvent(QPaintEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class EosTreeWidget : public QTreeWidget
{
public:
  EosTreeWidget(QWidget *parent);

protected:
  Logo *m_Logo;

  virtual void resizeEvent(QResizeEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class OpacityAction : public QAction
{
  Q_OBJECT

public:
  OpacityAction(int opacity, QObject *parent);

  virtual int GetOpacity() const { return m_Opacity; }

signals:
  void triggeredWithOpacity(int opacity);

private slots:
  void onToggled(bool checked);

protected:
  int m_Opacity;
};

////////////////////////////////////////////////////////////////////////////////

class OpacityMenu : public QMenu
{
  Q_OBJECT

public:
  OpacityMenu(QWidget *parent = 0);

  virtual void SetOpacity(int opacity);

signals:
  void opacityChanged(int opacity);

private slots:
  void onTriggeredWithOpacity(int opacity);

protected:
  typedef std::vector<OpacityAction *> OPACITY_ACTIONS;

  unsigned int m_IgnoreChanges;
  OPACITY_ACTIONS m_OpacityActions;
};

////////////////////////////////////////////////////////////////////////////////

class MainWindow : public QWidget, private Toy::Client
{
  Q_OBJECT

public:
  MainWindow(EosPlatform *platform, QWidget *parent = 0, Qt::WindowFlags f = Qt::WindowFlags());
  virtual ~MainWindow();

  virtual void FlushLogQ(EosLog::LOG_Q &logQ);

protected:
  virtual void closeEvent(QCloseEvent *event);

private slots:
  void onTick();
  void onNewFileClicked();
  void onOpenFileClicked();
  void onSaveFileClicked();
  void onSaveAsFileClicked();
  void onClearLogClicked();
  void onOpenLogClicked();
  void onSettingsChanged();
  void onAdvancedClicked();
  void onAdvancedChanged();
  void onMenuFramesEnabled(bool b);
  void onMenuAlwaysOnTop(bool b);
  void onMenuSnapToEdges();
  void onMenuOpacity(int opacity);
  void onMenuClearLabels();
  void onSettingsAddToy(int type);
  void onToysChanged();
  void onToysToggledMainWindow();
  void onToyTreeItemActivated(QTreeWidgetItem *item, int column);
  void onToyTreeCustomContextMenuRequested(const QPoint &p);
  void onToyTreeItemDeleted();
  void onToyTreeItemAdded();
  void onSystemTrayToggledMainWindow();
  void onSystemTrayToggleToys();
  void onSystemTrayExit();
  void onSystemTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
  enum EnumConstants
  {
    TOY_TREE_COL_ITEM = 0,
    TOY_TREE_COL_COUNT,

    TOY_TREE_ROLE_TOY_INDEX = Qt::UserRole,
    TOY_TREE_ROLE_TOY_TYPE
  };

  EosLog m_Log;
  EosLog::LOG_Q m_TempLogQ;
  LogWidget *m_LogWidget;
  QSettings m_Settings;
  int m_LogDepth;
  LogFile m_LogFile;
  QString m_FilePath;
  bool m_Unsaved;
  QAction *m_MenuActionFrames;
  QAction *m_MenuActionAlwaysOnTop;
  OpacityMenu *m_OpacityMenu;
  SettingsPanel *m_SettingsPanel;
  AdvancedPanel *m_Advanced;
  EosUdpOutThread *m_UdpOutThread;
  EosUdpInThread *m_UdpInThread;
  EosTcpClientThread *m_TcpClientThread;
  PACKET_Q m_RecvQ;
  NETEVENT_Q m_NetEventQ;
  EosTreeWidget *m_ToyTree;
  Toys *m_Toys;
  size_t m_ToyTreeToyIndex;
  Toy::EnumToyType m_ToyTreeType;
  QSystemTrayIcon *m_SystemTray;
  QMenu *m_SystemTrayMenu;
  EosPlatform *m_pPlatform;
  bool m_SystemIdleAllowed;

  virtual void Start();
  virtual void Shutdown();
  virtual void GetPersistentSavePath(QString &path) const;
  virtual void UpdateWindowTitle();
  virtual void RestoreLastFile();
  virtual bool LoadFile(const QString &path, bool setLastFile);
  virtual bool SaveFile(const QString &path, bool setLastFile);
  virtual bool SaveSettings(QStringList &lines);
  virtual bool LoadSettings(QStringList &lines, int &index);
  virtual void ClearRecvQ();
  virtual void ProcessRecvQ();
  virtual void ClearNetEventQ();
  virtual void ProcessNetEventQ();
  virtual bool ToyClient_Send(bool local, char *data, size_t size);
  virtual void ToyClient_ResourceRelativePathToAbsolute(QString &path);
  virtual void PopulateToyTree();
  virtual void MakeToyIcon(const Toy &toy, const QSize &iconSize, QIcon &icon) const;
  virtual void LoadAdvancedSettings();
  virtual void SaveAdvancedSettings();
  virtual void PromptForUnsavedChanges(bool &abortPendingOperation);
  virtual QMenuBar *InitMenuBar(bool systemMenuBar);
  virtual void SetSystemIdleAllowed(bool b);
};

////////////////////////////////////////////////////////////////////////////////

#endif
