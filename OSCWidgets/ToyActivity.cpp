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

#include "ToyActivity.h"
#include "Utils.h"
#include "OSCParser.h"

////////////////////////////////////////////////////////////////////////////////

bool FadeActivity::sFadeTiming::operator==(const sFadeTiming &other) const
{
	return (in==other.in && hold==other.hold && out==other.out);
}

////////////////////////////////////////////////////////////////////////////////

FadeActivity::FadeActivity(QWidget *parent)
	: FadeButton_NoTouch(parent)
	, m_FadeState(FADE_OFF)
	, m_FadeElapsed(0)
{
	m_ActivityTimer = new QTimer(this);
	connect(m_ActivityTimer, SIGNAL(timeout()), this, SLOT(onUpdate()));
}

////////////////////////////////////////////////////////////////////////////////

void FadeActivity::SetText(const QString &text)
{
	if(this->text() != text)
	{
		setText(text);
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeActivity::SetFadeTiming(const sFadeTiming &fadeTiming)
{
	if(m_FadeTiming != fadeTiming)
	{
		m_FadeTiming = fadeTiming;

		if(	m_FadeState==FADE_ON &&
			m_FadeTiming.hold!=static_cast<unsigned int>(FADE_HOLD_INFINITE) &&
			!m_ActivityTimer->isActive() )
		{
			StartActivityTimer();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeActivity::SetOn(bool b)
{
	if( b )
	{
		switch( m_FadeState )
		{
			case FADE_OFF:
				m_FadeElapsed = 0;
				m_FadeState = FADE_IN;
				StartActivityTimer();
				onUpdate();
				break;

			case FADE_IN:
				onUpdate();
				break;

			case FADE_ON:
				m_FadeElapsed = 0;
				break;

			case FADE_OUT:
				m_FadeElapsed = static_cast<unsigned int>( qRound((1.0f-GetFadePercent())*m_FadeTiming.in) );
				m_FadeState = FADE_IN;
				onUpdate();
				break;
		}
	}
	else
	{
		switch( m_FadeState )
		{
			case FADE_IN:
				m_FadeElapsed = static_cast<unsigned int>( qRound((1.0f-GetFadePercent())*m_FadeTiming.out) );
				m_FadeState = FADE_OUT;
				onUpdate();
				break;

			case FADE_ON:
				m_FadeElapsed = 0;
				m_FadeState = FADE_OUT;
				StartActivityTimer();
				onUpdate();
				break;

			case FADE_OUT:
				onUpdate();
				break;

			default:
				break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeActivity::StartActivityTimer()
{
	m_ActivityEosTimer.Start();
	m_ActivityTimer->start(20);
}

////////////////////////////////////////////////////////////////////////////////

float FadeActivity::GetFadeOpacity() const
{
	switch( m_FadeState )
	{
		case FADE_IN:
			return GetFadePercent();

		case FADE_ON:
			return 1.0;

		case FADE_OUT:
			return (1.0f - GetFadePercent());

		default:
			break;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

float FadeActivity::GetFadePercent() const
{
	switch( m_FadeState )
	{
		case FADE_IN:
			return ((m_FadeTiming.in==0) ? 1.0f : (m_FadeElapsed/static_cast<float>(m_FadeTiming.in)));

		case FADE_ON:
			return ((m_FadeTiming.hold==static_cast<unsigned int>(FADE_HOLD_INFINITE))
				? 0.0
				: ((m_FadeTiming.hold==0) ? 1.0f : (m_FadeElapsed/static_cast<float>(m_FadeTiming.hold))));

		case FADE_OUT:
			return ((m_FadeTiming.out==0) ? 1.0f : (m_FadeElapsed/static_cast<float>(m_FadeTiming.out)));

		default:
			break;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void FadeActivity::onUpdate()
{
	m_FadeElapsed += m_ActivityEosTimer.Restart();

	switch( m_FadeState )
	{
		case FADE_IN:
			{
				if(GetFadePercent() >= 1.0f)
				{
					m_FadeState = FADE_ON;
					m_FadeElapsed = 0;
				}
			}
			break;

		case FADE_ON:
			{
				if(m_FadeTiming.hold == static_cast<unsigned int>(FADE_HOLD_INFINITE))
				{
					m_ActivityTimer->stop();
				}
				else if(GetFadePercent() >= 1.0f)
				{
					m_FadeState = FADE_OUT;
					m_FadeElapsed = 0;
				}
			}
			break;

		case FADE_OUT:
			{
				if(GetFadePercent() >= 1.0f)
				{
					m_FadeState = FADE_OFF;
					m_ActivityTimer->stop();
				}
			}
			break;

		default:
			m_ActivityTimer->stop();
			break;
	}

	update();
}

////////////////////////////////////////////////////////////////////////////////

void FadeActivity::paintEvent(QPaintEvent* /*event*/)
{
	QRectF r( rect() );
	r.adjust(1, 1, -1, -1);
	
	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing);

	painter.setOpacity( GetFadeOpacity() );

	const QPixmap &pixmap = m_Images[m_ImageIndex].pixmap;
	if( pixmap.isNull() )
	{
		painter.setPen(Qt::NoPen);
		painter.setBrush( palette().color(QPalette::Button) );
		painter.drawRoundedRect(r, ROUNDED, ROUNDED);
	}
	else
	{
		painter.drawPixmap(	r.x() + qRound((r.width()-pixmap.width())*0.5),
							r.y() + qRound((r.height()-pixmap.height())*0.5),
							pixmap );
	}

	painter.setOpacity(1.0);

	if(m_Hover > 0)
	{
		qreal dy = (-m_Hover * BUTTON_RAISE);
		if(dy != 0)
			r.adjust(0, 0, 0, dy);
	}
	
	QColor textColor( palette().color(QPalette::ButtonText) );

	if( !text().isEmpty() )
	{
		if( m_Label.isEmpty() )
		{
			// just text centered
			painter.setPen(textColor);
			painter.drawText(r, Qt::AlignCenter|Qt::TextWordWrap, text());
		}
		else
		{
			// both text and label text
			QRectF textRect;
			painter.setPen(textColor);
			painter.drawText(r, Qt::AlignCenter|Qt::TextWordWrap|Qt::TextDontPrint, text(), &textRect);

			QRectF labelRect;
			painter.drawText(r, Qt::AlignCenter|Qt::TextWordWrap|Qt::TextDontPrint, m_Label, &labelRect);

			qreal h = (r.height() * 0.5 * (textRect.height()/labelRect.height()));
			painter.drawText(QRectF(r.x(),r.y(),r.width(),h), Qt::AlignCenter|Qt::TextWordWrap, text());
			painter.drawText(QRectF(r.x(),r.y()+h,r.width(),r.height()-h), Qt::AlignCenter|Qt::TextWordWrap, m_Label);
		}
	}
	else if( !m_Label.isEmpty() )
	{
		// just label text centered
		painter.setPen(textColor);
		painter.drawText(r, Qt::AlignCenter|Qt::TextWordWrap, m_Label);
	}
}

////////////////////////////////////////////////////////////////////////////////

ToyActivityWidget::ToyActivityWidget(QWidget *parent)
	: ToyWidget(parent)
	, m_FadeDuration( static_cast<unsigned int>(FadeActivity::FADE_HOLD_INFINITE) )
	, m_HoldDuration( static_cast<unsigned int>(FadeActivity::FADE_HOLD_INFINITE) )
{
	m_Min = "0.001";
	m_Max = "1000.0";
	m_Min2 = m_Max2 = QString();
	m_HelpText = tr("If Min or Max blank, flash on any activity\n\nOSC Trigger:\nNo Arguments = Flash\nArgument(inside Min/Max range) = On\nArgument(outside Min/Max range) = Off\n\nOptional:\nMin2 = Fade Duration (ms)\nMax2 = Hold Duration (ms)");

	m_Widget = new FadeActivity(this);	

	QPalette pal( m_Widget->palette() );
	pal.setColor(QPalette::Button, QColor(54,139,255));
	pal.setColor(QPalette::ButtonText, QColor(255,255,255));
	m_Widget->setPalette(pal);

	m_Color = pal.color(QPalette::Button);
	m_TextColor = pal.color(QPalette::ButtonText);
}

////////////////////////////////////////////////////////////////////////////////

void ToyActivityWidget::SetText(const QString &text)
{
	ToyWidget::SetText(text);
	static_cast<FadeActivity*>(m_Widget)->setText(m_Text);
}

////////////////////////////////////////////////////////////////////////////////

void ToyActivityWidget::SetImagePath(const QString &imagePath)
{
	ToyWidget::SetImagePath(imagePath);
	static_cast<FadeActivity*>(m_Widget)->SetImagePath(0, m_ImagePath);
}

////////////////////////////////////////////////////////////////////////////////

void ToyActivityWidget::SetColor(const QColor &color)
{
	ToyWidget::SetColor(color);
	QPalette pal( m_Widget->palette() );
	pal.setColor(QPalette::Button, m_Color);
	m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyActivityWidget::SetTextColor(const QColor &textColor)
{
	ToyWidget::SetTextColor(textColor);
	QPalette pal( m_Widget->palette() );
	pal.setColor(QPalette::ButtonText, m_TextColor);
	m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyActivityWidget::SetLabel(const QString &label)
{
	static_cast<FadeActivity*>(m_Widget)->SetLabel(label);
}

////////////////////////////////////////////////////////////////////////////////

void ToyActivityWidget::SetMin2(const QString &n)
{
	ToyWidget::SetMin2(n);

	if( m_Min2.isEmpty() )
		m_FadeDuration = static_cast<unsigned int>(FadeActivity::FADE_HOLD_INFINITE);
	else
		m_FadeDuration = m_Min2.toUInt();
}

////////////////////////////////////////////////////////////////////////////////

void ToyActivityWidget::SetMax2(const QString &n)
{
	ToyWidget::SetMax2(n);

	if( m_Max2.isEmpty() )
		m_HoldDuration = static_cast<unsigned int>(FadeActivity::FADE_HOLD_INFINITE);
	else
		m_HoldDuration = m_Max2.toUInt();
}

////////////////////////////////////////////////////////////////////////////////

void ToyActivityWidget::MakeFadeTiming(bool flash, FadeActivity::sFadeTiming &fadeTiming)
{
	if(m_FadeDuration == static_cast<unsigned int>(FadeActivity::FADE_HOLD_INFINITE))
	{
		// not specified, use default
		fadeTiming.in = fadeTiming.out = 200;
	}
	else
	{
		// use specified duration
		fadeTiming.in = fadeTiming.out = m_FadeDuration;
	}

	if( flash )
	{
		if(m_HoldDuration == static_cast<unsigned int>(FadeActivity::FADE_HOLD_INFINITE))
		{
			// not specified, use default
			fadeTiming.hold = 0;
		}
		else
		{
			// use specified duration
			fadeTiming.hold = m_HoldDuration;
		}
	}
	else
	{
		// hold forever
		fadeTiming.hold = static_cast<unsigned int>(FadeActivity::FADE_HOLD_INFINITE);
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyActivityWidget::Recv(const QString &path, const OSCArgument *args, size_t count)
{
	if(path == m_FeedbackPath)
    {
		FadeActivity *activity = static_cast<FadeActivity*>(m_Widget);

		if(	m_Min.isEmpty() ||
			m_Max.isEmpty() ||
			args==0 ||
			count==0 )
		{
			FadeActivity::sFadeTiming fadeTiming;
			MakeFadeTiming(/*flash*/true, fadeTiming);
			activity->SetFadeTiming(fadeTiming);
			activity->SetOn(true);
		}
		else
		{
			bool on = false;

			float f = 0;
			if( args[0].GetFloat(f) )
			{
				float rangeMin = m_Min.toFloat();
				float rangeMax = m_Max.toFloat();
				if(rangeMin > rangeMax)
					qSwap(rangeMin, rangeMax);
				if(f>=rangeMin && f<=rangeMax)
					on = true;
			}

			FadeActivity::sFadeTiming fadeTiming;
			MakeFadeTiming(/*flash*/false, fadeTiming);
			activity->SetFadeTiming(fadeTiming);
			activity->SetOn(on);
		}

    }
    else
    {
        std::string str;
        if(args && count>0)
        {
            if( !args[0].GetString(str) )
                str.clear();
        }

        SetLabel( QString::fromUtf8(str.c_str()) );
    }
}

////////////////////////////////////////////////////////////////////////////////

ToyActivityGrid::ToyActivityGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags)
	: ToyGrid(TOY_ACTIVITY_GRID, pClient, parent, flags)
{
}

////////////////////////////////////////////////////////////////////////////////

ToyWidget* ToyActivityGrid::CreateWidget()
{
	ToyActivityWidget *w = new ToyActivityWidget(this);
	return w;
}

////////////////////////////////////////////////////////////////////////////////

void ToyActivityGrid::ApplyDefaultSettings(ToyWidget *widget, size_t index)
{
	widget->SetText( QString::number(index) );

	QString pathName;
	Toy::GetDefaultPathName(m_Type, pathName);

	QString path = QString("/%1/%2").arg(pathName).arg(index);
	widget->SetFeedbackPath(path);
}

////////////////////////////////////////////////////////////////////////////////
