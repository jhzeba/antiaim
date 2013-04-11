/*
 * Copyright (c) 2008 Hrvoje Zeba <zeba.hrvoje@gmail.com>
 *
 *    This file is part of Antiaim.
 *
 *    Antiaim is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Antiaim is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Antiaim; if not, write to the Free Software Foundation,
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

#include <extdll.h>
#include <h_export.h>
#include <meta_api.h>

#include "antiaim.h"

#define AVG_SIZE 10

typedef Vector vector_t;

struct adata_t {
    vector_t p_origin;
    vector_t p_angle;
    vector_t e_origin;
};

struct pdata_t {
    float nn_ouput[AVG_SIZE];
    int   nn_in, nn_size;
        
    bool  e_present;
    adata_t adata;
};


pdata_t pdata[32];

FILE *record_fp = NULL;
ann *nn = NULL;

/* Taken from AMXX header files */
#if defined __linux__
#define EOFF 5
#else
#define EOFF 0
#endif

#if !defined __amd64__
#define TOFF (114 + EOFF)
#else
#define TOFF (139 + EOFF)
#endif
/********************************/

void aim_show_stats(const char *p_name)
{
    int p_index;

    for (p_index = 0; p_index < 32; p_index++) {
        edict_t *e = INDEXENT(p_index + 1);

        if (e && strcmp(STRING(e->v.netname), p_name) == 0)
            break;
    }

    if (p_index == 32) {
        UTIL_LogPrintf("player '%s' not found\n", p_name);
        return;
    }

    pdata_t& data = pdata[p_index];
    float cheater_index = 0;

    int size = (data.nn_size < AVG_SIZE) ? data.nn_size : AVG_SIZE;

    if (size == 0) {
        UTIL_LogPrintf("no data for player '%s'\n", p_name);
        return;
    }

    for (int i = 0; i < size; i++)
        cheater_index += data.nn_ouput[i];

    cheater_index /= size;

    UTIL_LogPrintf("%s: %.02f\n", p_name, cheater_index);
}

BOOL aim_ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
    pdata_t& data = pdata[ENGINE_CURRENT_PLAYER()];

    data.nn_in = 0;
    data.nn_size = 0;
        
    data.e_present = false;

    RETURN_META_VALUE(MRES_IGNORED, FALSE);
}

void aim_process_data(int p_index, string_t p_name, const vector_t& p_angle, const vector_t& p_origin, const vector_t& e_origin)
{
    pdata_t& data = pdata[p_index];
    adata_t& adata = data.adata;
        
    vector_t p_delta, e_delta;
    vector_t a_delta;

    vector_t e_dist;
    vector_t p_forward;

    float p_move, e_move;
    float p_angle_dist;

    float dist;
    float diff;

    float cosa, sina;

    a_delta = p_angle - adata.p_angle;
    p_angle_dist = sqrtf(a_delta.x * a_delta.x + a_delta.y * a_delta.y);

    p_delta = p_origin - adata.p_origin;
    p_move = sqrtf(p_delta.x * p_delta.x + p_delta.y * p_delta.y + p_delta.z * p_delta.z);

    if (data.e_present) {
        e_delta = e_origin - adata.e_origin;
        e_move = sqrtf(e_delta.x * e_delta.x + e_delta.y * e_delta.y + e_delta.z * e_delta.z);
    } else
        e_move = 0;

    e_dist = e_origin - p_origin;
    dist = sqrtf(e_dist.x * e_dist.x + e_dist.y * e_dist.y + e_dist.z * e_dist.z);

    e_dist.x /= dist;
    e_dist.y /= dist;
    e_dist.z /= dist;

    p_forward = gpGlobals->v_forward;

    cosa = (p_forward.x * e_dist.x + p_forward.y * e_dist.y + p_forward.z * e_dist.z);
    sina = sqrtf(1 - cosa * cosa);
        
    diff = dist * sina;

    if (record_fp != NULL)
        fprintf(record_fp, "%f %f %f %f %f %s\n", diff,  dist, p_angle_dist, p_move, e_move, STRING(p_name));

    if (nn) {
        ann::double_vec input(4);
        ann::double_vec output(1);

        input[0] = log(diff + 1);
        input[1] = log(p_angle_dist + 1);
        input[2] = log(p_move + 1);
        input[3] = log(e_move + 1);
                
        nn->calculate(input);
        nn->get_output(output);

        data.nn_ouput[data.nn_in] = output[0];
        data.nn_in++;

        if (data.nn_in == AVG_SIZE)
            data.nn_in = 0;

        data.nn_size++;

        if (CVAR_GET_FLOAT("aa_show") == 1)
            aim_show_stats(STRING(p_name));
    }
}

void aim_set_data_e(int p_index, const vector_t& p_angle, const vector_t& p_origin, const vector_t& e_origin)
{
    pdata_t& data = pdata[p_index];
    adata_t& adata = data.adata;

    adata.p_angle = p_angle;
    adata.p_origin = p_origin;
    adata.e_origin = e_origin;
        
    data.e_present = true;
}

void aim_set_data(int p_index, const vector_t& p_angle, const vector_t& p_origin)
{
    pdata_t& data = pdata[p_index];
    adata_t& adata = data.adata;

    adata.p_angle = p_angle;
    adata.p_origin = p_origin;
        
    data.e_present = false;
}

void aim_PlayerPostThink(edict_t *p_entity)
{
    if (CVAR_GET_FLOAT("aa_enable") != 1)
        return;

    int player = ENGINE_CURRENT_PLAYER();

    int p_team = *((int *)p_entity->pvPrivateData + TOFF);
    int e_team;

    vector_t p_origin, e_origin;
        
    TraceResult tr;
    edict_t *e_hit;

    bool data_set = false;

    if ((p_entity->v.flags & FL_FAKECLIENT) == 0) {
        p_origin = p_entity->v.origin + p_entity->v.view_ofs;
                
        if (p_entity->v.deadflag == DEAD_NO) {
            MAKE_VECTORS(p_entity->v.v_angle);
            TRACE_LINE(p_origin, p_origin + gpGlobals->v_forward * 100000, 0x100, p_entity, &tr);

            e_hit = tr.pHit;

            if (e_hit && strcmp(STRING(e_hit->v.classname), "player") == 0) {
                e_team = *((int *)e_hit->pvPrivateData + TOFF);

                if (p_team != e_team) {
                    e_origin = e_hit->v.origin + e_hit->v.view_ofs * 1.7;

                    if (p_entity->v.button & IN_ATTACK)
                        aim_process_data(player, p_entity->v.netname, p_entity->v.v_angle, p_origin, e_origin);

                    aim_set_data_e(player, p_entity->v.v_angle, p_origin, e_origin);

                    data_set = true;
                }
            }
        }
                
        if (!data_set)
            aim_set_data(player, p_entity->v.v_angle, p_origin);
    }

    RETURN_META(MRES_IGNORED);
}
