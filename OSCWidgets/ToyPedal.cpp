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

#include "ToyPedal.h"
#include "OSCParser.h"
#include "Utils.h"

#define PEDAL_TIMEFRAME	5000

////////////////////////////////////////////////////////////////////////////////

FadePedal::FadePedal(QWidget *parent)
	: FadeButton(parent)
	, m_State(STATE_IDLE)
	, m_Elapsed(0)
	, m_UpDuration(250)
	, m_DownDuration(1000)
{
	connect(this, SIGNAL(pressed()), this, SLOT(onPressed()));
	connect(this, SIGNAL(released()), this, SLOT(onReleased()));
}

////////////////////////////////////////////////////////////////////////////////

void FadePedal::Reset()
{
	SetState(STATE_IDLE);
}

////////////////////////////////////////////////////////////////////////////////

void FadePedal::SetState(EnumState state)
{
	if(m_State != state)
	{
		if(m_State==STATE_UP && state==STATE_DOWN)
		{
			// early down, so maintain percent
			float percent = ((m_UpDuration==0) ? 1.0f : (m_Elapsed/static_cast<float>(m_UpDuration)));
			m_Elapsed = static_cast<unsigned int>( qRound(m_DownDuration*(1.0f-percent)) );
		}
		else if(m_State==STATE_DOWN && state==STATE_UP)
		{
			// early up, so maintain percent
			float percent = ((m_DownDuration==0) ? 1.0f : (m_Elapsed/static_cast<float>(m_DownDuration)));
			m_Elapsed = static_cast<unsigned int>( qRound(m_UpDuration*(1.0f-percent)) );
		}
		else
			m_Elapsed = 0;
		
		m_State = state;
		Update(0);
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadePedal::Press()
{
	SetState(STATE_UP);
}

////////////////////////////////////////////////////////////////////////////////

void FadePedal::Release()
{
	SetState(STATE_DOWN);
}

////////////////////////////////////////////////////////////////////////////////

void FadePedal::Update(unsigned int ms)
{
	switch( m_State )
	{
		case STATE_UP:
			{
				m_Elapsed += ms;
				if(m_UpDuration==0 || m_Elapsed>=m_UpDuration)
				{
					Tick(1.0f);
					SetState(STATE_PEAK);
				}
				else
				{
					float percent = m_Elapsed/static_cast<float>(m_UpDuration);
					percent = ((sinf(PI_PLUS_PI_2 + percent*M_PI)+1.0f) * 0.5f);
					Tick(percent);
				}
			}
			break;
			
		case STATE_DOWN:
			{
				m_Elapsed += ms;
				if(m_DownDuration==0 || m_Elapsed>=m_DownDuration)
				{
					Tick(0);
					SetState(STATE_IDLE);
				}
				else
				{
					float percent = (1.0f - m_Elapsed/static_cast<float>(m_DownDuration));
					percent = ((sinf(PI_PLUS_PI_2 + percent*M_PI)+1.0f) * 0.5f);
					Tick(percent);
				}
			}
			break;

		default:
			break;
	}
	
	// age ticks, but always keep last ticked value
	if( !m_Ticks.empty() )
	{
		size_t removeCount = 0;
		for(TICKS::iterator i=m_Ticks.begin(); i!=m_Ticks.end(); i++)
		{
			if(i->elapsed >= PEDAL_TIMEFRAME)
				removeCount++;
			else
				i->elapsed += ms;
		}
	
		// leave most recent expired, so it can be displayed
		if(removeCount > 1)
		{
			--removeCount;
			for(size_t i=0; i<removeCount; i++)
				m_Ticks.erase( m_Ticks.begin() );
		}
		
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadePedal::Tick(float value)
{
	if( !m_Ticks.empty() )
	{
		const sTick &prevTick = m_Ticks.back();
		if(prevTick.value!=value && prevTick.state!=m_State)
			m_Ticks.push_back( sTick(m_State,0,prevTick.value) );	// zero step when state changes
	}
	
	m_Ticks.push_back( sTick(m_State,0,value) );
	emit tick(value);
}

////////////////////////////////////////////////////////////////////////////////

void FadePedal::resizeEvent(QResizeEvent *event)
{
	m_Canvas = QImage(size(), QImage::Format_ARGB32);
	FadeButton::resizeEvent(event);
}

////////////////////////////////////////////////////////////////////////////////

void FadePedal::paintEvent(QPaintEvent* /*event*/)
{
	float lastValue = (m_Ticks.empty() ? 0 : m_Ticks.back().value);
	float angle = (35 * lastValue);

	QRectF r( rect() );
	r.adjust(1, 1, -1, -1);
	
	QPainter painter;
	m_Canvas.fill(0);
	if( painter.begin(&m_Canvas) )
	{
		painter.setRenderHints(QPainter::Antialiasing);
	
		float brightness = m_Click;
		if(m_Hover > 0)
			brightness += (m_Hover*0.2f);

		const QPixmap &pixmap = m_Images[m_ImageIndex].pixmap;
		if( !pixmap.isNull() )
		{
			painter.setOpacity(1.0-(brightness*0.5));
			painter.drawPixmap(	r.x() + qRound((r.width()-pixmap.width())*0.5),
								r.y() + qRound((r.height()-pixmap.height())*0.5),
								pixmap );
			painter.setOpacity(1.0);
		}
	
		QColor brushColor( palette().color(QPalette::Button) );
		if(m_Hover > 0)
		{
			qreal t = (brightness * BUTTON_BRIGHTESS);
			brushColor.setRedF( qMin(brushColor.redF()+t,1.0) );
			brushColor.setGreenF( qMin(brushColor.greenF()+t,1.0) );
			brushColor.setBlueF( qMin(brushColor.blueF()+t,1.0) );
		}
		
		QColor lineColor(brushColor);
		Utils::MakeContrastingColor(1.0f, lineColor);
	
		if( !pixmap.isNull() )
		{
			QPainterPath clip;
			clip.addRoundedRect(r, ROUNDED, ROUNDED);
			painter.setClipPath(clip);

			painter.drawPixmap(	r.x() + qRound((r.width()-pixmap.width())*0.5),
								r.y() + qRound((r.height()-pixmap.height())*0.5),
								pixmap );

			painter.setClipping(false);
		}

		painter.setBrush(brushColor);
		painter.setPen( QPen(lineColor,BORDER) );
		painter.drawRoundedRect(r, ROUNDED, ROUNDED);
	
		if( m_Ticks.empty() )
			m_Points.clear();
		else
			m_Points.resize(static_cast<int>(m_Ticks.size() + 1));	// end point
	
		if( !m_Points.empty() )
		{
			float lineHeight = (r.height() * 0.8f);
			float lineX = (r.right() + HALF_BORDER);
			float lineY = (r.bottom() - 0.5f*(r.height()-lineHeight));
			float lineWidth = (r.width() + BORDER);
		
			float toPercent = 1.0f/PEDAL_TIMEFRAME;

			for(size_t i=0; i<m_Ticks.size(); i++)
			{
				const sTick &t = m_Ticks[i];
				QPointF &p = m_Points[static_cast<int>(i)];	// end point
			
				float percent = (t.elapsed * toPercent);
				p.setX(lineX - lineWidth*percent);
				p.setY(lineY - lineHeight*t.value);
			}
		
			// end point
			m_Points[m_Points.size()-1] = QPointF(lineX, m_Points[m_Points.size()-2].y());
		
			painter.drawPolyline(m_Points);
		}
	
		if(m_Hover > 0)
		{
			qreal dy = (-m_Hover * BUTTON_RAISE);
			if(dy != 0)
				r.adjust(0, 0, 0, dy);
		}
	
		QColor textColor( palette().color(QPalette::ButtonText) );
		if( !isEnabled() )
			textColor = textColor.darker(150);

		if( !text().isEmpty() )
		{
			if( m_Label.isEmpty() )
			{
				// just text centered
				painter.setFont( font() );
				painter.setPen(textColor);
				painter.drawText(r, Qt::AlignCenter|Qt::TextWordWrap, text());
			}
			else
			{
				// both text and label text
				QRectF textRect;
				painter.setFont( font() );
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
			painter.setFont( font() );
			painter.setPen(textColor);
			painter.drawText(r, Qt::AlignCenter|Qt::TextWordWrap, m_Label);
		}

		painter.end();
	}

	if( painter.begin(this) )
	{
		if(angle > 0.00001)
		{
			qreal center = (m_Canvas.width() * 0.5);
			QMatrix4x4 matrix;
			matrix.translate(-center, m_Canvas.height());
			matrix.rotate(angle, 1.0, 0, 0);

			painter.translate(center, 0);
			painter.setTransform(matrix.toTransform(300.0), /*combine*/true);
			painter.setRenderHint(QPainter::SmoothPixmapTransform);
			painter.drawImage(0, -m_Canvas.height(), m_Canvas);
		}
		else
			painter.drawImage(0, 0, m_Canvas);

		painter.end();
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadePedal::onPressed()
{
	Press();
}

////////////////////////////////////////////////////////////////////////////////

void FadePedal::onReleased()
{
	Release();
}

////////////////////////////////////////////////////////////////////////////////

ToyPedalWidget::ToyPedalWidget(QWidget *parent)
	: ToyWidget(parent)
{
	m_HelpText = tr("Min = Pedal Down\nMax = Pedal Up\n\nMin2 = Pedal Down Time (ms)\nMax2 = Pedal Up Time (ms)\n\nOSC Trigger:\nNo Arguments = Click\nArgument(1) = Press\nArgument(0) = Release");
	
	FadePedal *pedal = new FadePedal(this);
	
	m_Min2 = QString::number( pedal->GetUpDuration() );
	m_Max2 = QString::number( pedal->GetDownDuration() );

	m_Widget = pedal;
	connect(m_Widget, SIGNAL(tick(float)), this, SLOT(onTick(float)));
	
	QPalette pal( m_Widget->palette() );
	m_Color = pal.color(QPalette::Button);
	m_TextColor = pal.color(QPalette::ButtonText);
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalWidget::SetText(const QString &text)
{
	ToyWidget::SetText(text);
	static_cast<FadePedal*>(m_Widget)->setText(m_Text);
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalWidget::SetImagePath(const QString &imagePath)
{
	ToyWidget::SetImagePath(imagePath);
	static_cast<FadePedal*>(m_Widget)->SetImagePath(0, m_ImagePath);
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalWidget::SetColor(const QColor &color)
{
	ToyWidget::SetColor(color);
	QPalette pal( m_Widget->palette() );
	pal.setColor(QPalette::Button, m_Color);
	m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalWidget::SetTextColor(const QColor &textColor)
{
	ToyWidget::SetTextColor(textColor);
	QPalette pal( m_Widget->palette() );
	pal.setColor(QPalette::ButtonText, m_TextColor);
	m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalWidget::SetMin2(const QString &n)
{
	ToyWidget::SetMin2(n);
	unsigned int duration = m_Min2.toUInt();
	static_cast<FadePedal*>(m_Widget)->SetUpDuration(duration);
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalWidget::SetMax2(const QString &n)
{
	ToyWidget::SetMax2(n);
	unsigned int duration = m_Max2.toUInt();
	static_cast<FadePedal*>(m_Widget)->SetDownDuration(duration);
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalWidget::SetLabel(const QString &label)
{
	static_cast<FadePedal*>(m_Widget)->SetLabel(label);
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalWidget::Recv(const QString &path, const OSCArgument *args, size_t count)
{
    if(path == m_TriggerPath)
	{
		FadePedal *pedal = static_cast<FadePedal*>(m_Widget);

		bool edge = false;
		
		if(args && count>0)
		{
			bool press = false;
			if( args[0].GetBool(press) )
			{
				edge = true;
				
				if( press )
					pedal->Press();
				else
					pedal->Release();
			}
		}
		
		if( !edge )
		{
			pedal->Press();
			pedal->Release();
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

void ToyPedalWidget::Update(unsigned int ms)
{
	static_cast<FadePedal*>(m_Widget)->Update(ms);
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalWidget::onTick(float value)
{
	emit tick(this, value);
}

////////////////////////////////////////////////////////////////////////////////

ToyPedalGrid::ToyPedalGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags)
	: ToyGrid(TOY_PEDAL_GRID, pClient, parent, flags)
{
	m_Timer = new QTimer(this);
	connect(m_Timer, SIGNAL(timeout()), this, SLOT(onTimeout()));

	m_Press = new FadeButton(this);
	m_Press->setText( tr("Press All") );
	m_Press->resize( m_Press->sizeHint() );
	connect(m_Press, SIGNAL(pressed()), this, SLOT(onPressPressed()));
	connect(m_Press, SIGNAL(released()), this, SLOT(onPressReleased()));
	
	StartTimer();
}

////////////////////////////////////////////////////////////////////////////////

ToyWidget* ToyPedalGrid::CreateWidget()
{
	ToyPedalWidget *w = new ToyPedalWidget(this);
	connect(w, SIGNAL(tick(ToyPedalWidget*,float)), this, SLOT(onTick(ToyPedalWidget*,float)));
	return w;
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalGrid::StartTimer()
{
	m_ElapsedTimer.Start();
	m_Timer->start( Toy::GetPedalRefreshRateMS() );
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalGrid::StopTimer()
{
	m_Timer->stop();
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalGrid::UpdateLayout()
{
	QRect r( rect() );
	
	if(m_List.size() > 1)
	{
		m_Press->setGeometry(MARGIN2, r.bottom()-MARGIN-m_Press->height(), r.width() - MARGIN2*2, m_Press->height());
		r.adjust(0, 0, 0, -m_Press->height()-MARGIN);
		m_Press->show();
	}
	else
		m_Press->hide();
	
	UpdateLayoutForRect( r );
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalGrid::AutoSize(const QSize &widgetSize)
{
	int pressHeight = ((m_List.size()>1)
		? (m_Press->height() + MARGIN)
		: 0);

	resize(m_GridSize.width()*widgetSize.width() + MARGIN2, m_GridSize.height()*widgetSize.height() + pressHeight + MARGIN2);
	UpdateLayout();
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalGrid::onTick(ToyPedalWidget *pedal, float value)
{
	if(	m_pClient &&
		pedal &&
		!pedal->GetPath().isEmpty() )
	{
		QString path( pedal->GetPath() );
		bool local = Utils::MakeLocalOSCPath(false, path);

		OSCPacketWriter packetWriter( path.toUtf8().constData() );
		if(!pedal->GetMin().isEmpty() || !pedal->GetMax().isEmpty())
		{
			float minValue = pedal->GetMin().toFloat();
			float maxValue = pedal->GetMax().toFloat();
			value = (minValue + (maxValue-minValue)*value);
			packetWriter.AddFloat32(value);
		}

		size_t size;
		char *packet = packetWriter.Create(size);
		if( packet )
			m_pClient->ToyClient_Send(local, packet, size);
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalGrid::onTimeout()
{
	unsigned int ms = m_ElapsedTimer.Restart();
	for(WIDGET_LIST::const_iterator i=m_List.begin(); i!=m_List.end(); i++)
		static_cast<ToyPedalWidget*>(*i)->Update(ms);
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalGrid::onPressPressed()
{
	for(WIDGET_LIST::const_iterator i=m_List.begin(); i!=m_List.end(); i++)
		static_cast<ToyPedalWidget*>(*i)->GetPedal().Press();
}

////////////////////////////////////////////////////////////////////////////////

void ToyPedalGrid::onPressReleased()
{
	for(WIDGET_LIST::const_iterator i=m_List.begin(); i!=m_List.end(); i++)
		static_cast<ToyPedalWidget*>(*i)->GetPedal().Release();
}

////////////////////////////////////////////////////////////////////////////////
