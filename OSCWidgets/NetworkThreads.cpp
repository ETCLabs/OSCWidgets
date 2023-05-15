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

#include "NetworkThreads.h"
#include "EosUdp.h"
#include "EosTcp.h"
#include "EosTimer.h"

#ifdef WIN32
	#include <WinSock2.h>
#else
	#include <netinet/in.h>
#endif

////////////////////////////////////////////////////////////////////////////////

OSCHandler::OSCHandler(Client &client)
	: m_pClient(&client)
{
}

////////////////////////////////////////////////////////////////////////////////

bool OSCHandler::ProcessPacket(OSCParserClient &client, char *buf, size_t size)
{
	m_pClient->OSCHandlerClient_Recv(client, buf, size);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

EosUdpOutThread::EosUdpOutThread()
	: m_Port(0)
	, m_Run(false)
	, m_Mutex(QMutex::Recursive)
{
}

////////////////////////////////////////////////////////////////////////////////

EosUdpOutThread::~EosUdpOutThread()
{
	Stop();
}

////////////////////////////////////////////////////////////////////////////////

void EosUdpOutThread::Start(const QString &ip, unsigned short port)
{
	Stop();

	m_Ip = ip;
	m_Port = port;
	m_Run = true;
	m_NetEventQ.clear();
	start();
}

////////////////////////////////////////////////////////////////////////////////

void EosUdpOutThread::Stop()
{
	m_Run = false;
	wait();
	
	for(PACKET_Q::const_iterator i=m_Q.begin(); i!=m_Q.end(); i++)
		delete[] i->data;
	m_Q.clear();
}

////////////////////////////////////////////////////////////////////////////////

bool EosUdpOutThread::Send(sPacket &packet)
{
	if(packet.data && packet.size!=0)
	{
		m_Mutex.lock();
		m_Q.push_back(packet);
		m_Mutex.unlock();
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void EosUdpOutThread::Flush(EosLog::LOG_Q &logQ, NETEVENT_Q &netEventQ)
{
	m_Mutex.lock();
	m_Log.Flush(logQ);
	m_NetEventQ.swap(netEventQ);
	m_Mutex.unlock();
}

////////////////////////////////////////////////////////////////////////////////

void EosUdpOutThread::run()
{
	QString msg = QString("udp output %1:%2 thread started").arg(m_Ip).arg(m_Port);
	m_PrivateLog.AddInfo( msg.toUtf8().constData() );
	UpdateLog();
	
	const unsigned int ReconnectDelay = 5000;
	EosTimer reconnectTimer;
	
	m_Prefix = QString("OUT [%1:%2] ").arg(m_Ip).arg(m_Port).toUtf8().constData();
	
	// outer loop for auto-reconnect
	while( m_Run )
	{
		EosUdpOut *udpOut = EosUdpOut::Create();
		if( udpOut->Initialize(m_PrivateLog,m_Ip.toUtf8().constData(),m_Port) )
		{
			m_Mutex.lock();
			m_NetEventQ.push_back(NET_EVENT_CONNECTED);
			m_Mutex.unlock();
		
			OSCParser logParser;
			logParser.SetRoot(new OSCMethod());

			// run
			PACKET_Q q;
			while( m_Run )
			{
				m_Mutex.lock();
				m_Q.swap(q);
				m_Mutex.unlock();
				
				for(PACKET_Q::const_iterator i=q.begin(); i!=q.end(); i++)
				{
					if( udpOut->SendPacket(m_PrivateLog,i->data,static_cast<int>(i->size)) )
						logParser.PrintPacket(*this, i->data, i->size);
					delete[] i->data;
				}
				q.clear();
				
				UpdateLog();

				msleep(1);
			}
			
			m_Mutex.lock();
			m_NetEventQ.push_back(NET_EVENT_DISCONNECTED);
			m_Mutex.unlock();
		}
		
		delete udpOut;
		
		if( m_Run )
		{
			msg = QString("udp output %1:%2 reconnecting in %3...").arg(m_Ip).arg(m_Port).arg(ReconnectDelay/1000);
			m_PrivateLog.AddInfo( msg.toUtf8().constData() );
			UpdateLog();
		}

		reconnectTimer.Start();
		while(m_Run && !reconnectTimer.GetExpired(ReconnectDelay))
			msleep(10);
	}

	msg = QString("udp output %1:%2 thread ended").arg(m_Ip).arg(m_Port);
	m_PrivateLog.AddInfo( msg.toUtf8().constData() );
	UpdateLog();
}

////////////////////////////////////////////////////////////////////////////////

void EosUdpOutThread::OSCParserClient_Log(const std::string &message)
{
	m_LogMsg = (m_Prefix + message);
	m_PrivateLog.Add(EosLog::LOG_MSG_TYPE_SEND, m_LogMsg);
}

////////////////////////////////////////////////////////////////////////////////

void EosUdpOutThread::UpdateLog()
{
	m_Mutex.lock();
	m_Log.AddLog(m_PrivateLog);
	m_Mutex.unlock();

	m_PrivateLog.Clear();
}

////////////////////////////////////////////////////////////////////////////////

EosUdpInThread::EosUdpInThread()
	: m_Port(0)
	, m_Run(false)
	, m_Mutex(QMutex::Recursive)
{
}

////////////////////////////////////////////////////////////////////////////////

EosUdpInThread::~EosUdpInThread()
{
	Stop();
}

////////////////////////////////////////////////////////////////////////////////

void EosUdpInThread::Start(const QString &ip, unsigned short port)
{
	Stop();

	m_Ip = ip;
	m_Port = port;
	m_Run = true;
	start();
}

////////////////////////////////////////////////////////////////////////////////

void EosUdpInThread::Stop()
{
	m_Run = false;
	wait();
	
	for(PACKET_Q::const_iterator i=m_Q.begin(); i!=m_Q.end(); i++)
		delete[] i->data;
	m_Q.clear();
}

////////////////////////////////////////////////////////////////////////////////

void EosUdpInThread::Flush(EosLog::LOG_Q &logQ, PACKET_Q &recvQ)
{
	recvQ.clear();
	
	m_Mutex.lock();
	m_Log.Flush(logQ);
	m_Q.swap(recvQ);
	m_Mutex.unlock();
}

////////////////////////////////////////////////////////////////////////////////

void EosUdpInThread::run()
{
	QString msg = QString("udp input %1:%2 thread started").arg(m_Ip).arg(m_Port);
	m_PrivateLog.AddInfo( msg.toUtf8().constData() );
	UpdateLog();

	const size_t ReconnectDelay = 5000;
	EosTimer reconnectTimer;

	// outer loop for auto-reconnect
	while( m_Run )
	{
		EosUdpIn *udpIn = EosUdpIn::Create();
		if( udpIn->Initialize(m_PrivateLog,m_Ip.toUtf8().constData(),m_Port) )
		{
			sockaddr_in addr;
			
			OSCParser parser;
			parser.SetRoot(new OSCHandler(*this));
			
			char *scratch = 0;
			size_t scratchSize = 0;

			// run
			while( m_Run )
			{
				int len = 0;
				int addrSize = static_cast<int>( sizeof(addr) );
				const char *data = udpIn->RecvPacket(m_PrivateLog, 100, 0, len, &addr, &addrSize);
				if(data && len>0)
				{
					QHostAddress host( reinterpret_cast<const sockaddr*>(&addr) );
					m_Prefix = QString("IN  [%1:%2] ").arg( host.toString() ).arg(m_Port).toUtf8().constData();
					parser.PrintPacket(*this, data, static_cast<size_t>(len));

					if(scratch!=0 && scratchSize<static_cast<size_t>(len))
					{
						delete[] scratch;
						scratch = 0;
					}

					if(scratch == 0)
					{
						scratchSize = static_cast<size_t>(len);
						scratch = new char[scratchSize];
					}

					memcpy(scratch, data, static_cast<size_t>(len));
					parser.ProcessPacket(*this, scratch, static_cast<size_t>(len));
				}
			
				UpdateLog();

				msleep(1);
			}

			if(scratch != 0)
				delete[] scratch;
		}

		delete udpIn;

		if( m_Run )
		{
			msg = QString("udp input %1:%2 reconnecting in %3...").arg(m_Ip).arg(m_Port).arg(ReconnectDelay/1000);
			m_PrivateLog.AddInfo( msg.toUtf8().constData() );
			UpdateLog();
		}

		reconnectTimer.Start();
		while(m_Run && !reconnectTimer.GetExpired(ReconnectDelay))
			msleep(10);
	}
	
	msg = QString("udp input %1:%2 thread ended").arg(m_Ip).arg(m_Port);
	m_PrivateLog.AddInfo( msg.toUtf8().constData() );
	UpdateLog();
}

////////////////////////////////////////////////////////////////////////////////

void EosUdpInThread::UpdateLog()
{
	m_Mutex.lock();
	m_Log.AddLog(m_PrivateLog);
	m_Mutex.unlock();

	m_PrivateLog.Clear();
}

////////////////////////////////////////////////////////////////////////////////

void EosUdpInThread::OSCParserClient_Log(const std::string &message)
{
	m_LogMsg = (m_Prefix + message);
	m_PrivateLog.Add(EosLog::LOG_MSG_TYPE_RECV, m_LogMsg);
}

////////////////////////////////////////////////////////////////////////////////

void EosUdpInThread::OSCHandlerClient_Recv(OSCParserClient& /*client*/, char *buf, size_t size)
{
	if(buf!=0 && size!=0)
	{
		sPacket packet;
		packet.size = size;
		packet.data = new char[packet.size];
		memcpy(packet.data, buf, packet.size);

		m_Mutex.lock();
		m_Q.push_back(packet);
		m_Mutex.unlock();
	}
}

////////////////////////////////////////////////////////////////////////////////

EosTcpClientThread::EosTcpClientThread()
	: m_Port(0)
	, m_Run(false)
	, m_Mutex(QMutex::Recursive)
	, m_LogMsgType(EosLog::LOG_MSG_TYPE_INFO)
{
}

////////////////////////////////////////////////////////////////////////////////

EosTcpClientThread::~EosTcpClientThread()
{
	Stop();
}

////////////////////////////////////////////////////////////////////////////////

void EosTcpClientThread::Start(const QString &ip, unsigned short port, OSCStream::EnumFrameMode frameMode)
{
	Stop();

	m_Ip = ip;
	m_Port = port;
	m_FrameMode = frameMode;
	m_Run = true;
	m_NetEventQ.clear();
	start();
}

////////////////////////////////////////////////////////////////////////////////

void EosTcpClientThread::Stop()
{
	m_Run = false;
	wait();
	
	for(PACKET_Q::const_iterator i=m_SendQ.begin(); i!=m_SendQ.end(); i++)
		delete[] i->data;
	m_SendQ.clear();
	
	for(PACKET_Q::const_iterator i=m_RecvQ.begin(); i!=m_RecvQ.end(); i++)
		delete[] i->data;
	m_RecvQ.clear();
}

////////////////////////////////////////////////////////////////////////////////

bool EosTcpClientThread::Send(sPacket &packet)
{
	if(packet.data && packet.size!=0)
	{
		m_Mutex.lock();
		m_SendQ.push_back(packet);
		m_Mutex.unlock();
		return true;
	}	
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void EosTcpClientThread::Flush(EosLog::LOG_Q &logQ, PACKET_Q &recvQ, NETEVENT_Q &netEventQ)
{
	recvQ.clear();
	
	m_Mutex.lock();
	m_Log.Flush(logQ);
	m_RecvQ.swap(recvQ);
	m_NetEventQ.swap(netEventQ);
	m_Mutex.unlock();
}

////////////////////////////////////////////////////////////////////////////////

void EosTcpClientThread::run()
{
	QString msg = QString("tcp client %1:%2 thread started").arg(m_Ip).arg(m_Port);
	m_PrivateLog.AddInfo( msg.toUtf8().constData() );
	UpdateLog();
	
	const size_t ReconnectDelay = 5000;
	EosTimer reconnectTimer;

	// outer loop for auto-reconnect
	while( m_Run )
	{
		EosTcp *tcp = EosTcp::Create();

		if( tcp->Initialize(m_PrivateLog,m_Ip.toUtf8().constData(),m_Port) )
		{
			OSCParser parser;
			parser.SetRoot(new OSCHandler(*this));
			std::string inPrefix = QString("TCPIN [%1:%2] ").arg(m_Ip).arg(m_Port).toUtf8().constData();
			std::string outPrefix = QString("TCPOUT [%1:%2] ").arg(m_Ip).arg(m_Port).toUtf8().constData();
			
			sPacket packet;

			// connect
			while(m_Run && tcp->GetConnectState()==EosTcp::CONNECT_IN_PROGRESS)
			{
				tcp->Tick(m_PrivateLog);
				UpdateLog();
				msleep(10);
			}

			UpdateLog();

			// send/recv while connected
			if(m_Run && tcp->GetConnectState()==EosTcp::CONNECT_CONNECTED)
			{
				m_Mutex.lock();
				m_NetEventQ.push_back(NET_EVENT_CONNECTED);
				m_Mutex.unlock();
			
				PACKET_Q sendQ;
				OSCStream oscStream(m_FrameMode);
				do
				{
					size_t len = 0;
					const char *data = tcp->Recv(m_PrivateLog, 100, len);
					
					oscStream.Add(data, len);
					
					while( m_Run )
					{
						packet.data = oscStream.GetNextFrame(packet.size);
						if(packet.data && packet.size!=0)
						{
							m_Prefix = inPrefix;
							m_LogMsgType = EosLog::LOG_MSG_TYPE_RECV;
							parser.PrintPacket(*this, packet.data, packet.size);
							parser.ProcessPacket(*this, packet.data, packet.size);
							delete[] packet.data;
						}
						else
							break;
					}

					msleep(1);

					m_Mutex.lock();
					m_SendQ.swap(sendQ);
					m_Mutex.unlock();

					sPacket framedPacket;
					for(PACKET_Q::iterator i=sendQ.begin(); m_Run && i!=sendQ.end(); i++)
					{
						framedPacket.size = i->size;
						framedPacket.data = OSCStream::CreateFrame(m_FrameMode, i->data, framedPacket.size);
						if(framedPacket.data && framedPacket.size!=0)
						{
							if( tcp->Send(m_PrivateLog,framedPacket.data,framedPacket.size) )
							{
								m_Prefix = outPrefix;
								m_LogMsgType = EosLog::LOG_MSG_TYPE_SEND;
								parser.PrintPacket(*this, i->data, i->size);
							}
							delete[] framedPacket.data;
						}
						delete[] i->data;
					}
					sendQ.clear();
				
					UpdateLog();

					msleep(1);
				}
				while(m_Run && tcp->GetConnectState()==EosTcp::CONNECT_CONNECTED);
				
				m_Mutex.lock();
				m_NetEventQ.push_back(NET_EVENT_DISCONNECTED);
				m_Mutex.unlock();
			}
		}

		delete tcp;

		if( m_Run )
		{
			msg = QString("tcp client %1:%2 reconnecting in %3...").arg(m_Ip).arg(m_Port).arg(ReconnectDelay/1000);
			m_PrivateLog.AddInfo( msg.toUtf8().constData() );
			UpdateLog();
		}

		reconnectTimer.Start();
		while(m_Run && !reconnectTimer.GetExpired(ReconnectDelay))
			msleep(10);
	}
	
	msg = QString("tcp client %1:%2 thread ended").arg(m_Ip).arg(m_Port);
	m_PrivateLog.AddInfo( msg.toUtf8().constData() );
	UpdateLog();
}

////////////////////////////////////////////////////////////////////////////////

void EosTcpClientThread::UpdateLog()
{
	m_Mutex.lock();
	m_Log.AddLog(m_PrivateLog);
	m_Mutex.unlock();

	m_PrivateLog.Clear();
}

////////////////////////////////////////////////////////////////////////////////

void EosTcpClientThread::OSCParserClient_Log(const std::string &message)
{
	m_LogMsg = (m_Prefix + message);
	m_PrivateLog.Add(m_LogMsgType, m_LogMsg);
}

////////////////////////////////////////////////////////////////////////////////

void EosTcpClientThread::OSCHandlerClient_Recv(OSCParserClient& /*client*/, char *buf, size_t size)
{
	if(buf!=0 && size!=0)
	{
		sPacket packet;
		packet.size = size;
		packet.data = new char[packet.size];
		memcpy(packet.data, buf, packet.size);

		m_Mutex.lock();
		m_RecvQ.push_back(packet);
		m_Mutex.unlock();
	}
}

////////////////////////////////////////////////////////////////////////////////
