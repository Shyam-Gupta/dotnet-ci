//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information. 
//

/*=============================================================
**
** Source: helper.c
**
** Purpose: This helper process sets up a block of memory, then 
** raises an exception to pass that memory location back to the
** parent process. When the parent process is done calling WriteProcessMemory
** we check here that it was written properly.
**
**
**============================================================*/

#include <palsuite.h>
const int MY_EXCEPTION=999;

int __cdecl main(int argc, char *argv[])
{
     
    char* Memory;
    char* TheArray[1];
    int i;

    if(0 != (PAL_Initialize(argc, argv)))
    {
        return FAIL;
    }

    Memory = (char*)VirtualAlloc(NULL, 4096, MEM_COMMIT, PAGE_READONLY);
    
    if(Memory == NULL)
    {
        Fail("ERROR: Attempted to allocate two pages, but the VirtualAlloc "
             "call failed.  GetLastError() returned %d.\n",GetLastError());
    }


    TheArray[0] = Memory;
   
    
    /* Need to sleep for a couple seconds.  Otherwise this process
       won't be being debugged when the first exception is raised.
    */
    Sleep(4000);
    
    RaiseException(MY_EXCEPTION, 0, 1, (ULONG_PTR*)TheArray);

    for(i=0; i<4096; ++i)
    {
        if(Memory[i] != '\0')
        {
            Fail("ERROR: The memory should be unchanged after the "
                 "invalid call to WriteProcessMemory, but the char "
                 "at index %d has changed.\n",i);
        }
    }
    
   

    

    PAL_Terminate();
    return PASS;
}
