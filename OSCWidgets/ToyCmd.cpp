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

#include "ToyCmd.h"
#include "OSCParser.h"
#include "Utils.h"

////////////////////////////////////////////////////////////////////////////////

FadeCmd::FadeCmd(QWidget *parent)
  : QLineEdit(parent)
{
}

////////////////////////////////////////////////////////////////////////////////

void FadeCmd::AutoSizeFont()
{
  QFont fnt = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  fnt.setStyleHint(QFont::TypeWriter);
  fnt.setPixelSize(qMax(10, qRound(height() * 0.6)));
  setFont(fnt);
}

////////////////////////////////////////////////////////////////////////////////

void FadeCmd::resizeEvent(QResizeEvent *event)
{
  AutoSizeFont();
  QLineEdit::resizeEvent(event);
}

////////////////////////////////////////////////////////////////////////////////

ToyCmdWidget::ToyCmdWidget(QWidget *parent)
  : ToyWidget(parent)
{
  m_HelpText = tr("OSC Trigger:\nNo Arguments = Click\nArgument(1) = Press\nArgument(0) = Release");

  m_Widget = new QWidget(this);
  QGridLayout *layout = new QGridLayout(m_Widget);
  layout->setContentsMargins(2, 2, 2, 2);
  layout->setSpacing(4);

  m_Cmd = new FadeCmd(m_Widget);
  m_Cmd->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  connect(m_Cmd, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
  connect(m_Cmd, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));
  layout->addWidget(m_Cmd, 0, 1);

  m_Send = new FadeButton(m_Widget);
  m_Send->setToolTip(tr("Send Command"));
  m_Send->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
  connect(m_Send, SIGNAL(clicked(bool)), this, SLOT(onSendClicked(bool)));
  layout->addWidget(m_Send, 0, 2);

  QPalette pal(m_Send->palette());
  m_Color = pal.color(QPalette::Button);
  m_TextColor = pal.color(QPalette::ButtonText);

  SetColor(m_Color);
  SetTextColor(m_TextColor);
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdWidget::SetText(const QString &text)
{
  ToyWidget::SetText(text);
  m_Send->setText(m_Text);
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdWidget::SetImagePath(const QString &imagePath)
{
  ToyWidget::SetImagePath(imagePath);
  m_Send->SetImagePath(0, imagePath);
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdWidget::SetPath(const QString &path)
{
  ToyWidget::SetPath(path);
  m_Cmd->setText(m_Path);
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdWidget::SetColor(const QColor &color)
{
  ToyWidget::SetColor(color);

  QPalette pal = m_Cmd->palette();
  pal.setColor(QPalette::Base, m_Color);
  m_Cmd->setPalette(pal);

  pal = m_Send->palette();
  pal.setColor(QPalette::Button, m_Color);
  m_Send->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdWidget::SetTextColor(const QColor &textColor)
{
  ToyWidget::SetTextColor(textColor);

  QPalette pal = m_Cmd->palette();
  pal.setColor(QPalette::Text, m_TextColor);
  m_Cmd->setPalette(pal);

  pal = m_Send->palette();
  pal.setColor(QPalette::ButtonText, m_TextColor);
  m_Send->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdWidget::Recv(const QString &path, const OSCArgument *args, size_t count)
{
  if (path == m_TriggerPath)
  {
    bool edge = false;

    if (args && count > 0)
    {
      bool press = false;
      if (args[0].GetBool(press))
      {
        edge = true;

        if (press)
          m_Send->Press();
        else
          m_Send->Release();
      }
    }

    if (!edge)
    {
      m_Send->Press();
      m_Send->Release();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdWidget::UpdateToolTip()
{
  // no tool tips, info would be redundant for this widget
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdWidget::onEditingFinished()
{
  SetPath(m_Cmd->text());
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdWidget::onReturnPressed()
{
  SetPath(m_Cmd->text());
  emit send(this);
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdWidget::onSendClicked(bool /*checked*/)
{
  emit send(this);
}

////////////////////////////////////////////////////////////////////////////////

ToyCmdGrid::ToyCmdGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags)
  : ToyGrid(TOY_CMD_GRID, pClient, parent, flags)
  , m_SendAllIndex(0)
  , m_IgnoreUpdates(0)
{
  m_SendAllTimer = new QTimer(this);
  connect(m_SendAllTimer, SIGNAL(timeout()), this, SLOT(onSendAllTimeout()));

  m_SendAll = new FadeButton(this);
  m_SendAll->setToolTip(tr("Send All Commands"));
  connect(m_SendAll, SIGNAL(clicked(bool)), this, SLOT(onSendAllClicked(bool)));

  m_Startup = new QCheckBox(tr("Send On Connect"), this);
  m_Startup->resize(m_Startup->sizeHint());
  connect(m_Startup, SIGNAL(stateChanged(int)), this, SLOT(onStartupStateChanged(int)));

  m_ButtonsHeight = qMax(m_SendAll->sizeHint().height(), m_Startup->height());

  UpdateSendAllButton();
}

////////////////////////////////////////////////////////////////////////////////

ToyWidget *ToyCmdGrid::CreateWidget()
{
  ToyCmdWidget *w = new ToyCmdWidget(this);
  connect(w, SIGNAL(send(ToyCmdWidget *)), this, SLOT(onSend(ToyCmdWidget *)));
  return w;
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdGrid::SetSendOnConnect(bool b)
{
  ToyGrid::SetSendOnConnect(b);

  m_IgnoreUpdates++;
  m_Startup->setChecked(b);
  m_IgnoreUpdates--;
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdGrid::Connected()
{
  if (m_SendOnConnect)
    SendAll();
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdGrid::SendAll()
{
  if (Toy::GetCmdSendAllDelayMS() == 0)
  {
    m_SendAllTimer->stop();
    for (WIDGET_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
      onSend(static_cast<ToyCmdWidget *>(*i));
  }
  else
  {
    m_SendAllIndex = 0;
    m_SendAllTimer->start(Toy::GetCmdSendAllDelayMS());
  }

  UpdateSendAllButton();
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdGrid::ApplyDefaultSettings(ToyWidget *widget, size_t index)
{
  QString path;
  Toy::GetDefaultPathName(m_Type, path);

  switch (index)
  {
    case 2:
      widget->SetText(tr("no args"));
      widget->SetPath(QString("/%1").arg(path));
      break;

    case 3:
      widget->SetText(tr("float arg"));
      widget->SetPath(QString("/%1=1.0").arg(path));
      break;

    case 4:
      widget->SetText(tr("integer arg"));
      widget->SetPath(QString("/%1=1").arg(path));
      break;

    case 5:
      widget->SetText(tr("string arg"));
      widget->SetPath(QString("/%1=hello world").arg(path));
      break;

    case 6:
      widget->SetText(tr("multiple args"));
      widget->SetPath(QString("/%1=1.0,hello world").arg(path));
      break;

    default:
      widget->SetText(tr("Send"));
      widget->SetPath(QString("/%1/%2").arg(path).arg(index));
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdGrid::UpdateLayout()
{
  QRect r(rect());

  int y = (r.bottom() - MARGIN - m_ButtonsHeight);

  if (m_List.size() > 1)
  {
    m_SendAll->setGeometry(MARGIN2, y, r.width() - MARGIN * 3 - m_Startup->width(), m_ButtonsHeight);
    m_SendAll->show();
    m_Startup->setGeometry(m_SendAll->x() + m_SendAll->width() + MARGIN, y, m_Startup->width(), m_ButtonsHeight);
  }
  else
  {
    m_SendAll->hide();
    m_Startup->setGeometry(r.right() - MARGIN - m_Startup->width(), y, m_Startup->width(), m_ButtonsHeight);
  }

  r.adjust(0, 0, 0, -m_ButtonsHeight - MARGIN);
  UpdateLayoutForRect(r);
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdGrid::AutoSize(const QSize &widgetSize)
{
  resize(m_GridSize.width() * widgetSize.width() + MARGIN2, m_GridSize.height() * widgetSize.height() + m_ButtonsHeight + MARGIN * 3);
  UpdateLayout();
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdGrid::UpdateSendAllButton()
{
  if (m_SendAllTimer->isActive())
  {
    m_SendAll->setText(tr("Sending %1 of %2...").arg(m_SendAllIndex + 1).arg(m_List.size()));
    m_SendAll->setEnabled(false);
  }
  else
  {
    m_SendAll->setText(tr("Send All"));
    m_SendAll->setEnabled(true);
  }
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdGrid::onSend(ToyCmdWidget *cmd)
{
  if (m_pClient && cmd && !cmd->GetPath().isEmpty())
  {
    QString path(cmd->GetPath());
    bool local = Utils::MakeLocalOSCPath(false, path);

    size_t size = 0;
    char *packet = OSCPacketWriter::CreateForString(path.toUtf8(), size);
    if (packet && size != 0)
      m_pClient->ToyClient_Send(local, packet, size);
  }
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdGrid::onSendAllClicked(bool /*checked*/)
{
  SendAll();
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdGrid::onSendAllTimeout()
{
  if (m_SendAllIndex < m_List.size())
  {
    ToyCmdWidget *w = static_cast<ToyCmdWidget *>(m_List[m_SendAllIndex]);
    onSend(w);
    m_SendAllIndex++;
  }

  if (m_SendAllIndex >= m_List.size())
    m_SendAllTimer->stop();

  UpdateSendAllButton();
}

////////////////////////////////////////////////////////////////////////////////

void ToyCmdGrid::onStartupStateChanged(int /*state*/)
{
  if (m_IgnoreUpdates == 0)
  {
    ToyGrid::SetSendOnConnect(m_Startup->isChecked());
    emit changed();
  }
}

////////////////////////////////////////////////////////////////////////////////
