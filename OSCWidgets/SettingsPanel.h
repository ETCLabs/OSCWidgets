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
#ifndef SETTINGS_PANEL_H
#define SETTINGS_PANEL_H

#ifndef QT_INCLUDE_H
#include "QtInclude.h"
#endif

#ifndef OSC_PARSER_H
#include "OSCParser.h"
#endif

////////////////////////////////////////////////////////////////////////////////

#define SETTING_LOG_DEPTH "LogDepth"
#define SETTING_FILE_DEPTH "FileDepth"
#define SETTING_LAST_FILE "LastFile"
#define SETTING_ENCODER_DEGREES_PER_TICK "EncoderDegreesPerTick"
#define SETTING_FEEDBACK_DELAY "FeedbackDelay"
#define SETTING_CMD_SEND_ALL_DELAY "CmdSendAllDelay"
#define SETTING_METRO_REFRESH_RATE "MetroRefreshRate"
#define SETTING_SINE_REFRESH_RATE "SineWaveRefreshRate"
#define SETTING_PEDAL_REFRESH_RATE "PedalRefreshRate"

////////////////////////////////////////////////////////////////////////////////

class AddToyButton : public QPushButton
{
  Q_OBJECT

public:
  AddToyButton(int type, QWidget *parent);

signals:
  void addToy(int type);

private slots:
  void onClicked(bool checked);

protected:
  int m_Type;
};

////////////////////////////////////////////////////////////////////////////////

class AdvancedPanel : public QWidget
{
  Q_OBJECT

public:
  AdvancedPanel(QWidget *parent);

  virtual void Load();
  virtual void Save();

signals:
  void changed();

private slots:
  void onApplyClicked(bool checked);
  void onRestoreDefaultsClicked(bool checked);

protected:
  QLineEdit *m_EncoderDegreesPerTick;
  QLineEdit *m_FeedbackDelay;
  QLineEdit *m_CmdSendAllDelay;
  QLineEdit *m_MetroRefreshRate;
  QLineEdit *m_SineRefreshRate;
  QLineEdit *m_PedalRefreshRate;
  QLineEdit *m_FlickerRefreshRate;
};

////////////////////////////////////////////////////////////////////////////////

class SettingsPanel : public QWidget
{
  Q_OBJECT

public:
  SettingsPanel(QWidget *parent);

  virtual OSCStream::EnumFrameMode GetMode() const;
  virtual void SetMode(OSCStream::EnumFrameMode mode);
  virtual void GetIP(QString &ip) const;
  virtual void SetIP(const QString &ip);
  virtual unsigned short GetPort1() const;
  virtual void SetPort1(unsigned short port);
  virtual unsigned short GetPort2() const;
  virtual void SetPort2(unsigned short port2);

  virtual unsigned short GetUdpOutputPort() const { return GetPort1(); }
  virtual unsigned short GetUdpInputPort() const { return GetPort2(); }
  virtual unsigned short GetTcpPort() const { return GetPort1(); }

signals:
  void changed();
  void addToy(int type);

private slots:
  void onModeChanged(int index);
  void onApplyClicked(bool checked);
  void onAddToy(int type);

protected:
  QComboBox *m_Mode;
  QLineEdit *m_Ip;
  QLabel *m_PortLabel;
  QSpinBox *m_Port;
  QLabel *m_Port2Label;
  QSpinBox *m_Port2;

  virtual void UpdateMode();
};

////////////////////////////////////////////////////////////////////////////////

#endif
