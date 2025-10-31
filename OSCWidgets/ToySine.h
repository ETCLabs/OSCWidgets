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
#ifndef TOY_SINE_H
#define TOY_SINE_H

#ifndef TOY_BUTTON_H
#include "ToyButton.h"
#endif

////////////////////////////////////////////////////////////////////////////////

class FadeSine : public FadeButton
{
  Q_OBJECT

public:
  FadeSine(QWidget *parent);

  virtual void ReCenter();
  virtual float GetPos() const { return m_Pos; }
  virtual void SetPos(float pos)
  {
    m_Pos = pos;
    update();
  }
  virtual void SetText(const QString &text);
  virtual void SetLabel(const QString &label);
  virtual void Update(unsigned int ms);
  virtual float GetBPM() const { return m_BPM; }
  virtual void SetBPM(float bpm);
  virtual bool GetPaused() const { return m_Paused; }
  virtual void SetPaused(bool b);

signals:
  void tick(float value);

private slots:
  void onClicked(bool checked);

protected:
  enum EnumConstants
  {
    NUM_POINTS = 50
  };

  int m_TextMargin;
  int m_LabelMargin;
  float m_Pos;
  float m_Speed;
  float m_BPM;
  bool m_Paused;
  QPointF m_Points[NUM_POINTS];

  virtual void UpdateSpeed();
  virtual void AutoSizeFont();
  virtual void UpdateMargins();
  virtual void resizeEvent(QResizeEvent *event);
  virtual void paintEvent(QPaintEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class ToySineWidget : public ToyWidget
{
  Q_OBJECT

public:
  ToySineWidget(QWidget *parent);

  virtual void SetText(const QString &text);
  virtual void SetImagePath(const QString &imagePath);
  virtual void SetColor(const QColor &color);
  virtual void SetTextColor(const QColor &textColor);
  virtual bool HasTriggerPath() const { return true; }
  virtual void SetBPM(const QString &bpm);
  virtual bool HasBPM() const { return true; }
  virtual void SetLabel(const QString &label);
  virtual void Recv(const QString &path, const OSCArgument *args, size_t count);
  virtual void Update(unsigned int ms);
  virtual FadeSine &GetSine() { return *static_cast<FadeSine *>(m_Widget); }

signals:
  void tick(ToySineWidget *, float value);

private slots:
  void onTick(float value);
};

////////////////////////////////////////////////////////////////////////////////

class ToySineGrid : public ToyGrid
{
  Q_OBJECT

public:
  ToySineGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags);

  virtual void StartTimer();
  virtual void StopTimer();

private slots:
  void onTick(ToySineWidget *, float value);
  void onTimeout();
  void onPlayClicked(bool checked);
  void onPauseClicked(bool checked);
  void onReCenterClicked(bool checked);
  void onFanClicked(bool checked);

protected:
  QTimer *m_Timer;
  EosTimer m_ElapsedTimer;
  FadeButton *m_Play;
  FadeButton *m_Pause;
  FadeButton *m_ReCenter;
  FadeButton *m_Fan;

  virtual ToyWidget *CreateWidget();
  virtual QSize GetDefaultWidgetSize() const { return QSize(180, 120); }
  virtual void UpdateLayout();
  virtual void AutoSize(const QSize &widgetSize);
};

////////////////////////////////////////////////////////////////////////////////

#endif
