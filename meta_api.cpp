/*
 * This file was adopted from the Metamod's stub plugin
 * and modified to suit the needs of the Antiaim package.
 *
 *   Copyright (c) 2008 Hrvoje Zeba <zeba.hrvoje@gmail.com>
 *
 */

/*
 * Copyright (c) 2001-2003 Will Day <willday@hpgx.net>
 *
 *    This file is part of Metamod.
 *
 *    Metamod is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Metamod is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Metamod; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#include <extdll.h>                     // always
#include <meta_api.h>           // of course
#include "sdk_util.h"           // UTIL_LogPrintf, etc

#include "antiaim.h"

// Must provide at least one of these..
static META_FUNCTIONS gMetaFunctionTable = {
    NULL,                   // pfnGetEntityAPI                              HL SDK; called before game DLL
    NULL,                   // pfnGetEntityAPI_Post                 META; called after game DLL
    GetEntityAPI2,  // pfnGetEntityAPI2                             HL SDK2; called before game DLL
    NULL,                   // pfnGetEntityAPI2_Post                META; called after game DLL
    NULL,                   // pfnGetNewDLLFunctions                HL SDK2; called before game DLL
    NULL,                   // pfnGetNewDLLFunctions_Post   META; called after game DLL
    GetEngineFunctions,     // pfnGetEngineFunctions        META; called before HL engine
    NULL,                   // pfnGetEngineFunctions_Post   META; called after HL engine
};

// Description of plugin
plugin_info_t Plugin_info = {
    META_INTERFACE_VERSION, // ifvers
    "antiaim",      // name
    "0.1",  // version
    "2007/10/29",   // date
    "Hrvoje Zeba <hrvoje.zeba@fer.hr>",     // author
    "none", // url
    "ANTIAIM",      // logtag, all caps please
    PT_ANYTIME,     // (when) loadable
    PT_ANYTIME,     // (when) unloadable
};

// Global vars from metamod:
meta_globals_t *gpMetaGlobals;          // metamod globals
gamedll_funcs_t *gpGamedllFuncs;        // gameDLL function tables
mutil_funcs_t *gpMetaUtilFuncs;         // metamod utility functions

// Metamod requesting info about this plugin:
//  ifvers                      (given) interface_version metamod is using
//  pPlugInfo           (requested) struct with info about plugin
//  pMetaUtilFuncs      (given) table of utility functions provided by metamod
C_DLLEXPORT int Meta_Query(char * /*ifvers */, plugin_info_t **pPlugInfo,
                           mutil_funcs_t *pMetaUtilFuncs) 
{
    // Give metamod our plugin_info struct
    *pPlugInfo=&Plugin_info;
    // Get metamod utility function table.
    gpMetaUtilFuncs=pMetaUtilFuncs;
    return(TRUE);
}

cvar_t aa_enable = {"aa_enable", "0", FCVAR_EXTDLL};
cvar_t aa_show = {"aa_show", "0", FCVAR_EXTDLL};

void ServerCommand_Record()
{
    char arg1[512];

    snprintf(arg1, 512, CMD_ARGV(2));

    if (arg1[0] == 0) {
        UTIL_LogPrintf("Usage: %s %s <file>\n", CMD_ARGV(0), CMD_ARGV(1));
        return;
    }
        
    if (record_fp != NULL) {
        UTIL_LogPrintf("Already recording aim data\n");
        return;
    }

    if ((record_fp = fopen(arg1, "ab")) == NULL)
        UTIL_LogPrintf("%s: unable to open the file\n", arg1);
    else
        UTIL_LogPrintf("Recording aim data to '%s'\n", arg1);
}

void ServerCommand_Stop()
{
    if (record_fp != NULL) {
        fclose(record_fp);
        record_fp = NULL;

        UTIL_LogPrintf("Aim data recording stopped\n");
    }
}

void ServerCommand_Show()
{
    char arg1[512];

    snprintf(arg1, 512, CMD_ARGV(2));

    if (arg1[0] == 0) {
        UTIL_LogPrintf("Usage: %s %s <player name>\n", CMD_ARGV(0), CMD_ARGV(1));
        return;
    }

    aim_show_stats(arg1);
}

