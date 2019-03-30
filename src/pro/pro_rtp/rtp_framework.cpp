/*
 * Copyright (C) 2018 Eric Tung <libpronet@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file is part of LibProNet (http://www.libpro.org)
 */

#include "rtp_framework.h"
#include "rtp_packet.h"
#include "rtp_port_allocator.h"
#include "rtp_service.h"
#include "rtp_session_mcast.h"
#include "rtp_session_mcast_ex.h"
#include "rtp_session_sslclient_ex.h"
#include "rtp_session_sslserver_ex.h"
#include "rtp_session_tcpclient.h"
#include "rtp_session_tcpclient_ex.h"
#include "rtp_session_tcpserver.h"
#include "rtp_session_tcpserver_ex.h"
#include "rtp_session_udpclient.h"
#include "rtp_session_udpclient_ex.h"
#include "rtp_session_udpserver.h"
#include "rtp_session_udpserver_ex.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_file_monitor.h"
#include "../pro_util/pro_ssl_util.h"
#include "../pro_util/pro_stl.h"
#include "../pro_util/pro_version.h"
#include "../pro_util/pro_z.h"

#if defined(WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#endif

#include <cassert>
#include <cstddef>

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

#define TRACE_EXT_NAME ".pro_rtp.trace"

#if defined(WIN32) && !defined(_WIN32_WCE)
CProFileMonitor               g_fileMonitor;
#endif

static CRtpPortAllocator*     g_s_udpPortAllocator   = NULL;
static CRtpPortAllocator*     g_s_tcpPortAllocator   = NULL;
static volatile unsigned long g_s_keepaliveInSeconds = 60;
static volatile unsigned long g_s_flowctrlInSeconds  = 1;
static volatile unsigned long g_s_statInSeconds      = 5;
static size_t                 g_s_udpSockBufSizeRecv[256]; /* mmType0 ~ mmType255 */
static size_t                 g_s_udpSockBufSizeSend[256]; /* mmType0 ~ mmType255 */
static size_t                 g_s_udpRecvPoolSize[256];    /* mmType0 ~ mmType255 */
static size_t                 g_s_tcpSockBufSizeRecv[256]; /* mmType0 ~ mmType255 */
static size_t                 g_s_tcpSockBufSizeSend[256]; /* mmType0 ~ mmType255 */
static size_t                 g_s_tcpRecvPoolSize[256];    /* mmType0 ~ mmType255 */

/////////////////////////////////////////////////////////////////////////////
////

PRO_RTP_API
void
PRO_CALLTYPE
ProRtpInit()
{
    static bool s_flag = false;
    if (s_flag)
    {
        return;
    }
    s_flag = true;

    ProNetInit();

    g_s_udpPortAllocator = new CRtpPortAllocator;
    g_s_tcpPortAllocator = new CRtpPortAllocator;

    for (int i = 0; i < 256; ++i)
    {
        g_s_udpSockBufSizeRecv[i] = 1024 * 56;
        g_s_udpSockBufSizeSend[i] = 1024 * 56;
        g_s_udpRecvPoolSize[i]    = 1024 * 65;

        g_s_tcpSockBufSizeRecv[i] = 1024 * 56;
        g_s_tcpSockBufSizeSend[i] = 1024 * 8;
        g_s_tcpRecvPoolSize[i]    = 1024 * 65;
    }

#if defined(WIN32) && !defined(_WIN32_WCE)
    char exeRoot[1024] = "";
    exeRoot[sizeof(exeRoot) - 1] = '\0';
    ::GetModuleFileName(NULL, exeRoot, sizeof(exeRoot) - 1);
    ::GetLongPathName(exeRoot, exeRoot, sizeof(exeRoot) - 1);
    strcat(exeRoot, TRACE_EXT_NAME);
    g_fileMonitor.Init(exeRoot);
    g_fileMonitor.UpdateFileExist();
#endif
}

