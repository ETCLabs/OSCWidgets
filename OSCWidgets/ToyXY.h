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
#ifndef TOY_XY_H
#define TOY_XY_H

#ifndef TOY_BUTTON_H
#include "ToyButton.h"
#endif

////////////////////////////////////////////////////////////////////////////////

class FadeXY : public FadeButton
{
  Q_OBJECT

public:
  FadeXY(QWidget *parent);

  virtual void SetText(const QString &text);
  virtual void SetLabel(const QString &label);
  virtual void SetPos(const QPointF &pos);
  virtual void RecvPos(const QPointF &pos);
  virtual void TriggerPos(const QPointF &pos);
  virtual const QPointF &GetPos() const { return m_Pos; }
  virtual bool GetMouseDown() const { return m_MouseDown; }

signals:
  void posChanged(const QPointF &pos);

private slots:
  void onRecvTimeout();

protected:
  struct sRecvPos
  {
    bool pending;
    QPointF pos;
    QTimer *timer;
  };

  QPointF m_Pos;
  sRecvPos m_RecvPos;
  bool m_MouseDown;
  int m_TextMargin;
  int m_LabelMargin;
  QImage m_Canvas;

  virtual void UpdateMargins();
  virtual void AutoSizeFont();
  virtual void SetPosPrivate(const QPointF &pos, bool user);
  virtual QPoint PosToWidgetPos(const QPointF &pos);
  virtual QPointF WidgetPosToPos(const QPoint &pos);
  virtual void ClipWidgetPosForPainting(const QRectF &r, QPoint &pos) const;
  virtual void resizeEvent(QResizeEvent *event);
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);
  virtual void paintEvent(QPaintEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class ToyXYWidget : public ToyWidget
{
  Q_OBJECT

public:
  ToyXYWidget(QWidget *parent);

  virtual const QPointF &GetPos() const;
  virtual void SetText(const QString &text);
  virtual void SetImagePath(const QString &imagePath);
  virtual void SetColor(const QColor &color);
  virtual void SetTextColor(const QColor &textColor);
  virtual bool HasPath2() const { return true; }
  virtual bool HasFeedbackPath() const { return true; }
  virtual bool HasTriggerPath() const { return true; }
  virtual bool HasMinMax2() const { return true; }
  virtual void SetLabel(const QString &label);
  virtual void Recv(const QString &path, const OSCArgument *args, size_t count);

signals:
  void posChanged(ToyXYWidget *);

private slots:
  void onPosChanged(const QPointF &pos);
};

////////////////////////////////////////////////////////////////////////////////

class ToyXYGrid : public ToyGrid
{
  Q_OBJECT

public:
  ToyXYGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags);

private slots:
  void onPosChanged(ToyXYWidget *);

protected:
  virtual ToyWidget *CreateWidget();
  virtual QSize GetDefaultWidgetSize() const { return QSize(200, 200); }
};

////////////////////////////////////////////////////////////////////////////////

#endif
