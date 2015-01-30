//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//
// ============================================================
//
// AssemblyEntry.hpp
//


//
// Defines the AssemblyEntry class
//
// ============================================================

#ifndef __BINDER__ASSEMBLY_ENTRY_HPP__
#define __BINDER__ASSEMBLY_ENTRY_HPP__

#include "bindertypes.hpp"
#include "assemblyname.hpp"

namespace BINDER_SPACE
{
    class AssemblyEntry
    {
    public:
        AssemblyEntry()
        {
            m_pAssemblyName = NULL;
        }
        virtual ~AssemblyEntry()
        {
            SAFE_RELEASE(m_pAssemblyName);
        }
    
        AssemblyName *GetAssemblyName(BOOL fAddRef = FALSE)
        {
            AssemblyName *pAssemblyName = m_pAssemblyName;

            if (fAddRef && (pAssemblyName != NULL))
            {
                pAssemblyName->AddRef();
            }
            return pAssemblyName;
        }

        void SetAssemblyName(AssemblyName *pAssemblyName, BOOL fAddRef = TRUE)
        {
            SAFE_RELEASE(m_pAssemblyName);

            if (fAddRef && (pAssemblyName != NULL))
            {
                pAssemblyName->AddRef();
            }

            m_pAssemblyName = pAssemblyName;
        }
    protected:
        AssemblyName *m_pAssemblyName;
    };
};

#endif
