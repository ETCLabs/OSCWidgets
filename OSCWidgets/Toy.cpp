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

#include "Toy.h"
#include "Toys.h"
#include "EosLog.h"
#include "EditPanel.h"
#include "ToyPedal.h"
#include "ToySlider.h"
#include "ToyXY.h"
#include "ToyEncoder.h"
#include "ToyMetro.h"
#include "ToySine.h"
#include "ToyCmd.h"
#include "ToyFlicker.h"
#include "ToyActivity.h"
#include "ToyLabel.h"
#include "ToyWindow.h"

#ifdef WIN32
	#include <Windows.h>
	#include "resource.h"
	#include <QtWinExtras/QtWin>
#endif

////////////////////////////////////////////////////////////////////////////////

#define SNAP_TO_EDGE_PIX	100

////////////////////////////////////////////////////////////////////////////////

float Toy::sm_EncoderRadiansPerTick = 0;
unsigned int Toy::sm_FeedbackDelayMS = 0;
unsigned int Toy::sm_CmdSendAllDelayMS = 0;
unsigned int Toy::sm_MetroRefreshRateMS = 0;
unsigned int Toy::sm_SineRefreshRateMS = 0;
unsigned int Toy::sm_PedalRefreshRateMS = 0;
unsigned int Toy::sm_FlickerRefreshRateMS = 0;

////////////////////////////////////////////////////////////////////////////////

Toy::Toy(EnumToyType type, Client *pClient, QWidget *parent, Qt::WindowFlags flags)
	: QWidget(parent, flags)
	, m_Type(type)
	, m_pClient(pClient)
{
	QPalette pal( palette() );
	pal.setColor(QPalette::ButtonText, QColor(200,200,200));
	setPalette(pal);
	
	setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);

	SetDefaultWindowIcon(*this);
}

////////////////////////////////////////////////////////////////////////////////

Toy::~Toy()
{
}

////////////////////////////////////////////////////////////////////////////////

