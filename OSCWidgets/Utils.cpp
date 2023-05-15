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

#include "Utils.h"

////////////////////////////////////////////////////////////////////////////////

PixmapCache *PixmapCache::sm_Instance = 0;

#ifdef WIN32

#define MOUSE_TOUCH_SIGNATURE	0xff515780
#define MOUSE_TOUCH_MASK		0xffffff80

HHOOK Utils::sm_MouseMsgHook = 0;
void *Utils::sm_pFuncRegisterTouchWindow = 0;
void *Utils::sm_pFuncUnregisterTouchWindow = 0;

#endif

#define OSC_LOCAL	"local:"
#define COLOR_MAG	60

////////////////////////////////////////////////////////////////////////////////

QString Utils::QuotedString(const QString &str)
{
	// "test" -> """test"""
	// test,  -> "test,"

	QString quoted(str);
	quoted.replace("\"", "\"\"");
	if(quoted.contains('\"') || quoted.contains(','))
	{
		quoted.prepend("\"");
		quoted.append("\"");
	}
	return quoted;
}

////////////////////////////////////////////////////////////////////////////////

void Utils::GetItemsFromQuotedString(const QString &str, QStringList &items)
{
	items.clear();

	int len = str.size();
	int index = 0;
	bool quoted = false;
	for(int i=0; i<=len; i++)
	{
		if(i>=len || (str[i]==QChar(',') && !quoted))
		{
			int itemLen = (i - index);

			QString item;
			item.reserve(itemLen);
			for(int j=0; j<itemLen; ++j)
			{
				int itemIndex = (index + j);
				if(str[itemIndex] == QChar('\"'))
				{
					if((j+1)<itemLen && str[itemIndex+1]==QChar('\"'))
					{
						item.append(QChar('\"'));
						++j;
					}
				}
				else
					item.append(str[itemIndex]);
			}

			items.push_back(item.trimmed());

			index = (i+1);
		}
		else if(str[i] == QChar('\"'))
		{
			if( !quoted )
				quoted = true;
			else if((i+1)<len && str[i+1]==QChar('\"'))
				++i;
			else
				quoted = false;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

PixmapCache::PixmapCache()
{
}

////////////////////////////////////////////////////////////////////////////////

PixmapCache::~PixmapCache()
{
	Clear();
}

////////////////////////////////////////////////////////////////////////////////

void PixmapCache::Clear()
{
	for(PIXMAP_LIST::const_iterator i=m_List.begin(); i!=m_List.end(); i++)
		delete i->second.pixmap;
	m_List.clear();
}

////////////////////////////////////////////////////////////////////////////////

const QPixmap* PixmapCache::Create(const QString &path)
{
	if( path.isEmpty() )
		return 0;

	PIXMAP_LIST::iterator i = m_List.find(path);
	if(i == m_List.end())
	{
		sPixmapCacheItem item = {new QPixmap(path), 1};
		m_List[path] = item;
		return item.pixmap;
	}

	i->second.refCount++;
	return i->second.pixmap;
}

////////////////////////////////////////////////////////////////////////////////

void PixmapCache::Destroy(const QString &path)
{
	if( !path.isEmpty() )
	{
		PIXMAP_LIST::iterator i = m_List.find(path);
		if(i != m_List.end())
		{
			if(i->second.refCount != 0)
				i->second.refCount--;

			if(i->second.refCount == 0)
			{
				delete i->second.pixmap;
				m_List.erase(i);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

const QPixmap* PixmapCache::Get(const QString &path) const
{
	PIXMAP_LIST::const_iterator i = m_List.find(path);
	return ((i==m_List.end()) ? 0 : i->second.pixmap);
}

////////////////////////////////////////////////////////////////////////////////

void PixmapCache::Get(const QString &path, QPixmap &pixmap) const
{
	const QPixmap *p = Get(path);
	pixmap = (p ? (*p) : QPixmap());
}

////////////////////////////////////////////////////////////////////////////////

void PixmapCache::GetScaledToFit(const QString &path, const QSize &size, QPixmap &pixmap) const
{
	const QPixmap *p = Get(path);
	if(p && !p->isNull() && !size.isEmpty())
	{
		QRect r(0, 0, p->width(), p->height());
		if( FitRectInBounds(QRect(0,0,size.width(),size.height()),Qt::AlignCenter,r) )
			pixmap = p->scaled(r.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		else
			pixmap = QPixmap();
	}
	else
		pixmap = QPixmap();
}

////////////////////////////////////////////////////////////////////////////////

void PixmapCache::GetScaledToFill(const QString &path, const QSize &size, QPixmap &pixmap) const
{
	const QPixmap *p = Get(path);
	if(p && !p->isNull() && !size.isEmpty())
	{
		// scale to bounds width
		float t = (size.width() / static_cast<float>(p->width()));
		int h = qRound(p->height() * t);
		if(h < size.height())
			t = (size.height() / static_cast<float>(p->height()));

		pixmap = p->scaled(QSize(qRound(p->width()*t),qRound(p->height()*t)), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	}
	else
		pixmap = QPixmap();
}

////////////////////////////////////////////////////////////////////////////////

void PixmapCache::Instantiate()
{
	if( !sm_Instance )
		sm_Instance = new PixmapCache();
}

////////////////////////////////////////////////////////////////////////////////

void PixmapCache::Shutdown()
{
	if( sm_Instance )
	{
		delete sm_Instance;
		sm_Instance = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool PixmapCache::FitRectInBounds(const QRect &bounds, Qt::Alignment alignment, QRect &r)
{
	if(!bounds.size().isEmpty() && !r.size().isEmpty())
	{
		// does it fit?
		if(r.width()>bounds.width() || r.height()>bounds.height())
		{
			// nope, need to shrink it down

			// scale to bounds width
			float t = (bounds.width() / static_cast<float>(r.width()));
			int h = qRound(r.height() * t);
			if(h > bounds.height())
			{
				// doesn't fit, scale to bounds height
				t = (bounds.height() / static_cast<float>(r.height()));
				r.setWidth( qRound(r.width()*t) );
				r.setHeight( bounds.height() );
			}
			else
			{
				// it fits, great success
				r.setWidth( bounds.width() );
				r.setHeight(h);
			}
		}

		// horizontal alignment
		if( alignment.testFlag(Qt::AlignHCenter) )
			r.moveLeft( qRound(bounds.left() + (bounds.width()-r.width())*0.5f) );
		else if( alignment.testFlag(Qt::AlignRight) )
			r.moveLeft(bounds.right() - r.width());
		else
			r.moveLeft( bounds.left() );

		// vertical alignment
		if( alignment.testFlag(Qt::AlignVCenter) )
			r.moveTop( qRound(bounds.top() + (bounds.height()-r.height())*0.5f) );
		else if( alignment.testFlag(Qt::AlignBottom) )
			r.moveTop(bounds.bottom() - r.height());
		else
			r.moveTop( bounds.top() );

		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

void Utils::RegisterTouchWidget(QWidget &widget)
{
#ifdef WIN32
	widget.winId();
#endif

	widget.setAttribute(Qt::WA_AcceptTouchEvents);

#ifdef WIN32
	if(sm_pFuncRegisterTouchWindow==0 || sm_pFuncUnregisterTouchWindow==0)
	{
        HMODULE hModule = GetModuleHandle(L"user32");
		if( hModule )
		{
			sm_pFuncRegisterTouchWindow = GetProcAddress(hModule,"RegisterTouchWindow");
			if( sm_pFuncRegisterTouchWindow )
				sm_pFuncUnregisterTouchWindow = GetProcAddress(hModule,"UnregisterTouchWindow");
		}
	}

	if(sm_pFuncRegisterTouchWindow && sm_pFuncUnregisterTouchWindow)
	{
        reinterpret_cast<FuncUnregisterTouchWindow>(sm_pFuncUnregisterTouchWindow)((HWND)widget.winId());
        reinterpret_cast<FuncRegisterTouchWindow>(sm_pFuncRegisterTouchWindow)((HWND)widget.winId(),TWF_WANTPALM);
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////

#ifdef WIN32

LRESULT CALLBACK EosMouseMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	// when drag-n-drop in progress, do not filter anything
	if(nCode >= 0)
	{
		// consume fake mouse events from Windows touch handling
		MOUSEHOOKSTRUCT *info = reinterpret_cast<MOUSEHOOKSTRUCT*>(lParam);
		if(info && (info->dwExtraInfo&MOUSE_TOUCH_MASK)==MOUSE_TOUCH_SIGNATURE)
			return 1;
	}

	return CallNextHookEx(0, nCode, wParam, lParam);
}

#endif

////////////////////////////////////////////////////////////////////////////////

void Utils::BlockFakeMouseEvents(bool b)
{
	Q_UNUSED(b);
#ifdef WIN32
	if( b )
	{
		if(sm_MouseMsgHook == 0)
			sm_MouseMsgHook = SetWindowsHookEx(WH_MOUSE, EosMouseMsgProc, 0, GetCurrentThreadId());
	}
	else if( sm_MouseMsgHook )
	{
		UnhookWindowsHookEx(sm_MouseMsgHook);
		sm_MouseMsgHook = 0;
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////

bool Utils::IsLocalOSCPath(const QString &path)
{
	return path.startsWith(OSC_LOCAL, Qt::CaseInsensitive);
}

////////////////////////////////////////////////////////////////////////////////

bool Utils::MakeLocalOSCPath(bool b, QString &path)
{
	if( b )
	{
		if( !IsLocalOSCPath(path) )
		{
			path.prepend(OSC_LOCAL);
			return true;
		}
	}
	else if( IsLocalOSCPath(path) )
	{
		path = path.right(path.size()-QString(OSC_LOCAL).size());
		return true;
	}
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool Utils::IsBrightColor(const QColor &color)
{
	qreal luminance = (0.299*color.redF() + 0.587*color.greenF() + 0.114*color.blueF());
	return (luminance > 0.5);
}

////////////////////////////////////////////////////////////////////////////////

void Utils::MakeContrastingColor(float percent, QColor &color)
{
	return (IsBrightColor(color) ? MakeDarkerColor(percent,color) : MakeBrighterColor(percent,color));
}

////////////////////////////////////////////////////////////////////////////////

void Utils::MakeBrighterColor(float percent, QColor &color)
{
	int n = qRound(COLOR_MAG * percent);
	color.setRed( qMin(color.red()+n,255) );
	color.setGreen( qMin(color.green()+n,255) );
	color.setBlue( qMin(color.blue()+n,255) );
}

////////////////////////////////////////////////////////////////////////////////

void Utils::MakeDarkerColor(float percent, QColor &color)
{
	int n = qRound(COLOR_MAG * percent);
	color.setRed( qMax(color.red()-n,0) );
	color.setGreen( qMax(color.green()-n,0) );
	color.setBlue( qMax(color.blue()-n,0) );
}

////////////////////////////////////////////////////////////////////////////////

void Utils::Snap(int snap, int &value)
{
	value = ((snap==0) ? 0 : (qRound(static_cast<float>(value)/snap) * snap));
}

////////////////////////////////////////////////////////////////////////////////

void Utils::Snap(int snap, QPoint &point)
{
	Snap(snap, point.rx());
	Snap(snap, point.ry());
}

////////////////////////////////////////////////////////////////////////////////
