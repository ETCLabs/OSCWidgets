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

#include "MainWindow.h"
#include "SettingsPanel.h"
#include "LogWidget.h"
#include "Utils.h"
#include "EosPlatform.h"
#include <time.h>

////////////////////////////////////////////////////////////////////////////////

#define APP_VERSION "1.0.0"

#define MIN_OPACITY 10

#ifdef WIN32
#define SYSTEM_MENU_BAR false
#define EXIT_OPTION true
#else
#define SYSTEM_MENU_BAR true
#define EXIT_OPTION false
#endif

////////////////////////////////////////////////////////////////////////////////

Logo::Logo(const QString &path, QWidget *parent)
  : QWidget(parent)
  , m_Original(path)
{
}

////////////////////////////////////////////////////////////////////////////////

void Logo::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
  if (!m_Original.isNull())
  {
    m_Scaled = QPixmap::fromImage(m_Original).scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if (!m_Scaled.isNull())
      update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void Logo::paintEvent(QPaintEvent * /*event*/)
{
  if (!m_Scaled.isNull())
  {
    QPainter painter(this);
    painter.drawPixmap(0, 0, m_Scaled);
  }
}

////////////////////////////////////////////////////////////////////////////////

EosTreeWidget::EosTreeWidget(QWidget *parent)
  : QTreeWidget(parent)
{
  m_Logo = new Logo(":/assets/images/ETCLogo.png", this);
  m_Logo->lower();
}

////////////////////////////////////////////////////////////////////////////////

void EosTreeWidget::resizeEvent(QResizeEvent *event)
{
  QTreeWidget::resizeEvent(event);
  int logoSize = qRound(0.8 * qMin(width(), height()));
  int scrollBarHeight = 0;
  QScrollBar *scrollBar = horizontalScrollBar();
  if (scrollBar && scrollBar->isVisible())
    scrollBarHeight = scrollBar->height();
  m_Logo->setGeometry(QRect(qRound((width() - logoSize) * 0.5), height() - scrollBarHeight - logoSize, logoSize, logoSize));
}

////////////////////////////////////////////////////////////////////////////////

OpacityAction::OpacityAction(int opacity, QObject *parent)
  : QAction(parent)
  , m_Opacity(opacity)
{
  setText(QString("%1%").arg(m_Opacity));
  setCheckable(true);
  connect(this, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)));
}

////////////////////////////////////////////////////////////////////////////////

void OpacityAction::onToggled(bool /*checked*/)
{
  emit triggeredWithOpacity(m_Opacity);
}

////////////////////////////////////////////////////////////////////////////////

OpacityMenu::OpacityMenu(QWidget *parent /* =0 */)
  : QMenu(parent)
  , m_IgnoreChanges(0)
{
  setTitle(tr("Opacity"));
  for (int i = MIN_OPACITY; i <= 100; i += 10)
  {
    OpacityAction *opacityAction = new OpacityAction(i, this);
    connect(opacityAction, SIGNAL(triggeredWithOpacity(int)), this, SLOT(onTriggeredWithOpacity(int)));
    m_OpacityActions.push_back(opacityAction);
    addAction(opacityAction);
  }
}

////////////////////////////////////////////////////////////////////////////////

void OpacityMenu::SetOpacity(int opacity)
{
  if (opacity < MIN_OPACITY)
    opacity = MIN_OPACITY;
  else if (opacity > 100)
    opacity = 100;

  OpacityAction *closest = 0;
  int minDelta = 0;
  for (OPACITY_ACTIONS::const_iterator i = m_OpacityActions.begin(); i != m_OpacityActions.end(); i++)
  {
    OpacityAction *action = *i;

    int delta = abs(action->GetOpacity() - opacity);
    if (i == m_OpacityActions.begin() || (delta < minDelta))
    {
      minDelta = delta;
      closest = action;
      if (minDelta == 0)
        break;  // best posssible match
    }
  }

  for (OPACITY_ACTIONS::const_iterator i = m_OpacityActions.begin(); i != m_OpacityActions.end(); i++)
  {
    OpacityAction *action = *i;
    action->setChecked(action == closest);
  }
}

////////////////////////////////////////////////////////////////////////////////

void OpacityMenu::onTriggeredWithOpacity(int opacity)
{
  if (m_IgnoreChanges == 0)
  {
    m_IgnoreChanges++;
    SetOpacity(opacity);
    m_IgnoreChanges--;

    emit opacityChanged(opacity);
  }
}

////////////////////////////////////////////////////////////////////////////////

