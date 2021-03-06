/*
 * Copyright (C) 2018-2019 Eric Tung <libpronet@gmail.com>
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
 * This file is part of LibProNet (https://github.com/libpronet/libpronet)
 */

/*
 * This is an implementation of the "Template Method" pattern.
 */

#if !defined(RTP_SESSION_BASE_H)
#define RTP_SESSION_BASE_H

#include "rtp_base.h"
#include "rtp_packet.h"
#include "../pro_net/pro_net.h"
#include "../pro_util/pro_bsd_wrapper.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_thread_mutex.h"
#include "../pro_util/pro_timer_factory.h"

/////////////////////////////////////////////////////////////////////////////
////

#define RTP_SESSION_PROTOCOL_VERSION 2

/////////////////////////////////////////////////////////////////////////////
////

class CRtpSessionBase
:
public IRtpSession,
public IProTransportObserver,
public IProOnTimer,
public CProRefCount
{
public:

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

protected:

    CRtpSessionBase(bool suspendRecv);

    virtual ~CRtpSessionBase();

    virtual void PRO_CALLTYPE GetInfo(RTP_SESSION_INFO* info) const;

    virtual void PRO_CALLTYPE GetAck(RTP_SESSION_ACK* ack) const;

    virtual void PRO_CALLTYPE GetSyncId(unsigned char syncId[14]) const;

    virtual PRO_SSL_SUITE_ID PRO_CALLTYPE GetSslSuite(
        char suiteName[64]
        ) const;

    virtual PRO_INT64 PRO_CALLTYPE GetSockId() const;

    virtual const char* PRO_CALLTYPE GetLocalIp(char localIp[64]) const;

    virtual unsigned short PRO_CALLTYPE GetLocalPort() const;

    virtual const char* PRO_CALLTYPE GetRemoteIp(char remoteIp[64]) const;

    virtual unsigned short PRO_CALLTYPE GetRemotePort() const;

    virtual void PRO_CALLTYPE SetRemoteIpAndPort(
        const char*    remoteIp,  /* = NULL */
        unsigned short remotePort /* = 0 */
        )
    {
    }

    virtual bool PRO_CALLTYPE IsTcpConnected() const;

    virtual bool PRO_CALLTYPE IsReady() const;

    virtual bool PRO_CALLTYPE SendPacket(
        IRtpPacket* packet,
        bool*       tryAgain         /* = NULL */
        );

    virtual bool PRO_CALLTYPE SendPacketByTimer(
        IRtpPacket*   packet,
        unsigned long sendDurationMs /* = 0 */
        )
    {
        return (false);
    }

    virtual void PRO_CALLTYPE GetSendOnSendTick(
        PRO_INT64* onSendTick1,      /* = NULL */
        PRO_INT64* onSendTick2       /* = NULL */
        ) const;

    virtual void PRO_CALLTYPE RequestOnSend();

    virtual void PRO_CALLTYPE SuspendRecv();

    virtual void PRO_CALLTYPE ResumeRecv();

    virtual bool PRO_CALLTYPE AddMcastReceiver(const char* mcastIp)
    {
        return (false);
    }

    virtual void PRO_CALLTYPE RemoveMcastReceiver(const char* mcastIp)
    {
    }

    virtual void PRO_CALLTYPE EnableInput(bool enable)
    {
    }

    virtual void PRO_CALLTYPE EnableOutput(bool enable)
    {
    }

    virtual void PRO_CALLTYPE SetOutputRedline(
        unsigned long redlineBytes,   /* = 0 */
        unsigned long redlineFrames,  /* = 0 */
        unsigned long redlineDelayMs  /* = 0 */
        )
    {
    }

    virtual void PRO_CALLTYPE GetOutputRedline(
        unsigned long* redlineBytes,  /* = NULL */
        unsigned long* redlineFrames, /* = NULL */
        unsigned long* redlineDelayMs /* = NULL */
        ) const
    {
    }

    virtual void PRO_CALLTYPE GetFlowctrlInfo(
        float*         srcFrameRate, /* = NULL */
        float*         srcBitRate,   /* = NULL */
        float*         outFrameRate, /* = NULL */
        float*         outBitRate,   /* = NULL */
        unsigned long* cachedBytes,  /* = NULL */
        unsigned long* cachedFrames  /* = NULL */
        ) const
    {
    }

    virtual void PRO_CALLTYPE ResetFlowctrlInfo()
    {
    }

    virtual void PRO_CALLTYPE GetInputStat(
        float*      frameRate, /* = NULL */
        float*      bitRate,   /* = NULL */
        float*      lossRate,  /* = NULL */
        PRO_UINT64* lossCount  /* = NULL */
        ) const
    {
    }

    virtual void PRO_CALLTYPE GetOutputStat(
        float*      frameRate, /* = NULL */
        float*      bitRate,   /* = NULL */
        float*      lossRate,  /* = NULL */
        PRO_UINT64* lossCount  /* = NULL */
        ) const
    {
    }

    virtual void PRO_CALLTYPE ResetInputStat()
    {
    }

    virtual void PRO_CALLTYPE ResetOutputStat()
    {
    }

    virtual void PRO_CALLTYPE SetMagic(PRO_INT64 magic);

    virtual PRO_INT64 PRO_CALLTYPE GetMagic() const;

    virtual void PRO_CALLTYPE OnSend(
        IProTransport* trans,
        PRO_UINT64     actionId
        );

    virtual void PRO_CALLTYPE OnClose(
        IProTransport* trans,
        long           errorCode,
        long           sslCode
        );

    virtual void PRO_CALLTYPE OnHeartbeat(IProTransport* trans);

    virtual void PRO_CALLTYPE OnTimer(
        void*      factory,
        PRO_UINT64 timerId,
        PRO_INT64  userData
        );

    virtual void Fini()
    {
    }

protected:

    const bool              m_suspendRecv;
    RTP_SESSION_INFO        m_info;
    RTP_SESSION_ACK         m_ack;
    PRO_INT64               m_magic;
    IRtpSessionObserver*    m_observer;
    IProReactor*            m_reactor;
    IProTransport*          m_trans;
    pbsd_sockaddr_in        m_localAddr;
    pbsd_sockaddr_in        m_remoteAddr;
    pbsd_sockaddr_in        m_remoteAddrConfig; /* for udp */
    PRO_INT64               m_dummySockId;
    PRO_UINT64              m_actionId;
    PRO_INT64               m_initTick;
    PRO_INT64               m_sendTick;
    PRO_INT64               m_onSendTick1;      /* for tcp, tcp_ex, ssl_ex */
    PRO_INT64               m_onSendTick2;      /* for tcp, tcp_ex, ssl_ex */
    PRO_INT64               m_peerAliveTick;
    PRO_UINT64              m_timeoutTimerId;
    PRO_UINT64              m_onOkTimerId;
    bool                    m_tcpConnected;     /* for tcp, tcp_ex, ssl_ex */
    bool                    m_handshakeOk;      /* for udp_ex, tcp_ex, ssl_ex */
    bool                    m_onOkCalled;
    CRtpPacket*             m_bigPacket;
    mutable CProThreadMutex m_lock;

    bool                    m_canUpcall;
    CProThreadMutex         m_lockUpcall;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_BASE_H */
