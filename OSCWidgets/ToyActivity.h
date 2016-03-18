// Copyright (c) 2016 Electronic Theatre Controls, Inc., http://www.etcconnect.com
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
#ifndef TOY_ACTIVITY_H
#define TOY_ACTIVITY_H

#ifndef TOY_WIDGET_H
#include "ToyWidget.h"
#endif

#ifndef TOY_GRID_H
#include "ToyGrid.h"
#endif

////////////////////////////////////////////////////////////////////////////////

class FadeActivity
	: public FadeButton
{
	Q_OBJECT

public:
	enum EnumConstants
	{
		FADE_HOLD_INFINITE	= 0xffffffff
	};

	struct sFadeTiming
	{
		sFadeTiming()
			: in(200)
			, hold( static_cast<unsigned int>(FADE_HOLD_INFINITE) )
			, out(500)
		{}

		bool operator==(const sFadeTiming &other) const;
		bool operator!=(const sFadeTiming &other) const {return !((*this)==other);}

		unsigned int	in;
		unsigned int	hold;
		unsigned int	out;
	};

	FadeActivity(QWidget *parent);

	virtual void SetText(const QString &text);
	virtual void SetFadeTiming(const sFadeTiming &fadeTiming);
	virtual void SetDefaultFadeTiming(bool flash);
	virtual void SetOn(bool b);

private slots:
	void onUpdate();

protected:
	enum EnumFadeState
	{
		FADE_OFF,
		FADE_IN,
		FADE_ON,
		FADE_OUT
	};

	QTimer			*m_ActivityTimer;
	EosTimer		m_ActivityEosTimer;
	sFadeTiming		m_FadeTiming;
	EnumFadeState	m_FadeState;
	unsigned int	m_FadeElapsed;

	virtual float GetFadeOpacity() const;
	virtual float GetFadePercent() const;
	virtual void StartActivityTimer();
	virtual void paintEvent(QPaintEvent *event);
};

////////////////////////////////////////////////////////////////////////////////

class ToyActivityWidget
	: public ToyWidget
{
public:
	ToyActivityWidget(QWidget *parent);
	
	virtual void SetText(const QString &text);
	virtual void SetImagePath(const QString &imagePath);
	virtual void SetColor(const QColor &color);
	virtual void SetTextColor(const QColor &textColor);
	virtual void Recv(const QString &path, const OSCArgument *args, size_t count);
	virtual void SetLabel(const QString &label);
	virtual bool HasPath() const {return false;}
    virtual bool HasFeedbackPath() const {return true;}
};

////////////////////////////////////////////////////////////////////////////////

class ToyActivityGrid
	: public ToyGrid
{
public:
	ToyActivityGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags);

protected:
	virtual ToyWidget* CreateWidget();
	virtual void ApplyDefaultSettings(ToyWidget *widget, size_t index);
};

////////////////////////////////////////////////////////////////////////////////

#endif