void ServerCommand_Load()
{
    char arg1[512];

    snprintf(arg1, 512, CMD_ARGV(2));

    if (arg1[0] == 0) {
        UTIL_LogPrintf("Usage: %s %s <file>\n", CMD_ARGV(0), CMD_ARGV(1));
        return;
    }

    if (!nn)
        nn = new ann();

    if (nn->load(arg1) == false) {
        delete nn;
        nn = NULL;

        UTIL_LogPrintf("%s: unable to load the nn file!\n", arg1);
    }
}

void ServerCommand_Help()
{
    UTIL_LogPrintf("Usage: %s <command> [options]\n", CMD_ARGV(0));
    UTIL_LogPrintf(" commands:\n");
    UTIL_LogPrintf("   record - record the aim data to a file\n");
    UTIL_LogPrintf("   stop   - stop the recording\n");
    UTIL_LogPrintf("   show   - show player stats\n");
    UTIL_LogPrintf("   load   - load the nn file\n");
    UTIL_LogPrintf("   help   - show this help\n");
}

void ServerCommand_aa()
{
    char cmd[128];

    if (CVAR_GET_FLOAT("aa_enable") != 1) {
        UTIL_LogPrintf("Antiaim disabled - set the aa_enable cvar to '1'\n", CMD_ARGV(0));
        return;
    }

    snprintf(cmd, 128, CMD_ARGV(1));

    if (strcmp(cmd, "record") == 0)
        ServerCommand_Record();
    else if (strcmp(cmd, "stop") == 0)
        ServerCommand_Stop();
    else if (strcmp(cmd, "show") == 0)
        ServerCommand_Show();
    else if (strcmp(cmd, "load") == 0)
        ServerCommand_Load();
    else if (strcmp(cmd, "help") == 0)
        ServerCommand_Help();
    else
        UTIL_LogPrintf("Unknown command, try '%s help' for help\n", CMD_ARGV(0));
}

// Metamod attaching plugin to the server.
//  now                         (given) current phase, ie during map, during changelevel, or at startup
//  pFunctionTable      (requested) table of function tables this plugin catches
//  pMGlobals           (given) global vars from metamod
//  pGamedllFuncs       (given) copy of function tables from game dll
C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME /* now */, 
                            META_FUNCTIONS *pFunctionTable, meta_globals_t *pMGlobals, 
                            gamedll_funcs_t *pGamedllFuncs) 
{
    if(!pMGlobals) {
        LOG_ERROR(PLID, "Meta_Attach called with null pMGlobals");
        return(FALSE);
    }
    gpMetaGlobals=pMGlobals;
    if(!pFunctionTable) {
        LOG_ERROR(PLID, "Meta_Attach called with null pFunctionTable");
        return(FALSE);
    }
    memcpy(pFunctionTable, &gMetaFunctionTable, sizeof(META_FUNCTIONS));
    gpGamedllFuncs=pGamedllFuncs;

    // print a message to notify about plugin attaching
    LOG_CONSOLE(PLID, "%s: plugin attaching", Plugin_info.name);
    LOG_MESSAGE(PLID, "%s: plugin attaching", Plugin_info.name);

    // ask the engine to register the server commands this plugin uses
    REG_SVR_COMMAND("aa", ServerCommand_aa);

    // ask the engine to register the CVARs this plugin uses
    CVAR_REGISTER(&aa_enable);
    CVAR_REGISTER(&aa_show);

    nn = new ann();
        
    if (nn->load("cstrike/aa_cs.nn") == false) {
        delete nn;
        nn = NULL;

        UTIL_LogPrintf("unable to load the nn file!\n");
    }

    return(TRUE);
}

// Metamod detaching plugin from the server.
// now          (given) current phase, ie during map, etc
// reason       (given) why detaching (refresh, console unload, forced unload, etc)
C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME /* now */, 
                            PL_UNLOAD_REASON /* reason */) 
{
    ServerCommand_Stop();

    if (nn) {
        delete nn;
        nn = NULL;
    }
        
    return(TRUE);
}