PRO_RTP_API
void
PRO_CALLTYPE
ProRtpVersion(unsigned char* major, /* = NULL */
              unsigned char* minor, /* = NULL */
              unsigned char* patch) /* = NULL */
{
    if (major != NULL)
    {
        *major = PRO_VER_MAJOR;
    }
    if (minor != NULL)
    {
        *minor = PRO_VER_MINOR;
    }
    if (patch != NULL)
    {
        *patch = PRO_VER_PATCH;
    }
}

PRO_RTP_API
IRtpPacket*
PRO_CALLTYPE
CreateRtpPacket(const void*       payloadBuffer,
                unsigned long     payloadSize,
                RTP_EXT_PACK_MODE packMode)      /* = RTP_EPM_DEFAULT */
{
    CRtpPacket* const packet =
        CRtpPacket::CreateInstance(payloadBuffer, payloadSize, packMode);

    return (packet);
}

PRO_RTP_API
IRtpPacket*
PRO_CALLTYPE
CreateRtpPacketSpace(unsigned long     payloadSize,
                     RTP_EXT_PACK_MODE packMode) /* = RTP_EPM_DEFAULT */
{
    CRtpPacket* const packet = CRtpPacket::CreateInstance(payloadSize, packMode);

    return (packet);
}

PRO_RTP_API
IRtpPacket*
PRO_CALLTYPE
CloneRtpPacket(const IRtpPacket* packet)
{
    CRtpPacket* const newPacket = CRtpPacket::Clone(packet);

    return (newPacket);
}

PRO_RTP_API
IRtpPacket*
PRO_CALLTYPE
ParseRtpStreamToPacket(const void* streamBuffer,
                       PRO_UINT16  streamSize)
{
    RTP_HEADER  hdr;
    const char* payloadBuffer = NULL;
    PRO_UINT16  payloadSize   = 0;

    const bool ret = CRtpPacket::ParseRtpBuffer(
        (char*)streamBuffer,
        streamSize,
        hdr,
        payloadBuffer,
        payloadSize
        );
    if (!ret || payloadBuffer == NULL || payloadSize == 0)
    {
        return (NULL);
    }

    hdr.v  = 2;
    hdr.p  = 0;
    hdr.x  = 0;
    hdr.cc = 0;

    IRtpPacket* const packet =
        CRtpPacket::CreateInstance(payloadBuffer, payloadSize, RTP_EPM_DEFAULT);
    if (packet == NULL)
    {
        return (NULL);
    }

    RTP_HEADER* const packetHdr =
        (RTP_HEADER*)((char*)packet->GetPayloadBuffer() - sizeof(RTP_HEADER));
    *packetHdr = hdr;

    return (packet);
}

PRO_RTP_API
const void*
PRO_CALLTYPE
FindRtpStreamFromPacket(const IRtpPacket* packet,
                        PRO_UINT16*       streamSize)
{
    assert(packet != NULL);
    assert(streamSize != NULL);
    if (packet == NULL || streamSize == NULL)
    {
        return (NULL);
    }

    if (packet->GetPackMode() != RTP_EPM_DEFAULT)
    {
        return (NULL);
    }

    const char* const payloadBuffer = (char*)packet->GetPayloadBuffer();
    const PRO_UINT16  payloadSize   = packet->GetPayloadSize16();

    *streamSize = payloadSize + sizeof(RTP_HEADER);

    return (payloadBuffer - sizeof(RTP_HEADER));
}

PRO_RTP_API
void
PRO_CALLTYPE
SetRtpPortRange(unsigned short minUdpPort,
                unsigned short maxUdpPort,
                unsigned short minTcpPort,
                unsigned short maxTcpPort)
{
    assert(g_s_udpPortAllocator != NULL);
    assert(g_s_tcpPortAllocator != NULL);

    g_s_udpPortAllocator->SetPortRange(minUdpPort, maxUdpPort);
    g_s_tcpPortAllocator->SetPortRange(minTcpPort, maxTcpPort);
}

