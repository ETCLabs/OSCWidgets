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
#ifndef TOY_PEDAL_H
#define TOY_PEDAL_H

#ifndef TOY_BUTTON_H
#include "ToyButton.h"
#endif

////////////////////////////////////////////////////////////////////////////////

class FadePedal : public FadeButton
{
  Q_OBJECT

public:
  FadePedal(QWidget *parent);

  virtual void Reset();
  virtual void Update(unsigned int ms);
  virtual unsigned int GetUpDuration() const { return m_UpDuration; }
  virtual void SetUpDuration(unsigned int ms) { m_UpDuration = ms; }
  virtual unsigned int GetDownDuration() const { return m_DownDuration; }
  virtual void SetDownDuration(unsigned int ms) { m_DownDuration = ms; }
  virtual void Press();
  virtual void Release();

signals:
  void tick(float value);

private slots:
  void onPressed();
  void onReleased();

protected:
  enum EnumState
  {
    STATE_IDLE,
    STATE_UP,
    STATE_PEAK,
    STATE_DOWN
  };

  enum EnumConstants
  {
    NUM_POINTS = 50
  };

  struct sTick
  {
    sTick()
      : state(STATE_IDLE)
      , elapsed(0)
      , value(0)
    {
    }
    sTick(EnumState State, unsigned int Elapsed, float Value)
      : state(State)
      , elapsed(Elapsed)
      , value(Value)
    {
    }
    EnumState state;
    unsigned int elapsed;
    float value;
  };

  typedef std::vector<sTick> TICKS;

  EnumState m_State;
  unsigned int m_Elapsed;
  unsigned int m_UpDuration;
  unsigned int m_DownDuration;
  TICKS m_Ticks;
  QPolygonF m_Points;
  QImage m_Canvas;

  virtual void Tick(float value);
  virtual void SetState(EnumState state);
  virtual void paintEvent(QPaintEvent *event);
  virtual void resizeEvent(QResizeEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class ToyPedalWidget : public ToyWidget
{
  Q_OBJECT

public:
  ToyPedalWidget(QWidget *parent);

  virtual void SetText(const QString &text);
  virtual void SetImagePath(const QString &imagePath);
  virtual void SetColor(const QColor &color);
  virtual void SetTextColor(const QColor &textColor);
  virtual bool HasTriggerPath() const { return true; }
  virtual bool HasMinMax2() const { return true; }
  virtual void SetMin2(const QString &n);
  virtual void SetMax2(const QString &n);
  virtual void SetLabel(const QString &label);
  virtual void Recv(const QString &path, const OSCArgument *args, size_t count);
  virtual void Update(unsigned int ms);
  virtual FadePedal &GetPedal() { return *static_cast<FadePedal *>(m_Widget); }

signals:
  void tick(ToyPedalWidget *, float value);

private slots:
  void onTick(float value);
};

////////////////////////////////////////////////////////////////////////////////

class ToyPedalGrid : public ToyGrid
{
  Q_OBJECT

public:
  ToyPedalGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags);

  virtual void GetDefaultGridSize(QSize &gridSize) const { gridSize = QSize(5, 1); }
  virtual void StartTimer();
  virtual void StopTimer();

private slots:
  void onTick(ToyPedalWidget *, float value);
  void onTimeout();
  void onPressPressed();
  void onPressReleased();

protected:
  QTimer *m_Timer;
  EosTimer m_ElapsedTimer;
  FadeButton *m_Press;

  virtual ToyWidget *CreateWidget();
  virtual void UpdateLayout();
  virtual void AutoSize(const QSize &widgetSize);
};

////////////////////////////////////////////////////////////////////////////////

#endif
