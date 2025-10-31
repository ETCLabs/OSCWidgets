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

#include "ToyFlicker.h"
#include "OSCParser.h"
#include "Utils.h"

////////////////////////////////////////////////////////////////////////////////

FadeFlicker::FadeFlicker(QWidget *parent)
  : FadeButton(parent)
  , m_TextMargin(0)
  , m_LabelMargin(0)
  , m_Value(0)
  , m_MinTimeScale(0)
  , m_MaxTimeScale(0)
  , m_BPM(600)
  , m_MsPerBeat(0)
  , m_Elapsed(0)
  , m_Paused(true)
{
  connect(this, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
  UpdateMsPerBeat();
}

////////////////////////////////////////////////////////////////////////////////

void FadeFlicker::SetText(const QString &text)
{
  if (this->text() != text)
  {
    setText(text);
    UpdateMargins();
    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeFlicker::SetLabel(const QString &label)
{
  if (m_Label != label)
  {
    m_Label = label;
    UpdateMargins();
    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeFlicker::SetTimeScaleRange(float minTimeScale, float maxTimeScale)
{
  if (m_MinTimeScale != minTimeScale || m_MaxTimeScale != maxTimeScale)
  {
    m_MinTimeScale = minTimeScale;
    m_MaxTimeScale = maxTimeScale;
    UpdateMsPerBeat();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeFlicker::AutoSizeFont()
{
  QFont fnt(font());
  int d = qMin(width(), qMax(m_TextMargin, m_LabelMargin));
  fnt.setPixelSize(qMax(10, qRound(d * 0.7)));
  setFont(fnt);
}

////////////////////////////////////////////////////////////////////////////////

void FadeFlicker::UpdateMargins()
{
  int textMargin = (text().isEmpty() ? 0 : qRound(height() * 0.1));
  int labelMargin = (m_Label.isEmpty() ? 0 : qRound(height() * 0.1));

  if (m_TextMargin != textMargin || m_LabelMargin != labelMargin)
  {
    m_TextMargin = textMargin;
    m_LabelMargin = labelMargin;
    UpdateFlickerRect();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeFlicker::UpdateFlickerRect()
{
  m_FlickerRect = rect();
  m_FlickerRect.adjust(0, m_TextMargin, 0, -m_LabelMargin);
  update();
}

////////////////////////////////////////////////////////////////////////////////

void FadeFlicker::SetBPM(float bpm)
{
  if (m_BPM != bpm)
  {
    m_BPM = bpm;
    UpdateMsPerBeat();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeFlicker::SetPaused(bool b)
{
  if (m_Paused != b)
  {
    m_Paused = b;
    m_Elapsed = 0;
    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeFlicker::Update(unsigned int ms)
{
  if (!m_Paused && ms != 0 && m_MsPerBeat != 0)
  {
    m_Elapsed += ms;

    while (m_MsPerBeat != 0 && m_Elapsed >= m_MsPerBeat)
    {
      m_Elapsed -= m_MsPerBeat;
      m_Value = (rand() / static_cast<float>(RAND_MAX));
      emit valueChanged(m_Value);
      if (HasTimeScale())
        UpdateMsPerBeat();
      update();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

bool FadeFlicker::HasTimeScale() const
{
  return (m_MinTimeScale > 0 && m_MaxTimeScale > 0);
}

////////////////////////////////////////////////////////////////////////////////

unsigned int FadeFlicker::GetMsPerBeat(float bpm, float timeScale)
{
  float scaledBMP = (bpm * timeScale);

  unsigned int msPerBeat = ((scaledBMP > 0) ? (60000.0f / scaledBMP) : 0);

  if (bpm > 0 && msPerBeat == 0)
    msPerBeat = 1;

  return msPerBeat;
}

////////////////////////////////////////////////////////////////////////////////

void FadeFlicker::UpdateMsPerBeat()
{
  float timeScale = 1.0f;

  if (HasTimeScale())
  {
    float t = (rand() / static_cast<float>(RAND_MAX));
    timeScale = (m_MinTimeScale * t + m_MaxTimeScale * (1.0f - t));
  }

  m_MsPerBeat = GetMsPerBeat(m_BPM, timeScale);
}

////////////////////////////////////////////////////////////////////////////////

void FadeFlicker::resizeEvent(QResizeEvent *event)
{
  FadeButton::resizeEvent(event);
  UpdateMargins();
  UpdateFlickerRect();
}

////////////////////////////////////////////////////////////////////////////////

void FadeFlicker::paintEvent(QPaintEvent * /*event*/)
{
  QPainter painter(this);

  painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

  QPainterPath clip;
  clip.addRoundedRect(m_FlickerRect, ROUNDED, ROUNDED);
  painter.setClipPath(clip);

  const QPixmap &pixmap = m_Images[m_ImageIndex].pixmap;
  if (!pixmap.isNull())
  {
    painter.drawPixmap(m_FlickerRect.x() + qRound((m_FlickerRect.width() - pixmap.width()) * 0.5), m_FlickerRect.y() + qRound((m_FlickerRect.height() - pixmap.height()) * 0.5), pixmap);
  }

  painter.setClipping(false);

  float brightness = m_Click;
  if (m_Hover > 0)
    brightness += (m_Hover * 0.2f);

  QColor color(palette().color(QPalette::Button));
  if (brightness > 0)
  {
    qreal t = (brightness * BUTTON_BRIGHTESS);
    color.setRedF(qMin(color.redF() + t, 1.0));
    color.setGreenF(qMin(color.greenF() + t, 1.0));
    color.setBlueF(qMin(color.blueF() + t, 1.0));
  }

  painter.setPen(QPen(color, BORDER, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin));
  painter.setBrush(Qt::NoBrush);
  painter.drawRoundedRect(m_FlickerRect, ROUNDED, ROUNDED);

  painter.setClipPath(clip);
  int h = qRound(m_FlickerRect.height() * m_Value);
  painter.fillRect(m_FlickerRect.x(), m_FlickerRect.bottom() - h, m_FlickerRect.width(), h, color);
  painter.setClipping(false);

  if (!text().isEmpty())
  {
    int hoverRaise = qRound(m_Hover * BUTTON_RAISE);

    painter.setFont(font());
    painter.setPen(palette().color(QPalette::ButtonText));
    painter.drawText(QRect(0, 0, width(), m_FlickerRect.y() - hoverRaise), Qt::AlignCenter | Qt::TextWordWrap | Qt::TextDontClip, text());

    if (!m_Label.isEmpty())
      painter.drawText(QRect(0, m_FlickerRect.bottom(), width(), height() - m_FlickerRect.bottom() + hoverRaise), Qt::AlignCenter | Qt::TextWordWrap | Qt::TextDontClip, m_Label);
  }
  else if (!m_Label.isEmpty())
  {
    int hoverRaise = qRound(m_Hover * BUTTON_RAISE);
    painter.setFont(font());
    painter.setPen(palette().color(QPalette::ButtonText));
    painter.drawText(QRect(0, m_FlickerRect.bottom(), width(), height() - m_FlickerRect.bottom() + hoverRaise), Qt::AlignCenter | Qt::TextWordWrap | Qt::TextDontClip, m_Label);
  }

  if (m_Paused)
  {
    painter.setOpacity(0.8);
    color = color.lighter(250);
    painter.setPen(color);
    painter.setBrush(BG_COLOR);
    int pauseSize = qRound(0.6 * qMin(m_FlickerRect.width(), m_FlickerRect.height()));
    QRect pauseRect(m_FlickerRect.x() + qRound((m_FlickerRect.width() - pauseSize) * 0.5), m_FlickerRect.y() + qRound((m_FlickerRect.height() - pauseSize) * 0.5), pauseSize, pauseSize);
    painter.drawRoundedRect(pauseRect, ROUNDED, ROUNDED);

    int pw = qRound(pauseSize * 0.35);
    int ph = qRound(pauseSize * 0.7);
    pauseRect.adjust(pw, ph, -pw, -ph);
    painter.setPen(QPen(color, pauseSize * 0.1, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(pauseRect.left(), pauseRect.top(), pauseRect.left(), pauseRect.bottom());
    painter.drawLine(pauseRect.right(), pauseRect.top(), pauseRect.right(), pauseRect.bottom());
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeFlicker::onClicked(bool /*checked*/)
{
  SetPaused(!m_Paused);
}

////////////////////////////////////////////////////////////////////////////////

ToyFlickerWidget::ToyFlickerWidget(QWidget *parent)
  : ToyWidget(parent)
{
  m_HelpText = tr("Flicker between Min and Max\n\nRandomize Timing:\nMin2=Min Timing Scale\nMax2=Max Timing Scale\n\nOSC Trigger:\nNo Arguments = Play\nArgument(0) = Pause\nArgument(1) = Play");

  m_Min = "0";
  m_Max = "1";
  m_Min2 = m_Max2 = QString();

  m_Widget = new FadeFlicker(this);
  connect(m_Widget, SIGNAL(valueChanged(float)), this, SLOT(onValueChanged(float)));

  m_BPM = QString::number(static_cast<FadeFlicker *>(m_Widget)->GetBPM());

  QPalette pal(m_Widget->palette());
  m_Color = pal.color(QPalette::Button);
  m_TextColor = pal.color(QPalette::ButtonText);
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerWidget::SetText(const QString &text)
{
  ToyWidget::SetText(text);
  static_cast<FadeFlicker *>(m_Widget)->SetText(m_Text);
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerWidget::SetImagePath(const QString &imagePath)
{
  ToyWidget::SetImagePath(imagePath);
  static_cast<FadeFlicker *>(m_Widget)->SetImagePath(0, m_ImagePath);
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerWidget::SetColor(const QColor &color)
{
  ToyWidget::SetColor(color);
  QPalette pal(m_Widget->palette());
  pal.setColor(QPalette::Button, m_Color);
  m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerWidget::SetTextColor(const QColor &textColor)
{
  ToyWidget::SetTextColor(textColor);
  QPalette pal(m_Widget->palette());
  pal.setColor(QPalette::ButtonText, m_TextColor);
  m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerWidget::SetMin2(const QString &n)
{
  ToyWidget::SetMin2(n);
  UpdateTimeScaleRange();
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerWidget::SetMax2(const QString &n)
{
  ToyWidget::SetMax2(n);
  UpdateTimeScaleRange();
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerWidget::UpdateTimeScaleRange()
{
  float minValue = 0;
  float maxValue = 0;

  if (m_Min2.isEmpty())
  {
    if (!m_Max2.isEmpty())
      minValue = maxValue = m_Max2.toFloat();
  }
  else if (m_Max2.isEmpty())
  {
    minValue = maxValue = m_Min2.toFloat();
  }
  else
  {
    minValue = m_Min2.toFloat();
    maxValue = m_Max2.toFloat();
  }

  static_cast<FadeFlicker *>(m_Widget)->SetTimeScaleRange(minValue, maxValue);
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerWidget::SetBPM(const QString &bpm)
{
  ToyWidget::SetBPM(bpm);
  static_cast<FadeFlicker *>(m_Widget)->SetBPM(m_BPM.toFloat());
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerWidget::SetLabel(const QString &label)
{
  static_cast<FadeFlicker *>(m_Widget)->SetLabel(label);
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerWidget::Recv(const QString &path, const OSCArgument *args, size_t count)
{
  if (path == m_TriggerPath)
  {
    bool paused = false;

    if (args && count > 0)
    {
      if (!args[0].GetBool(paused))
        paused = false;
    }

    static_cast<FadeFlicker *>(m_Widget)->SetPaused(paused);
  }
  else
  {
    std::string str;
    if (args && count > 0)
    {
      if (!args[0].GetString(str))
        str.clear();
    }

    SetLabel(QString::fromUtf8(str.c_str()));
  }
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerWidget::Update(unsigned int ms)
{
  static_cast<FadeFlicker *>(m_Widget)->Update(ms);
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerWidget::onValueChanged(float value)
{
  emit valueChanged(this, value);
}

////////////////////////////////////////////////////////////////////////////////

ToyFlickerGrid::ToyFlickerGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags)
  : ToyGrid(TOY_FLICKER_GRID, pClient, parent, flags)
{
  m_Timer = new QTimer(this);
  connect(m_Timer, SIGNAL(timeout()), this, SLOT(onTimeout()));

  m_Play = new FadeButton(this);
  m_Play->setText(tr("Play All"));
  m_Play->resize(m_Play->sizeHint());
  connect(m_Play, SIGNAL(clicked(bool)), this, SLOT(onPlayClicked(bool)));

  m_Pause = new FadeButton(this);
  m_Pause->setText(tr("Pause All"));
  connect(m_Pause, SIGNAL(clicked(bool)), this, SLOT(onPauseClicked(bool)));

  StartTimer();
}

////////////////////////////////////////////////////////////////////////////////

ToyWidget *ToyFlickerGrid::CreateWidget()
{
  ToyFlickerWidget *w = new ToyFlickerWidget(this);
  connect(w, SIGNAL(valueChanged(ToyFlickerWidget *, float)), this, SLOT(onValueChanged(ToyFlickerWidget *, float)));
  return w;
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerGrid::StartTimer()
{
  m_ElapsedTimer.Start();
  m_Timer->start(Toy::GetFlickerRefreshRateMS());
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerGrid::StopTimer()
{
  m_Timer->stop();
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerGrid::UpdateLayout()
{
  QRect r(rect());

  if (m_List.size() > 1)
  {
    int x = MARGIN2;

    if (r.width() > r.height())
    {
      int w = (r.width() - 2 * MARGIN2 - MARGIN) / 4;
      int y = (r.bottom() - MARGIN2 - m_Play->height());

      r.adjust(0, 0, 0, -m_Play->height() - MARGIN);

      m_Play->setGeometry(x, y, w, m_Play->height());
      x += (w + MARGIN);

      m_Pause->setGeometry(x, y, w, m_Play->height());
    }
    else
    {
      int w = (r.width() - 2 * MARGIN2);
      int totalH = ((m_Play->height() + MARGIN) * 2);
      int y = (r.bottom() - totalH);
      r.adjust(0, 0, 0, -totalH);

      m_Play->setGeometry(x, y, w, m_Play->height());
      y += (m_Play->height() + MARGIN);

      m_Pause->setGeometry(x, y, w, m_Play->height());
    }

    m_Play->show();
    m_Pause->show();
  }
  else
  {
    m_Play->hide();
    m_Pause->hide();
  }

  UpdateLayoutForRect(r);
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerGrid::AutoSize(const QSize &widgetSize)
{
  QSize autoSize(m_GridSize.width() * widgetSize.width() + MARGIN2, m_GridSize.height() * widgetSize.height() + MARGIN2);

  if (m_List.size() > 1)
  {
    if (autoSize.width() > autoSize.height())
      autoSize.rheight() += (m_Play->height() + MARGIN);
    else
      autoSize.rheight() += ((m_Play->height() + MARGIN) * 2);
  }

  resize(autoSize);
  UpdateLayout();
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerGrid::onValueChanged(ToyFlickerWidget *flicker, float value)
{
  if (m_pClient && flicker && !flicker->GetPath().isEmpty())
  {
    QString path(flicker->GetPath());
    bool local = Utils::MakeLocalOSCPath(false, path);

    OSCPacketWriter packetWriter(path.toUtf8().constData());

    if (flicker->GetMin().isEmpty())
    {
      if (!flicker->GetMax().isEmpty())
        packetWriter.AddFloat32(flicker->GetMax().toFloat());
    }
    else if (flicker->GetMax().isEmpty())
    {
      packetWriter.AddFloat32(flicker->GetMin().toFloat());
    }
    else
    {
      float minValue = flicker->GetMin().toFloat();
      float maxValue = flicker->GetMax().toFloat();
      value = (minValue + (maxValue - minValue) * value);
      packetWriter.AddFloat32(value);
    }

    size_t size;
    char *packet = packetWriter.Create(size);
    if (packet)
      m_pClient->ToyClient_Send(local, packet, size);
  }
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerGrid::onTimeout()
{
  unsigned int ms = m_ElapsedTimer.Restart();
  for (WIDGET_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    static_cast<ToyFlickerWidget *>(*i)->Update(ms);
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerGrid::onPlayClicked(bool /*checked*/)
{
  for (WIDGET_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    static_cast<ToyFlickerWidget *>(*i)->GetFlicker().SetPaused(false);
}

////////////////////////////////////////////////////////////////////////////////

void ToyFlickerGrid::onPauseClicked(bool /*checked*/)
{
  for (WIDGET_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    static_cast<ToyFlickerWidget *>(*i)->GetFlicker().SetPaused(true);
}

////////////////////////////////////////////////////////////////////////////////