PRO_RTP_API
void
PRO_CALLTYPE
GetRtpPortRange(unsigned short* minUdpPort, /* = NULL */
                unsigned short* maxUdpPort, /* = NULL */
                unsigned short* minTcpPort, /* = NULL */
                unsigned short* maxTcpPort) /* = NULL */
{
    assert(g_s_udpPortAllocator != NULL);
    assert(g_s_tcpPortAllocator != NULL);

    unsigned short minUdpPort2 = 0;
    unsigned short maxUdpPort2 = 0;
    unsigned short minTcpPort2 = 0;
    unsigned short maxTcpPort2 = 0;
    g_s_udpPortAllocator->GetPortRange(minUdpPort2, maxUdpPort2);
    g_s_tcpPortAllocator->GetPortRange(minTcpPort2, maxTcpPort2);

    if (minUdpPort != NULL)
    {
        *minUdpPort = minUdpPort2;
    }
    if (maxUdpPort != NULL)
    {
        *maxUdpPort = maxUdpPort2;
    }
    if (minTcpPort != NULL)
    {
        *minTcpPort = minTcpPort2;
    }
    if (maxTcpPort != NULL)
    {
        *maxTcpPort = maxTcpPort2;
    }
}

PRO_RTP_API
unsigned short
PRO_CALLTYPE
AllocRtpUdpPort()
{
    assert(g_s_udpPortAllocator != NULL);

    return (g_s_udpPortAllocator->AllocPort());
}

PRO_RTP_API
unsigned short
PRO_CALLTYPE
AllocRtpTcpPort()
{
    assert(g_s_tcpPortAllocator != NULL);

    return (g_s_tcpPortAllocator->AllocPort());
}

PRO_RTP_API
void
PRO_CALLTYPE
SetRtpKeepaliveTimeout(unsigned long keepaliveInSeconds) /* = 60 */
{
    assert(keepaliveInSeconds > 0);
    if (keepaliveInSeconds == 0)
    {
        return;
    }

    g_s_keepaliveInSeconds = keepaliveInSeconds;
}

PRO_RTP_API
unsigned long
PRO_CALLTYPE
GetRtpKeepaliveTimeout()
{
    return (g_s_keepaliveInSeconds);
}

PRO_RTP_API
void
PRO_CALLTYPE
SetRtpFlowctrlTimeSpan(unsigned long flowctrlInSeconds) /* = 1 */
{
    assert(flowctrlInSeconds > 0);
    if (flowctrlInSeconds == 0)
    {
        return;
    }

    g_s_flowctrlInSeconds = flowctrlInSeconds;
}

PRO_RTP_API
unsigned long
PRO_CALLTYPE
GetRtpFlowctrlTimeSpan()
{
    return (g_s_flowctrlInSeconds);
}

PRO_RTP_API
void
PRO_CALLTYPE
SetRtpStatTimeSpan(unsigned long statInSeconds) /* = 5 */
{
    assert(statInSeconds > 0);
    if (statInSeconds == 0)
    {
        return;
    }

    g_s_statInSeconds = statInSeconds;
}

PRO_RTP_API
unsigned long
PRO_CALLTYPE
GetRtpStatTimeSpan()
{
    return (g_s_statInSeconds);
}

PRO_RTP_API
void
PRO_CALLTYPE
SetRtpUdpSocketParams(RTP_MM_TYPE mmType,
                      size_t      sockBufSizeRecv, /* = 0 */
                      size_t      sockBufSizeSend, /* = 0 */
                      size_t      recvPoolSize)    /* = 0 */
{
    if (sockBufSizeRecv > 0)
    {
        g_s_udpSockBufSizeRecv[mmType] = sockBufSizeRecv;
    }
    if (sockBufSizeSend > 0)
    {
        g_s_udpSockBufSizeSend[mmType] = sockBufSizeSend;
    }
    if (recvPoolSize > 0)
    {
        g_s_udpRecvPoolSize[mmType]    = recvPoolSize;
    }
}

