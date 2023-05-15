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

#include "ToyWidget.h"
#include "EditPanel.h"
#include "Toy.h"
#include "Utils.h"

////////////////////////////////////////////////////////////////////////////////

ToyWidget::ToyWidget(QWidget *parent)
	: QWidget(parent)
	, m_Mode(MODE_DEFAULT)
	, m_Visible(true)
	, m_Widget(nullptr)
	, m_Min("0")
	, m_Max("1")
	, m_Min2("0")
	, m_Max2("1")
{
	m_EditButton = new EditButton(this);
	m_EditButton->hide();
	connect(m_EditButton, SIGNAL(clicked(bool)), this, SLOT(onEditButtonClicked(bool)));
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::SetMode(EnumMode mode)
{
	if(m_Mode != mode)
	{
		m_Mode = mode;
		UpdateMode();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::UpdateMode()
{
	switch( m_Mode )
	{
		case MODE_DEFAULT:
			m_EditButton->hide();
			break;
			
		case MODE_EDIT:
			m_EditButton->raise();
			m_EditButton->show();
			break;
	}

	UpdateVisible();
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::SetVisible(bool b)
{
	if(m_Visible != b)
	{
		m_Visible = b;
		UpdateVisible();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::UpdateVisible()
{
	setVisible(m_Visible || m_Mode==MODE_EDIT);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::SetLabelPath(const QString &labelPath)
{
	if(m_LabelPath != labelPath)
	{
		m_LabelPath = labelPath;
		UpdateToolTip();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::SetFeedbackPath(const QString &feedbackPath)
{
	if(m_FeedbackPath != feedbackPath)
	{
		m_FeedbackPath = feedbackPath;
		UpdateToolTip();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::SetTriggerPath(const QString &triggerPath)
{
	if(m_TriggerPath != triggerPath)
	{
		m_TriggerPath = triggerPath;
		UpdateToolTip();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::SetPath(const QString &path)
{
	if(m_Path != path)
	{
		m_Path = path;
		UpdateToolTip();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::SetPath2(const QString &path)
{
	if(m_Path2 != path)
	{
		m_Path2 = path;
		UpdateToolTip();
	}
}

////////////////////////////////////////////////////////////////////////////////

bool ToyWidget::GetSelected() const
{
	return m_EditButton->GetSelected();
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::SetSelected(bool selected)
{
	m_EditButton->SetSelected(selected);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::UpdateToolTip()
{
	QString tip;

	if(HasPath() && !m_Path.isEmpty())
		tip = tr("OSC Output: %1").arg(m_Path);
	
	if(HasPath2() && !m_Path2.isEmpty())
	{
		if( tip.isEmpty() )
			tip = tr("OSC Output: %1").arg(m_Path2);
		else
			tip = tr("\nOSC Output 2: %1").arg(m_Path2);
	}

	if( !m_LabelPath.isEmpty() )
	{
		if( !tip.isEmpty() )
			tip.append("\n");
		tip.append( tr("OSC Label: %1").arg(m_LabelPath) );
	}

	if( !m_FeedbackPath.isEmpty() )
	{
		if( !tip.isEmpty() )
			tip.append("\n");
		tip.append( tr("OSC Feedback: %1").arg(m_FeedbackPath) );
	}

	if( !m_TriggerPath.isEmpty() )
	{
		if( !tip.isEmpty() )
			tip.append("\n");
		tip.append( tr("OSC Trigger: %1").arg(m_TriggerPath) );
	}

	m_Widget->setToolTip(tip);
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::SetLabel(const QString &)
{
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::ClearLabel()
{
	SetLabel( QString() );
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::Recv(const QString& /*path*/, const OSCArgument* /*args*/, size_t /*count*/)
{
}

////////////////////////////////////////////////////////////////////////////////

bool ToyWidget::Save(EosLog &log, const QString &path, QStringList &lines)
{
	QString line;

	QString imagePath(m_ImagePath);
	Toy::ResourceAbsolutePathToRelative(&log, path, imagePath);
	
	QString imagePath2(m_ImagePath2);
	Toy::ResourceAbsolutePathToRelative(&log, path, imagePath2);

	line.append( QString("%1").arg(static_cast<int>(m_Visible ? 1 : 0)) );
	line.append( QString(", %1").arg(Utils::QuotedString(m_Path)) );
	line.append( QString(", %1").arg(Utils::QuotedString(m_Path2)) );
	line.append( QString(", %1").arg(Utils::QuotedString(m_LabelPath)) );
	line.append( QString(", %1").arg(Utils::QuotedString(m_FeedbackPath)) );
	line.append( QString(", %1").arg(Utils::QuotedString(m_TriggerPath)) );
	line.append( QString(", %1").arg(Utils::QuotedString(m_Text)) );
	line.append( QString(", %1").arg(Utils::QuotedString(imagePath)) );
	line.append( QString(", %1").arg(Utils::QuotedString(imagePath2)) );
	line.append( QString(", %1").arg(m_Color.rgba(),0,16) );
	line.append( QString(", %1").arg(m_Color2.rgba(),0,16) );
	line.append( QString(", %1").arg(m_TextColor.rgba(),0,16) );
	line.append( QString(", %1").arg(m_TextColor2.rgba(),0,16) );
	line.append( QString(", %1").arg(m_Min) );
	line.append( QString(", %1").arg(m_Max) );
	line.append( QString(", %1").arg(m_Min2) );
	line.append( QString(", %1").arg(m_Max2) );
	line.append( QString(", %1").arg(m_BPM) );

	lines << line;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool ToyWidget::Load(EosLog &log, const QString &path, QStringList &lines, int &index)
{
	if(index>=0 && index<lines.size())
	{
		QStringList items;
		Utils::GetItemsFromQuotedString(lines[index++], items);

		if(items.size() > 0)
			m_Visible = (items[0].toInt() != 0);
		UpdateVisible();

		if(items.size() > 1)
			SetPath( items[1] );
		
		if(items.size() > 2)
			SetPath2( items[2] );

		if(items.size() > 3)
			SetLabelPath( items[3] );

		if(HasFeedbackPath() && items.size() > 4)
			SetFeedbackPath( items[4] );

		if(HasTriggerPath() && items.size() > 5)
			SetTriggerPath( items[5] );

		if(items.size() > 6)
			SetText( items[6] );

		if(items.size() > 7)
		{
			QString imagePath( items[7] );
			Toy::ResourceRelativePathToAbsolute(&log, path, imagePath);
			SetImagePath(imagePath);
		}
		
		if(HasImagePath2() && items.size()>8)
		{
			QString imagePath2( items[8] );
			Toy::ResourceRelativePathToAbsolute(&log, path, imagePath2);
			SetImagePath2(imagePath2);
		}

		if(items.size() > 9)
			SetColor( QColor::fromRgba(items[9].toUInt(0,16)) );
		
		if(HasColor2() && items.size()>10)
			SetColor2( QColor::fromRgba(items[10].toUInt(0,16)) );

		if(items.size() > 11)
			SetTextColor( QColor::fromRgba(items[11].toUInt(0,16)) );
		
		if(HasTextColor2() && items.size()>12)
			SetTextColor2( QColor::fromRgba(items[12].toUInt(0,16)) );

		if(items.size() > 13)
			SetMin( items[13] );

		if(items.size() > 14)
			SetMax( items[14] );

		if( HasMinMax2() )
		{
			if(items.size() > 15)
				SetMin2( items[15] );

			if(items.size() > 16)
				SetMax2( items[16] );
		}

		if(HasBPM() && items.size()>17)
			SetBPM( items[17] );

		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	m_Widget->setGeometry(MARGIN, MARGIN, width()-MARGIN2, height()-MARGIN2);
	m_EditButton->setGeometry(0, 0, width(), height());
}

////////////////////////////////////////////////////////////////////////////////

void ToyWidget::onEditButtonClicked(bool /*checked*/)
{
	emit edit(this);
}

////////////////////////////////////////////////////////////////////////////////
