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
#ifndef TOY_SLIDER_H
#define TOY_SLIDER_H

#ifndef TOY_BUTTON_H
#include "ToyButton.h"
#endif

////////////////////////////////////////////////////////////////////////////////

class FadeSlider : public FadeButton
{
  Q_OBJECT

public:
  FadeSlider(QWidget *parent);

  virtual void SetText(const QString &text);
  virtual void SetLabel(const QString &label);
  virtual void SetPercent(float percent);
  virtual void RecvPercent(float percent);
  virtual void TriggerPercent(float percent);
  virtual float GetPercent() const { return m_Percent; }
  virtual bool GetMouseDown() const { return m_MouseDown; }

signals:
  void percentChanged(float percent);

private slots:
  void onRecvTimeout();

protected:
  struct sRecvPercent
  {
    bool pending;
    float percent;
    QTimer *timer;
  };

  float m_Percent;
  sRecvPercent m_RecvPercent;
  bool m_MouseDown;
  int m_TextMargin;
  int m_LabelMargin;
  QImage m_Canvas;

  virtual void UpdateMargins();
  virtual void AutoSizeFont();
  virtual void SetPercentPrivate(float percent, bool user);
  virtual int PercentToPos(float percent);
  virtual float PosToPercent(int pos);
  virtual void resizeEvent(QResizeEvent *event);
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);
  virtual void paintEvent(QPaintEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class ToySliderWidget : public ToyWidget
{
  Q_OBJECT

public:
  ToySliderWidget(QWidget *parent);

  virtual float GetPercent() const;
  virtual void SetText(const QString &text);
  virtual void SetImagePath(const QString &imagePath);
  virtual void SetColor(const QColor &color);
  virtual void SetTextColor(const QColor &textColor);
  virtual bool HasFeedbackPath() const { return true; }
  virtual bool HasTriggerPath() const { return true; }
  virtual void SetLabel(const QString &label);
  virtual void Recv(const QString &path, const OSCArgument *args, size_t count);

signals:
  void percentChanged(ToySliderWidget *);

private slots:
  void onPercentChanged(float percent);
};

////////////////////////////////////////////////////////////////////////////////

class ToySliderGrid : public ToyGrid
{
  Q_OBJECT

public:
  ToySliderGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags);

  virtual void GetDefaultGridSize(QSize &gridSize) const { gridSize = QSize(5, 1); }

private slots:
  void onPercentChanged(ToySliderWidget *);

protected:
  virtual ToyWidget *CreateWidget();
  virtual QSize GetDefaultWidgetSize() const { return QSize(60, 250); }
};

////////////////////////////////////////////////////////////////////////////////

#endif