MainWindow::MainWindow(EosPlatform *platform, QWidget *parent /*=0*/, Qt::WindowFlags f /*=Qt::WindowFlags()*/)
  : QWidget(parent, f)
  , m_Settings("ETC", "OSCWidgets")
  , m_LogDepth(200)
  , m_Unsaved(false)
  , m_UdpOutThread(0)
  , m_UdpInThread(0)
  , m_TcpClientThread(0)
  , m_ToyTreeToyIndex(0)
  , m_ToyTreeType(Toy::TOY_INVALID)
  , m_pPlatform(platform)
  , m_SystemIdleAllowed(true)
{
  Utils::BlockFakeMouseEvents(true);

  Toy::RestoreDefaultSettings();
  Toy::SetDefaultWindowIcon(*this);

  m_SystemTray = new QSystemTrayIcon(QIcon(":/assets/images/SystemTrayIcon.png"), this);
  m_SystemTrayMenu = new QMenu(0);
  m_SystemTrayMenu->addAction(QIcon(":/assets/images/MenuIconHome.png"), tr("Toggle Main Window"), this, SLOT(onSystemTrayToggledMainWindow()));
  m_SystemTrayMenu->addAction(QIcon(":/assets/images/MenuIconVisibility.png"), tr("Toggle Toys"), this, SLOT(onSystemTrayToggleToys()));
  m_SystemTrayMenu->addAction(QIcon(":/assets/images/MenuIconExit.png"), EXIT_OPTION ? tr("Exit") : tr("Quit"), this, SLOT(onSystemTrayExit()));
  connect(m_SystemTray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(onSystemTrayActivated(QSystemTrayIcon::ActivationReason)));
  m_SystemTray->setToolTip(QString("OSCWidgets\nv%1").arg(APP_VERSION));
  m_SystemTray->show();

  m_LogDepth = m_Settings.value(SETTING_LOG_DEPTH, m_LogDepth).toInt();
  if (m_LogDepth < 1)
    m_LogDepth = 1;
  m_Settings.setValue(SETTING_LOG_DEPTH, m_LogDepth);

  LoadAdvancedSettings();
  SaveAdvancedSettings();

  m_LogFile.Initialize(QDir(QDir::tempPath()).absoluteFilePath("OSCWidgets.txt"), m_Settings.value(SETTING_FILE_DEPTH, 10000).toInt());

  QGridLayout *layout = new QGridLayout(this);

  int row = 0;
  QMenuBar *menuBar = InitMenuBar(SYSTEM_MENU_BAR);
  if (menuBar)
    layout->setMenuBar(menuBar);

  QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
  layout->addWidget(splitter, row, 0);

  QSplitter *leftSplitter = new QSplitter(Qt::Vertical, splitter);
  splitter->addWidget(leftSplitter);

  QScrollArea *scrollArea = new QScrollArea(leftSplitter);
  scrollArea->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  leftSplitter->addWidget(scrollArea);

  m_SettingsPanel = new SettingsPanel(0);
  connect(m_SettingsPanel, SIGNAL(changed()), this, SLOT(onSettingsChanged()));
  connect(m_SettingsPanel, SIGNAL(addToy(int)), this, SLOT(onSettingsAddToy(int)));
  scrollArea->setWidget(m_SettingsPanel);

  m_Advanced = new AdvancedPanel(this);
  connect(m_Advanced, SIGNAL(changed()), this, SLOT(onAdvancedChanged()));
  m_Advanced->hide();

  m_ToyTree = new EosTreeWidget(this);
  m_ToyTree->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
  QPalette treePal(m_ToyTree->palette());
  QColor treeBase(palette().color(QPalette::Window));
  treeBase.setAlpha(220);
  treePal.setColor(QPalette::Base, treeBase);
  QColor treeAltBase(palette().color(QPalette::AlternateBase));
  treeAltBase.setAlpha(70);
  treePal.setColor(QPalette::AlternateBase, treeAltBase);
  m_ToyTree->setPalette(treePal);
  m_ToyTree->setColumnCount(TOY_TREE_COL_COUNT);
  m_ToyTree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_ToyTree, SIGNAL(itemActivated(QTreeWidgetItem *, int)), this, SLOT(onToyTreeItemActivated(QTreeWidgetItem *, int)));
  connect(m_ToyTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onToyTreeCustomContextMenuRequested(const QPoint &)));
  splitter->addWidget(m_ToyTree);

  QWidget *logBase = new QWidget(leftSplitter);
  QGridLayout *logLayout = new QGridLayout(logBase);
  leftSplitter->addWidget(logBase);

  m_LogWidget = new LogWidget(m_LogDepth, logBase);
  logLayout->addWidget(m_LogWidget, 0, 0);

  m_Log.AddInfo(QString("OSCWidgets v%1").arg(APP_VERSION).toUtf8().constData());
  m_Log.AddDebug("Icons designed by Freepik: http://www.flaticon.com/packs/ios7-set-lined-1");

  m_Toys = new Toys(this, this);
  connect(m_Toys, SIGNAL(changed()), this, SLOT(onToysChanged()));
  connect(m_Toys, SIGNAL(toggleMainWindow()), this, SLOT(onToysToggledMainWindow()));
  if (m_MenuActionFrames)
    m_MenuActionFrames->setChecked(m_Toys->GetFramesEnabled());
  if (m_MenuActionAlwaysOnTop)
    m_MenuActionAlwaysOnTop->setChecked(m_Toys->GetTopMost());
  if (m_OpacityMenu)
    m_OpacityMenu->SetOpacity(m_Toys->GetOpacity());

  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(onTick()));
  timer->start(100);

  PopulateToyTree();
  RestoreLastFile();
  UpdateWindowTitle();

  // resize(800, 380);
}

