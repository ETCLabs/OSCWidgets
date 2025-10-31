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
#ifndef FADE_BUTTON_H
#define FADE_BUTTON_H

#ifndef QT_INCLUDE_H
#include "QtInclude.h"
#endif

#ifndef EOS_TIMER_H
#include "EosTimer.h"
#endif

////////////////////////////////////////////////////////////////////////////////

class FadeButton : public QPushButton
{
  Q_OBJECT

public:
  enum EnumConstants
  {
    NUM_IMAGES = 2
  };

  FadeButton(QWidget *parent);
  FadeButton(bool touchEnabled, QWidget *parent);
  virtual ~FadeButton();

  virtual const QString &GetLabel() const { return m_Label; }
  virtual void SetLabel(const QString &label);
  virtual const QString &GetImagePath(size_t index) const;
  virtual void SetImagePath(size_t index, const QString &imagePath);
  virtual void SetImageIndex(size_t index);
  virtual void Press(bool user = true);
  virtual void Release(bool user = true);
  virtual void Flash();

private slots:
  void onPressed();
  void onReleased();
  void onClickTimeout();
  void onHoverTimeout();

private:
  void Construct(bool touchEnabled);

protected:
  struct sImage
  {
    QString path;
    QPixmap pixmap;
  };

  float m_Click;
  QTimer *m_ClickTimer;
  EosTimer m_ClickTimestamp;
  float m_Hover;
  QTimer *m_HoverTimer;
  EosTimer m_HoverTimestamp;
  QString m_Label;
  sImage m_Images[NUM_IMAGES];
  size_t m_ImageIndex;

  virtual void StartClick();
  virtual void StopClick();
  virtual void SetClick(float percent);
  virtual void StartHover();
  virtual void StopHover();
  virtual void SetHover(float percent);
  virtual void AutoSizeFont();
  virtual void UpdateImage(size_t index);
  virtual void RenderBackground(QPainter &painter, QRectF &r);
  virtual void resizeEvent(QResizeEvent *event);
  virtual void paintEvent(QPaintEvent *event);
  virtual bool event(QEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class FadeButton_NoTouch : public FadeButton
{
public:
  FadeButton_NoTouch(QWidget *parent)
    : FadeButton(/*touchEnabled*/ false, parent)
  {
  }
};

////////////////////////////////////////////////////////////////////////////////

#endif
