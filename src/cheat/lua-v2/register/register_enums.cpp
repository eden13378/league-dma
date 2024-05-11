#include "pch.hpp"

#include "../state.hpp"
#include "../../features/helper.hpp"
#include "../../features/prediction.hpp"
#include "../../features/spell_detector.hpp"
#include "../../utils/input.hpp"

namespace lua {
        auto LuaState::register_enums( sol::state& state ) -> void{
        state.globals( ).new_enum< utils::Input::EMouseButton >(
            "e_mouse_button",
            { { "left", utils::Input::EMouseButton::left }, { "right", utils::Input::EMouseButton::right } }
        );

        state.globals( ).new_enum< features::Prediction::EHitchance >(
            "e_hitchance",
            {
                { "low", features::Prediction::EHitchance::low },
                { "medium", features::Prediction::EHitchance::medium },
                { "high", features::Prediction::EHitchance::high },
                { "very_high", features::Prediction::EHitchance::very_high },
                { "immobile", features::Prediction::EHitchance::immobile },
                { "invalid", features::Prediction::EHitchance::invalid }
            }
        );

        state.globals( ).new_enum< utils::Input::EKeyState >(
            "e_key_state",
            { { "key_down", utils::Input::EKeyState::key_down }, { "key_up", utils::Input::EKeyState::key_up } }
        );

        state.globals( ).new_enum< features::EDamageType >(
            "e_damage_type",
            {
                { "true_damage", features::EDamageType::true_damage },
                { "physical_damage", features::EDamageType::physical_damage },
                { "magic_damage", features::EDamageType::magic_damage }
            }
        );

        state.globals( ).new_enum< features::SpellDetector::ESpecialSpell >(
            "e_special_spell",
            {
                {
                    "none",
                    features::SpellDetector::ESpecialSpell::none
                },
                {
                    "ashe_r",
                    features::SpellDetector::ESpecialSpell::ashe_r
                },
                {
                    "jinx_r",
                    features::SpellDetector::ESpecialSpell::jinx_r
                }
            }
        );

        state.globals( ).new_enum< utils::EKey >(
            "e_key",
            {
                {
                    "lbutton",
                    utils::EKey::lbutton
                },
                {
                    "rbutton",
                    utils::EKey::rbutton
                },
                {
                    "cancel",
                    utils::EKey::cancel
                },
                {
                    "mbutton",
                    utils::EKey::mbutton
                },
                {
                    "xbutton1",
                    utils::EKey::xbutton1
                },
                {
                    "xbutton2",
                    utils::EKey::xbutton2
                },
                {
                    "back",
                    utils::EKey::back
                },
                {
                    "tab",
                    utils::EKey::tab
                },
                {
                    "clear",
                    utils::EKey::clear
                },
                {
                    "return_key",
                    utils::EKey::return_key
                },
                {
                    "shift",
                    utils::EKey::shift
                },
                {
                    "control",
                    utils::EKey::control
                },
                {
                    "menu",
                    utils::EKey::menu
                },
                {
                    "pause",
                    utils::EKey::pause
                },
                {
                    "capital",
                    utils::EKey::capital
                },
                {
                    "kana",
                    utils::EKey::kana
                },
                {
                    "hanguel",
                    utils::EKey::hanguel
                },
                {
                    "hangul",
                    utils::EKey::hangul
                },
                {
                    "escape",
                    utils::EKey::escape
                },
                {
                    "convert",
                    utils::EKey::convert
                },
                {
                    "nonconvert",
                    utils::EKey::nonconvert
                },
                {
                    "accept",
                    utils::EKey::accept
                },
                {
                    "modechange",
                    utils::EKey::modechange
                },
                {
                    "space",
                    utils::EKey::space
                },
                {
                    "prior",
                    utils::EKey::prior
                },
                {
                    "next",
                    utils::EKey::next
                },
                {
                    "end",
                    utils::EKey::end
                },
                {
                    "home",
                    utils::EKey::home
                },
                {
                    "left",
                    utils::EKey::left
                },
                {
                    "up",
                    utils::EKey::up
                },
                {
                    "right",
                    utils::EKey::right
                },
                {
                    "down",
                    utils::EKey::down
                },
                {
                    "select",
                    utils::EKey::select
                },
                {
                    "print",
                    utils::EKey::print
                },
                {
                    "execute",
                    utils::EKey::execute
                },
                {
                    "snapshot",
                    utils::EKey::snapshot
                },
                {
                    "insert",
                    utils::EKey::insert
                },
                {
                    "delete_key",
                    utils::EKey::delete_key
                },
                {
                    "help",
                    utils::EKey::help
                },
                {
                    "_0",
                    utils::EKey::_0
                },
                {
                    "_1",
                    utils::EKey::_1
                },
                {
                    "_2",
                    utils::EKey::_2
                },
                {
                    "_3",
                    utils::EKey::_3
                },
                {
                    "_4",
                    utils::EKey::_4
                },
                {
                    "_5",
                    utils::EKey::_5
                },
                {
                    "_6",
                    utils::EKey::_6
                },
                {
                    "_7",
                    utils::EKey::_7
                },
                {
                    "_8",
                    utils::EKey::_8
                },
                {
                    "_9",
                    utils::EKey::_9
                },
                {
                    "A",
                    utils::EKey::A
                },
                {
                    "B",
                    utils::EKey::B
                },
                {
                    "C",
                    utils::EKey::C
                },
                {
                    "D",
                    utils::EKey::D
                },
                {
                    "E",
                    utils::EKey::E
                },
                {
                    "F",
                    utils::EKey::F
                },
                {
                    "G",
                    utils::EKey::G
                },
                {
                    "H",
                    utils::EKey::H
                },
                {
                    "I",
                    utils::EKey::I
                },
                {
                    "J",
                    utils::EKey::J
                },
                {
                    "K",
                    utils::EKey::K
                },
                {
                    "L",
                    utils::EKey::L
                },
                {
                    "M",
                    utils::EKey::M
                },
                {
                    "N",
                    utils::EKey::N
                },
                {
                    "O",
                    utils::EKey::O
                },
                {
                    "P",
                    utils::EKey::P
                },
                {
                    "Q",
                    utils::EKey::Q
                },
                {
                    "R",
                    utils::EKey::R
                },
                {
                    "S",
                    utils::EKey::S
                },
                {
                    "T",
                    utils::EKey::T
                },
                {
                    "U",
                    utils::EKey::U
                },
                {
                    "V",
                    utils::EKey::V
                },
                {
                    "W",
                    utils::EKey::W
                },
                {
                    "X",
                    utils::EKey::X
                },
                {
                    "Y",
                    utils::EKey::Y
                },
                {
                    "Z",
                    utils::EKey::Z
                },
                {
                    "n0",
                    utils::EKey::n0
                },
                {
                    "n1",
                    utils::EKey::n1
                },
                {
                    "n2",
                    utils::EKey::n2
                },
                {
                    "n3",
                    utils::EKey::n3
                },
                {
                    "n4",
                    utils::EKey::n4
                },
                {
                    "n5",
                    utils::EKey::n5
                },
                {
                    "n6",
                    utils::EKey::n6
                },
                {
                    "n7",
                    utils::EKey::n7
                },
                {
                    "n8",
                    utils::EKey::n8
                },
                {
                    "n9",
                    utils::EKey::n9
                },
                {
                    "f1",
                    utils::EKey::f1
                }
            }
        );

        state.globals( ).new_enum< Object::EObjectTypeFlags >(
            "e_object_type_flags",
            {
                { "game_object", Object::EObjectTypeFlags::game_object },
                { "neutral_camp", Object::EObjectTypeFlags::neutral_camp },
                { "dead_object", Object::EObjectTypeFlags::dead_object },
                { "invalid_object", Object::EObjectTypeFlags::invalid_object },
                { "ai_base_common", Object::EObjectTypeFlags::ai_base_common },
                { "attackable_unit", Object::EObjectTypeFlags::attackable_unit },
                { "ai", Object::EObjectTypeFlags::ai },
                { "minion", Object::EObjectTypeFlags::minion },
                { "hero", Object::EObjectTypeFlags::hero },
                { "turret", Object::EObjectTypeFlags::turret },
                { "missile", Object::EObjectTypeFlags::missile },
                { "building", Object::EObjectTypeFlags::building }
            }
        );

        // state.globals(  ).new_enum(  )

        state.globals( ).new_enum< ESpellSlot >(
            "e_spell_slot",
            {
                { "q", ESpellSlot::q },
                { "w", ESpellSlot::w },
                { "e", ESpellSlot::e },
                { "r", ESpellSlot::r },
                { "d", ESpellSlot::d },
                { "f", ESpellSlot::f },
                { "item1", ESpellSlot::item1 },
                { "item2", ESpellSlot::item2 },
                { "item3", ESpellSlot::item3 },
                { "item4", ESpellSlot::item4 },
                { "item5", ESpellSlot::item5 },
                { "item6", ESpellSlot::item6 },
                { "item7", ESpellSlot::item7 },
                { "recall", ESpellSlot::recall }
            }
        );

        state.globals( ).new_enum< features::EBuffType >(
            "e_buff_type",
            {
                { "internal", features::EBuffType::internal },
                { "aura", features::EBuffType::aura },
                { "combat_enchancer", features::EBuffType::combat_enchancer },
                { "combat_dehancer", features::EBuffType::combat_dehancer },
                { "spell_shield", features::EBuffType::spell_shield },
                { "stun", features::EBuffType::stun },
                { "invisibility", features::EBuffType::invisibility },
                { "silence", features::EBuffType::silence },
                { "taunt", features::EBuffType::taunt },
                { "berserk", features::EBuffType::berserk },
                { "polymorph", features::EBuffType::polymorph },
                { "slow", features::EBuffType::slow },
                { "snare", features::EBuffType::snare },
                { "damage", features::EBuffType::damage },
                { "heal", features::EBuffType::heal },
                { "haste", features::EBuffType::haste },
                { "spell_immunity", features::EBuffType::spell_immunity },
                { "physical_immunity", features::EBuffType::physical_immunity },
                { "invulnerability", features::EBuffType::invulnerability },
                { "attack_speed_slow", features::EBuffType::attack_speed_slow },
                { "near_sight", features::EBuffType::near_sight },
                { "currency", features::EBuffType::currency },
                { "fear", features::EBuffType::fear },
                { "charm", features::EBuffType::charm },
                { "poison", features::EBuffType::poison },
                { "suppression", features::EBuffType::suppression },
                { "blind", features::EBuffType::blind },
                { "counter", features::EBuffType::counter },
                { "shred", features::EBuffType::shred },
                { "flee", features::EBuffType::flee },
                { "knockup", features::EBuffType::knockup },
                { "knockback", features::EBuffType::knockback },
                { "disarm", features::EBuffType::disarm },
                { "grounded", features::EBuffType::grounded },
                { "drowsy", features::EBuffType::drowsy },
                { "asleep", features::EBuffType::asleep },
                { "obscured", features::EBuffType::obscured },
                { "clickproof_to_enemies", features::EBuffType::clickproof_to_enemies },
                { "un_killable", features::EBuffType::un_killable }
            }
        );
        enum class e_spell_type {
            none,
            linear,
            circle
        };

        state.globals( ).new_enum< e_spell_type >(
            "e_spell_type",
            {
                { "none", e_spell_type::none },
                { "linear", e_spell_type::linear },
                { "circle", e_spell_type::circle }
            }
        );
    }

}
