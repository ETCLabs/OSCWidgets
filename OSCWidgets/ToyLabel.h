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
#ifndef TOY_LABEL_H
#define TOY_LABEL_H

#ifndef TOY_WIDGET_H
#include "ToyWidget.h"
#endif

#ifndef TOY_GRID_H
#include "ToyGrid.h"
#endif

////////////////////////////////////////////////////////////////////////////////

class FadeLabel : public FadeButton_NoTouch
{
public:
  FadeLabel(QWidget *parent);

protected:
  virtual void paintEvent(QPaintEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class ToyLabelWidget : public ToyWidget
{
public:
  ToyLabelWidget(Toy::Client *pClient, QWidget *parent);

  virtual void SetText(const QString &text);
  virtual void SetImagePath(const QString &imagePath);
  virtual void SetColor(const QColor &color);
  virtual void SetColor2(const QColor &color2);
  virtual bool HasColor2() const { return true; }
  virtual void SetTextColor(const QColor &textColor);
  virtual void Recv(const QString &path, const OSCArgument *args, size_t count);
  virtual void SetLabel(const QString &label);
  virtual void ClearLabel();
  virtual bool HasPath() const { return false; }
  virtual bool HasMinMax() const { return false; }
  virtual bool HasFeedbackPath() const { return false; }
  virtual bool HasTriggerPath() const { return true; }

protected:
  enum EnumConstants
  {
    IMAGE_PATH_INDEX_DEFAULT = 0,
    IMAGE_PATH_INDEX_FROM_TRIGGER = 1
  };

  Toy::Client *m_pClient;
};

////////////////////////////////////////////////////////////////////////////////

class ToyLabelGrid : public ToyGrid
{
public:
  ToyLabelGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags);

protected:
  virtual ToyWidget *CreateWidget();
  virtual QSize GetDefaultWidgetSize() const { return QSize(200, 40); }
  virtual void ApplyDefaultSettings(ToyWidget *widget, size_t index);
};

////////////////////////////////////////////////////////////////////////////////

#endif
