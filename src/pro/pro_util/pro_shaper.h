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

#if !defined(____PRO_SHAPER_H____)
#define ____PRO_SHAPER_H____

#include "pro_a.h"
#include "pro_memory_pool.h"

/////////////////////////////////////////////////////////////////////////////
////

class CProShaper
{
public:

    CProShaper();

    void SetMaxBitRate(double maxBitRate);

    double GetMaxBitRate() const;

    double CalcGreenBits();

    void FlushGreenBits(double dataBits);

    void Reset();

private:

    double    m_maxBitRate;
    double    m_greenBits;
    PRO_INT64 m_startTick;

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* ____PRO_SHAPER_H____ */
