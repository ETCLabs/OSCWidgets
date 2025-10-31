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

#include "ToySine.h"
#include "OSCParser.h"
#include "Utils.h"

////////////////////////////////////////////////////////////////////////////////

FadeSine::FadeSine(QWidget *parent)
  : FadeButton(parent)
  , m_TextMargin(0)
  , m_LabelMargin(0)
  , m_Pos(0)
  , m_BPM(60.0f)
  , m_Paused(true)
{
  connect(this, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
  UpdateSpeed();
}

////////////////////////////////////////////////////////////////////////////////

void FadeSine::ReCenter()
{
  m_Pos = 0;
  update();
}

////////////////////////////////////////////////////////////////////////////////

void FadeSine::SetText(const QString &text)
{
  if (this->text() != text)
  {
    setText(text);
    UpdateMargins();
    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeSine::SetLabel(const QString &label)
{
  if (m_Label != label)
  {
    m_Label = label;
    UpdateMargins();
    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeSine::AutoSizeFont()
{
  QFont fnt(font());
  int d = qMin(width(), qMax(m_TextMargin, m_LabelMargin));
  fnt.setPixelSize(qMax(10, qRound(d * 0.7)));
  setFont(fnt);
}

////////////////////////////////////////////////////////////////////////////////

void FadeSine::UpdateMargins()
{
  int textMargin = (text().isEmpty() ? 0 : qRound(height() * 0.1));
  int labelMargin = (m_Label.isEmpty() ? 0 : qRound(height() * 0.1));

  if (m_TextMargin != textMargin || m_LabelMargin != labelMargin)
  {
    m_TextMargin = textMargin;
    m_LabelMargin = labelMargin;
    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeSine::SetBPM(float bpm)
{
  if (m_BPM != bpm)
  {
    m_BPM = bpm;
    UpdateSpeed();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeSine::SetPaused(bool b)
{
  if (m_Paused != b)
  {
    m_Paused = b;
    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeSine::UpdateSpeed()
{
  float beatsPerMillisecond = (m_BPM / 60000);
  m_Speed = (beatsPerMillisecond * M_PI);
}

////////////////////////////////////////////////////////////////////////////////

void FadeSine::Update(unsigned int ms)
{
  if (!m_Paused && ms != 0)
  {
    m_Pos = fmod(m_Pos + ms * m_Speed, static_cast<float>(TWO_PI));
    float percent = (sinf(m_Pos) + 1) * 0.5f;
    emit tick(percent);
    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeSine::resizeEvent(QResizeEvent *event)
{
  FadeButton::resizeEvent(event);
  UpdateMargins();
}

////////////////////////////////////////////////////////////////////////////////

void FadeSine::paintEvent(QPaintEvent * /*event*/)
{
  QPainter painter(this);

  painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

  QRect r(rect());
  r.adjust(HALF_BORDER, HALF_BORDER + m_TextMargin, -HALF_BORDER, -HALF_BORDER - m_LabelMargin);

  QPainterPath clip;
  clip.addRoundedRect(r, ROUNDED, ROUNDED);
  painter.setClipPath(clip);

  const QPixmap &pixmap = m_Images[m_ImageIndex].pixmap;
  if (!pixmap.isNull())
  {
    painter.drawPixmap(r.x() + qRound((r.width() - pixmap.width()) * 0.5), r.y() + qRound((r.height() - pixmap.height()) * 0.5), pixmap);
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

  float toPercent = 1.0f / NUM_POINTS;
  float sineHeight = (r.height() * 0.8f);
  float sineX = (r.x() - HALF_BORDER);
  float sineY = (r.y() + 0.5f * (r.height() - sineHeight));
  float sineWidth = (r.right() + HALF_BORDER - sineX);

  for (int i = 0; i < NUM_POINTS; i++)
  {
    float percent = (1.0f - i * toPercent);
    float t = (TWO_PI * percent);
    float y = ((sinf(m_Pos + t) + 1.0f) * 0.5f);
    m_Points[i].setX(sineX + percent * sineWidth);
    m_Points[i].setY(sineY + sineHeight * (1.0f - y));
  }

  painter.setPen(QPen(color, BORDER, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin));
  painter.setBrush(Qt::NoBrush);
  painter.drawPolyline(m_Points, NUM_POINTS);
  painter.drawRoundedRect(r, ROUNDED, ROUNDED);

  if (!text().isEmpty())
  {
    int hoverRaise = qRound(m_Hover * BUTTON_RAISE);

    painter.setFont(font());
    painter.setPen(palette().color(QPalette::ButtonText));
    painter.drawText(QRect(0, 0, width(), r.y() - hoverRaise), Qt::AlignCenter | Qt::TextWordWrap | Qt::TextDontClip, text());

    if (!m_Label.isEmpty())
      painter.drawText(QRect(0, r.bottom(), width(), height() - r.bottom() + hoverRaise), Qt::AlignCenter | Qt::TextWordWrap | Qt::TextDontClip, m_Label);
  }
  else if (!m_Label.isEmpty())
  {
    int hoverRaise = qRound(m_Hover * BUTTON_RAISE);
    painter.setFont(font());
    painter.setPen(palette().color(QPalette::ButtonText));
    painter.drawText(QRect(0, r.bottom(), width(), height() - r.bottom() + hoverRaise), Qt::AlignCenter | Qt::TextWordWrap | Qt::TextDontClip, m_Label);
  }

  if (m_Paused)
  {
    painter.setOpacity(0.8);
    color = color.lighter(250);
    painter.setPen(color);
    painter.setBrush(BG_COLOR);
    int pauseSize = qRound(0.6 * qMin(r.width(), r.height()));
    QRect pauseRect(r.x() + qRound((r.width() - pauseSize) * 0.5), r.y() + qRound((r.height() - pauseSize) * 0.5), pauseSize, pauseSize);
    painter.drawRoundedRect(pauseRect, ROUNDED, ROUNDED);

    int w = qRound(pauseSize * 0.35);
    int h = qRound(pauseSize * 0.7);
    pauseRect.adjust(w, h, -w, -h);
    painter.setPen(QPen(color, pauseSize * 0.1, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(pauseRect.left(), pauseRect.top(), pauseRect.left(), pauseRect.bottom());
    painter.drawLine(pauseRect.right(), pauseRect.top(), pauseRect.right(), pauseRect.bottom());
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeSine::onClicked(bool /*checked*/)
{
  if (m_Paused)
  {
    ReCenter();
    SetPaused(false);
  }
  else
  {
    SetPaused(true);
  }
}

////////////////////////////////////////////////////////////////////////////////

ToySineWidget::ToySineWidget(QWidget *parent)
  : ToyWidget(parent)
{
  m_HelpText = tr("Min=Peak\nMax=Valley\n\nOSC Trigger:\nNo Arguments = Play\nArgument(0) = Pause\nArgument(1) = Play");

  m_Widget = new FadeSine(this);
  connect(m_Widget, SIGNAL(tick(float)), this, SLOT(onTick(float)));

  m_BPM = QString::number(static_cast<FadeSine *>(m_Widget)->GetBPM());

  QPalette pal(m_Widget->palette());
  m_Color = pal.color(QPalette::Button);
  m_TextColor = pal.color(QPalette::ButtonText);
}

////////////////////////////////////////////////////////////////////////////////

void ToySineWidget::SetText(const QString &text)
{
  ToyWidget::SetText(text);
  static_cast<FadeSine *>(m_Widget)->SetText(m_Text);
}

////////////////////////////////////////////////////////////////////////////////

void ToySineWidget::SetImagePath(const QString &imagePath)
{
  ToyWidget::SetImagePath(imagePath);
  static_cast<FadeSine *>(m_Widget)->SetImagePath(0, m_ImagePath);
}

////////////////////////////////////////////////////////////////////////////////

void ToySineWidget::SetColor(const QColor &color)
{
  ToyWidget::SetColor(color);
  QPalette pal(m_Widget->palette());
  pal.setColor(QPalette::Button, m_Color);
  m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToySineWidget::SetTextColor(const QColor &textColor)
{
  ToyWidget::SetTextColor(textColor);
  QPalette pal(m_Widget->palette());
  pal.setColor(QPalette::ButtonText, m_TextColor);
  m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToySineWidget::SetBPM(const QString &bpm)
{
  ToyWidget::SetBPM(bpm);
  float n = m_BPM.toFloat();
  static_cast<FadeSine *>(m_Widget)->SetBPM(qBound(0.0f, n, 300.0f));
}

////////////////////////////////////////////////////////////////////////////////

void ToySineWidget::SetLabel(const QString &label)
{
  static_cast<FadeSine *>(m_Widget)->SetLabel(label);
}

////////////////////////////////////////////////////////////////////////////////

void ToySineWidget::Recv(const QString &path, const OSCArgument *args, size_t count)
{
  if (path == m_TriggerPath)
  {
    bool paused = false;

    if (args && count > 0)
    {
      if (!args[0].GetBool(paused))
        paused = false;
    }

    static_cast<FadeSine *>(m_Widget)->SetPaused(paused);
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

void ToySineWidget::Update(unsigned int ms)
{
  static_cast<FadeSine *>(m_Widget)->Update(ms);
}

////////////////////////////////////////////////////////////////////////////////

void ToySineWidget::onTick(float value)
{
  emit tick(this, value);
}

////////////////////////////////////////////////////////////////////////////////

ToySineGrid::ToySineGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags)
  : ToyGrid(TOY_SINE_GRID, pClient, parent, flags)
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

  m_ReCenter = new FadeButton(this);
  m_ReCenter->setText(tr("Reset All"));
  connect(m_ReCenter, SIGNAL(clicked(bool)), this, SLOT(onReCenterClicked(bool)));

  m_Fan = new FadeButton(this);
  m_Fan->setText(tr("Fan"));
  connect(m_Fan, SIGNAL(clicked(bool)), this, SLOT(onFanClicked(bool)));

  StartTimer();
}

////////////////////////////////////////////////////////////////////////////////

ToyWidget *ToySineGrid::CreateWidget()
{
  ToySineWidget *w = new ToySineWidget(this);
  connect(w, SIGNAL(tick(ToySineWidget *, float)), this, SLOT(onTick(ToySineWidget *, float)));
  return w;
}

////////////////////////////////////////////////////////////////////////////////

void ToySineGrid::StartTimer()
{
  m_ElapsedTimer.Start();
  m_Timer->start(Toy::GetSineRefreshRateMS());
}

////////////////////////////////////////////////////////////////////////////////

void ToySineGrid::StopTimer()
{
  m_Timer->stop();
}

////////////////////////////////////////////////////////////////////////////////

void ToySineGrid::UpdateLayout()
{
  QRect r(rect());

  if (m_List.size() > 1)
  {
    int x = MARGIN2;

    if (r.width() > r.height())
    {
      int w = (r.width() - 2 * MARGIN2 - 3 * MARGIN) / 4;
      int y = (r.bottom() - MARGIN2 - m_Play->height());

      r.adjust(0, 0, 0, -m_Play->height() - MARGIN);

      m_Play->setGeometry(x, y, w, m_Play->height());
      x += (w + MARGIN);

      m_Pause->setGeometry(x, y, w, m_Play->height());
      x += (w + MARGIN);

      m_ReCenter->setGeometry(x, y, w, m_Play->height());
      x += (w + MARGIN);

      m_Fan->setGeometry(x, y, w, m_Play->height());
    }
    else
    {
      int w = (r.width() - 2 * MARGIN2);
      int totalH = ((m_Play->height() + MARGIN) * 4);
      int y = (r.bottom() - totalH);
      r.adjust(0, 0, 0, -totalH);

      m_Play->setGeometry(x, y, w, m_Play->height());
      y += (m_Play->height() + MARGIN);

      m_Pause->setGeometry(x, y, w, m_Play->height());
      y += (m_Play->height() + MARGIN);

      m_ReCenter->setGeometry(x, y, w, m_Play->height());
      y += (m_Play->height() + MARGIN);

      m_Fan->setGeometry(x, y, w, m_Play->height());
    }

    m_Play->show();
    m_Pause->show();
    m_ReCenter->show();
    m_Fan->show();
  }
  else
  {
    m_Play->hide();
    m_Pause->hide();
    m_ReCenter->hide();
    m_Fan->hide();
  }

  UpdateLayoutForRect(r);
}

////////////////////////////////////////////////////////////////////////////////

void ToySineGrid::AutoSize(const QSize &widgetSize)
{
  QSize autoSize(m_GridSize.width() * widgetSize.width() + MARGIN2, m_GridSize.height() * widgetSize.height() + MARGIN2);

  if (m_List.size() > 1)
  {
    if (autoSize.width() > autoSize.height())
      autoSize.rheight() += (m_Play->height() + MARGIN);
    else
      autoSize.rheight() += ((m_Play->height() + MARGIN) * 4);
  }

  resize(autoSize);
  UpdateLayout();
}

////////////////////////////////////////////////////////////////////////////////

void ToySineGrid::onTick(ToySineWidget *sine, float value)
{
  if (m_pClient && sine && !sine->GetPath().isEmpty())
  {
    QString path(sine->GetPath());
    bool local = Utils::MakeLocalOSCPath(false, path);

    OSCPacketWriter packetWriter(path.toUtf8().constData());

    if (!sine->GetMin().isEmpty() || !sine->GetMax().isEmpty())
    {
      float minValue = sine->GetMin().toFloat();
      float maxValue = sine->GetMax().toFloat();
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

void ToySineGrid::onTimeout()
{
  unsigned int ms = m_ElapsedTimer.Restart();
  for (WIDGET_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    static_cast<ToySineWidget *>(*i)->Update(ms);
}

////////////////////////////////////////////////////////////////////////////////

void ToySineGrid::onPlayClicked(bool /*checked*/)
{
  for (WIDGET_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    static_cast<ToySineWidget *>(*i)->GetSine().SetPaused(false);
}

////////////////////////////////////////////////////////////////////////////////

void ToySineGrid::onPauseClicked(bool /*checked*/)
{
  for (WIDGET_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    static_cast<ToySineWidget *>(*i)->GetSine().SetPaused(true);
}

////////////////////////////////////////////////////////////////////////////////

void ToySineGrid::onReCenterClicked(bool /*checked*/)
{
  for (WIDGET_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    static_cast<ToySineWidget *>(*i)->GetSine().ReCenter();
}

////////////////////////////////////////////////////////////////////////////////

void ToySineGrid::onFanClicked(bool /*checked*/)
{
  size_t count = m_List.size();
  if (count > 1)
  {
    float t = static_cast<float>(TWO_PI / count);
    for (size_t i = 0; i < count; i++)
      static_cast<ToySineWidget *>(m_List[i])->GetSine().SetPos(t * i);
  }
}

////////////////////////////////////////////////////////////////////////////////
