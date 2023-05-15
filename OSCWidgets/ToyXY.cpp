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

#include "ToyXY.h"
#include "OSCParser.h"
#include "Utils.h"

////////////////////////////////////////////////////////////////////////////////

FadeXY::FadeXY(QWidget *parent)
	: FadeButton(parent)
	, m_Pos(0.5, 0.5)
	, m_MouseDown(false)
	, m_TextMargin(0)
	, m_LabelMargin(0)
{
	m_RecvPos.pending = false;
	m_RecvPos.pos = QPointF(0, 0);
	m_RecvPos.timer = new QTimer(this);
	connect(m_RecvPos.timer, SIGNAL(timeout()), this, SLOT(onRecvTimeout()));
}

////////////////////////////////////////////////////////////////////////////////

void FadeXY::SetText(const QString &text)
{
	if(this->text() != text)
	{
		setText(text);
		UpdateMargins();
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeXY::SetLabel(const QString &label)
{
	if(m_Label != label)
	{
		m_Label = label;
		UpdateMargins();
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeXY::SetPos(const QPointF &pos)
{
	SetPosPrivate(pos, /*user*/false);
}

////////////////////////////////////////////////////////////////////////////////

void FadeXY::SetPosPrivate(const QPointF &pos, bool user)
{
	if(m_Pos != pos)
	{
		m_Pos = pos;
		if( user )
			emit posChanged(m_Pos);
		update();
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeXY::RecvPos(const QPointF &pos)
{
	if( m_MouseDown )
	{
		if(!m_RecvPos.pending || m_RecvPos.pos!=pos)
		{
			m_RecvPos.pos = pos;
			m_RecvPos.pending = true;
			update();
		}
	}
	else
	{
		m_RecvPos.pending = false;
		m_RecvPos.timer->stop();
		SetPos(pos);
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeXY::TriggerPos(const QPointF &pos)
{
	if( !m_MouseDown )
		SetPosPrivate(pos, /*user*/true);
}

////////////////////////////////////////////////////////////////////////////////

void FadeXY::AutoSizeFont()
{
	QFont fnt( font() );
	int d = qMin(width(), qMax(m_TextMargin,m_LabelMargin));
	fnt.setPixelSize( qMax(10,qRound(d*0.333)) );
	setFont(fnt);
}

////////////////////////////////////////////////////////////////////////////////

QPoint FadeXY::PosToWidgetPos(const QPointF &pos)
{
    return QPoint(	qRound((double)width() * pos.x()),
                    m_TextMargin + qRound((double)height()-m_LabelMargin-m_TextMargin)*(1.0f-pos.y()) );
}

////////////////////////////////////////////////////////////////////////////////

QPointF FadeXY::WidgetPosToPos(const QPoint &pos)
{
	float w = width();
	float h = (height() - m_TextMargin - m_LabelMargin);
	float wPercent = ((w==0) ? 0 : (pos.x()/w));
	float hPercent = ((h==0) ? 0 : (1.0f - (pos.y()-m_TextMargin)/h));
	return QPointF(qBound(0.0f,wPercent,1.0f), qBound(0.0f,hPercent,1.0f));
}

////////////////////////////////////////////////////////////////////////////////

void FadeXY::ClipWidgetPosForPainting(const QRectF &r, QPoint &pos) const
{
	int minX = (r.x() + BORDER + 1);
	if(pos.x() < minX)
	{
		pos.setX(minX);
	}
	else
	{
		int maxX = (r.right() - BORDER - 1);
		if(pos.x() > maxX)
			pos.setX(maxX);
	}

	int minY = (r.y() + BORDER + 1);
	if(pos.y() < minY)
	{
		pos.setY(minY);
	}
	else
	{
		int maxY = (r.bottom() - BORDER - 1);
		if(pos.y() > maxY)
			pos.setY(maxY);
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeXY::UpdateMargins()
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

void FadeXY::resizeEvent(QResizeEvent *event)
{
	m_Canvas = QImage(size(), QImage::Format_ARGB32);
	UpdateMargins();
	FadeButton::resizeEvent(event);
}

////////////////////////////////////////////////////////////////////////////////

void FadeXY::mousePressEvent(QMouseEvent *event)
{
	FadeButton::mousePressEvent(event);
	if(event->button() != Qt::RightButton)
	{
		m_MouseDown = true;
		m_RecvPos.timer->stop();
		SetPosPrivate(WidgetPosToPos(event->pos()), /*user*/true);
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeXY::mouseMoveEvent(QMouseEvent *event)
{
	FadeButton::mouseMoveEvent(event);
	SetPosPrivate(WidgetPosToPos(event->pos()), /*user*/true);
}

////////////////////////////////////////////////////////////////////////////////

void FadeXY::mouseReleaseEvent(QMouseEvent *event)
{
	FadeButton::mouseReleaseEvent(event);
	m_MouseDown = false;
	if(m_RecvPos.pending && m_RecvPos.pos!=m_Pos)
		m_RecvPos.timer->start( Toy::GetFeedbackDelayMS() );
	update();
}

////////////////////////////////////////////////////////////////////////////////

void FadeXY::paintEvent(QPaintEvent* /*event*/)
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

		painter.setRenderHint(QPainter::Antialiasing, false);

		painter.setOpacity(0.15);
		QPoint pos = PosToWidgetPos( QPointF(0.5,0.5) );
		painter.drawLine(pos.x(), r.y(), pos.x(), r.bottom());
		painter.drawLine(r.x(), pos.y(), r.right(), pos.y());
		painter.setOpacity(1.0);

		painter.setClipping(true);

		if( m_RecvPos.pending )
		{
			painter.setPen( QPen(color.darker(125),BORDER) );

			pos = PosToWidgetPos(m_RecvPos.pos);
			ClipWidgetPosForPainting(r, pos);
			painter.drawLine(pos.x(), r.y(), pos.x(), r.bottom());
			painter.drawLine(r.x(), pos.y(), r.right(), pos.y());

			painter.setPen( QPen(color,BORDER) );
		}

		pos = PosToWidgetPos(m_Pos);
		ClipWidgetPosForPainting(r, pos);
		painter.drawLine(pos.x(), r.y(), pos.x(), r.bottom());
		painter.drawLine(r.x(), pos.y(), r.right(), pos.y());

		painter.end();

		if( painter.begin(this) )
		{
			painter.drawImage(0, 0, m_Canvas);
			
			int hoverRaise = qRound(m_Hover * BUTTON_RAISE);

			if( !text().isEmpty() )
			{
				painter.setPen( palette().color(QPalette::ButtonText) );
				painter.drawText(QRect(0,0,width(),r.y()-hoverRaise), Qt::AlignCenter|Qt::TextWordWrap|Qt::TextDontClip, text());

				if( !m_Label.isEmpty() )
					painter.drawText(QRect(0,r.bottom(),width(),height()-r.bottom()+hoverRaise), Qt::AlignCenter|Qt::TextWordWrap|Qt::TextDontClip, m_Label);
			}
			else if( !m_Label.isEmpty() )
			{
				painter.setPen( palette().color(QPalette::ButtonText) );
				painter.drawText(QRect(0,r.bottom(),width(),height()-r.bottom()+hoverRaise), Qt::AlignCenter|Qt::TextWordWrap|Qt::TextDontClip, m_Label);
			}
		
			painter.end();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void FadeXY::onRecvTimeout()
{
	m_RecvPos.timer->stop();

	if( m_RecvPos.pending )
	{
		m_RecvPos.pending = false;
		SetPos( m_RecvPos.pos );
	}
}

////////////////////////////////////////////////////////////////////////////////

ToyXYWidget::ToyXYWidget(QWidget *parent)
	: ToyWidget(parent)
{
	m_HelpText = tr("OSC Output = X\nOSC Output 2 = Y\n\nLeave OSC Output 2 blank for single, combined (X,Y) output\n\nMin = Left\nMax = Right\n\nMin2 = Bottom\nMax2 = Top");

	m_Min = "-1";
	m_Max = "1";
	m_Min2 = "-1";
	m_Max2 = "1";

	m_Widget = new FadeXY(this);
	connect(m_Widget, SIGNAL(posChanged(const QPointF&)), this, SLOT(onPosChanged(const QPointF&)));
	
	QPalette pal( m_Widget->palette() );
	m_Color = pal.color(QPalette::Button);
	m_TextColor = pal.color(QPalette::ButtonText);
}

////////////////////////////////////////////////////////////////////////////////

const QPointF& ToyXYWidget::GetPos() const
{
	return static_cast<FadeXY*>(m_Widget)->GetPos();
}

////////////////////////////////////////////////////////////////////////////////

void ToyXYWidget::SetText(const QString &text)
{
	ToyWidget::SetText(text);
	static_cast<FadeXY*>(m_Widget)->SetText(m_Text);
}

////////////////////////////////////////////////////////////////////////////////

void ToyXYWidget::SetImagePath(const QString &imagePath)
{
	ToyWidget::SetImagePath(imagePath);
	static_cast<FadeXY*>(m_Widget)->SetImagePath(0, m_ImagePath);
}

////////////////////////////////////////////////////////////////////////////////

void ToyXYWidget::SetColor(const QColor &color)
{
	ToyWidget::SetColor(color);
	QPalette pal( m_Widget->palette() );
	pal.setColor(QPalette::Button, m_Color);
	m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyXYWidget::SetTextColor(const QColor &textColor)
{
	ToyWidget::SetTextColor(textColor);
	QPalette pal( m_Widget->palette() );
	pal.setColor(QPalette::ButtonText, m_TextColor);
	m_Widget->setPalette(pal);
}

////////////////////////////////////////////////////////////////////////////////

void ToyXYWidget::SetLabel(const QString &label)
{
	static_cast<FadeXY*>(m_Widget)->SetLabel(label);
}

////////////////////////////////////////////////////////////////////////////////

void ToyXYWidget::Recv(const QString &path, const OSCArgument *args, size_t count)
{
	if(args && count>0)
	{
		bool isFeedback = (path == m_FeedbackPath);
		bool isTrigger = (!isFeedback && path==m_TriggerPath);
		if(isFeedback || isTrigger)
		{
			FadeXY *xy = static_cast<FadeXY*>(m_Widget);

			QPointF pos(0, 0);

			float value = 0;
			if( args[0].GetFloat(value) )
				pos.setX(value);

			if(count>1 && args[1].GetFloat(value) )
				pos.setY(value);

			float minX = m_Min.toFloat();
			float maxX = m_Max.toFloat();
			float minY = m_Min2.toFloat();
			float maxY = m_Max2.toFloat();
			float rangeX = (maxX - minX);
			float rangeY = (maxY - minY);
			pos.setX(((rangeX==0) ? 0 : (pos.x()-minX)/rangeX));
			pos.setY(((rangeY==0) ? 0 : (pos.y()-minY)/rangeY));

			if( isFeedback )
				xy->RecvPos(pos);
			else
				xy->TriggerPos(pos);
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

void ToyXYWidget::onPosChanged(const QPointF& /*pos*/)
{
	emit posChanged(this);
}

////////////////////////////////////////////////////////////////////////////////

ToyXYGrid::ToyXYGrid(Client *pClient, QWidget *parent, Qt::WindowFlags flags)
	: ToyGrid(TOY_XY_GRID, pClient, parent, flags)
{
}

////////////////////////////////////////////////////////////////////////////////

ToyWidget* ToyXYGrid::CreateWidget()
{
	ToyXYWidget *w = new ToyXYWidget(this);
	connect(w, SIGNAL(posChanged(ToyXYWidget*)), this, SLOT(onPosChanged(ToyXYWidget*)));
	return w;
}

////////////////////////////////////////////////////////////////////////////////

void ToyXYGrid::onPosChanged(ToyXYWidget *xy)
{
	if(	m_pClient &&
		xy &&
		!(xy->GetPath().isEmpty() && xy->GetPath2().isEmpty()) )
	{
		float minX = xy->GetMin().toFloat();
		float maxX = xy->GetMax().toFloat();
		float minY = xy->GetMin2().toFloat();
		float maxY = xy->GetMax2().toFloat();
		float x = (minX + (maxX-minX)*xy->GetPos().x());
		float y = (minY + (maxY-minY)*xy->GetPos().y());
		
		QString path( xy->GetPath() );
		QString path2( xy->GetPath2() );
		if( path.isEmpty() )
		{
			path = path2;
			path2.clear();
		}
		
		if( path2.isEmpty() )
		{
			// combined packet
			bool local = Utils::MakeLocalOSCPath(false, path);

			OSCPacketWriter packetWriter( path.toUtf8().constData() );
			packetWriter.AddFloat32(x);
			packetWriter.AddFloat32(y);
				
			size_t size;
			char *packet = packetWriter.Create(size);
			if( packet )
				m_pClient->ToyClient_Send(local, packet, size);
		}
		else
		{
			// x
			bool local = Utils::MakeLocalOSCPath(false, path);
			
			OSCPacketWriter packetWriter( path.toUtf8().constData() );
			packetWriter.AddFloat32(x);
			
			size_t size;
			char *packet = packetWriter.Create(size);
			if( packet )
				m_pClient->ToyClient_Send(local, packet, size);
			
			// y
			local = Utils::MakeLocalOSCPath(false, path2);
			
			OSCPacketWriter packetWriter2( path2.toUtf8().constData() );
			packetWriter2.AddFloat32(y);
			
			packet = packetWriter2.Create(size);
			if( packet )
				m_pClient->ToyClient_Send(local, packet, size);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
