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

#include "ToyMetro.h"
#include "OSCParser.h"
#include "Utils.h"

#define METRO_ARM_PEN 4

////////////////////////////////////////////////////////////////////////////////

FadeMetro::FadeMetro(QWidget *parent)
  : FadeButton(parent)
  , m_TextMargin(0)
  , m_LabelMargin(0)
  , m_Pos(0)
  , m_BPM(60)
  , m_ArmLength(0)
  , m_Paused(true)
{
  connect(this, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
  UpdateSpeed();
}

////////////////////////////////////////////////////////////////////////////////

void FadeMetro::ReCenter()
{
  m_Pos = 0;

  if (!m_Paused)
    emit tick(TICK_POS_CENTER);

  update();
}

////////////////////////////////////////////////////////////////////////////////

void FadeMetro::SetText(const QString &text)
{
  if (this->text() != text)
  {
    setText(text);
    UpdateMargins();
    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeMetro::SetLabel(const QString &label)
{
  if (m_Label != label)
  {
    m_Label = label;
    UpdateMargins();
    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeMetro::AutoSizeFont()
{
  QFont fnt(font());
  int d = qMin(width(), qMax(m_TextMargin, m_LabelMargin));
  fnt.setPixelSize(qMax(10, qRound(d * 0.7)));
  setFont(fnt);
}

////////////////////////////////////////////////////////////////////////////////

void FadeMetro::UpdateMargins()
{
  int textMargin = (text().isEmpty() ? 0 : qRound(height() * 0.1));
  int labelMargin = (m_Label.isEmpty() ? 0 : qRound(height() * 0.1));

  if (m_TextMargin != textMargin || m_LabelMargin != labelMargin)
  {
    m_TextMargin = textMargin;
    m_LabelMargin = labelMargin;
    UpdateMetroRect();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeMetro::UpdateMetroRect()
{
  m_MetroRect = rect();
  m_MetroRect.adjust(METRO_ARM_PEN, METRO_ARM_PEN + m_TextMargin, -METRO_ARM_PEN, -METRO_ARM_PEN - m_LabelMargin);
  float lenForWidth = sqrtf(2 * powf(m_MetroRect.width() * 0.5f, 2.0f));
  float lenForHeight = m_MetroRect.height();
  m_ArmLength = qMin(lenForWidth, lenForHeight);
  update();
}

////////////////////////////////////////////////////////////////////////////////

void FadeMetro::SetBPM(float bpm)
{
  if (m_BPM != bpm)
  {
    m_BPM = bpm;
    UpdateSpeed();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeMetro::SetPaused(bool b)
{
  if (m_Paused != b)
  {
    m_Paused = b;

    if (!m_Paused && GetTickPos() == TICK_POS_CENTER)
      emit tick(TICK_POS_CENTER);

    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeMetro::UpdateSpeed()
{
  float beatsPerMillisecond = (m_BPM / 60000);
  m_Speed = (beatsPerMillisecond * M_PI);
}

////////////////////////////////////////////////////////////////////////////////

int FadeMetro::GetSegment() const
{
  return GetSegmentForPos(m_Pos);
}

////////////////////////////////////////////////////////////////////////////////

int FadeMetro::GetSegmentForPos(float pos) const
{
  return (pos / TWO_PI * 3.99999);
}

////////////////////////////////////////////////////////////////////////////////

FadeMetro::EnumTickPos FadeMetro::GetTickPos() const
{
  return GetTickPosForSegment(GetSegment());
}

////////////////////////////////////////////////////////////////////////////////

FadeMetro::EnumTickPos FadeMetro::GetTickPosForSegment(int segment) const
{
  switch (segment)
  {
    case 1: return TICK_POS_RIGHT;
    case 3: return TICK_POS_LEFT;
  }

  return TICK_POS_CENTER;
}

////////////////////////////////////////////////////////////////////////////////

void FadeMetro::Update(unsigned int ms)
{
  if (!m_Paused && ms != 0)
  {
    float prevPos = m_Pos;
    m_Pos = fmod(m_Pos + ms * m_Speed, static_cast<float>(TWO_PI));

    int prevSegment = GetSegmentForPos(prevPos);
    int segment = GetSegmentForPos(m_Pos);

    if (segment != prevSegment)
      emit tick(GetTickPosForSegment(segment));

    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeMetro::resizeEvent(QResizeEvent *event)
{
  FadeButton::resizeEvent(event);
  UpdateMargins();
  UpdateMetroRect();
}

////////////////////////////////////////////////////////////////////////////////

void FadeMetro::paintEvent(QPaintEvent * /*event*/)
{
  QPainter painter(this);

  painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

  QPainterPath clip;
  clip.addRoundedRect(m_MetroRect, ROUNDED, ROUNDED);
  painter.setClipPath(clip);

  const QPixmap &pixmap = m_Images[m_ImageIndex].pixmap;
  if (!pixmap.isNull())
  {
    painter.drawPixmap(m_MetroRect.x() + qRound((m_MetroRect.width() - pixmap.width()) * 0.5), m_MetroRect.y() + qRound((m_MetroRect.height() - pixmap.height()) * 0.5), pixmap);
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

  float t = sinf(m_Pos);
  const float TRAVEL_45_DEGREES_IN_RADIANS = 0.785398163f;
  float radians = (t * TRAVEL_45_DEGREES_IN_RADIANS);
  float armPenWidth = (BORDER * 2);

  QPointF base(m_MetroRect.x() + m_MetroRect.width() * 0.5, m_MetroRect.bottom());

  QColor axisColor(color);
  axisColor.setAlpha(180);
  painter.setPen(QPen(axisColor, HALF_BORDER));
  painter.drawLine(base.x(), base.y(), base.x(), m_MetroRect.y());

  painter.setPen(QPen(color, BORDER, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin));
  painter.setBrush(Qt::NoBrush);
  painter.drawRoundedRect(m_MetroRect, ROUNDED, ROUNDED);

  QPointF tip(base.x() + m_ArmLength * sinf(radians), m_MetroRect.height() - m_ArmLength * cosf(radians));
  painter.setPen(QPen(color, armPenWidth, Qt::SolidLine, Qt::RoundCap));
  painter.drawLine(base, tip);

  if (!text().isEmpty())
  {
    int hoverRaise = qRound(m_Hover * BUTTON_RAISE);

    painter.setFont(font());
    painter.setPen(palette().color(QPalette::ButtonText));
    painter.drawText(QRect(0, 0, width(), m_MetroRect.y() - hoverRaise), Qt::AlignCenter | Qt::TextWordWrap | Qt::TextDontClip, text());

    if (!m_Label.isEmpty())
      painter.drawText(QRect(0, m_MetroRect.bottom(), width(), height() - m_MetroRect.bottom() + hoverRaise), Qt::AlignCenter | Qt::TextWordWrap | Qt::TextDontClip, m_Label);
  }
  else if (!m_Label.isEmpty())
  {
    int hoverRaise = qRound(m_Hover * BUTTON_RAISE);
    painter.setFont(font());
    painter.setPen(palette().color(QPalette::ButtonText));
    painter.drawText(QRect(0, m_MetroRect.bottom(), width(), height() - m_MetroRect.bottom() + hoverRaise), Qt::AlignCenter | Qt::TextWordWrap | Qt::TextDontClip, m_Label);
  }

  if (m_Paused)
  {
    painter.setOpacity(0.8);
    color = color.lighter(250);
    painter.setPen(color);
    painter.setBrush(BG_COLOR);
    int pauseSize = qRound(0.6 * qMin(m_MetroRect.width(), m_MetroRect.height()));
    QRect pauseRect(m_MetroRect.x() + qRound((m_MetroRect.width() - pauseSize) * 0.5), m_MetroRect.y() + qRound((m_MetroRect.height() - pauseSize) * 0.5), pauseSize, pauseSize);
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

void FadeMetro::onClicked(bool /*checked*/)
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

ToyMetroWidget::ToyMetroWidget(QWidget *parent)
  : ToyWidget(parent)
{
  m_HelpText = tr("If Min or Max blank, tick on center\n\nOtherwise, tick at ends\n\nOSC Trigger:\nNo Arguments = Play\nArgument(0) = Pause\nArgument(1) = Play");

  m_Min = QString();
  m_Max = "1";

  m_Widget = new FadeMetro(this);
  connect(m_Widget, SIGNAL(tick(int)), this, SLOT(onTick(int)));

  m_BPM = QString::number(static_cast<FadeMetro *>(m_Widget)->GetBPM());

  QPalette pal(m_Widget->palette());
  m_Color = pal.color(QPalette::Button);
  m_TextColor = pal.color(QPalette::ButtonText);
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroWidget::SetText(const QString &text)
{
  ToyWidget::SetText(text);
  static_cast<FadeMetro *>(m_Widget)->SetText(m_Text);
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroWidget::SetImagePath(const QString &imagePath)
{
  ToyWidget::SetImagePath(imagePath);
  static_cast<FadeMetro *>(m_Widget)->SetImagePath(0, m_ImagePath);
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroWidget::SetColor(const QColor &color)
{
  ToyWidget::SetColor(color);
  QPalette pal(m_Widget->palette());
  pal.setColor(QPalette::Button, m_Color);
  m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroWidget::SetTextColor(const QColor &textColor)
{
  ToyWidget::SetTextColor(textColor);
  QPalette pal(m_Widget->palette());
  pal.setColor(QPalette::ButtonText, m_TextColor);
  m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroWidget::SetBPM(const QString &bpm)
{
  ToyWidget::SetBPM(bpm);
  float n = m_BPM.toFloat();
  static_cast<FadeMetro *>(m_Widget)->SetBPM(qBound(0.0f, n, 600.0f));
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroWidget::SetLabel(const QString &label)
{
  static_cast<FadeMetro *>(m_Widget)->SetLabel(label);
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroWidget::Recv(const QString &path, const OSCArgument *args, size_t count)
{
  if (path == m_TriggerPath)
  {
    bool paused = false;

    if (args && count > 0)
    {
      if (!args[0].GetBool(paused))
        paused = false;
    }

    static_cast<FadeMetro *>(m_Widget)->SetPaused(paused);
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

void ToyMetroWidget::Update(unsigned int ms)
{
  static_cast<FadeMetro *>(m_Widget)->Update(ms);
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroWidget::onTick(int pos)
{
  emit tick(this, pos);
}

////////////////////////////////////////////////////////////////////////////////

ToyMetroGrid::ToyMetroGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags)
  : ToyGrid(TOY_METRO_GRID, pClient, parent, flags)
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

ToyWidget *ToyMetroGrid::CreateWidget()
{
  ToyMetroWidget *w = new ToyMetroWidget(this);
  connect(w, SIGNAL(tick(ToyMetroWidget *, int)), this, SLOT(onTick(ToyMetroWidget *, int)));
  return w;
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroGrid::StartTimer()
{
  m_ElapsedTimer.Start();
  m_Timer->start(Toy::GetMetroRefreshRateMS());
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroGrid::StopTimer()
{
  m_Timer->stop();
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroGrid::UpdateLayout()
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

void ToyMetroGrid::AutoSize(const QSize &widgetSize)
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

void ToyMetroGrid::onTick(ToyMetroWidget *metro, int pos)
{
  if (m_pClient && metro && !metro->GetPath().isEmpty())
  {
    QString value;
    bool forceStrArg = false;

    if (metro->GetMin().isEmpty())
    {
      if (pos == FadeMetro::TICK_POS_CENTER)
      {
        if (!metro->GetMax().isEmpty())
          value = metro->GetMax();
      }
      else
        return;
    }
    else if (metro->GetMax().isEmpty())
    {
      if (pos == FadeMetro::TICK_POS_CENTER)
        value = metro->GetMin();
      else
        return;
    }
    else if (metro->GetMin() == metro->GetMax())
    {
      if (pos == FadeMetro::TICK_POS_CENTER)
      {
        value = metro->GetMax();

        if (!OSCArgument::IsFloatString(metro->GetMin().toUtf8().constData()) || !OSCArgument::IsFloatString(metro->GetMax().toUtf8().constData()))
          forceStrArg = true;  // if either is non-numeric, send both as strings
      }
      else
        return;
    }
    else if (pos == FadeMetro::TICK_POS_LEFT || pos == FadeMetro::TICK_POS_RIGHT)
    {
      value = ((pos == FadeMetro::TICK_POS_LEFT) ? metro->GetMin() : metro->GetMax());

      if (!OSCArgument::IsFloatString(metro->GetMin().toUtf8().constData()) || !OSCArgument::IsFloatString(metro->GetMax().toUtf8().constData()))
        forceStrArg = true;  // if either is non-numeric, send both as strings
    }
    else
      return;

    QString path(metro->GetPath());
    bool local = Utils::MakeLocalOSCPath(false, path);

    OSCPacketWriter packetWriter(path.toUtf8().constData());

    if (!value.isEmpty())
    {
      QByteArray ba(value.toUtf8());
      if (!forceStrArg && OSCArgument::IsFloatString(ba.constData()))
        packetWriter.AddFloat32(value.toFloat());
      else
        packetWriter.AddString(ba.constData());
    }

    size_t size;
    char *packet = packetWriter.Create(size);
    if (packet)
      m_pClient->ToyClient_Send(local, packet, size);
  }
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroGrid::onTimeout()
{
  unsigned int ms = m_ElapsedTimer.Restart();
  for (WIDGET_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    static_cast<ToyMetroWidget *>(*i)->Update(ms);
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroGrid::onPlayClicked(bool /*checked*/)
{
  for (WIDGET_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    static_cast<ToyMetroWidget *>(*i)->GetMetro().SetPaused(false);
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroGrid::onPauseClicked(bool /*checked*/)
{
  for (WIDGET_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    static_cast<ToyMetroWidget *>(*i)->GetMetro().SetPaused(true);
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroGrid::onReCenterClicked(bool /*checked*/)
{
  for (WIDGET_LIST::const_iterator i = m_List.begin(); i != m_List.end(); i++)
    static_cast<ToyMetroWidget *>(*i)->GetMetro().ReCenter();
}

////////////////////////////////////////////////////////////////////////////////

void ToyMetroGrid::onFanClicked(bool /*checked*/)
{
  size_t count = m_List.size();
  if (count > 1)
  {
    float t = static_cast<float>(TWO_PI / count);
    for (size_t i = 0; i < count; i++)
      static_cast<ToyMetroWidget *>(m_List[i])->GetMetro().SetPos(t * i);
  }
}

////////////////////////////////////////////////////////////////////////////////
