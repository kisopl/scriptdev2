/* Copyright (C) 2006 - 2013 ScriptDev2 <http://www.scriptdev2.com/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Boss_Golemagg
SD%Complete: 80
SDComment: Rager need to be tied to boss (Despawn on boss-death)
SDCategory: Molten Core
EndScriptData */

#include "precompiled.h"
#include "molten_core.h"

enum
{
    SPELL_MAGMA_SPLASH      = 13879,
    SPELL_PYROBLAST         = 20228,
    SPELL_EARTHQUAKE        = 19798,
    SPELL_ENRAGE            = 19953,
    SPELL_GOLEMAGG_TRUST    = 20553,

    // Core Rager
    EMOTE_LOW_HP            = -1409002,
    SPELL_MANGLE            = 19820
};

struct MANGOS_DLL_DECL boss_golemaggAI : public ScriptedAI
{
    boss_golemaggAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiPyroblastTimer;
    uint32 m_uiEarthquakeTimer;
    uint32 m_uiBuffTimer;
    bool m_bEnraged;

    void Reset()
    {
        m_uiPyroblastTimer  = 7 * IN_MILLISECONDS;
        m_uiEarthquakeTimer = 3 * IN_MILLISECONDS;
        m_uiBuffTimer       = 1.5 * IN_MILLISECONDS;
        m_bEnraged = false;

        m_creature->CastSpell(m_creature, SPELL_MAGMA_SPLASH, true);
    }

    void Aggro(Unit* pWho)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_GOLEMAGG, IN_PROGRESS);
    }

    void JustDied(Unit* pKiller)
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_GOLEMAGG, DONE);
    }

    void JustReachedHome()
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_GOLEMAGG, FAIL);
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // Pyroblast
        if (m_uiPyroblastTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_PYROBLAST) == CAST_OK)
                    m_uiPyroblastTimer = 7 * IN_MILLISECONDS;
            }
        }
        else
            m_uiPyroblastTimer -= uiDiff;

        // Enrage
        if (!m_bEnraged && m_creature->GetHealthPercent() < 10.0f)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK)
                m_bEnraged = true;
        }

        // Earthquake
        if (m_bEnraged)
        {
            if (m_uiEarthquakeTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_EARTHQUAKE) == CAST_OK)
                    m_uiEarthquakeTimer = 3 * IN_MILLISECONDS;
            }
            else
                m_uiEarthquakeTimer -= uiDiff;
        }

        // Golemagg's Trust
        if (m_uiBuffTimer < uiDiff)
        {
            DoCastSpellIfCan(m_creature, SPELL_GOLEMAGG_TRUST);
            m_uiBuffTimer = 1.5 * IN_MILLISECONDS;
        }
        else
            m_uiBuffTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

struct MANGOS_DLL_DECL mob_core_ragerAI : public ScriptedAI
{
    mob_core_ragerAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;
    uint32 m_uiMangleTimer;

    void Reset()
    {
        m_uiMangleTimer = 7 * IN_MILLISECONDS;              // These times are probably wrong
    }

    void DamageTaken(Unit* pDoneBy, uint32& uiDamage)
    {
        if (m_creature->GetHealthPercent() < 50.0f)
        {
            if (m_pInstance && m_pInstance->GetData(TYPE_GOLEMAGG) != DONE)
            {
                DoScriptText(EMOTE_LOW_HP, m_creature);
                m_creature->SetHealth(m_creature->GetMaxHealth());
                uiDamage = 0;
            }
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // Mangle
        if (m_uiMangleTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_MANGLE) == CAST_OK)
                m_uiMangleTimer = 10 * IN_MILLISECONDS;
        }
        else
            m_uiMangleTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_golemagg(Creature* pCreature)
{
    return new boss_golemaggAI(pCreature);
}

CreatureAI* GetAI_mob_core_rager(Creature* pCreature)
{
    return new mob_core_ragerAI(pCreature);
}

void AddSC_boss_golemagg()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_golemagg";
    pNewScript->GetAI = &GetAI_boss_golemagg;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "mob_core_rager";
    pNewScript->GetAI = &GetAI_mob_core_rager;
    pNewScript->RegisterSelf();
}
