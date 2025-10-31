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

#pragma once
#ifndef NETWORK_THREADS_H
#define NETWORK_THREADS_H

#ifndef QT_INCLUDE_H
#include "QtInclude.h"
#endif

#ifndef EOS_LOG_H
#include "EosLog.h"
#endif

#ifndef OSC_PARSER_H
#include "OSCParser.h"
#endif

#include <vector>

////////////////////////////////////////////////////////////////////////////////

struct sPacket
{
  char *data;
  size_t size;
};

enum EnumNetworkEvent
{
  NET_EVENT_CONNECTED,
  NET_EVENT_DISCONNECTED
};

typedef std::vector<sPacket> PACKET_Q;
typedef std::vector<EnumNetworkEvent> NETEVENT_Q;

////////////////////////////////////////////////////////////////////////////////

class OSCHandler : public OSCMethod
{
public:
  class Client
  {
  public:
    virtual void OSCHandlerClient_Recv(OSCParserClient &client, char *buf, size_t size) = 0;
  };

  OSCHandler(Client &client);

  virtual bool ProcessPacket(OSCParserClient &client, char *buf, size_t size);

protected:
  Client *m_pClient;
};

////////////////////////////////////////////////////////////////////////////////

class EosUdpOutThread : public QThread, private OSCParserClient
{
public:
  EosUdpOutThread();
  virtual ~EosUdpOutThread();

  virtual void Start(const QString &ip, unsigned short port);
  virtual void Stop();
  virtual bool Send(sPacket &packet);
  virtual void Flush(EosLog::LOG_Q &logQ, NETEVENT_Q &netEventQ);

protected:
  QString m_Ip;
  unsigned short m_Port;
  bool m_Run;
  EosLog m_Log;
  EosLog m_PrivateLog;
  PACKET_Q m_Q;
  NETEVENT_Q m_NetEventQ;
  QRecursiveMutex m_Mutex;
  std::string m_Prefix;
  std::string m_LogMsg;

  virtual void run();
  virtual void UpdateLog();

private:
  virtual void OSCParserClient_Log(const std::string &message);
  virtual void OSCParserClient_Send(const char *, size_t) {}
};

////////////////////////////////////////////////////////////////////////////////

class EosUdpInThread : public QThread, private OSCParserClient, private OSCHandler::Client
{
public:
  EosUdpInThread();
  virtual ~EosUdpInThread();

  virtual void Start(const QString &ip, unsigned short port);
  virtual void Stop();
  virtual void Flush(EosLog::LOG_Q &logQ, PACKET_Q &recvQ);

protected:
  QString m_Ip;
  unsigned short m_Port;
  bool m_Run;
  EosLog m_Log;
  EosLog m_PrivateLog;
  PACKET_Q m_Q;
  QRecursiveMutex m_Mutex;
  std::string m_Prefix;
  std::string m_LogMsg;

  virtual void run();
  virtual void UpdateLog();

private:
  virtual void OSCParserClient_Log(const std::string &message);
  virtual void OSCParserClient_Send(const char *, size_t) {}
  virtual void OSCHandlerClient_Recv(OSCParserClient &client, char *buf, size_t size);
};

////////////////////////////////////////////////////////////////////////////////

class EosTcpClientThread : public QThread, private OSCParserClient, private OSCHandler::Client
{
public:
  EosTcpClientThread();
  virtual ~EosTcpClientThread();

  virtual void Start(const QString &ip, unsigned short port, OSCStream::EnumFrameMode frameMode);
  virtual void Stop();
  virtual bool Send(sPacket &packet);
  virtual void Flush(EosLog::LOG_Q &logQ, PACKET_Q &recvQ, NETEVENT_Q &netEventQ);

protected:
  QString m_Ip;
  unsigned short m_Port;
  OSCStream::EnumFrameMode m_FrameMode;
  bool m_Run;
  EosLog m_Log;
  EosLog m_PrivateLog;
  PACKET_Q m_RecvQ;
  PACKET_Q m_SendQ;
  NETEVENT_Q m_NetEventQ;
  QRecursiveMutex m_Mutex;
  std::string m_Prefix;
  std::string m_LogMsg;
  EosLog::EnumLogMsgType m_LogMsgType;

  virtual void run();
  virtual void UpdateLog();

private:
  virtual void OSCParserClient_Log(const std::string &message);
  virtual void OSCParserClient_Send(const char *, size_t) {}
  virtual void OSCHandlerClient_Recv(OSCParserClient &client, char *buf, size_t size);
};

////////////////////////////////////////////////////////////////////////////////

#endif