PRO_RTP_API
void
PRO_CALLTYPE
GetRtpUdpSocketParams(RTP_MM_TYPE    mmType,
                      unsigned long* sockBufSizeRecv, /* = NULL */
                      unsigned long* sockBufSizeSend, /* = NULL */
                      unsigned long* recvPoolSize)    /* = NULL */
{
    if (sockBufSizeRecv != NULL)
    {
        *sockBufSizeRecv = (unsigned long)g_s_udpSockBufSizeRecv[mmType];
    }
    if (sockBufSizeSend != NULL)
    {
        *sockBufSizeSend = (unsigned long)g_s_udpSockBufSizeSend[mmType];
    }
    if (recvPoolSize != NULL)
    {
        *recvPoolSize    = (unsigned long)g_s_udpRecvPoolSize[mmType];
    }
}

PRO_RTP_API
void
PRO_CALLTYPE
SetRtpTcpSocketParams(RTP_MM_TYPE mmType,
                      size_t      sockBufSizeRecv, /* = 0 */
                      size_t      sockBufSizeSend, /* = 0 */
                      size_t      recvPoolSize)    /* = 0 */
{
    if (sockBufSizeRecv > 0)
    {
        g_s_tcpSockBufSizeRecv[mmType] = sockBufSizeRecv;
    }
    if (sockBufSizeSend > 0)
    {
        g_s_tcpSockBufSizeSend[mmType] = sockBufSizeSend;
    }
    if (recvPoolSize > 0)
    {
        g_s_tcpRecvPoolSize[mmType]    = recvPoolSize;
    }
}

PRO_RTP_API
void
PRO_CALLTYPE
GetRtpTcpSocketParams(RTP_MM_TYPE    mmType,
                      unsigned long* sockBufSizeRecv, /* = NULL */
                      unsigned long* sockBufSizeSend, /* = NULL */
                      unsigned long* recvPoolSize)    /* = NULL */
{
    if (sockBufSizeRecv != NULL)
    {
        *sockBufSizeRecv = (unsigned long)g_s_tcpSockBufSizeRecv[mmType];
    }
    if (sockBufSizeSend != NULL)
    {
        *sockBufSizeSend = (unsigned long)g_s_tcpSockBufSizeSend[mmType];
    }
    if (recvPoolSize != NULL)
    {
        *recvPoolSize    = (unsigned long)g_s_tcpRecvPoolSize[mmType];
    }
}

PRO_RTP_API
IRtpService*
PRO_CALLTYPE
CreateRtpService(const PRO_SSL_SERVER_CONFIG* sslConfig,        /* = NULL */
                 IRtpServiceObserver*         observer,
                 IProReactor*                 reactor,
                 RTP_MM_TYPE                  mmType,
                 unsigned short               serviceHubPort,
                 unsigned long                timeoutInSeconds) /* = 0 */
{
    ProRtpInit();

    CRtpService* const service = CRtpService::CreateInstance(sslConfig, mmType);
    if (service == NULL)
    {
        return (NULL);
    }

    if (!service->Init(observer, reactor, serviceHubPort, timeoutInSeconds))
    {
        service->Release();

        return (NULL);
    }

    return ((IRtpService*)service);
}

PRO_RTP_API
void
PRO_CALLTYPE
DeleteRtpService(IRtpService* service)
{
    if (service == NULL)
    {
        return;
    }

    CRtpService* const p = (CRtpService*)service;
    p->Fini();
    p->Release();
}

PRO_RTP_API
bool
PRO_CALLTYPE
CheckRtpServiceData(PRO_UINT64  serviceNonce,
                    const char* servicePassword,
                    const char  clientPasswordHash[32])
{
    char servicePasswordHash[32];
    ProCalcPasswordHash(serviceNonce, servicePassword, servicePasswordHash);

    return (memcmp(clientPasswordHash, servicePasswordHash, 32) == 0);
}

PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionUdpclient(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp,   /* = NULL */
                          unsigned short          localPort) /* = 0 */
{
    ProRtpInit();

    CRtpSessionUdpclient* const session = CRtpSessionUdpclient::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, localIp, localPort))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionUdpserver(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp,   /* = NULL */
                          unsigned short          localPort) /* = 0 */
{
    ProRtpInit();

    CRtpSessionUdpserver* const session = CRtpSessionUdpserver::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, localIp, localPort))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionTcpclient(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             remoteIp,
                          unsigned short          remotePort,
                          const char*             localIp,          /* = NULL */
                          unsigned long           timeoutInSeconds) /* = 0 */
{
    ProRtpInit();

    CRtpSessionTcpclient* const session = CRtpSessionTcpclient::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, remoteIp, remotePort, localIp, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionTcpserver(IRtpSessionObserver*    observer,
                          IProReactor*            reactor,
                          const RTP_SESSION_INFO* localInfo,
                          const char*             localIp,          /* = NULL */
                          unsigned short          localPort,        /* = 0 */
                          unsigned long           timeoutInSeconds) /* = 0 */
{
    ProRtpInit();

    CRtpSessionTcpserver* const session = CRtpSessionTcpserver::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, localIp, localPort, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionUdpclientEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const char*             localIp,          /* = NULL */
                            unsigned long           timeoutInSeconds) /* = 0 */
{
    ProRtpInit();

    CRtpSessionUdpclientEx* const session = CRtpSessionUdpclientEx::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, remoteIp, remotePort, localIp, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionUdpserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             localIp,          /* = NULL */
                            unsigned short          localPort,        /* = 0 */
                            unsigned long           timeoutInSeconds) /* = 0 */
{
    ProRtpInit();

    CRtpSessionUdpserverEx* const session = CRtpSessionUdpserverEx::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, localIp, localPort, timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionTcpclientEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            const char*             remoteIp,
                            unsigned short          remotePort,
                            const char*             password,         /* = NULL */
                            const char*             localIp,          /* = NULL */
                            unsigned long           timeoutInSeconds) /* = 0 */
{
    ProRtpInit();

    CRtpSessionTcpclientEx* const session = CRtpSessionTcpclientEx::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, remoteIp, remotePort, password, localIp,
        timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionTcpserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            PRO_INT64               sockId,
                            bool                    unixSocket)
{
    ProRtpInit();

    CRtpSessionTcpserverEx* const session = CRtpSessionTcpserverEx::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, sockId, unixSocket))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionSslclientEx(IRtpSessionObserver*         observer,
                            IProReactor*                 reactor,
                            const RTP_SESSION_INFO*      localInfo,
                            const PRO_SSL_CLIENT_CONFIG* sslConfig,
                            const char*                  sslServiceName,   /* = NULL */
                            const char*                  remoteIp,
                            unsigned short               remotePort,
                            const char*                  password,         /* = NULL */
                            const char*                  localIp,          /* = NULL */
                            unsigned long                timeoutInSeconds) /* = 0 */
{
    ProRtpInit();

    CRtpSessionSslclientEx* const session =
        CRtpSessionSslclientEx::CreateInstance(localInfo, sslConfig, sslServiceName);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, remoteIp, remotePort, password, localIp,
        timeoutInSeconds))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionSslserverEx(IRtpSessionObserver*    observer,
                            IProReactor*            reactor,
                            const RTP_SESSION_INFO* localInfo,
                            PRO_SSL_CTX*            sslCtx,
                            PRO_INT64               sockId,
                            bool                    unixSocket)
{
    ProRtpInit();

    CRtpSessionSslserverEx* const session =
        CRtpSessionSslserverEx::CreateInstance(localInfo, sslCtx);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, sockId, unixSocket))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionMcast(IRtpSessionObserver*    observer,
                      IProReactor*            reactor,
                      const RTP_SESSION_INFO* localInfo,
                      const char*             mcastIp,
                      unsigned short          mcastPort, /* = 0 */
                      const char*             localIp)   /* = NULL */
{
    ProRtpInit();

    CRtpSessionMcast* const session = CRtpSessionMcast::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, mcastIp, mcastPort, localIp))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

