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

#include "ToyLabel.h"
#include "Utils.h"
#include "OSCParser.h"
#include "FadeButton.h"

////////////////////////////////////////////////////////////////////////////////

FadeLabel::FadeLabel(QWidget *parent)
  : FadeButton_NoTouch(parent)
{
}

////////////////////////////////////////////////////////////////////////////////

void FadeLabel::paintEvent(QPaintEvent * /*event*/)
{
  QRectF r(rect());
  r.adjust(1, 1, -1, -1);

  QPainter painter(this);
  painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

  QColor penColor(palette().color(QPalette::Button));
  if (penColor.alpha() > 0)
    painter.setPen(QPen(penColor, 2));
  else
    painter.setPen(Qt::NoPen);
  painter.setBrush(palette().color(QPalette::Window));
  RenderBackground(painter, r);

  sImage &img = m_Images[m_ImageIndex];
  if (!img.pixmap.isNull())
  {
    painter.drawPixmap(r.x() + qRound((r.width() - img.pixmap.width()) * 0.5), r.y() + qRound((r.height() - img.pixmap.height()) * 0.5), img.pixmap);
  }

  QColor textColor(palette().color(QPalette::ButtonText));
  if (!isEnabled())
    textColor = textColor.darker(150);

  if (!text().isEmpty())
  {
    if (m_Label.isEmpty())
    {
      // just text centered
      painter.setPen(textColor);
      painter.drawText(r, Qt::AlignCenter | Qt::TextWordWrap, text());
    }
    else
    {
      // both text and label text
      QRectF textRect;
      painter.setPen(textColor);
      painter.drawText(r, Qt::AlignCenter | Qt::TextWordWrap | Qt::TextDontPrint, text(), &textRect);

      QRectF labelRect;
      painter.drawText(r, Qt::AlignCenter | Qt::TextWordWrap | Qt::TextDontPrint, m_Label, &labelRect);

      qreal h = (r.height() * 0.5 * (textRect.height() / labelRect.height()));
      painter.drawText(QRectF(r.x(), r.y(), r.width(), h), Qt::AlignCenter | Qt::TextWordWrap, text());
      painter.drawText(QRectF(r.x(), r.y() + h, r.width(), r.height() - h), Qt::AlignCenter | Qt::TextWordWrap, m_Label);
    }
  }
  else if (!m_Label.isEmpty())
  {
    // just label text centered
    painter.setPen(textColor);
    painter.drawText(r, Qt::AlignCenter | Qt::TextWordWrap, m_Label);
  }
}

////////////////////////////////////////////////////////////////////////////////

ToyLabelWidget::ToyLabelWidget(Toy::Client *pClient, QWidget *parent)
  : ToyWidget(parent)
  , m_pClient(pClient)
{
  m_HelpText = tr("OSC Trigger:\nImage Path");

  m_Color = QColor(60, 60, 60);
  m_Color2 = QColor(40, 40, 40, 0);

  m_Widget = new FadeLabel(this);
  QPalette pal(m_Widget->palette());
  m_TextColor = pal.color(QPalette::ButtonText);
  pal.setColor(QPalette::Window, m_Color);
  pal.setColor(QPalette::Button, m_Color2);
  m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyLabelWidget::SetText(const QString &text)
{
  ToyWidget::SetText(text);
  static_cast<FadeLabel *>(m_Widget)->setText(m_Text);
}

////////////////////////////////////////////////////////////////////////////////

void ToyLabelWidget::SetImagePath(const QString &imagePath)
{
  ToyWidget::SetImagePath(imagePath);
  static_cast<FadeLabel *>(m_Widget)->SetImagePath(0, m_ImagePath);
}

////////////////////////////////////////////////////////////////////////////////

void ToyLabelWidget::SetColor(const QColor &color)
{
  ToyWidget::SetColor(color);

  QPalette pal(m_Widget->palette());
  pal.setColor(QPalette::Window, m_Color);
  m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyLabelWidget::SetColor2(const QColor &color2)
{
  ToyWidget::SetColor2(color2);

  QPalette pal(m_Widget->palette());
  pal.setColor(QPalette::Button, m_Color2);
  m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyLabelWidget::SetTextColor(const QColor &textColor)
{
  ToyWidget::SetTextColor(textColor);

  QPalette pal(m_Widget->palette());
  pal.setColor(QPalette::ButtonText, m_TextColor);
  m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyLabelWidget::SetLabel(const QString &label)
{
  static_cast<FadeLabel *>(m_Widget)->SetLabel(label);
}

////////////////////////////////////////////////////////////////////////////////

void ToyLabelWidget::ClearLabel()
{
  ToyWidget::ClearLabel();
  static_cast<FadeLabel *>(m_Widget)->SetImageIndex(IMAGE_PATH_INDEX_DEFAULT);
}

////////////////////////////////////////////////////////////////////////////////

void ToyLabelWidget::Recv(const QString &path, const OSCArgument *args, size_t count)
{
  QString str;
  if (args && count > 0)
  {
    std::string s;
    if (args[0].GetString(s))
      str = QString::fromUtf8(s.c_str());
  }

  if (path == m_TriggerPath)
  {
    FadeLabel *label = static_cast<FadeLabel *>(m_Widget);
    if (m_pClient)
      m_pClient->ToyClient_ResourceRelativePathToAbsolute(str);
    label->SetImagePath(IMAGE_PATH_INDEX_FROM_TRIGGER, str);
    label->SetImageIndex(IMAGE_PATH_INDEX_FROM_TRIGGER);
  }
  else
    SetLabel(str);
}

////////////////////////////////////////////////////////////////////////////////

ToyLabelGrid::ToyLabelGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags)
  : ToyGrid(TOY_LABEL_GRID, pClient, parent, flags)
{
}

////////////////////////////////////////////////////////////////////////////////

ToyWidget *ToyLabelGrid::CreateWidget()
{
  return (new ToyLabelWidget(m_pClient, this));
}

////////////////////////////////////////////////////////////////////////////////

void ToyLabelGrid::ApplyDefaultSettings(ToyWidget *widget, size_t index)
{
  widget->SetText(tr("Label"));

  QString pathName;
  Toy::GetDefaultPathName(m_Type, pathName);

  QString path = QString("/%1/%2").arg(pathName).arg(index);
  widget->SetLabelPath(path);
}

////////////////////////////////////////////////////////////////////////////////
