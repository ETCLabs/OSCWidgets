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

#include "ToySlider.h"
#include "OSCParser.h"
#include "Utils.h"

////////////////////////////////////////////////////////////////////////////////

FadeSlider::FadeSlider(QWidget *parent)
	: FadeButton(parent)
	, m_Percent(0)
	, m_MouseDown(false)
	, m_TextMargin(0)
	, m_LabelMargin(0)
{
	m_RecvPercent.pending = false;
	m_RecvPercent.percent = 0;
	m_RecvPercent.timer = new QTimer(this);
	connect(m_RecvPercent.timer, SIGNAL(timeout()), this, SLOT(onRecvTimeout()));
}

////////////////////////////////////////////////////////////////////////////////

void FadeSlider::SetText(const QString &text)
{
	if(this->text() != text)
	{
		setText(text);
		UpdateMargins();
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeSlider::SetLabel(const QString &label)
{
	if(m_Label != label)
	{
		m_Label = label;
		UpdateMargins();
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeSlider::SetPercent(float percent)
{
	SetPercentPrivate(percent, /*user*/false);
}

////////////////////////////////////////////////////////////////////////////////

void FadeSlider::SetPercentPrivate(float percent, bool user)
{
	if(m_Percent != percent)
	{
		m_Percent = percent;
		if( user )
			emit percentChanged(m_Percent);
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeSlider::RecvPercent(float percent)
{
	if( m_MouseDown )
	{
		if(!m_RecvPercent.pending || m_RecvPercent.percent!=percent)
		{
			m_RecvPercent.percent = percent;
			m_RecvPercent.pending = true;
			update();
		}
	}
	else
	{
		m_RecvPercent.pending = false;
		m_RecvPercent.timer->stop();
		SetPercent(percent);
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeSlider::TriggerPercent(float percent)
{
	if( !m_MouseDown )
		SetPercentPrivate(percent, /*user*/true);
}

////////////////////////////////////////////////////////////////////////////////

void FadeSlider::AutoSizeFont()
{
	QFont fnt( font() );
	int d = qMin(width(), qMax(m_TextMargin,m_LabelMargin));
	fnt.setPixelSize( qMax(10,qRound(d*0.333)) );
	setFont(fnt);
}

////////////////////////////////////////////////////////////////////////////////

int FadeSlider::PercentToPos(float percent)
{
	return (m_TextMargin + qRound((height()-m_LabelMargin-m_TextMargin)*(1.0f-percent)));
}

////////////////////////////////////////////////////////////////////////////////

float FadeSlider::PosToPercent(int pos)
{
	float h = (height() - m_TextMargin - m_LabelMargin);
	float percent = ((h==0) ? 0 : (1.0f - (pos-m_TextMargin)/h));
	return qBound(0.0f, percent, 1.0f);
}

////////////////////////////////////////////////////////////////////////////////

void FadeSlider::UpdateMargins()
{
	int textMargin = (text().isEmpty() ? 0 : qRound(height()*0.1));
	int labelMargin = (m_Label.isEmpty() ? 0 : qRound(height()*0.1));

	if(m_TextMargin!=textMargin || m_LabelMargin!=labelMargin)
	{
		m_TextMargin = textMargin;
		m_LabelMargin = labelMargin;
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeSlider::resizeEvent(QResizeEvent *event)
{
	m_Canvas = QImage(size(), QImage::Format_ARGB32);
	UpdateMargins();
	FadeButton::resizeEvent(event);
}

////////////////////////////////////////////////////////////////////////////////

void FadeSlider::mousePressEvent(QMouseEvent *event)
{
	FadeButton::mousePressEvent(event);
	if(event->button() != Qt::RightButton)
	{
		m_MouseDown = true;
		m_RecvPercent.timer->stop();
		SetPercentPrivate(PosToPercent(event->pos().y()), /*user*/true);
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeSlider::mouseMoveEvent(QMouseEvent *event)
{
	FadeButton::mouseMoveEvent(event);
	if( m_MouseDown )
		SetPercentPrivate(PosToPercent(event->pos().y()), /*user*/true);
}

////////////////////////////////////////////////////////////////////////////////

void FadeSlider::mouseReleaseEvent(QMouseEvent *event)
{
	FadeButton::mouseReleaseEvent(event);
	
	if( m_MouseDown )
	{
		m_MouseDown = false;
		if(m_RecvPercent.pending && m_RecvPercent.percent!=m_Percent)
			m_RecvPercent.timer->start( Toy::GetFeedbackDelayMS() );
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeSlider::paintEvent(QPaintEvent* /*event*/)
{
	m_Canvas.fill(0);

	QRectF r( rect() );
	r.adjust(1+HALF_BORDER, 1+HALF_BORDER+m_TextMargin, -1-HALF_BORDER, -1-HALF_BORDER-m_LabelMargin);
	
	QPainter painter;
	if( painter.begin(&m_Canvas) )
	{
		painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing);
	
		float brightness = m_Click;
		if(m_Hover > 0)
			brightness += (m_Hover*0.2f);

		QColor color( palette().color(QPalette::Button) );
		if(brightness > 0)
		{
			qreal t = (brightness * BUTTON_BRIGHTESS);
			color.setRedF( qMin(color.redF()+t,1.0) );
			color.setGreenF( qMin(color.greenF()+t,1.0) );
			color.setBlueF( qMin(color.blueF()+t,1.0) );
		}

		QPainterPath clip;
		clip.addRoundedRect(r, ROUNDED, ROUNDED);
		painter.setClipPath(clip);

		const QPixmap &pixmap = m_Images[m_ImageIndex].pixmap;
		if( !pixmap.isNull() )
		{
			painter.drawPixmap(	r.x() + qRound((r.width()-pixmap.width())*0.5),
								r.y() + qRound((r.height()-pixmap.height())*0.5),
								pixmap );
		}

		painter.setClipping(false);

		painter.setPen( QPen(color,BORDER,Qt::SolidLine,Qt::SquareCap,Qt::RoundJoin) );
		painter.setBrush(Qt::NoBrush);
		painter.drawRoundedRect(r, ROUNDED, ROUNDED);

		painter.setClipping(true);

		if( !pixmap.isNull() )
			painter.setOpacity(0.8);

		int y = PercentToPos(m_Percent);

		if( m_RecvPercent.pending )
		{
			int w = qRound(r.width() * 0.75);
			painter.fillRect(QRect(r.x(),y,w,height()-y+1), color);

			int y2 = PercentToPos(m_RecvPercent.percent);
			int x = (r.x() + w);
			painter.fillRect(QRect(x,y2,r.width()-x+1,height()-y2+1), color.darker(125));
		}
		else
			painter.fillRect(QRect(r.x(),y,r.width(),height()-y+1), color);

		painter.setOpacity(1.0);

		int hoverRaise = qRound(m_Hover * BUTTON_RAISE);
		if(m_Hover > 0)
		{
			y -= hoverRaise;
			painter.setOpacity(m_Hover);

			QFont fnt( font() );
			fnt.setPixelSize( qRound(width()*0.4) );
			painter.setFont(fnt);

			QString num = QString::number( qRound(m_Percent*100) );

			QRectF textRect(r.x(), y-r.height(), r.width(), r.height());
			painter.drawText(textRect, Qt::AlignHCenter|Qt::AlignBottom, num);

			textRect = QRectF(r.x(), y, r.width(), r.height());
			painter.setPen(Qt::white);
			painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
			painter.drawText(textRect, Qt::AlignHCenter|Qt::AlignBottom, num);
			painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
			painter.setOpacity(1.0);
		}

		painter.end();

		if( painter.begin(this) )
		{
			painter.drawImage(0, 0, m_Canvas);
			
			if( !text().isEmpty() )
			{
				painter.setFont( font() );
				painter.setPen( palette().color(QPalette::ButtonText) );
				painter.drawText(QRect(0,0,width(),r.y()-hoverRaise), Qt::AlignCenter|Qt::TextWordWrap|Qt::TextDontClip, text());

				if( !m_Label.isEmpty() )
					painter.drawText(QRect(0,r.bottom(),width(),height()-r.bottom()+hoverRaise), Qt::AlignCenter|Qt::TextWordWrap|Qt::TextDontClip, m_Label);
			}
			else if( !m_Label.isEmpty() )
			{
				painter.setFont( font() );
				painter.setPen( palette().color(QPalette::ButtonText) );
				painter.drawText(QRect(0,r.bottom(),width(),height()-r.bottom()+hoverRaise), Qt::AlignCenter|Qt::TextWordWrap|Qt::TextDontClip, m_Label);
			}
			
			painter.end();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeSlider::onRecvTimeout()
{
	m_RecvPercent.timer->stop();

	if( m_RecvPercent.pending )
	{
		m_RecvPercent.pending = false;
		SetPercent( m_RecvPercent.percent );
	}
}

////////////////////////////////////////////////////////////////////////////////

ToySliderWidget::ToySliderWidget(QWidget *parent)
	: ToyWidget(parent)
{
	m_HelpText = tr("Min = Fader Bottom\nMax = Fader Top\n\nOSC Trigger:\nArgument = Fader Position");

	m_Widget = new FadeSlider(this);
	connect(m_Widget, SIGNAL(percentChanged(float)), this, SLOT(onPercentChanged(float)));
	
	QPalette pal( m_Widget->palette() );
	m_Color = pal.color(QPalette::Button);
	m_TextColor = pal.color(QPalette::ButtonText);
}

////////////////////////////////////////////////////////////////////////////////

float ToySliderWidget::GetPercent() const
{
	return static_cast<FadeSlider*>(m_Widget)->GetPercent();
}

////////////////////////////////////////////////////////////////////////////////

void ToySliderWidget::SetText(const QString &text)
{
	ToyWidget::SetText(text);
	static_cast<FadeSlider*>(m_Widget)->SetText(m_Text);
}

////////////////////////////////////////////////////////////////////////////////

void ToySliderWidget::SetImagePath(const QString &imagePath)
{
	ToyWidget::SetImagePath(imagePath);
	static_cast<FadeSlider*>(m_Widget)->SetImagePath(0, m_ImagePath);
}

////////////////////////////////////////////////////////////////////////////////

void ToySliderWidget::SetColor(const QColor &color)
{
	ToyWidget::SetColor(color);
	QPalette pal( m_Widget->palette() );
	pal.setColor(QPalette::Button, m_Color);
	m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToySliderWidget::SetTextColor(const QColor &textColor)
{
	ToyWidget::SetTextColor(textColor);
	QPalette pal( m_Widget->palette() );
	pal.setColor(QPalette::ButtonText, m_TextColor);
	m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToySliderWidget::SetLabel(const QString &label)
{
	static_cast<FadeSlider*>(m_Widget)->SetLabel(label);
}

////////////////////////////////////////////////////////////////////////////////

void ToySliderWidget::Recv(const QString &path, const OSCArgument *args, size_t count)
{
	if(args && count>0)
	{
		bool isFeedback = (path == m_FeedbackPath);
		bool isTrigger = (!isFeedback && path==m_TriggerPath);
		if(isFeedback || isTrigger)
		{
			FadeSlider *slider = static_cast<FadeSlider*>(m_Widget);

			float value = 0;
			if( !args[0].GetFloat(value) )
				value = 0;

			float minValue = m_Min.toFloat();
			float maxValue = m_Max.toFloat();
			float range = (maxValue - minValue);
			value = ((range==0) ? 0 : (value-minValue)/range);
			if(value < 0)
				value = 0;
			else if(value > 1.0f)
				value = 1.0f;

			if( isFeedback )
				slider->RecvPercent(value);
			else
				slider->TriggerPercent(value);
		}
		else
		{
			std::string str;
			if( !args[0].GetString(str) )
				str.clear();

			SetLabel( QString::fromUtf8(str.c_str()) );
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToySliderWidget::onPercentChanged(float /*percent*/)
{
	emit percentChanged(this);
}

////////////////////////////////////////////////////////////////////////////////

ToySliderGrid::ToySliderGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags)
	: ToyGrid(TOY_SLIDER_GRID, pClient, parent, flags)
{
}

////////////////////////////////////////////////////////////////////////////////

ToyWidget* ToySliderGrid::CreateWidget()
{
	ToySliderWidget *w = new ToySliderWidget(this);
	connect(w, SIGNAL(percentChanged(ToySliderWidget*)), this, SLOT(onPercentChanged(ToySliderWidget*)));
	return w;
}

////////////////////////////////////////////////////////////////////////////////

void ToySliderGrid::onPercentChanged(ToySliderWidget *slider)
{
	if(	m_pClient &&
		slider &&
		!slider->GetPath().isEmpty() )
	{
		QString path( slider->GetPath() );
		bool local = Utils::MakeLocalOSCPath(false, path);

		OSCPacketWriter packetWriter( path.toUtf8().constData() );
		
		if(!slider->GetMin().isEmpty() || !slider->GetMax().isEmpty())
		{
			float minValue = slider->GetMin().toFloat();
			float maxValue = slider->GetMax().toFloat();
			float value = (minValue + (maxValue-minValue)*slider->GetPercent());
			packetWriter.AddFloat32(value);
		}
			
		size_t size;
		char *packet = packetWriter.Create(size);
		if( packet )
			m_pClient->ToyClient_Send(local, packet, size);
	}
}

////////////////////////////////////////////////////////////////////////////////