Toy* Toy::Create(EnumToyType type, Client *pClient, QWidget *parent, Qt::WindowFlags flags)
{
	switch( type )
	{
		case TOY_BUTTON_GRID:	return (new ToyButtonGrid(pClient,parent,flags));
		case TOY_PEDAL_GRID:	return (new ToyPedalGrid(pClient,parent,flags));
		case TOY_SLIDER_GRID:	return (new ToySliderGrid(pClient,parent,flags));
		case TOY_ENCODER_GRID:	return (new ToyEncoderGrid(pClient,parent,flags));
		case TOY_XY_GRID:		return (new ToyXYGrid(pClient,parent,flags));
		case TOY_CMD_GRID:		return (new ToyCmdGrid(pClient,parent,flags));
		case TOY_METRO_GRID:	return (new ToyMetroGrid(pClient,parent,flags));
		case TOY_SINE_GRID:		return (new ToySineGrid(pClient,parent,flags));
		case TOY_FLICKER_GRID:	return (new ToyFlickerGrid(pClient,parent,flags));
		case TOY_ACTIVITY_GRID:	return (new ToyActivityGrid(pClient,parent,flags));
		case TOY_LABEL_GRID:	return (new ToyLabelGrid(pClient,parent,flags));
		case TOY_WINDOW:		return (new ToyWindow(pClient,parent,flags));
	}
	
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

void Toy::GetName(EnumToyType type, QString &name)
{
	switch( type )
	{
		case TOY_BUTTON_GRID:	name=qApp->tr("Buttons"); break;
		case TOY_PEDAL_GRID:	name=qApp->tr("Pedals"); break;
		case TOY_SLIDER_GRID:	name=qApp->tr("Faders"); break;
		case TOY_ENCODER_GRID:	name=qApp->tr("Encoders"); break;
		case TOY_XY_GRID:		name=qApp->tr("XYs"); break;
		case TOY_METRO_GRID:	name=qApp->tr("Metronomes"); break;
		case TOY_SINE_GRID:		name=qApp->tr("Sine Waves"); break;
		case TOY_CMD_GRID:		name=qApp->tr("Commands"); break;
		case TOY_FLICKER_GRID:	name=qApp->tr("Flicker"); break;
		case TOY_ACTIVITY_GRID:	name=qApp->tr("Activity"); break;
		case TOY_LABEL_GRID:	name=qApp->tr("Label"); break;
		case TOY_WINDOW:		name=qApp->tr("Window"); break;
		default:				name.clear(); break;
	}
}

////////////////////////////////////////////////////////////////////////////////

void Toy::GetDefaultPathName(EnumToyType type, QString &name)
{
	switch( type )
	{
		case TOY_BUTTON_GRID:	name=qApp->tr("button"); break;
		case TOY_PEDAL_GRID:	name=qApp->tr("pedal"); break;
		case TOY_SLIDER_GRID:	name=qApp->tr("fader"); break;
		case TOY_ENCODER_GRID:	name=qApp->tr("encoder"); break;
		case TOY_XY_GRID:		name=qApp->tr("xy"); break;
		case TOY_METRO_GRID:	name=qApp->tr("metro"); break;
		case TOY_SINE_GRID:		name=qApp->tr("sine"); break;
		case TOY_CMD_GRID:		name=qApp->tr("cmd"); break;
		case TOY_FLICKER_GRID:	name=qApp->tr("flicker"); break;
		case TOY_ACTIVITY_GRID:	name=qApp->tr("activity"); break;
		case TOY_LABEL_GRID:	name=qApp->tr("label"); break;
		case TOY_WINDOW:		name=qApp->tr("window"); break;
		default:				name.clear(); break;
	}
}

////////////////////////////////////////////////////////////////////////////////

void Toy::SetDefaultWindowIcon(QWidget &w)
{
	// NOTE: looks cleaner without window icons on Mac
	Q_UNUSED(w);
#ifdef WIN32
	static QIcon *sIcon = 0;
	if( !sIcon )
	{
		sIcon = new QIcon();

		const int iconSizes[] = {512, 256, 128, 64, 32, 16};
		const size_t numIcons = sizeof(iconSizes)/sizeof(iconSizes[0]);
		for(size_t i=0; i<numIcons; i++)
		{
			HICON hIcon = static_cast<HICON>( LoadImage(GetModuleHandle(0),MAKEINTRESOURCE(IDI_ICON1),IMAGE_ICON,iconSizes[i],iconSizes[i],LR_LOADTRANSPARENT) );
			if( hIcon )
			{
                sIcon->addPixmap( QtWin::fromHICON(hIcon) );
				DestroyIcon(hIcon);
			}
		}
	}

	if( !sIcon->isNull() )
		w.setWindowIcon( *sIcon );
#endif
}

////////////////////////////////////////////////////////////////////////////////

void Toy::SnapToEdges()
{
	SnapToScreen(*this, SNAP_TO_EDGE_PIX);
}

////////////////////////////////////////////////////////////////////////////////

void Toy::SnapToScreen(QWidget &w, int threshold)
{
	QDesktopWidget *desktop = QApplication::desktop();
	if( desktop )
	{
		Qt::WindowStates ws = w.windowState();
		if(!ws.testFlag(Qt::WindowMinimized) && !ws.testFlag(Qt::WindowMaximized))
		{
			QRect fr( w.frameGeometry() );
			QSize frameSize(fr.size() - w.size());
			QRect dr( desktop->availableGeometry(fr.center()) );
			if(!fr.isEmpty() && !dr.isEmpty())
			{
				// top/bottom snap
				if(qAbs(fr.top()-dr.top()) <= threshold)
				{
					if(qAbs(fr.bottom()-dr.bottom()) <= threshold)
						fr.setHeight( dr.height() );	// snap to both

					fr.moveTo(fr.x(), dr.top());
				}
				else if(qAbs(fr.bottom()-dr.bottom()) <= threshold)
					fr.moveTo(fr.x(), dr.bottom()-fr.height()+1);

				// left/right snap
				if(qAbs(fr.left()-dr.left()) <= threshold)
				{
					if(qAbs(fr.right()-dr.right()) <= threshold)
						fr.setWidth( dr.width() );	// snap to both

					fr.moveTo(dr.left(), fr.y());
				}
				else if(qAbs(fr.right()-dr.right()) <= threshold)
					fr.moveTo(dr.right()-fr.width()+1, fr.y());

				w.move( fr.topLeft() );
				w.resize(fr.size() - frameSize);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void Toy::ClipToScreen(QWidget &w)
{
	QDesktopWidget *desktop = QApplication::desktop();
	if( desktop )
	{
		QRect fr( w.frameGeometry() );
		QRect dr( desktop->availableGeometry(fr.center()) );

		// top/bottom clip
		if(fr.top() < dr.top())
			fr.moveTo(fr.x(), dr.top());
		else if(fr.bottom() > dr.bottom())
			fr.moveTo(fr.x(), dr.bottom()-fr.height()+1);

		// left/right clip
		if(fr.left() < dr.left())
			fr.moveTo(dr.left(), fr.y());
		else if(fr.right() > dr.right())
			fr.moveTo(dr.right()-fr.width()+1, fr.y());

		w.move( fr.topLeft() );
	}
}

////////////////////////////////////////////////////////////////////////////////

void Toy::ResourceRelativePathToAbsolute(EosLog *log, const QString &filePath, QString &resourcePath)
{
	if( !resourcePath.isEmpty() )
	{
		QFileInfo fi(resourcePath);
		if( !fi.isAbsolute() )
		{
			QDir fileDir( QFileInfo(filePath).absoluteDir() );
			resourcePath = QDir::cleanPath( fileDir.absoluteFilePath(resourcePath) );
		}
		
		if( log )
		{
			if( QFileInfo(resourcePath).exists() )
			{
				QString msg = tr("Loaded resource \"%1\" @ \"%2\"")
				.arg( QDir::toNativeSeparators(fi.filePath()) )
				.arg( QDir::toNativeSeparators(resourcePath) );
				log->AddDebug( msg.toUtf8().constData() );
			}
			else
			{
				QString msg = tr("Missing resource \"%1\" @ \"%2\"")
				.arg( QDir::toNativeSeparators(fi.filePath()) )
				.arg( QDir::toNativeSeparators(resourcePath) );
				log->AddError( msg.toUtf8().constData() );
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void Toy::ResourceAbsolutePathToRelative(EosLog *log, const QString &filePath, QString &resourcePath)
{
	if( !resourcePath.isEmpty() )
	{
		QFileInfo fi(resourcePath);
		if( fi.isAbsolute() )
		{
			QDir fileDir( QFileInfo(filePath).absoluteDir() );
			resourcePath = QDir::cleanPath( fileDir.relativeFilePath(fi.absoluteFilePath()) );
		}
		
		if( log )
		{
			QString sanityCheck(resourcePath);
			Toy::ResourceRelativePathToAbsolute(/*log*/0, filePath, sanityCheck);
			
			if( QFileInfo(sanityCheck).isFile() )
			{
				QString msg = tr("Saved resource \"%1\" @ \"%2\"")
				.arg( QDir::toNativeSeparators(fi.filePath()) )
				.arg( QDir::toNativeSeparators(resourcePath) );
				log->AddDebug( msg.toUtf8().constData() );
			}
			else
			{
				QString msg = tr("Failed to save resource \"%1\" @ \"%2\"")
				.arg( QDir::toNativeSeparators(fi.filePath()) )
				.arg( QDir::toNativeSeparators(resourcePath) );
				log->AddError( msg.toUtf8().constData() );
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void Toy::RestoreDefaultSettings()
{
	sm_EncoderRadiansPerTick = 0.034906585f;	// 2 degrees
	sm_FeedbackDelayMS = 3000;
	sm_CmdSendAllDelayMS = 500;
	sm_MetroRefreshRateMS = 10;
	sm_SineRefreshRateMS = 10;
	sm_PedalRefreshRateMS = 10;
	sm_FlickerRefreshRateMS = 10;
}

////////////////////////////////////////////////////////////////////////////////
