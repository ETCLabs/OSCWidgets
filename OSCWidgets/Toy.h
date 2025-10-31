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
#ifndef TOY_H
#define TOY_H

#ifndef QT_INCLUDE_H
#include "QtInclude.h"
#endif

#ifndef EOS_TIMER_H
#include "EosTimer.h"
#endif

#ifndef TOY_MATH_H
#include "ToyMath.h"
#endif

#include <vector>

class EosLog;
class OSCArgument;
class ToyWidget;

////////////////////////////////////////////////////////////////////////////////

class Toy : public QWidget
{
  Q_OBJECT

public:
  enum EnumToyType
  {
    TOY_BUTTON_GRID = 0,
    TOY_PEDAL_GRID,
    TOY_SLIDER_GRID,
    TOY_ENCODER_GRID,
    TOY_XY_GRID,
    TOY_METRO_GRID,
    TOY_CMD_GRID,
    TOY_SINE_GRID,
    TOY_FLICKER_GRID,
    TOY_ACTIVITY_GRID,
    TOY_LABEL_GRID,
    TOY_WINDOW,

    TOY_COUNT,
    TOY_INVALID
  };

  class Client
  {
  public:
    virtual bool ToyClient_Send(bool local, char *data, size_t size) = 0;
    virtual void ToyClient_ResourceRelativePathToAbsolute(QString &path) = 0;
  };

  typedef std::multimap<QString, ToyWidget *> RECV_WIDGETS;
  typedef std::pair<QString, ToyWidget *> RECV_WIDGETS_PAIR;
  typedef std::pair<RECV_WIDGETS::const_iterator, RECV_WIDGETS::const_iterator> RECV_WIDGETS_RANGE;

  Toy(EnumToyType type, Client *pClient, QWidget *parent, Qt::WindowFlags flags);
  virtual ~Toy();

  EnumToyType GetType() const { return m_Type; }
  virtual void AddRecvWidgets(RECV_WIDGETS &recvWidgets) const = 0;
  virtual void SnapToEdges();
  virtual bool Save(EosLog &log, const QString &path, QStringList &lines) = 0;
  virtual bool Load(EosLog &log, const QString &path, QStringList &lines, int &index) = 0;
  virtual void Clear() = 0;
  virtual const QString &GetText() const = 0;
  virtual void SetText(const QString &text) = 0;
  virtual const QString &GetImagePath() const = 0;
  virtual void SetImagePath(const QString &imagePath) = 0;
  virtual const QColor &GetColor() const = 0;
  virtual void SetColor(const QColor &color) = 0;
  virtual bool HasColor2() const = 0;
  virtual const QColor &GetColor2() const = 0;
  virtual void SetColor2(const QColor &color2) = 0;
  virtual bool HasTextColor() const = 0;
  virtual const QColor &GetTextColor() const = 0;
  virtual void SetTextColor(const QColor &textColor) = 0;
  virtual void GetName(QString &name) const = 0;
  virtual void SetGridSize(const QSize &gridSize) = 0;
  virtual const QSize &GetGridSize() const = 0;
  virtual void GetDefaultGridSize(QSize &gridSize) const = 0;
  virtual void ClearLabels() = 0;
  virtual void StartTimer() {}
  virtual void Connected() {}
  virtual void Disconnected() {}

  static Toy *Create(EnumToyType type, Client *pClient, QWidget *parent, Qt::WindowFlags flags);
  static void GetName(EnumToyType type, QString &name);
  static void GetDefaultPathName(EnumToyType type, QString &path);
  static void SnapToScreen(QWidget &w, int threshold);
  static void ClipToScreen(QWidget &w);
  static void SetDefaultWindowIcon(QWidget &w);
  static void ResourceRelativePathToAbsolute(EosLog *log, const QString &filePath, QString &resourcePath);
  static void ResourceAbsolutePathToRelative(EosLog *log, const QString &filePath, QString &resourcePath);

  static float GetEncoderRadiansPerTick() { return sm_EncoderRadiansPerTick; }
  static void SetEncoderRadiansPerTick(float n) { sm_EncoderRadiansPerTick = qBound(0.0043633232f, n, static_cast<float>(M_PI)); }
  static unsigned int GetFeedbackDelayMS() { return sm_FeedbackDelayMS; }
  static void SetFeedbackDelayMS(unsigned int n) { sm_FeedbackDelayMS = qBound(static_cast<unsigned int>(1), n, static_cast<unsigned int>(60000)); }
  static unsigned int GetCmdSendAllDelayMS() { return sm_CmdSendAllDelayMS; }
  static void SetCmdSendAllDelayMS(unsigned int n) { sm_CmdSendAllDelayMS = qBound(static_cast<unsigned int>(0), n, static_cast<unsigned int>(60000)); }
  static unsigned int GetMetroRefreshRateMS() { return sm_MetroRefreshRateMS; }
  static void SetMetroRefreshRateMS(unsigned int n) { sm_MetroRefreshRateMS = qBound(static_cast<unsigned int>(1), n, static_cast<unsigned int>(250)); }
  static unsigned int GetSineRefreshRateMS() { return sm_SineRefreshRateMS; }
  static void SetSineRefreshRateMS(unsigned int n) { sm_SineRefreshRateMS = qBound(static_cast<unsigned int>(1), n, static_cast<unsigned int>(250)); }
  static unsigned int GetPedalRefreshRateMS() { return sm_PedalRefreshRateMS; }
  static void SetPedalRefreshRateMS(unsigned int n) { sm_PedalRefreshRateMS = qBound(static_cast<unsigned int>(1), n, static_cast<unsigned int>(250)); }
  static unsigned int GetFlickerRefreshRateMS() { return sm_FlickerRefreshRateMS; }
  static void SetFlickerRefreshRateMS(unsigned int n) { sm_FlickerRefreshRateMS = qBound(static_cast<unsigned int>(1), n, static_cast<unsigned int>(60000)); }
  static void RestoreDefaultSettings();

signals:
  void changed();
  void recvWidgetsChanged();
  void closing(Toy *toy);
  void toggleMainWindow();

protected:
  const EnumToyType m_Type;
  Client *m_pClient;

  static float sm_EncoderRadiansPerTick;
  static unsigned int sm_FeedbackDelayMS;
  static unsigned int sm_CmdSendAllDelayMS;
  static unsigned int sm_MetroRefreshRateMS;
  static unsigned int sm_SineRefreshRateMS;
  static unsigned int sm_PedalRefreshRateMS;
  static unsigned int sm_FlickerRefreshRateMS;
};

////////////////////////////////////////////////////////////////////////////////

#endif
