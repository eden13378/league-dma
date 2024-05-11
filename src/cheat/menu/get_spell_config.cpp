#include "pch.hpp"

#include "menu.hpp"

namespace menu {
    auto get_spell_config( hash_t champion_name, ESpellSlot slot, bool special_spell) -> std::optional< SpellConfigT >{
        SpellConfigT spell_cfg{ };
        bool valid{ };

        switch ( champion_name ) {
        case ct_hash( "Aatrox" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.aatrox_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.aatrox_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.aatrox_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.aatrox_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.aatrox_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.aatrox_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Ahri" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ahri_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ahri_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ahri_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ahri_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ahri_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ahri_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Akali" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.akali_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.akali_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.akali_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Akshan" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.akshan_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.akshan_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.akshan_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Alistar" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.alistar_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.alistar_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.alistar_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Amumu" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.amumu_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.amumu_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.amumu_q_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.amumu_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.amumu_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.amumu_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Anivia" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.anivia_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.anivia_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.anivia_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Annie" ):
        {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.annie_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.annie_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.annie_w_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.annie_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.annie_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.annie_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Aphelios" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.aphelios_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.aphelios_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.aphelios_q_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.aphelios_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.aphelios_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.aphelios_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Ashe" ):
        {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ashe_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ashe_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ashe_w_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ashe_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ashe_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ashe_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "AurelionSol" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.asol_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.asol_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.asol_q_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.asol_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.asol_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.asol_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Azir" ):
        {
            switch ( slot ) {
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.azir_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.azir_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.azir_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Bard" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.bard_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.bard_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.bard_q_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.bard_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.bard_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.bard_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Belveth" ):
        {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.belveth_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.belveth_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.belveth_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Blitzcrank" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.blitzcrank_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.blitzcrank_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.blitzcrank_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Brand" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.brand_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.brand_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.brand_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.brand_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.brand_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.brand_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Braum" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.braum_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.braum_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.braum_q_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.braum_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.braum_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.braum_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Caitlyn" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.caitlyn_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.caitlyn_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.caitlyn_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.caitlyn_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.caitlyn_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.caitlyn_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Camille" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.camille_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.camille_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.camille_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Cassiopeia" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.cass_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.cass_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.cass_q_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.cass_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.cass_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.cass_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Chogath" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.chogath_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.chogath_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.chogath_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.chogath_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.chogath_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.chogath_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Corki" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.corki_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.corki_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.corki_q_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.corki_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.corki_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.corki_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Darius" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.darius_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.darius_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.darius_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Diana" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.diana_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.diana_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.diana_q_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.diana_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.diana_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.diana_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "DrMundo" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.mundo_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.mundo_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.mundo_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Draven" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.draven_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.draven_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.draven_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.draven_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.draven_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.draven_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Ekko" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ekko_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ekko_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ekko_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ekko_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ekko_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ekko_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Elise" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.elise_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.elise_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.elise_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Evelynn" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.evelynn_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.evelynn_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.evelynn_q_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.evelynn_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.evelynn_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.evelynn_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Ezreal" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ezreal_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ezreal_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ezreal_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ezreal_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ezreal_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ezreal_w_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ezreal_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ezreal_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ezreal_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Fiora" ):
        {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.fiora_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.fiora_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.fiora_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Fizz" ):
        {
            switch ( slot ) {
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.fizz_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.fizz_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.fizz_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Galio" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.galio_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.galio_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.galio_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.galio_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.galio_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.galio_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Gnar" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.gnar_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.gnar_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.gnar_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.gnar_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.gnar_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.gnar_w_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.gnar_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.gnar_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.gnar_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Gragas" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.gragas_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.gragas_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.gragas_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.gragas_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.gragas_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.gragas_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.gragas_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.gragas_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.gragas_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Graves" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.graves_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.graves_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.graves_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.graves_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.graves_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.graves_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Hecarim" ):
        {
            switch ( slot ) {
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.hecarim_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.hecarim_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.hecarim_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Heimerdinger" ):
        {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.heimer_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.heimer_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.heimer_w_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.heimer_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.heimer_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.heimer_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Illaoi" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.illaoi_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.illaoi_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.illaoi_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.illaoi_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.illaoi_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.illaoi_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Irelia" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.irelia_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.irelia_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.irelia_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.irelia_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.irelia_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.irelia_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Ivern" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ivern_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ivern_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ivern_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Janna" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.janna_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.janna_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.janna_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Jarvan" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.jarvan_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.jarvan_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.jarvan_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Jayce" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.jayce_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.jayce_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.jayce_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Jhin" ):
        {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.jhin_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.jhin_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.jhin_w_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.jhin_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.jhin_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.jhin_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Jinx" ):
        {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.jinx_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.jinx_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.jinx_w_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.jinx_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.jinx_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.jinx_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Kaisa" ):
        {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.kaisa_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.kaisa_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.kaisa_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Karma" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.karma_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.karma_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.karma_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Karthus" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.karthus_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.karthus_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.karthus_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Kassadin" ):
        {
            switch ( slot ) {
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.kassadin_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.kassadin_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.kassadin_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Kayle" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.kayle_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.kayle_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.kayle_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Kayn" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.kayn_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.kayn_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.kayn_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.kayn_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.kayn_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.kayn_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Kennen" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.kennen_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.kennen_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.kennen_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Khazix" ):
        {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.khazix_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.khazix_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.khazix_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Kled" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.kled_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.kled_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.kled_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.kled_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.kled_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.kled_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "KogMaw" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.kogmaw_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.kogmaw_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.kogmaw_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.kogmaw_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.kogmaw_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.kogmaw_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.kogmaw_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.kogmaw_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.kogmaw_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Leblanc" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.leblanc_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.leblanc_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.leblanc_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "LeeSin" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.leesin_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.leesin_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.leesin_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Leona" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.leona_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.leona_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.leona_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.leona_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.leona_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.leona_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Lillia" ):
        {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.lillia_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.lillia_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.lillia_w_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.lillia_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.lillia_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.lillia_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Lissandra" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.lissandra_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.lissandra_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.lissandra_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.lissandra_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.lissandra_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.lissandra_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Lucian" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.lucian_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.lucian_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.lucian_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.lucian_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.lucian_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.lucian_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Lulu" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.lulu_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.lulu_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.lulu_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Lux" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.lux_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.lux_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.lux_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.lux_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.lux_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.lux_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.lux_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.lux_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.lux_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Malphite" ):
        {
            switch ( slot ) {
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.malphite_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.malphite_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.malphite_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Malzahar" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.malz_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.malz_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.malz_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "MissFortune" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.mf_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.mf_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.mf_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Mordekaiser" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.mord_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.mord_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.mord_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.mord_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.mord_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.mord_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Morgana" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.morg_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.morg_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.morg_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Nami" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.nami_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.nami_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.nami_q_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.nami_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.nami_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.nami_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Nautilus" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.naut_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.naut_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.naut_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Neeko" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.neeko_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.neeko_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.neeko_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.neeko_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.neeko_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.neeko_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Nidalee" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.nidalee_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.nidalee_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.nidalee_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Nocturne" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.nocturne_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.nocturne_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.nocturne_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Nunu" ):
        {
            switch ( slot ) {
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.nunu_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.nunu_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.nunu_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Olaf" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.olaf_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.olaf_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.olaf_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Orianna" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.orianna_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.orianna_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.orianna_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Ornn" ):
        {
            switch ( slot ) {
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ornn_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ornn_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ornn_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Pantheon" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.panth_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.panth_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.panth_q_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.panth_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.panth_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.panth_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Poppy" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.poppy_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.poppy_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.poppy_q_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.poppy_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.poppy_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.poppy_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Pyke" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.pyke_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.pyke_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.pyke_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.pyke_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.pyke_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.pyke_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.pyke_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.pyke_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.pyke_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Qiyana" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.qiyana_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.qiyana_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.qiyana_q_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.qiyana_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.qiyana_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.qiyana_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Quinn" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.quinn_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.quinn_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.quinn_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Rakan" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.rakan_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.rakan_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.rakan_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.rakan_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.rakan_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.rakan_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Reksai" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.reksai_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.reksai_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.reksai_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Rell" ):
        {
            switch ( slot ) {
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.rell_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.rell_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.rell_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Renata" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.renata_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.renata_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.renata_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.renata_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.renata_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.renata_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.renata_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.renata_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.renata_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Rengar" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.rengar_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.rengar_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.rengar_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Riven" ):
        {
            switch ( slot ) {
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.riven_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.riven_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.riven_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Rumble" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.rumble_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.rumble_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.rumble_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Ryze" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ryze_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ryze_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ryze_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Samira" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.samira_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.samira_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.samira_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Senna" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.senna_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.senna_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.senna_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.senna_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.senna_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.senna_w_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.senna_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.senna_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.senna_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Sejuani" ):
        {
            switch ( slot ) {
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.sejuani_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.sejuani_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.sejuani_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Seraphine" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.seraphine_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.seraphine_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.seraphine_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.seraphine_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.seraphine_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.seraphine_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.seraphine_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.seraphine_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.seraphine_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Shen" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.shen_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.shen_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.shen_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Shyvana" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.shyv_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.shyv_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.shyv_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Sion" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.sion_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.sion_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.sion_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.sion_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.sion_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.sion_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Sivir" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.sivir_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.sivir_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.sivir_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Sona" ):
        {
            switch ( slot ) {
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.sona_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.sona_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.sona_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Soraka" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.soraka_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.soraka_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.soraka_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Swain" ):
        {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.swain_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.swain_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.swain_w_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.swain_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.swain_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.swain_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Sylas" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.sylas_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.sylas_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.sylas_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.sylas_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.sylas_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.sylas_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Syndra" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.syndra_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.syndra_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.syndra_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.syndra_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.syndra_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.syndra_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "TahmKench" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.kench_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.kench_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.kench_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.kench_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.kench_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.kench_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Talon" ):
        {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.talon_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.talon_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.talon_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Taliyah" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.taliyah_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.taliyah_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.taliyah_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.taliyah_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.taliyah_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.taliyah_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Thresh" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.thresh_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.thresh_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.thresh_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.thresh_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.thresh_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.thresh_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Tryndamere" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.trynd_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.trynd_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.trynd_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "TwistedFate" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.tf_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.tf_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.tf_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Twitch" ):
        {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.twitch_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.twitch_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.twitch_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Urgot" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.urgot_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.urgot_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.urgot_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.urgot_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.urgot_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.urgot_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.urgot_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.urgot_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.urgot_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Varus" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.varus_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.varus_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.varus_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.varus_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.varus_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.varus_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.varus_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.varus_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.varus_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Veigar" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.veigar_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.veigar_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.veigar_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.veigar_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.veigar_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.veigar_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "VelKoz" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.velkoz_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.velkoz_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.velkoz_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.velkoz_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.velkoz_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.velkoz_w_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.velkoz_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.velkoz_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.velkoz_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash("Viego"):
        {
            switch (slot) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.viego_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.viego_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.viego_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.viego_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.viego_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.viego_w_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.viego_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.viego_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.viego_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Vex" ): {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.vex_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.vex_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.vex_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.vex_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.vex_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.vex_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.vex_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.vex_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.vex_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Vi" ): {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.vi_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.vi_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.vi_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Viktor" ): {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.viktor_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.viktor_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.viktor_w_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.viktor_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.viktor_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.viktor_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Warwick" ): {
            switch ( slot ) {
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.warwick_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.warwick_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.warwick_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Xayah" ): {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.xayah_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.xayah_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.xayah_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Xerath" ): {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.xerath_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.xerath_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.xerath_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.xerath_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.xerath_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.xerath_w_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.xerath_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.xerath_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.xerath_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.xerath_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.xerath_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.xerath_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "XinZhao" ): {
            switch ( slot ) {
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.xin_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.xin_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.xin_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Yasuo" ): {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                if(special_spell) {
                    valid = true;
                    spell_cfg.spell_enabled = g_config->evade_spells.yasuo_q3_enabled;
                    spell_cfg.spell_danger = g_config->evade_spells.yasuo_q3_danger;
                    spell_cfg.spell_health_threshold = g_config->evade_spells.yasuo_q3_min_dodge_health;
                    break;
                }
                
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.yasuo_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.yasuo_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.yasuo_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Yone" ): {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.yone_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.yone_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.yone_q_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.yone_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.yone_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.yone_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Zac" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.zac_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.zac_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.zac_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Zed" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.zed_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.zed_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.zed_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Zeri" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.zeri_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.zeri_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.zeri_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.zeri_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.zeri_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.zeri_w_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Ziggs" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ziggs_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ziggs_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ziggs_q_min_dodge_health;
                break;
            }
            case ESpellSlot::w:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ziggs_w_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ziggs_w_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ziggs_w_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ziggs_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ziggs_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ziggs_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.ziggs_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.ziggs_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.ziggs_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Zilean" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.zilean_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.zilean_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.zilean_q_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Zoe" ):
        {
            switch ( slot ) {
            case ESpellSlot::q:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.zoe_q_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.zoe_q_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.zoe_q_min_dodge_health;
                break;
            }
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.zoe_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.zoe_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.zoe_e_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        case ct_hash( "Zyra" ):
        {
            switch ( slot ) {
            case ESpellSlot::e:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.zyra_e_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.zyra_e_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.zyra_e_min_dodge_health;
                break;
            }
            case ESpellSlot::r:
            {
                valid = true;
                spell_cfg.spell_enabled = g_config->evade_spells.zyra_r_enabled;
                spell_cfg.spell_danger = g_config->evade_spells.zyra_r_danger;
                spell_cfg.spell_health_threshold = g_config->evade_spells.zyra_r_min_dodge_health;
                break;
            }
            default:
                break;
            }

            break;
        }
        default:
            break;
        }

        if ( !valid )  return std::nullopt;
            
            return std::make_optional( spell_cfg );
    }
}
