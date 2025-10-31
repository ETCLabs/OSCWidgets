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

#include "LogFile.h"
#include <time.h>

////////////////////////////////////////////////////////////////////////////////

LogFile::LogFile()
	: m_Run(false)
	, m_FileDepth(0)
{
}

////////////////////////////////////////////////////////////////////////////////

LogFile::~LogFile()
{
	Shutdown();
}

////////////////////////////////////////////////////////////////////////////////

void LogFile::Initialize(const QString &path, int fileDepth)
{
	Shutdown();

	if(fileDepth > 0)
	{
		m_Run = true;
		m_Q.clear();
		m_Path = path;
		m_FileDepth = fileDepth;
		start();
	}
}

////////////////////////////////////////////////////////////////////////////////

void LogFile::Shutdown()
{
	m_Run = false;
	wait();
}

////////////////////////////////////////////////////////////////////////////////

void LogFile::Log(EosLog::LOG_Q &logQ)
{
	if( m_Run )
	{
		m_Mutex.lock();
		m_Q.swap(logQ);
		m_Mutex.unlock();
	}
}

////////////////////////////////////////////////////////////////////////////////

void LogFile::run()
{
	while( m_Run )
	{
		QFile file(m_Path);
		if( file.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text) )
		{
			QTextStream stream( &file );
            stream.setEncoding(QStringConverter::Utf8);

			EosLog::LOG_Q q;
			int lineCount = 0;
			bool restart = false;

			for(;;)
			{
				q.clear();
				m_Mutex.lock();
				q.swap(m_Q);
				m_Mutex.unlock();

				for(EosLog::LOG_Q::const_iterator i=q.begin(); i!=q.end(); i++)
				{
					const EosLog::sLogMsg logMsg = *i;

					tm *t = localtime( &logMsg.timestamp );

					QString msgText;
					if( logMsg.text.c_str() )
						msgText = QString::fromUtf8( logMsg.text.c_str() );

					QString itemText = QString("[ %1:%2:%3 ]  %4")
						.arg(t->tm_hour, 2)
						.arg(t->tm_min, 2, 10, QChar('0'))
						.arg(t->tm_sec, 2, 10, QChar('0'))
						.arg( msgText );

					stream << itemText;
					stream << "\n";

					if(++lineCount > m_FileDepth)
					{
						restart = true;
						break;
					}
				}

				if(restart || !m_Run)
					break;

				stream.flush();

				msleep(100);
			}

			stream.flush();
			file.close();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
