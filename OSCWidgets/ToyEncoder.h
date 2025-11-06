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
#ifndef TOY_ENCODER_H
#define TOY_ENCODER_H

#ifndef TOY_BUTTON_H
#include "ToyButton.h"
#endif

////////////////////////////////////////////////////////////////////////////////

class FadeEncoder : public FadeButton
{
  Q_OBJECT

public:
  FadeEncoder(QWidget *parent);

  virtual void SetText(const QString &text);
  virtual void SetLabel(const QString &label);
  virtual void Tick(int tickCount);

signals:
  void tick(float radians);

protected:
  struct sPosF
  {
    sPosF()
      : x(0)
      , y(0)
    {
    }
    sPosF(float X, float Y)
      : x(X)
      , y(Y)
    {
    }
    sPosF(const QPoint &p)
      : x(p.x())
      , y(p.y())
    {
    }
    float x;
    float y;
  };

  bool m_MouseDown;
  sPosF m_MousePos;
  int m_TextMargin;
  int m_LabelMargin;

  virtual void AutoSizeFont();
  virtual void UpdateMargins();
  virtual float PosToRadians(const sPosF &pos) const;
  virtual float GetTickRadians(const sPosF &a, const sPosF &b) const;
  virtual void resizeEvent(QResizeEvent *event);
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);
  virtual void paintEvent(QPaintEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class ToyEncoderWidget : public ToyWidget
{
  Q_OBJECT

public:
  ToyEncoderWidget(QWidget *parent);

  virtual void GetDefaultGridSize(QSize &gridSize) const { gridSize = QSize(4, 1); }
  virtual void SetText(const QString &text);
  virtual void SetImagePath(const QString &imagePath);
  virtual void SetColor(const QColor &color);
  virtual void SetTextColor(const QColor &textColor);
  virtual void SetLabel(const QString &label);
  virtual void Recv(const QString &path, const OSCArgument *args, size_t count);
  virtual bool HasTriggerPath() const { return true; }

signals:
  void tick(ToyEncoderWidget *, float radians);

private slots:
  void onTick(float radians);
};

////////////////////////////////////////////////////////////////////////////////

class ToyEncoderGrid : public ToyGrid
{
  Q_OBJECT

public:
  ToyEncoderGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags);

private slots:
  void onTick(ToyEncoderWidget *, float radians);

protected:
  virtual ToyWidget *CreateWidget();
  virtual QSize GetDefaultWidgetSize() const { return QSize(160, 160); }
};

////////////////////////////////////////////////////////////////////////////////

#endif
