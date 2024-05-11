#pragma once
#include "../../menu/menu.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules
{
    class teemo_module final : public IModule
    {
    public:
        virtual ~teemo_module() = default;

        auto get_name() -> hash_t override { return ct_hash("teemo_module"); }

        auto get_champion_name() -> hash_t override { return ct_hash("Teemo"); }

        auto initialize() -> void override { m_priority_list = { q_spell }; }

        auto initialize_menu() -> void override
        {
            const auto navigation = g_window->push(_("teemo"), menu_order::champion_module);
            const auto q_settings = navigation->add_section(_("q settings"));
            const auto drawings = navigation->add_section(_("drawings"));

            q_settings->checkbox(_("enable"), g_config->teemo.q_enabled);
            q_settings->checkbox(_("harass q"), g_config->teemo.q_harass);
            q_settings->checkbox(_("only if can reset aa"), g_config->teemo.q_aa_reset);
           // q_settings->checkbox(_("killsteal q"), g_config->teemo.q_aa_reset);

            drawings->checkbox(_("draw q range"), g_config->teemo.q_draw_range);
            drawings->checkbox(_("draw q target"), g_config->teemo.q_draw_spell_target);
        }

        auto on_draw() -> void override
        {
            if (!g_config->teemo.q_draw_range->get<bool>() && !g_config->teemo.q_draw_spell_target->get<bool>() ||
                g_local->is_dead())
                return;

            g_local.update();


            if (g_config->teemo.q_draw_range->get<bool>()) {
                const auto q = g_local->spell_book.get_spell_slot(ESpellSlot::q);

                if (q && q->level > 0)
                {
                    g_render->circle_3d(g_local->position, Color(200, 0, 255, 200), m_q_range, 2, 72, 2.f);
                }   
            }

            if (g_config->teemo.q_draw_spell_target->get<bool>()) draw_spells();
        }

        auto run() -> void override {
            initialize_spell_slots();

            if (g_features->orbwalker->in_action() || g_features->orbwalker->in_attack() ||
                g_features->evade->is_active())
                return;

            switch (g_features->orbwalker->get_mode())
            {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_q();
                break;
            case Orbwalker::EOrbwalkerMode::harass:
            case Orbwalker::EOrbwalkerMode::laneclear: 
                if (g_config->teemo.q_harass->get<bool>() && !helper::is_position_under_turret(g_local->position)) {
                    spell_q();
                }
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q() -> bool override
        {
            if (!g_config->teemo.q_enabled->get<bool>() || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(true)) return false;

            auto target = g_features->target_selector->get_default_target(false);
            if (!target || !g_features->orbwalker->is_attackable(target->index, m_q_range, false, true)) return false;

            if (g_config->teemo.q_aa_reset->get<bool>() && target->is_hero() && target->is_enemy() &&
                !g_features->orbwalker->should_reset_aa() && g_features->orbwalker->can_attack(target->index))
                return false;

            if (cast_spell(ESpellSlot::q, target->network_id)) {

                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast();

                return true;
            }

            return false;
        }

        auto spell_w() -> bool override { return false; }

        auto spell_e() -> bool override { return false; }

        auto spell_r() -> bool override { return false; }

        auto draw_spells() -> void {

            auto sci = g_local->spell_book.get_spell_cast_info();

            Vec2 sp{};
            Vec2 ep{};

            if (sci && sci->slot == (int)ESpellSlot::q) {

                auto target = g_entity_list.get_by_index(sci->get_target_index());
                if (!target) return;

                if (!world_to_screen(g_local->position, sp) || !world_to_screen(target->position, ep)) return;

                float modifier = std::clamp( ( *g_time - sci->start_time ) / sci->windup_time, 0.f, 1.f);
                modifier       = utils::ease::ease_out_quad(modifier);

                float thickness = 14.f - 12.f * modifier;

                Color draw_color = Color(200, 0, 255);

                draw_color.r = 255 - 100 * modifier;
                draw_color.g = 255 - 255 * modifier;
                //draw_color.b = 255 - 255 * modifier;

                g_render->blur_line(sp, ep, draw_color, thickness);
            }



        }

    protected:
    private:
        float m_last_q_time{};
        float m_q_range{ 680.f };
    };
} // namespace features::champion_modules
