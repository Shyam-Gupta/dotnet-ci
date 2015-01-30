//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//
/* ------------------------------------------------------------------------- *
 * DebuggerRegDisplayHelper.cpp -- implementation of the platform-dependent 
// 

 *                                 methods for transferring information between
 *                                 REGDISPLAY and DebuggerREGDISPLAY
 * ------------------------------------------------------------------------- */

#include "stdafx.h"

void CopyREGDISPLAY(REGDISPLAY* pDst, REGDISPLAY* pSrc)
{
    memcpy((BYTE*)pDst, (BYTE*)pSrc, sizeof(REGDISPLAY));

    pDst->pContext = pSrc->pContext;

    if (pSrc->pCurrentContextPointers == &(pSrc->ctxPtrsOne))
    {
        pDst->pCurrentContextPointers = &(pDst->ctxPtrsOne);
        pDst->pCallerContextPointers  = &(pDst->ctxPtrsTwo);
    }
    else
    {
        pDst->pCurrentContextPointers = &(pDst->ctxPtrsTwo);
        pDst->pCallerContextPointers  = &(pDst->ctxPtrsOne);
    }

    if (pSrc->pCurrentContext == &(pSrc->ctxOne))
    {
        pDst->pCurrentContext = &(pDst->ctxOne);
        pDst->pCallerContext  = &(pDst->ctxTwo);
    }
    else
    {
        pDst->pCurrentContext = &(pDst->ctxTwo);
        pDst->pCallerContext  = &(pDst->ctxOne);
    }
}