////////////////////////////////////////////////////////////////////////////////

MainWindow::~MainWindow()
{
  Shutdown();

  if (m_Toys)
  {
    delete m_Toys;
    m_Toys = 0;
  }

  m_LogFile.Shutdown();

  m_SystemTray->setContextMenu(0);

  if (m_SystemTrayMenu)
  {
    delete m_SystemTrayMenu;
    m_SystemTrayMenu = 0;
  }

  Utils::BlockFakeMouseEvents(false);
}

////////////////////////////////////////////////////////////////////////////////

QMenuBar *MainWindow::InitMenuBar(bool systemMenuBar)
{
  QMenuBar *menuBar = new QMenuBar(systemMenuBar ? 0 : this);

  QMenu *fileMenu = menuBar->addMenu(tr("&File"));
  fileMenu->addAction(QIcon(":/assets/images/MenuIconNew.png"), tr("&New"), this, SLOT(onNewFileClicked()));
  fileMenu->addAction(QIcon(":/assets/images/MenuIconOpen.png"), tr("&Open..."), this, SLOT(onOpenFileClicked()));
  fileMenu->addAction(QIcon(":/assets/images/MenuIconSave.png"), tr("&Save"), this, SLOT(onSaveFileClicked()));
  fileMenu->addAction(tr("Save &As..."), this, SLOT(onSaveAsFileClicked()));
  fileMenu->addSeparator();
  fileMenu->addAction(QIcon(":/assets/images/MenuIconSettings.png"), tr("Ad&vanced..."), this, SLOT(onAdvancedClicked()));
  fileMenu->addSeparator();
  fileMenu->addAction(QIcon(":/assets/images/MenuIconExit.png"), EXIT_OPTION ? tr("E&xit") : tr("Close"), this, SLOT(onSystemTrayExit()));

  QMenu *windowsMenu = menuBar->addMenu(tr("&Windows"));
  m_MenuActionFrames = windowsMenu->addAction(tr("Frames"));
  if (m_MenuActionFrames)
  {
    m_MenuActionFrames->setCheckable(true);
    connect(m_MenuActionFrames, SIGNAL(toggled(bool)), this, SLOT(onMenuFramesEnabled(bool)));
  }
  m_MenuActionAlwaysOnTop = windowsMenu->addAction(tr("Always on Top"));
  if (m_MenuActionAlwaysOnTop)
  {
    m_MenuActionAlwaysOnTop->setCheckable(true);
    connect(m_MenuActionAlwaysOnTop, SIGNAL(toggled(bool)), this, SLOT(onMenuAlwaysOnTop(bool)));
  }
  windowsMenu->addAction(QIcon(":/assets/images/MenuIconScreen.png"), tr("Snap To Screen"), this, SLOT(onMenuSnapToEdges()));
  m_OpacityMenu = new OpacityMenu();
  m_OpacityMenu->setIcon(QIcon(":/assets/images/MenuIconView.png"));
  connect(m_OpacityMenu, SIGNAL(opacityChanged(int)), this, SLOT(onMenuOpacity(int)));
  windowsMenu->addMenu(m_OpacityMenu);

  QMenu *oscMenu = menuBar->addMenu("&OSC");
  oscMenu->addAction(QIcon(":/assets/images/MenuIconRefresh.png"), tr("&Clear OSC Labels"), this, SLOT(onMenuClearLabels()));

  QMenu *logMenu = menuBar->addMenu("&Log");
  logMenu->addAction(QIcon(":/assets/images/MenuIconRefresh.png"), tr("&Clear"), this, SLOT(onClearLogClicked()));
  logMenu->addAction(QIcon(":/assets/images/MenuIconLog.png"), tr("&View"), this, SLOT(onOpenLogClicked()));

  return (systemMenuBar ? 0 : menuBar);
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::FlushLogQ(EosLog::LOG_Q &logQ)
{
  if (!logQ.empty())
  {
    // add timestamp text
    for (EosLog::LOG_Q::iterator i = logQ.begin(); i != logQ.end(); i++)
    {
      EosLog::sLogMsg &logMsg = *i;

      tm *t = localtime(&logMsg.timestamp);

      QString msgText;
      if (logMsg.text.c_str())
        msgText = QString::fromUtf8(logMsg.text.c_str());

      QString itemText = QString("[%1:%2:%3] %4").arg(t->tm_hour, 2).arg(t->tm_min, 2, 10, QChar('0')).arg(t->tm_sec, 2, 10, QChar('0')).arg(msgText);

      logMsg.text = itemText.toUtf8().constData();
    }

    // add to widget
    m_LogWidget->Log(logQ);

    // add to file
    m_LogFile.Log(logQ);
  }
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::Shutdown()
{
  if (m_TcpClientThread)
  {
    m_TcpClientThread->Stop();
    ClearRecvQ();
    ClearNetEventQ();
    m_TcpClientThread->Flush(m_TempLogQ, m_RecvQ, m_NetEventQ);
    m_Log.AddQ(m_TempLogQ);
    delete m_TcpClientThread;
    m_TcpClientThread = 0;
  }

  if (m_UdpInThread)
  {
    m_UdpInThread->Stop();
    ClearRecvQ();
    m_UdpInThread->Flush(m_TempLogQ, m_RecvQ);
    m_Log.AddQ(m_TempLogQ);

    delete m_UdpInThread;
    m_UdpInThread = 0;
  }

  if (m_UdpOutThread)
  {
    m_UdpOutThread->Stop();
    ClearRecvQ();
    ClearNetEventQ();
    m_UdpOutThread->Flush(m_TempLogQ, m_NetEventQ);
    m_Log.AddQ(m_TempLogQ);
    delete m_UdpOutThread;
    m_UdpOutThread = 0;
  }

  ClearRecvQ();
  ClearNetEventQ();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::ClearRecvQ()
{
  for (PACKET_Q::const_iterator i = m_RecvQ.begin(); i != m_RecvQ.end(); i++)
    delete[] i->data;
  m_RecvQ.clear();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::ClearNetEventQ()
{
  m_NetEventQ.clear();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::GetPersistentSavePath(QString &path) const
{
  path = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).absoluteFilePath("save.oscwidgets.txt");
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::UpdateWindowTitle()
{
  QString title(tr("OSCWidgets"));
  if (!m_FilePath.isEmpty())
  {
    title.append(" :: ");
    if (m_Unsaved)
      title.append("*");
    title.append(QDir::toNativeSeparators(m_FilePath));
  }
  else if (m_Unsaved)
    title.append("*");
  setWindowTitle(title);
}

////////////////////////////////////////////////////////////////////////////////

bool MainWindow::LoadFile(const QString &path, bool setLastFile)
{
  QFile f(path);
  if (f.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QStringList lines;
    {
      QTextStream textStream(&f);
      QString contents(textStream.readAll());
      contents.remove('\r');
      lines = contents.split('\n', Qt::KeepEmptyParts);
    }

    f.close();

    m_FilePath = path;
    m_Log.AddInfo(tr("Loaded \"%1\"").arg(QDir::toNativeSeparators(m_FilePath)).toUtf8().constData());

    int lineIndex = 0;
    LoadSettings(lines, lineIndex);
    m_Toys->Load(m_Log, path, lines, lineIndex);
    Start();

    if (setLastFile)
    {
      m_Settings.setValue(SETTING_LAST_FILE, m_FilePath);
      m_Unsaved = false;
    }

    UpdateWindowTitle();
    PopulateToyTree();

    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MainWindow::SaveFile(const QString &path, bool setLastFile)
{
  bool success = true;

  QDir().mkpath(QFileInfo(path).absolutePath());

  QFile f(path);
  if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
  {
    QStringList lines;
    SaveSettings(lines);
    m_Toys->Save(m_Log, path, lines);

    QTextStream textStream(&f);
    textStream.setEncoding(QStringConverter::Utf8);

    for (QStringList::iterator i = lines.begin(); i != lines.end(); i++)
      textStream << *i << "\n";

    textStream.flush();
    f.close();

    m_FilePath = path;

    if (setLastFile)
    {
      m_Unsaved = false;
      m_Settings.setValue(SETTING_LAST_FILE, m_FilePath);
    }

    UpdateWindowTitle();

    m_Log.AddInfo(tr("Saved \"%1\"").arg(QDir::toNativeSeparators(m_FilePath)).toUtf8().constData());
  }
  else
    success = false;

  if (!success)
  {
    QMessageBox mb(QMessageBox::NoIcon, tr("OSCWidgets"), tr("Unable to save file \"%1\"\n\n%2").arg(path).arg(f.errorString()), QMessageBox::Ok, this);
    mb.setIconPixmap(QPixmap(":/assets/images/IconWarning.png"));
    mb.exec();
  }

  return success;
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::RestoreLastFile()
{
  QString path = m_Settings.value(SETTING_LAST_FILE).toString();
  if (!path.isEmpty())
  {
    if (LoadFile(path, /*setLastFile*/ true))
      return;  // success
  }

  // fall-back to loading persistent file
  GetPersistentSavePath(path);

  if (LoadFile(path, /*setLastFile*/ false))
  {
    m_Unsaved = true;
    m_Settings.setValue(SETTING_LAST_FILE, QString());
  }
  else
    Start();  // start with default settings
}

////////////////////////////////////////////////////////////////////////////////

bool MainWindow::SaveSettings(QStringList &lines)
{
  QString ip;
  m_SettingsPanel->GetIP(ip);

  QString line;

  line.append(QString("%1").arg(static_cast<int>(m_SettingsPanel->GetMode())));
  line.append(QString(", %1").arg(Utils::QuotedString(ip)));
  line.append(QString(", %1").arg(m_SettingsPanel->GetPort1()));
  line.append(QString(", %1").arg(m_SettingsPanel->GetPort2()));
  line.append(QString(", %1").arg(static_cast<int>(m_Toys->GetFramesEnabled() ? 1 : 0)));
  line.append(QString(", %1").arg(static_cast<int>(m_Toys->GetTopMost() ? 1 : 0)));
  line.append(QString(", %1").arg(m_Toys->GetOpacity()));

  lines << line;

  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MainWindow::LoadSettings(QStringList &lines, int &index)
{
  if (index >= 0 && index < lines.size())
  {
    QStringList items;
    Utils::GetItemsFromQuotedString(lines[index++], items);

    if (items.size() > 0)
    {
      int n = items[0].toInt();
      if (n < 0 || n >= OSCStream::FRAME_MODE_COUNT)
        n = OSCStream::FRAME_MODE_INVALID;
      m_SettingsPanel->SetMode(static_cast<OSCStream::EnumFrameMode>(n));
    }

    if (items.size() > 1)
      m_SettingsPanel->SetIP(items[1]);

    if (items.size() > 2)
      m_SettingsPanel->SetPort1(items[2].toUShort());

    if (items.size() > 3)
      m_SettingsPanel->SetPort2(items[3].toUShort());

    if (m_MenuActionFrames && items.size() > 4)
      m_MenuActionFrames->setChecked(items[4].toInt() != 0);

    if (m_MenuActionAlwaysOnTop && items.size() > 5)
      m_MenuActionAlwaysOnTop->setChecked(items[5].toInt() != 0);

    if (items.size() > 6)
    {
      int n = items[6].toInt();
      if (n < MIN_OPACITY)
        n = MIN_OPACITY;
      else if (n > 100)
        n = 100;
      m_OpacityMenu->SetOpacity(n);
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::Start()
{
  Shutdown();

  OSCStream::EnumFrameMode mode = m_SettingsPanel->GetMode();

  QString ip;
  m_SettingsPanel->GetIP(ip);

  switch (mode)
  {
    case OSCStream::FRAME_MODE_1_0:
    case OSCStream::FRAME_MODE_1_1:
    {
      m_TcpClientThread = new EosTcpClientThread();
      m_TcpClientThread->Start(ip, m_SettingsPanel->GetTcpPort(), mode);
    }
    break;

    default:
    {
      m_UdpOutThread = new EosUdpOutThread();
      m_UdpOutThread->Start(ip, m_SettingsPanel->GetUdpOutputPort());

      m_UdpInThread = new EosUdpInThread();
      m_UdpInThread->Start(QString("0.0.0.0"), m_SettingsPanel->GetUdpInputPort());
    }
    break;
  }
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::ProcessRecvQ()
{
  for (PACKET_Q::const_iterator i = m_RecvQ.begin(); i != m_RecvQ.end(); i++)
  {
    m_Toys->Recv(i->data, i->size);
    delete[] i->data;
  }

  m_RecvQ.clear();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::ProcessNetEventQ()
{
  for (NETEVENT_Q::const_iterator i = m_NetEventQ.begin(); i != m_NetEventQ.end(); i++)
  {
    switch (*i)
    {
      case NET_EVENT_CONNECTED: m_Toys->Connected(); break;

      case NET_EVENT_DISCONNECTED: m_Toys->Disconnected(); break;
    }
  }

  m_NetEventQ.clear();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::MakeToyIcon(const Toy &toy, const QSize &iconSize, QIcon &icon) const
{
  QImage canvas(iconSize, QImage::Format_ARGB32);
  canvas.fill(0);

  QPixmap toyIcon(toy.GetImagePath());
  if (!toyIcon.isNull())
    toyIcon = toyIcon.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  QPainter painter;
  if (painter.begin(&canvas))
  {
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    if (toyIcon.isNull())
    {
      QRect r(1, 1, iconSize.width() - 2, iconSize.height() - 2);
      painter.setPen(Qt::NoPen);
      painter.setBrush(toy.GetColor());
      painter.drawRoundedRect(r, 4, 4);
    }
    else
      painter.drawPixmap(0, 0, toyIcon);

    painter.end();
  }

  icon = QIcon(QPixmap::fromImage(canvas));
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::PopulateToyTree()
{
  m_ToyTree->clear();

  const Toys::TOY_LIST &toys = m_Toys->GetList();
  m_ToyTree->setHeaderLabel(tr("Widgets // %1").arg(toys.size()));
  m_ToyTree->setAlternatingRowColors(true);

  bool hasToys = false;

  QIcon icon;
  QString str;
  for (unsigned int i = 0; i < Toy::TOY_COUNT; i++)
  {
    Toy::EnumToyType toyType = static_cast<Toy::EnumToyType>(i);
    Toy::GetName(toyType, str);

    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setData(TOY_TREE_COL_ITEM, TOY_TREE_ROLE_TOY_TYPE, i);

    for (Toys::TOY_LIST::const_iterator j = toys.begin(); j != toys.end(); j++)
    {
      const Toy *toy = *j;
      if (toy->GetType() == toyType)
      {
        QTreeWidgetItem *child = new QTreeWidgetItem();
        MakeToyIcon(*toy, QSize(16, 16), icon);
        child->setIcon(TOY_TREE_COL_ITEM, icon);
        child->setText(TOY_TREE_COL_ITEM, toy->GetText());
        unsigned int toyIndex = static_cast<unsigned int>(j - toys.begin());
        child->setData(TOY_TREE_COL_ITEM, TOY_TREE_ROLE_TOY_INDEX, toyIndex);
        item->addChild(child);
        hasToys = true;
      }
    }

    str.append(QString(" // %1").arg(item->childCount()));
    item->setText(0, str);
    m_ToyTree->addTopLevelItem(item);
  }

  SetSystemIdleAllowed(!hasToys);
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::LoadAdvancedSettings()
{
  float degrees = (Toy::GetEncoderRadiansPerTick() * (180 / M_PI));
  float radians = (m_Settings.value(SETTING_ENCODER_DEGREES_PER_TICK, degrees).toFloat() * (M_PI / 180));
  Toy::SetEncoderRadiansPerTick(radians);
  Toy::SetFeedbackDelayMS(m_Settings.value(SETTING_FEEDBACK_DELAY, Toy::GetFeedbackDelayMS()).toUInt());
  Toy::SetCmdSendAllDelayMS(m_Settings.value(SETTING_CMD_SEND_ALL_DELAY, Toy::GetCmdSendAllDelayMS()).toUInt());
  Toy::SetMetroRefreshRateMS(m_Settings.value(SETTING_METRO_REFRESH_RATE, Toy::GetMetroRefreshRateMS()).toUInt());
  Toy::SetSineRefreshRateMS(m_Settings.value(SETTING_SINE_REFRESH_RATE, Toy::GetSineRefreshRateMS()).toUInt());
  Toy::SetPedalRefreshRateMS(m_Settings.value(SETTING_PEDAL_REFRESH_RATE, Toy::GetPedalRefreshRateMS()).toUInt());
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::SaveAdvancedSettings()
{
  float degrees = (Toy::GetEncoderRadiansPerTick() * (180 / M_PI));
  m_Settings.setValue(SETTING_ENCODER_DEGREES_PER_TICK, degrees);
  m_Settings.setValue(SETTING_FEEDBACK_DELAY, Toy::GetFeedbackDelayMS());
  m_Settings.setValue(SETTING_CMD_SEND_ALL_DELAY, Toy::GetCmdSendAllDelayMS());
  m_Settings.setValue(SETTING_METRO_REFRESH_RATE, Toy::GetMetroRefreshRateMS());
  m_Settings.setValue(SETTING_SINE_REFRESH_RATE, Toy::GetSineRefreshRateMS());
  m_Settings.setValue(SETTING_PEDAL_REFRESH_RATE, Toy::GetPedalRefreshRateMS());
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::PromptForUnsavedChanges(bool &abortPendingOperation)
{
  abortPendingOperation = false;

  if (m_Unsaved)
  {
    QMessageBox mb(QMessageBox::NoIcon, tr("OSCWidgets"), tr("Do you want to save changes?"), QMessageBox::NoButton, this);
    mb.setIconPixmap(QPixmap(":/assets/images/IconQuestion.png"));
    QPushButton *saveButton = mb.addButton(tr("Save"), QMessageBox::AcceptRole);
    mb.addButton(tr("Don't Save"), QMessageBox::DestructiveRole);
    QPushButton *cancelButton = mb.addButton(tr("Cancel"), QMessageBox::RejectRole);

    mb.exec();

    if (mb.clickedButton() == saveButton)
    {
      onSaveFileClicked();
      if (m_Unsaved)
      {
        // error saving
        abortPendingOperation = true;
      }
    }
    else if (mb.clickedButton() == cancelButton)
    {
      abortPendingOperation = true;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onTick()
{
  if (m_UdpOutThread)
  {
    ClearRecvQ();
    ClearNetEventQ();
    m_UdpOutThread->Flush(m_TempLogQ, m_NetEventQ);
    m_Log.AddQ(m_TempLogQ);
    ProcessNetEventQ();
    ProcessRecvQ();
  }

  if (m_UdpInThread)
  {
    ClearRecvQ();
    m_UdpInThread->Flush(m_TempLogQ, m_RecvQ);
    m_Log.AddQ(m_TempLogQ);
    ProcessRecvQ();
  }

  if (m_TcpClientThread)
  {
    ClearRecvQ();
    ClearNetEventQ();
    m_TcpClientThread->Flush(m_TempLogQ, m_RecvQ, m_NetEventQ);
    m_Log.AddQ(m_TempLogQ);
    ProcessNetEventQ();
    ProcessRecvQ();
  }

  m_Log.Flush(m_TempLogQ);
  FlushLogQ(m_TempLogQ);
  m_TempLogQ.clear();

  ClearRecvQ();
  ClearNetEventQ();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onNewFileClicked()
{
  bool abortPendingOperation = false;
  PromptForUnsavedChanges(abortPendingOperation);
  if (!abortPendingOperation)
  {
    Shutdown();
    m_Toys->Clear();
    m_FilePath.clear();
    m_Settings.setValue(SETTING_LAST_FILE, m_FilePath);
    QString path;
    GetPersistentSavePath(path);
    QFile::setPermissions(path, QFile::WriteOwner);
    QFile::remove(path);
    m_Unsaved = false;
    UpdateWindowTitle();
    PopulateToyTree();
    Start();
  }
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onOpenFileClicked()
{
  bool abortPendingOperation = false;
  PromptForUnsavedChanges(abortPendingOperation);
  if (!abortPendingOperation)
  {
    QString dir;
    QString lastFile = m_Settings.value(SETTING_LAST_FILE).toString();
    if (!lastFile.isEmpty())
      dir = QFileInfo(lastFile).absolutePath();
    if (dir.isEmpty() || !QFileInfo(dir).exists())
      dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString path = QFileDialog::getOpenFileName(this, tr("Open"), dir, tr("OSCWidgets File (*.oscwidgets.txt)\nAll Files (*)"), 0, QFileDialog::DontUseNativeDialog);
    if (!path.isEmpty())
    {
      if (!LoadFile(path, /*setLastFile*/ true))
      {
        QMessageBox mb(QMessageBox::NoIcon, tr("OSCWidgets"), tr("Unable to open file \"%1\"").arg(path), QMessageBox::Ok, this);
        mb.setIconPixmap(QPixmap(":/assets/images/IconWarning.png"));
        mb.exec();
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onSaveFileClicked()
{
  QString path = m_Settings.value(SETTING_LAST_FILE).toString();
  if (path.isEmpty())
    onSaveAsFileClicked();
  else
    SaveFile(path, /*setLastFile*/ true);
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onSaveAsFileClicked()
{
  QString dir;
  QString lastFile = m_Settings.value(SETTING_LAST_FILE).toString();
  if (!lastFile.isEmpty())
    dir = QFileInfo(lastFile).absolutePath();
  if (dir.isEmpty() || !QFileInfo(dir).exists())
    dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  QString path = QFileDialog::getSaveFileName(this, tr("Save As"), dir, tr("OSCWidgets File (*.oscwidgets.txt)"), 0, QFileDialog::DontUseNativeDialog);
  if (!path.isEmpty())
  {
    QFileInfo fi(path);
    if (fi.completeSuffix().isEmpty())
      path.append(".oscwidgets.txt");

    SaveFile(path, /*setLastFile*/ true);
  }
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onClearLogClicked()
{
  m_LogWidget->Clear();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onOpenLogClicked()
{
  const QString &path = m_LogFile.GetPath();

  if (QFileInfo(path).exists())
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::closeEvent(QCloseEvent *event)
{
  if (event->spontaneous())
  {
    // main window was closed only, just hide it instead
    hide();
    event->ignore();
    return;
  }
  
  if (m_Unsaved)
    setVisible(true);

  bool abortPendingOperation = false;
  PromptForUnsavedChanges(abortPendingOperation);
  if (abortPendingOperation)
  {
    event->ignore();
  }
  else
  {
    QString path;
    GetPersistentSavePath(path);
    SaveFile(path, /*setLastFile*/ false);
    QApplication::exit(0);
  }
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onSettingsChanged()
{
  if (!m_Unsaved)
  {
    m_Unsaved = true;
    UpdateWindowTitle();
  }

  Start();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onAdvancedClicked()
{
  m_Advanced->show();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onAdvancedChanged()
{
  SaveAdvancedSettings();
  m_Toys->RefreshAdvancedSettings();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onToysChanged()
{
  if (!m_Unsaved)
  {
    m_Unsaved = true;
    UpdateWindowTitle();
  }

  PopulateToyTree();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onToysToggledMainWindow()
{
  setVisible(!isVisible());
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onMenuFramesEnabled(bool b)
{
  m_Toys->SetFramesEnabled(b);
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onMenuAlwaysOnTop(bool b)
{
  m_Toys->SetTopMost(b);
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onMenuSnapToEdges()
{
  m_Toys->SnapToEdges();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onMenuOpacity(int opacity)
{
  m_Toys->SetOpacity(opacity);
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onMenuClearLabels()
{
  m_Toys->ClearLabels();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onSettingsAddToy(int type)
{
  m_Toys->AddToy(static_cast<Toy::EnumToyType>(type));
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onToyTreeItemActivated(QTreeWidgetItem *item, int column)
{
  if (item && column == TOY_TREE_COL_ITEM)
  {
    QVariant v(item->data(TOY_TREE_COL_ITEM, TOY_TREE_ROLE_TOY_INDEX));
    if (v.isNull())
    {
      v = item->data(TOY_TREE_COL_ITEM, TOY_TREE_ROLE_TOY_TYPE);
      if (!v.isNull())
        m_Toys->ActivateToys(static_cast<Toy::EnumToyType>(v.toUInt()));
    }
    else
      m_Toys->ActivateToy(static_cast<size_t>(v.toUInt()));
  }
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onToyTreeItemDeleted()
{
  const Toys::TOY_LIST &toys = m_Toys->GetList();
  if (m_ToyTreeToyIndex < toys.size())
  {
    const Toy *toy = toys[m_ToyTreeToyIndex];

    QString name;
    toy->GetName(name);

    QMessageBox mb(QMessageBox::NoIcon, tr("Delete"), tr("Are you sure you want to delete %1").arg(name), QMessageBox::Yes | QMessageBox::Cancel, this);
    mb.setIconPixmap(QPixmap(":/assets/images/IconQuestion.png"));
    if (mb.exec() == QMessageBox::Yes)
      m_Toys->DeleteToy(m_ToyTreeToyIndex);
  }
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onToyTreeItemAdded()
{
  if (m_ToyTreeType != Toy::TOY_INVALID)
    m_Toys->AddToy(m_ToyTreeType);
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onToyTreeCustomContextMenuRequested(const QPoint &p)
{
  QTreeWidgetItem *item = m_ToyTree->itemAt(p);
  if (item)
  {
    QVariant v(item->data(TOY_TREE_COL_ITEM, TOY_TREE_ROLE_TOY_INDEX));
    if (v.isNull())
    {
      v = item->data(TOY_TREE_COL_ITEM, TOY_TREE_ROLE_TOY_TYPE);
      if (!v.isNull())
      {
        unsigned int n = v.toUInt();
        if (n < Toy::TOY_COUNT)
        {
          m_ToyTreeType = static_cast<Toy::EnumToyType>(n);

          QString name;
          Toy::GetName(m_ToyTreeType, name);

          QMenu menu(this);
          menu.addAction(QIcon(":/assets/images/MenuIconAdd.png"), tr("Add %1").arg(name), this, SLOT(onToyTreeItemAdded()));
          QWidget *w = m_ToyTree->viewport();
          if (!w)
            w = m_ToyTree;
          menu.exec(w->mapToGlobal(p));
        }
      }
    }
    else
    {
      m_ToyTreeToyIndex = static_cast<size_t>(v.toUInt());
      const Toys::TOY_LIST &toys = m_Toys->GetList();
      if (m_ToyTreeToyIndex < toys.size())
      {
        const Toy *toy = toys[m_ToyTreeToyIndex];

        QString name;
        toy->GetName(name);

        QMenu menu(this);
        menu.addAction(QIcon(":/assets/images/MenuIconTrash.png"), tr("Delete %1...").arg(name), this, SLOT(onToyTreeItemDeleted()));
        QWidget *w = m_ToyTree->viewport();
        if (!w)
          w = m_ToyTree;
        menu.exec(w->mapToGlobal(p));
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onSystemTrayToggleToys()
{
  m_Toys->ActivateAllToys(!m_Toys->HasVisibleToys());
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onSystemTrayToggledMainWindow()
{
  setVisible(!isVisible());
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onSystemTrayExit()
{
  close();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onSystemTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
  switch (reason)
  {
    case QSystemTrayIcon::Context:
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::MiddleClick: m_SystemTrayMenu->popup(QCursor::pos()); break;

    case QSystemTrayIcon::DoubleClick: setVisible(true); break;
  }
}

////////////////////////////////////////////////////////////////////////////////

bool MainWindow::ToyClient_Send(bool local, char *buf, size_t size)
{
  if (buf && size != 0)
  {
    if (local)
    {
      m_Toys->Recv(buf, size);
      delete[] buf;
      return true;
    }
    else
    {
      sPacket packet;
      packet.data = buf;
      packet.size = size;

      if (m_UdpOutThread)
      {
        if (m_UdpOutThread->Send(packet))
          return true;
      }
      else if (m_TcpClientThread)
      {
        if (m_TcpClientThread->Send(packet))
          return true;
      }

      delete[] buf;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::ToyClient_ResourceRelativePathToAbsolute(QString &path)
{
  Toy::ResourceRelativePathToAbsolute(&m_Log, m_FilePath, path);
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::SetSystemIdleAllowed(bool b)
{
  if (m_SystemIdleAllowed != b)
  {
    m_SystemIdleAllowed = b;

    if (m_pPlatform)
    {
      if (m_SystemIdleAllowed)
      {
        std::string error;
        if (m_pPlatform->SetSystemIdleAllowed(true, "widgets stopped", error))
        {
          m_Log.AddInfo("widgets stopped, system idle allowed");
        }
        else
        {
          error.insert(0, "failed to allow system idle, ");
          m_Log.AddDebug(error);
        }
      }
      else
      {
        std::string error;
        if (m_pPlatform->SetSystemIdleAllowed(false, "widgets started", error))
        {
          m_Log.AddInfo("widgets started, system idle disabled");
        }
        else
        {
          error.insert(0, "failed to disable system idle, ");
          m_Log.AddDebug(error);
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