PRO_RTP_API
IRtpSession*
PRO_CALLTYPE
CreateRtpSessionMcastEx(IRtpSessionObserver*    observer,
                        IProReactor*            reactor,
                        const RTP_SESSION_INFO* localInfo,
                        const char*             mcastIp,
                        unsigned short          mcastPort, /* = 0 */
                        const char*             localIp)   /* = NULL */
{
    ProRtpInit();

    CRtpSessionMcastEx* const session = CRtpSessionMcastEx::CreateInstance(localInfo);
    if (session == NULL)
    {
        return (NULL);
    }

    if (!session->Init(observer, reactor, mcastIp, mcastPort, localIp))
    {
        session->Release();

        return (NULL);
    }

    return (session);
}

PRO_RTP_API
void
PRO_CALLTYPE
DeleteRtpSession(IRtpSession* session)
{
    if (session == NULL)
    {
        return;
    }

    RTP_SESSION_INFO sessionInfo;
    session->GetInfo(&sessionInfo);

    switch (sessionInfo.sessionType)
    {
    case RTP_ST_UDPCLIENT:
        {
            CRtpSessionUdpclient* const p = (CRtpSessionUdpclient*)session;
            p->Fini();
            p->Release();
            break;
        }

    case RTP_ST_UDPSERVER:
        {
            CRtpSessionUdpserver* const p = (CRtpSessionUdpserver*)session;
            p->Fini();
            p->Release();
            break;
        }

    case RTP_ST_TCPCLIENT:
        {
            CRtpSessionTcpclient* const p = (CRtpSessionTcpclient*)session;
            p->Fini();
            p->Release();
            break;
        }

    case RTP_ST_TCPSERVER:
        {
            CRtpSessionTcpserver* const p = (CRtpSessionTcpserver*)session;
            p->Fini();
            p->Release();
            break;
        }

    case RTP_ST_UDPCLIENT_EX:
        {
            CRtpSessionUdpclientEx* const p = (CRtpSessionUdpclientEx*)session;
            p->Fini();
            p->Release();
            break;
        }

    case RTP_ST_UDPSERVER_EX:
        {
            CRtpSessionUdpserverEx* const p = (CRtpSessionUdpserverEx*)session;
            p->Fini();
            p->Release();
            break;
        }

    case RTP_ST_TCPCLIENT_EX:
        {
            CRtpSessionTcpclientEx* const p = (CRtpSessionTcpclientEx*)session;
            p->Fini();
            p->Release();
            break;
        }

    case RTP_ST_TCPSERVER_EX:
        {
            CRtpSessionTcpserverEx* const p = (CRtpSessionTcpserverEx*)session;
            p->Fini();
            p->Release();
            break;
        }

    case RTP_ST_SSLCLIENT_EX:
        {
            CRtpSessionSslclientEx* const p = (CRtpSessionSslclientEx*)session;
            p->Fini();
            p->Release();
            break;
        }

    case RTP_ST_SSLSERVER_EX:
        {
            CRtpSessionSslserverEx* const p = (CRtpSessionSslserverEx*)session;
            p->Fini();
            p->Release();
            break;
        }

    case RTP_ST_MCAST:
        {
            CRtpSessionMcast* const p = (CRtpSessionMcast*)session;
            p->Fini();
            p->Release();
            break;
        }

    case RTP_ST_MCAST_EX:
        {
            CRtpSessionMcastEx* const p = (CRtpSessionMcastEx*)session;
            p->Fini();
            p->Release();
            break;
        }
    } /* end of switch (...) */
}

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
}
#endif
