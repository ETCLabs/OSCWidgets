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

#include "ToyEncoder.h"
#include "OSCParser.h"
#include "Utils.h"

#define ENCODER_SPAN 45

////////////////////////////////////////////////////////////////////////////////

FadeEncoder::FadeEncoder(QWidget *parent)
  : FadeButton(parent)
  , m_MouseDown(false)
  , m_TextMargin(0)
  , m_LabelMargin(0)
{
}

////////////////////////////////////////////////////////////////////////////////

void FadeEncoder::SetText(const QString &text)
{
  if (this->text() != text)
  {
    setText(text);
    UpdateMargins();
    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeEncoder::SetLabel(const QString &label)
{
  if (m_Label != label)
  {
    m_Label = label;
    UpdateMargins();
    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

float FadeEncoder::PosToRadians(const sPosF &pos) const
{
  float w = width();
  float h = (height() - m_TextMargin - m_LabelMargin);
  if (w > 0 && h > 0)
  {
    float x = (2.0f * (pos.x / w) - 1.0f);
    float y = (2.0f * (1.0f - (pos.y - m_TextMargin) / h) - 1.0f);
    float radians = atan2(y, x);
    if (radians < 0)
      radians += static_cast<float>(TWO_PI);
    return radians;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

float FadeEncoder::GetTickRadians(const sPosF &a, const sPosF &b) const
{
  float delta = (PosToRadians(a) - PosToRadians(b));
  if (delta > M_PI)
    delta -= (2.0f * static_cast<float>(M_PI));
  else if (delta < -M_PI)
    delta += (2.0f * static_cast<float>(M_PI));
  return delta;
}

////////////////////////////////////////////////////////////////////////////////

void FadeEncoder::AutoSizeFont()
{
  QFont fnt(font());
  int d = qMin(width(), qMax(m_TextMargin, m_LabelMargin));
  fnt.setPixelSize(qMax(10, qRound(d * 0.333)));
  setFont(fnt);
}

////////////////////////////////////////////////////////////////////////////////

void FadeEncoder::UpdateMargins()
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

void FadeEncoder::Tick(int tickCount)
{
  if (tickCount != 0)
  {
    float delta = ((tickCount > 0) ? (Toy::GetEncoderRadiansPerTick() * 1.00001f) : (Toy::GetEncoderRadiansPerTick() * -1.00001f));

    float radians = PosToRadians(m_MousePos);

    Press();

    int count = abs(tickCount);
    for (int i = 0; i < count; i++)
    {
      radians -= delta;
      emit tick(delta);
    }

    Release();

    float x = cosf(radians);
    float y = sinf(radians);

    float h = (height() - m_TextMargin - m_LabelMargin);
    m_MousePos.x = ((x + 1) * 0.5f * width());
    m_MousePos.y = (m_TextMargin + ((1.0f - (y + 1) * 0.5f)) * h);
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeEncoder::resizeEvent(QResizeEvent *event)
{
  m_Canvas = QImage(size(), QImage::Format_ARGB32);
  UpdateMargins();
  FadeButton::resizeEvent(event);
}

////////////////////////////////////////////////////////////////////////////////

void FadeEncoder::mousePressEvent(QMouseEvent *event)
{
  FadeButton::mousePressEvent(event);
  if (event->button() != Qt::RightButton)
  {
    m_MouseDown = true;
    m_MousePos = event->pos();
    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeEncoder::mouseMoveEvent(QMouseEvent *event)
{
  FadeButton::mouseMoveEvent(event);
  if (m_MouseDown)
  {
    float radians = GetTickRadians(m_MousePos, event->pos());
    if (fabsf(radians) >= Toy::GetEncoderRadiansPerTick())
    {
      emit tick(radians);
      m_MousePos = event->pos();
    }
    update();
  }
}

////////////////////////////////////////////////////////////////////////////////

void FadeEncoder::mouseReleaseEvent(QMouseEvent *event)
{
  FadeButton::mouseReleaseEvent(event);
  m_MouseDown = false;
  update();
}

////////////////////////////////////////////////////////////////////////////////

void FadeEncoder::paintEvent(QPaintEvent * /*event*/)
{
  m_Canvas.fill(0);

  QPainter painter;
  if (painter.begin(&m_Canvas))
  {
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

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

    QRect r(rect());

    r.adjust(1 + HALF_BORDER, 1 + HALF_BORDER + m_TextMargin, -1 - HALF_BORDER, -1 - HALF_BORDER - m_LabelMargin);

    if (r.width() > r.height())
    {
      r.translate(qRound((double)r.width() - r.height()) * 0.5, 0);
      r.setWidth(r.height());
    }
    else if (r.height() > r.width())
    {
      r.translate(0, qRound((double)r.height() - r.width()) * 0.5);
      r.setHeight(r.width());
    }

    int n = qRound(r.width() * 0.333);
    QRect r2(r.x() + qRound((r.width() - n) * 0.5), r.y() + qRound((r.height() - n) * 0.5), n, n);

    bool donutHole = false;

    const QPixmap &pixmap = m_Images[m_ImageIndex].pixmap;
    if (!pixmap.isNull())
    {
      QPainterPath clip;
      clip.addEllipse(r);
      painter.setClipPath(clip);
      painter.drawPixmap(r.x() + qRound((r.width() - pixmap.width()) * 0.5), r.y() + qRound((r.height() - pixmap.height()) * 0.5), pixmap);
      painter.setClipping(false);
      donutHole = true;
    }

    if (m_Click > 0)
    {
      painter.setPen(Qt::NoPen);
      painter.setBrush(color);
      float radians = PosToRadians(m_MousePos);
      float degrees = (radians * (180 / M_PI));
      painter.setOpacity(m_Click);
      painter.drawPie(r, qRound(degrees - ENCODER_SPAN * 0.5) * 16, ENCODER_SPAN * 16);
      painter.setOpacity(1.0);
      donutHole = true;
    }

    if (donutHole)
    {
      painter.setPen(Qt::NoPen);
      painter.setBrush(Qt::white);
      painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
      painter.drawEllipse(r2);
      painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(color, BORDER));
    painter.drawEllipse(r);
    painter.drawEllipse(r2);

    painter.end();

    if (painter.begin(this))
    {
      painter.drawImage(0, 0, m_Canvas);

      if (!text().isEmpty())
      {
        int hoverRaise = qRound(m_Hover * BUTTON_RAISE);

        painter.setPen(palette().color(QPalette::ButtonText));
        painter.drawText(QRect(0, 0, width(), r.y() - hoverRaise), Qt::AlignCenter | Qt::TextWordWrap | Qt::TextDontClip, text());

        if (!m_Label.isEmpty())
          painter.drawText(QRect(0, r.bottom(), width(), height() - r.bottom() + hoverRaise), Qt::AlignCenter | Qt::TextWordWrap | Qt::TextDontClip, m_Label);
      }
      else if (!m_Label.isEmpty())
      {
        int hoverRaise = qRound(m_Hover * BUTTON_RAISE);
        painter.setPen(palette().color(QPalette::ButtonText));
        painter.drawText(QRect(0, r.bottom(), width(), height() - r.bottom() + hoverRaise), Qt::AlignCenter | Qt::TextWordWrap | Qt::TextDontClip, m_Label);
      }

      painter.end();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

ToyEncoderWidget::ToyEncoderWidget(QWidget *parent)
  : ToyWidget(parent)
{
  m_HelpText = tr("Min = Counter-Clockwise Tick\nMax = Clockwise Tick\n\nOSC Trigger:\nNo Arguments = Single CW Tick\nArgument(X) = X CW Ticks\nArgument(-X) = X CCW Ticks");

  m_Min = "-1";
  m_Max = "1";

  m_Widget = new FadeEncoder(this);
  connect(m_Widget, SIGNAL(tick(float)), this, SLOT(onTick(float)));

  QPalette pal(m_Widget->palette());
  m_Color = pal.color(QPalette::Button);
  m_TextColor = pal.color(QPalette::ButtonText);
}

////////////////////////////////////////////////////////////////////////////////

void ToyEncoderWidget::SetText(const QString &text)
{
  ToyWidget::SetText(text);
  static_cast<FadeEncoder *>(m_Widget)->SetText(m_Text);
}

////////////////////////////////////////////////////////////////////////////////

void ToyEncoderWidget::SetImagePath(const QString &imagePath)
{
  ToyWidget::SetImagePath(imagePath);
  static_cast<FadeEncoder *>(m_Widget)->SetImagePath(0, m_ImagePath);
}

////////////////////////////////////////////////////////////////////////////////

void ToyEncoderWidget::SetColor(const QColor &color)
{
  ToyWidget::SetColor(color);
  QPalette pal(m_Widget->palette());
  pal.setColor(QPalette::Button, m_Color);
  m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyEncoderWidget::SetTextColor(const QColor &textColor)
{
  ToyWidget::SetTextColor(textColor);
  QPalette pal(m_Widget->palette());
  pal.setColor(QPalette::ButtonText, m_TextColor);
  m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyEncoderWidget::SetLabel(const QString &label)
{
  static_cast<FadeEncoder *>(m_Widget)->SetLabel(label);
}

////////////////////////////////////////////////////////////////////////////////

void ToyEncoderWidget::Recv(const QString &path, const OSCArgument *args, size_t count)
{
  if (path == m_TriggerPath)
  {
    FadeEncoder *encoder = static_cast<FadeEncoder *>(m_Widget);

    int tickCount = 1;

    if (args && count > 0)
    {
      int n = 0;
      if (args[0].GetInt(n))
        tickCount = n;
    }

    encoder->Tick(tickCount);
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

void ToyEncoderWidget::onTick(float radians)
{
  emit tick(this, radians);
}

////////////////////////////////////////////////////////////////////////////////

ToyEncoderGrid::ToyEncoderGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags)
  : ToyGrid(TOY_ENCODER_GRID, pClient, parent, flags)
{
}

////////////////////////////////////////////////////////////////////////////////

ToyWidget *ToyEncoderGrid::CreateWidget()
{
  ToyEncoderWidget *w = new ToyEncoderWidget(this);
  connect(w, SIGNAL(tick(ToyEncoderWidget *, float)), this, SLOT(onTick(ToyEncoderWidget *, float)));
  return w;
}

////////////////////////////////////////////////////////////////////////////////

void ToyEncoderGrid::onTick(ToyEncoderWidget *encoder, float radians)
{
  if (m_pClient && encoder && !encoder->GetPath().isEmpty())
  {
    QString path(encoder->GetPath());
    bool local = Utils::MakeLocalOSCPath(false, path);

    OSCPacketWriter packetWriter(path.toUtf8().constData());

    if (encoder->GetMin().isEmpty())
    {
      if (!encoder->GetMax().isEmpty())
        packetWriter.AddFloat32(encoder->GetMax().toFloat());
    }
    else if (encoder->GetMax().isEmpty())
    {
      packetWriter.AddFloat32(encoder->GetMin().toFloat());
    }
    else
    {
      float value = ((radians < 0) ? encoder->GetMin().toFloat() : encoder->GetMax().toFloat());
      packetWriter.AddFloat32(value);
    }

    size_t size;
    char *packet = packetWriter.Create(size);
    if (packet)
      m_pClient->ToyClient_Send(local, packet, size);
  }
}

////////////////////////////////////////////////////////////////////////////////
