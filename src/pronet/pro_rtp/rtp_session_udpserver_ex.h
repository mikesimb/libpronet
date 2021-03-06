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

#if !defined(RTP_SESSION_UDPSERVER_EX_H)
#define RTP_SESSION_UDPSERVER_EX_H

#include "rtp_session_base.h"

/////////////////////////////////////////////////////////////////////////////
////

struct RTP_UDPX_SYNC
{
    PRO_UINT16 CalcChecksum() const
    {
        PRO_UINT16 ret = 0;

        for (int i = 0; i < (int)sizeof(nonce); ++i)
        {
            ret += nonce[i];
        }

        return (ret);
    }

    PRO_UINT16    version;      /* the current protocol version is 02 */
    char          reserved[14]; /* zero value */
    unsigned char nonce[14];
    PRO_UINT16    checksum;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CRtpSessionUdpserverEx : public CRtpSessionBase
{
public:

    static CRtpSessionUdpserverEx* CreateInstance(
        const RTP_SESSION_INFO* localInfo
        );

    bool Init(
        IRtpSessionObserver* observer,
        IProReactor*         reactor,
        const char*          localIp,         /* = NULL */
        unsigned short       localPort,       /* = 0 */
        unsigned long        timeoutInSeconds /* = 0 */
        );

    virtual void Fini();

private:

    CRtpSessionUdpserverEx(const RTP_SESSION_INFO& localInfo);

    virtual ~CRtpSessionUdpserverEx();

    virtual void PRO_CALLTYPE OnRecv(
        IProTransport*          trans,
        const pbsd_sockaddr_in* remoteAddr
        );

    virtual void PRO_CALLTYPE OnTimer(
        void*      factory,
        PRO_UINT64 timerId,
        PRO_INT64  userData
        );

    bool DoHandshake2();

private:

    bool          m_syncReceived;
    RTP_UDPX_SYNC m_syncToPeer; /* network byte order */
    PRO_UINT64    m_syncTimerId;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_SESSION_UDPSERVER_EX_H */
