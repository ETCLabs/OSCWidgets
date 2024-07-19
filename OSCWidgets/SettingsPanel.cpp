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

#include "SettingsPanel.h"
#include "Toys.h"
#include "Utils.h"

////////////////////////////////////////////////////////////////////////////////

AddToyButton::AddToyButton(int type, QWidget *parent)
	: QPushButton(parent)
	, m_Type(type)
{
	QString label;
	Toy::GetName(static_cast<Toy::EnumToyType>(m_Type), label);
	setText(label);
	
	connect(this, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
}

////////////////////////////////////////////////////////////////////////////////

void AddToyButton::onClicked(bool /*checked*/)
{
	emit addToy(m_Type);
}

////////////////////////////////////////////////////////////////////////////////

AdvancedPanel::AdvancedPanel(QWidget *parent)
	: QWidget(parent, Qt::Tool)
{
	setWindowTitle( tr("Advanced Options") );

	QGridLayout *layout = new QGridLayout(this);

	int row = 0;
	m_EncoderDegreesPerTick = new QLineEdit(this);
	layout->addWidget(new QLabel(tr("Encoder degrees per tick"),this), row, 0);
	layout->addWidget(m_EncoderDegreesPerTick, row, 1);

	++row;
	m_FeedbackDelay = new QLineEdit(this);
	layout->addWidget(new QLabel(tr("Fader Feedback Delay (ms)"),this), row, 0);
	layout->addWidget(m_FeedbackDelay, row, 1);

	++row;
	m_CmdSendAllDelay = new QLineEdit(this);
	layout->addWidget(new QLabel(tr("Command Send All Delay (ms)"),this), row, 0);
	layout->addWidget(m_CmdSendAllDelay, row, 1);

	++row;
	m_MetroRefreshRate = new QLineEdit(this);
	layout->addWidget(new QLabel(tr("Metronome Refresh Rate (ms)"),this), row, 0);
	layout->addWidget(m_MetroRefreshRate, row, 1);

	++row;
	m_SineRefreshRate = new QLineEdit(this);
	layout->addWidget(new QLabel(tr("Sine Wave Refresh Rate (ms)"),this), row, 0);
	layout->addWidget(m_SineRefreshRate, row, 1);

	++row;
	m_PedalRefreshRate = new QLineEdit(this);
	layout->addWidget(new QLabel(tr("Pedal Refresh Rate (ms)"),this), row, 0);
	layout->addWidget(m_PedalRefreshRate, row, 1);
	
	++row;
	m_FlickerRefreshRate = new QLineEdit(this);
	layout->addWidget(new QLabel(tr("Flicker Refresh Rate (ms)"),this), row, 0);
	layout->addWidget(m_FlickerRefreshRate, row, 1);

	++row;
	QPushButton *button = new QPushButton(tr("Restore Defaults"), this);
	QPalette pal( button->palette() );
	pal.setColor(QPalette::Button, ERROR_COLOR);
	button->setPalette(pal);
	connect(button, SIGNAL(clicked(bool)), this, SLOT(onRestoreDefaultsClicked(bool)));
	layout->addWidget(button, row, 0);

	button = new QPushButton(tr("Apply"), this);
	connect(button, SIGNAL(clicked(bool)), this, SLOT(onApplyClicked(bool)));
	layout->addWidget(button, row, 1);

	Load();
}

////////////////////////////////////////////////////////////////////////////////

void AdvancedPanel::Load()
{
	float degrees = (Toy::GetEncoderRadiansPerTick() * (180/M_PI));
	m_EncoderDegreesPerTick->setText( QString::number(degrees) );
	m_FeedbackDelay->setText( QString::number(Toy::GetFeedbackDelayMS()) );
	m_CmdSendAllDelay->setText( QString::number(Toy::GetCmdSendAllDelayMS()) );
	m_MetroRefreshRate->setText( QString::number(Toy::GetMetroRefreshRateMS()) );
	m_SineRefreshRate->setText( QString::number(Toy::GetSineRefreshRateMS()) );
	m_PedalRefreshRate->setText( QString::number(Toy::GetPedalRefreshRateMS()) );
	m_FlickerRefreshRate->setText( QString::number(Toy::GetFlickerRefreshRateMS()) );
}

////////////////////////////////////////////////////////////////////////////////

void AdvancedPanel::Save()
{
	float radians = (m_EncoderDegreesPerTick->text().toFloat() * (M_PI/180));
	Toy::SetEncoderRadiansPerTick(radians);
	Toy::SetFeedbackDelayMS( m_FeedbackDelay->text().toUInt() );
	Toy::SetCmdSendAllDelayMS( m_CmdSendAllDelay->text().toUInt() );
	Toy::SetMetroRefreshRateMS( m_MetroRefreshRate->text().toUInt() );
	Toy::SetSineRefreshRateMS( m_SineRefreshRate->text().toUInt() );
	Toy::SetPedalRefreshRateMS( m_PedalRefreshRate->text().toUInt() );
	Toy::SetFlickerRefreshRateMS( m_FlickerRefreshRate->text().toUInt() );
}

////////////////////////////////////////////////////////////////////////////////

void AdvancedPanel::onApplyClicked(bool /*checked*/)
{
	Save();
	emit changed();
	close();
}

////////////////////////////////////////////////////////////////////////////////

void AdvancedPanel::onRestoreDefaultsClicked(bool /*checked*/)
{
	Toy::RestoreDefaultSettings();
	Load();
	emit changed();
}

////////////////////////////////////////////////////////////////////////////////

SettingsPanel::SettingsPanel(QWidget *parent)
	: QWidget(parent)
{
	QGridLayout *layout = new QGridLayout(this);
	
	m_Mode = new QComboBox(this);
	m_Mode->addItem(tr("UDP"), static_cast<int>(OSCStream::FRAME_MODE_INVALID));
	m_Mode->addItem(tr("TCP v1.0"), static_cast<int>(OSCStream::FRAME_MODE_1_0));
	m_Mode->addItem(tr("TCP v1.1"), static_cast<int>(OSCStream::FRAME_MODE_1_1));
	connect(m_Mode, SIGNAL(currentIndexChanged(int)), this, SLOT(onModeChanged(int)));
	layout->addWidget(m_Mode, 0, 0, 1, 2);
	
	m_Ip = new QLineEdit(this);
	m_Ip->setText("127.0.0.1");
	layout->addWidget(new QLabel(tr("IP"),this), 1, 0, 1, 1);
	layout->addWidget(m_Ip, 1, 1, 1, 1);
	
	m_PortLabel = new QLabel(this);
	m_Port = new QSpinBox(this);
	m_Port->setRange(0, 0xffff);
	m_Port->setValue(8000);
	layout->addWidget(m_PortLabel, 2, 0, 1, 1);
	layout->addWidget(m_Port, 2, 1, 1, 1);
	
	m_Port2Label = new QLabel(this);
	m_Port2 = new QSpinBox(this);
	m_Port2->setRange(0, 0xffff);
	m_Port2->setValue(8001);
	layout->addWidget(m_Port2Label, 3, 0, 1, 1);
	layout->addWidget(m_Port2, 3, 1, 1, 1);
	
	QPushButton *button = new QPushButton(QIcon(":/assets/images/MenuIconNetwork.png"), tr("Connect"), this);
	connect(button, SIGNAL(clicked(bool)), this, SLOT(onApplyClicked(bool)));
	layout->addWidget(button, 4, 0, 1, 2);

	int col = 2;
	const int numToyCols = 3;

	QLabel *label = new QLabel(tr("New Widgets"), this);
	label->setAlignment(Qt::AlignCenter);
	layout->addWidget(label, 0, col, 1, numToyCols);
		
	int addToyCol = 0;
	int addToyRow = 1;
	for(int i=0; i<Toy::TOY_COUNT; i++)
	{
		AddToyButton *addToyButton = new AddToyButton(i, this);
		addToyButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
		connect(addToyButton, SIGNAL(addToy(int)), this, SLOT(onAddToy(int)));
		layout->addWidget(addToyButton, addToyRow, col+addToyCol);

		if(++addToyCol >= numToyCols)
		{
			addToyCol = 0;
			addToyRow++;
		}
	}
	
	UpdateMode();
}

////////////////////////////////////////////////////////////////////////////////

OSCStream::EnumFrameMode SettingsPanel::GetMode() const
{
	int index = m_Mode->currentIndex();
	if(index >= 0)
	{
		QVariant v = m_Mode->itemData(index);
		if( v.isValid() )
		{
			int n = v.toInt();
			if(n>=0 && n<OSCStream::FRAME_MODE_COUNT)
				return static_cast<OSCStream::EnumFrameMode>(n);
		}
	}
	
	return OSCStream::FRAME_MODE_INVALID;
}

////////////////////////////////////////////////////////////////////////////////

void SettingsPanel::SetMode(OSCStream::EnumFrameMode mode)
{
	m_Mode->setCurrentIndex( m_Mode->findData(static_cast<int>(mode)) );
}

////////////////////////////////////////////////////////////////////////////////

void SettingsPanel::UpdateMode()
{
	OSCStream::EnumFrameMode mode = GetMode();
	
	switch( mode )
	{
		case OSCStream::FRAME_MODE_1_0:
		case OSCStream::FRAME_MODE_1_1:
			m_PortLabel->setText( tr("Port") );
			m_Port2Label->setText("--");
			m_Port2Label->setEnabled(false);
			m_Port2->setEnabled(false);
			break;
			
		default:
			m_PortLabel->setText( tr("Out Port") );
			m_Port2Label->setText( tr("In Port") );
			m_Port2Label->setEnabled(true);
			m_Port2->setEnabled(true);
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////

void SettingsPanel::GetIP(QString &ip) const
{
	ip = m_Ip->text();
}

////////////////////////////////////////////////////////////////////////////////

void SettingsPanel::SetIP(const QString &ip)
{
	m_Ip->setText(ip);
}

////////////////////////////////////////////////////////////////////////////////

unsigned short SettingsPanel::GetPort1() const
{
	return static_cast<unsigned short>( m_Port->value() );
}

////////////////////////////////////////////////////////////////////////////////

void SettingsPanel::SetPort1(unsigned short port)
{
	m_Port->setValue(port);
}

////////////////////////////////////////////////////////////////////////////////

unsigned short SettingsPanel::GetPort2() const
{
	return static_cast<unsigned short>( m_Port2->value() );
}

////////////////////////////////////////////////////////////////////////////////

void SettingsPanel::SetPort2(unsigned short port2)
{
	m_Port2->setValue(port2);
}

////////////////////////////////////////////////////////////////////////////////

void SettingsPanel::onModeChanged(int /*index*/)
{
	UpdateMode();
}

////////////////////////////////////////////////////////////////////////////////

void SettingsPanel::onApplyClicked(bool /*checked*/)
{
	emit changed();
}

////////////////////////////////////////////////////////////////////////////////

void SettingsPanel::onAddToy(int type)
{
	emit addToy(type);
}

////////////////////////////////////////////////////////////////////////////////
