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
#ifndef TOY_FLICKER_H
#define TOY_FLICKER_H

#ifndef TOY_BUTTON_H
#include "ToyButton.h"
#endif

////////////////////////////////////////////////////////////////////////////////

class FadeFlicker : public FadeButton
{
  Q_OBJECT

public:
  FadeFlicker(QWidget *parent);

  virtual void SetText(const QString &text);
  virtual void SetLabel(const QString &label);
  virtual void Update(unsigned int ms);
  virtual void SetTimeScaleRange(float minTimeScale, float maxTimeScale);
  virtual bool HasTimeScale() const;
  virtual float GetBPM() const { return m_BPM; }
  virtual void SetBPM(float bpm);
  virtual bool GetPaused() const { return m_Paused; }
  virtual void SetPaused(bool b);

  static unsigned int GetMsPerBeat(float bpm, float timeScale);

signals:
  void valueChanged(float value);

private slots:
  void onClicked(bool checked);

protected:
  int m_TextMargin;
  int m_LabelMargin;
  float m_Value;
  float m_MinTimeScale;
  float m_MaxTimeScale;
  float m_BPM;
  unsigned int m_MsPerBeat;
  unsigned int m_Elapsed;
  QRect m_FlickerRect;
  bool m_Paused;

  virtual void UpdateFlickerRect();
  virtual void AutoSizeFont();
  virtual void UpdateMargins();
  virtual void UpdateMsPerBeat();
  virtual void resizeEvent(QResizeEvent *event);
  virtual void paintEvent(QPaintEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class ToyFlickerWidget : public ToyWidget
{
  Q_OBJECT

public:
  ToyFlickerWidget(QWidget *parent);

  virtual void SetText(const QString &text);
  virtual void SetImagePath(const QString &imagePath);
  virtual void SetColor(const QColor &color);
  virtual void SetTextColor(const QColor &textColor);
  virtual bool HasTriggerPath() const { return true; }
  virtual bool HasMinMax2() const { return true; }
  virtual void SetMin2(const QString &n);
  virtual void SetMax2(const QString &n);
  virtual void SetBPM(const QString &bpm);
  virtual bool HasBPM() const { return true; }
  virtual void SetLabel(const QString &label);
  virtual void Recv(const QString &path, const OSCArgument *args, size_t count);
  virtual void Update(unsigned int ms);
  virtual FadeFlicker &GetFlicker() { return *static_cast<FadeFlicker *>(m_Widget); }

signals:
  void valueChanged(ToyFlickerWidget *, float value);

private slots:
  void onValueChanged(float value);

protected:
  virtual void UpdateTimeScaleRange();
};

////////////////////////////////////////////////////////////////////////////////

class ToyFlickerGrid : public ToyGrid
{
  Q_OBJECT

public:
  ToyFlickerGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags);

  virtual void StartTimer();
  virtual void StopTimer();

private slots:
  void onValueChanged(ToyFlickerWidget *, float value);
  void onTimeout();
  void onPlayClicked(bool checked);
  void onPauseClicked(bool checked);

protected:
  QTimer *m_Timer;
  EosTimer m_ElapsedTimer;
  FadeButton *m_Play;
  FadeButton *m_Pause;

  virtual ToyWidget *CreateWidget();
  virtual QSize GetDefaultWidgetSize() const { return QSize(80, 120); }
  virtual void UpdateLayout();
  virtual void AutoSize(const QSize &widgetSize);
};

////////////////////////////////////////////////////////////////////////////////

#endif
