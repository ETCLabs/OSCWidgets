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
#ifndef TOY_CMD_H
#define TOY_CMD_H

#ifndef TOY_BUTTON_H
#include "ToyButton.h"
#endif

////////////////////////////////////////////////////////////////////////////////

class FadeCmd : public QLineEdit
{
public:
  FadeCmd(QWidget *parent);

protected:
  virtual void AutoSizeFont();
  virtual void resizeEvent(QResizeEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class ToyCmdWidget : public ToyWidget
{
  Q_OBJECT

public:
  ToyCmdWidget(QWidget *parent);

  virtual void SetPath(const QString &text);
  virtual void SetText(const QString &text);
  virtual void SetImagePath(const QString &imagePath);
  virtual void SetColor(const QColor &color);
  virtual void SetTextColor(const QColor &textColor);
  virtual void Recv(const QString &path, const OSCArgument *args, size_t count);
  virtual bool HasMinMax() const { return false; }
  virtual bool HasTriggerPath() const { return true; }

signals:
  void send(ToyCmdWidget *);

private slots:
  void onEditingFinished();
  void onReturnPressed();
  void onSendClicked(bool checked);

protected:
  FadeCmd *m_Cmd;
  FadeButton *m_Send;

  virtual void UpdateToolTip();
};

////////////////////////////////////////////////////////////////////////////////

class ToyCmdGrid : public ToyGrid
{
  Q_OBJECT

public:
  ToyCmdGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags);

  virtual void GetDefaultGridSize(QSize &gridSize) const { gridSize = QSize(1, 6); }
  virtual void SetSendOnConnect(bool b);
  virtual void Connected();
  virtual void SendAll();

private slots:
  void onSend(ToyCmdWidget *);
  void onSendAllClicked(bool checked);
  void onSendAllTimeout();
  void onStartupStateChanged(int state);

protected:
  FadeButton *m_SendAll;
  QCheckBox *m_Startup;
  QTimer *m_SendAllTimer;
  size_t m_SendAllIndex;
  int m_ButtonsHeight;
  unsigned int m_IgnoreUpdates;

  virtual ToyWidget *CreateWidget();
  virtual QSize GetDefaultWidgetSize() const { return QSize(500, 30); }
  virtual void ApplyDefaultSettings(ToyWidget *widget, size_t index);
  virtual void UpdateLayout();
  virtual void UpdateSendAllButton();
  virtual void AutoSize(const QSize &widgetSize);
};

////////////////////////////////////////////////////////////////////////////////

#endif
