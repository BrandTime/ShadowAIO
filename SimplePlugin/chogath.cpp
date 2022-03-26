#include "../plugin_sdk/plugin_sdk.hpp"
#include "chogath.h"

namespace chogath
{

    // To declare a spell, it is necessary to create an object and registering it in load function
    script_spell* q = nullptr;
    script_spell* w = nullptr;
    script_spell* e = nullptr;
    script_spell* r = nullptr;
    script_spell* flash = nullptr;

    // Declaration of menu objects
    TreeTab* main_tab = nullptr;

    namespace draw_settings
    {
        TreeEntry* draw_range_q = nullptr;
        TreeEntry* q_color = nullptr;
        TreeEntry* draw_range_w = nullptr;
        TreeEntry* w_color = nullptr;
        TreeEntry* draw_range_r = nullptr;
        TreeEntry* r_color = nullptr;
    }

    namespace combo
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* use_e = nullptr;
        TreeEntry* use_r = nullptr;
        TreeEntry* r_use_flash_r = nullptr;
        TreeEntry* r_use_on_epic_monsters = nullptr;
        std::map<std::uint32_t, TreeEntry*> r_use_on;
    }

    namespace harass
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* use_e = nullptr;
    }

    namespace laneclear
    {
        TreeEntry* spell_farm = nullptr;
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* use_e = nullptr;
        TreeEntry* use_e_on_turret = nullptr;
    }

    namespace jungleclear
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* use_e = nullptr;
    }

    namespace fleemode
    {
        TreeEntry* use_q;
    }

    namespace hitchance
    {
        TreeEntry* q_hitchance = nullptr;
        TreeEntry* w_hitchance = nullptr;
    }


    // Event handler functions
    void on_update();
    void on_draw();
    void on_before_attack(game_object_script target, bool* process);

    // Declaring functions responsible for spell-logic
    //
    void q_logic();
    void w_logic();
    void e_logic();
    void r_logic();

    // Utils
    //
    bool can_use_r_on(game_object_script target);
    hit_chance get_hitchance(TreeEntry* entry);

    void load()
    {
        // Registering a spells
        //
        q = plugin_sdk->register_spell(spellslot::q, 950);
        q->set_skillshot(0.5f, 250.0f, 1200.0f, { }, skillshot_type::skillshot_circle);
        w = plugin_sdk->register_spell(spellslot::w, 650);
        e = plugin_sdk->register_spell(spellslot::e, myhero->get_attack_range());
        r = plugin_sdk->register_spell(spellslot::r, 175);

        if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
            flash = plugin_sdk->register_spell(spellslot::summoner1, 400.f);
        else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
            flash = plugin_sdk->register_spell(spellslot::summoner2, 400.f);


        // Create a menu according to the description in the "Menu Section"
        //
        main_tab = menu->create_tab("chogath", "Chogath");
        main_tab->set_assigned_texture(myhero->get_square_icon_portrait());
        {
            // Info
            //
            main_tab->add_separator(myhero->get_model() + ".aio", "ShadowAIO : " + myhero->get_model());

            auto combo = main_tab->add_tab(myhero->get_model() + ".combo", "Combo Settings");
            {
                combo::use_q = combo->add_checkbox(myhero->get_model() + ".combo.q", "Use Q", true);
                combo::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                combo::use_w = combo->add_checkbox(myhero->get_model() + ".combo.w", "Use W", true);
                combo::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                combo::use_e = combo->add_checkbox(myhero->get_model() + ".combo.e", "Use E", true);
                combo::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
                combo::use_r = combo->add_checkbox(myhero->get_model() + ".combo.r", "Use R", true);
                combo::use_r->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
                auto r_config = combo->add_tab(myhero->get_model() + ".combo.r.config", "R Config");
                {
                    combo::r_use_flash_r = r_config->add_checkbox(myhero->get_model() + ".combo.r.use_flash_r", "Use Flash + R above R range", true);
                    combo::r_use_on_epic_monsters = r_config->add_checkbox(myhero->get_model() + ".combo.r.use_on_epic_monsters", "Auto R on epic monsters", true);
                    auto use_r_on_tab = r_config->add_tab(myhero->get_model() + ".combo.r.use_on", "Use R On");
                    {
                        for (auto&& enemy : entitylist->get_enemy_heroes())
                        {
                            // In this case you HAVE to set should save to false since key contains network id which is unique per game
                            //
                            combo::r_use_on[enemy->get_network_id()] = use_r_on_tab->add_checkbox(std::to_string(enemy->get_network_id()), enemy->get_model(), true, false);

                            // Set texture to enemy square icon
                            //
                            combo::r_use_on[enemy->get_network_id()]->set_texture(enemy->get_square_icon_portrait());
                        }
                    }
                }
            }

            auto harass = main_tab->add_tab(myhero->get_model() + ".harass", "Harass Settings");
            {
                harass::use_q = harass->add_checkbox(myhero->get_model() + ".harass.q", "Use Q", false);
                harass::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                harass::use_w = harass->add_checkbox(myhero->get_model() + ".harass.w", "Use W", true);
                harass::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                harass::use_e = harass->add_checkbox(myhero->get_model() + ".harass.e", "Use E", false);
                harass::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
            }

            auto laneclear = main_tab->add_tab(myhero->get_model() + ".laneclear", "Lane Clear Settings");
            {
                laneclear::spell_farm = laneclear->add_hotkey(myhero->get_model() + ".laneclear.enabled", "Toggle Spell Farm", TreeHotkeyMode::Toggle, 'H', true);
                laneclear::use_q = laneclear->add_checkbox(myhero->get_model() + ".laneclear.q", "Use Q", false);
                laneclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                laneclear::use_w = laneclear->add_checkbox(myhero->get_model() + ".laneclear.w", "Use W", true);
                laneclear::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                laneclear::use_e = laneclear->add_checkbox(myhero->get_model() + ".laneclear.e", "Use E", true);
                laneclear::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
                laneclear::use_e_on_turret = laneclear->add_checkbox(myhero->get_model() + ".laneclear.e.on_turret", "Use E On Turret", true);
                laneclear::use_e_on_turret->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
            }

            auto jungleclear = main_tab->add_tab(myhero->get_model() + ".jungleclear", "Jungle Clear Settings");
            {
                jungleclear::use_q = jungleclear->add_checkbox(myhero->get_model() + ".jungleclear.q", "Use Q", true);
                jungleclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                jungleclear::use_w = jungleclear->add_checkbox(myhero->get_model() + ".jungleclear.w", "Use W", true);
                jungleclear::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                jungleclear::use_e = jungleclear->add_checkbox(myhero->get_model() + ".jungleclear.e", "Use E", true);
                jungleclear::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
            }


            auto fleemode = main_tab->add_tab(myhero->get_model() + ".fleemode", "Flee Mode");
            {
                fleemode::use_q = fleemode->add_checkbox(myhero->get_model() + ".flee.q", "Use Q", true);
                fleemode::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
            }

            auto hitchance = main_tab->add_tab(myhero->get_model() + ".hitchance", "Hitchance Settings");
            {
                hitchance::q_hitchance = hitchance->add_combobox(myhero->get_model() + ".hitchance.q", "Hitchance Q", { {"Low",nullptr},{"Medium",nullptr },{"High", nullptr},{"Very High",nullptr} }, 2);
                hitchance::w_hitchance = hitchance->add_combobox(myhero->get_model() + ".hitchance.e", "Hitchance W", { {"Low",nullptr},{"Medium",nullptr },{"High", nullptr},{"Very High",nullptr} }, 1);
            }

            auto draw_settings = main_tab->add_tab(myhero->get_model() + ".draw", "Drawings Settings");
            {
                float color[] = { 0.0f, 1.0f, 1.0f, 1.0f };
                draw_settings::draw_range_q = draw_settings->add_checkbox(myhero->get_model() + ".draw.q", "Draw Q range", true);
                draw_settings::draw_range_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                draw_settings::q_color = draw_settings->add_colorpick(myhero->get_model() + ".draw.q.color", "Q Color", color);
                draw_settings::draw_range_w = draw_settings->add_checkbox(myhero->get_model() + ".draw.w", "Draw W range", true);
                draw_settings::draw_range_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                draw_settings::w_color = draw_settings->add_colorpick(myhero->get_model() + ".draw.w.color", "W Color", color);
                draw_settings::draw_range_r = draw_settings->add_checkbox(myhero->get_model() + ".draw.r", "Draw R range", true);
                draw_settings::draw_range_r->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
                draw_settings::r_color = draw_settings->add_colorpick(myhero->get_model() + ".draw.r.color", "R Color", color);
            }
        }

        // To add a new event you need to define a function and call add_calback
        //
        event_handler<events::on_update>::add_callback(on_update);
        event_handler<events::on_draw>::add_callback(on_draw);
        event_handler<events::on_before_attack_orbwalker>::add_callback(on_before_attack);
    }

    void unload()
    {
        // Always remove all declared spells
        //
        plugin_sdk->remove_spell(q);
        plugin_sdk->remove_spell(w);
        plugin_sdk->remove_spell(e);
        plugin_sdk->remove_spell(r);

        // Remove menu tab
        //
        menu->delete_tab(main_tab);

        // VERY important to remove always ALL events
        //
        event_handler<events::on_update>::remove_handler(on_update);
        event_handler<events::on_draw>::remove_handler(on_draw);
        event_handler<events::on_before_attack_orbwalker>::remove_handler(on_before_attack);
    }

    // Main update script function
    void on_update()
    {
        if (myhero->is_dead())
        {
            return;
        }

        if (r->is_ready() && combo::use_r->get_bool())
        {
            r_logic();
        }

        // Very important if can_move ( extra_windup ) 
        // Extra windup is the additional time you have to wait after the aa
        // Too small time can interrupt the attack
        if (orbwalker->can_move(0.05f))
        {
            //Checking if the user has combo_mode() (Default SPACE)
            if (orbwalker->combo_mode())
            {
                if (q->is_ready() && combo::use_q->get_bool())
                {
                    q_logic();
                }

                if (w->is_ready() && combo::use_w->get_bool())
                {
                    w_logic();
                }

                if (e->is_ready() && combo::use_e->get_bool())
                {
                    e_logic();
                }
            }

            //Checking if the user has selected harass() (Default C)
            if (orbwalker->harass())
            {
                // Get a target from a given range
                auto target = target_selector->get_target(q->range(), damage_type::magical);

                // Always check an object is not a nullptr!
                if (target != nullptr)
                {
                    if (!myhero->is_under_enemy_turret())
                    {
                        if (q->is_ready() && harass::use_q->get_bool())
                        {
                            q_logic();
                        }

                        if (w->is_ready() && harass::use_w->get_bool())
                        {
                            w_logic();
                        }

                        if (e->is_ready() && harass::use_e->get_bool())
                        {
                            e_logic();
                        }
                    }
                }
            }

            // Checking if the user has selected flee_mode() (Default Z)
            if (orbwalker->flee_mode())
            {
                if (q->is_ready() && fleemode::use_q->get_bool())
                {
                    q_logic();
                }
            }

            // Checking if the user has selected lane_clear_mode() (Default V)
            if (orbwalker->lane_clear_mode() && laneclear::spell_farm->get_bool())
            {
                // Gets enemy minions from the entitylist
                auto lane_minions = entitylist->get_enemy_minions();

                // Gets jugnle mobs from the entitylist
                auto monsters = entitylist->get_jugnle_mobs_minions();

                // You can use this function to delete minions that aren't in the specified range
                lane_minions.erase(std::remove_if(lane_minions.begin(), lane_minions.end(), [](game_object_script x)
                    {
                        return !x->is_valid_target(q->range());
                    }), lane_minions.end());

                // You can use this function to delete monsters that aren't in the specified range
                monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](game_object_script x)
                    {
                        return !x->is_valid_target(q->range());
                    }), monsters.end());

                //std::sort -> sort lane minions by distance
                std::sort(lane_minions.begin(), lane_minions.end(), [](game_object_script a, game_object_script b)
                    {
                        return a->get_position().distance(myhero->get_position()) < b->get_position().distance(myhero->get_position());
                    });

                //std::sort -> sort monsters by max health
                std::sort(monsters.begin(), monsters.end(), [](game_object_script a, game_object_script b)
                    {
                        return a->get_max_health() > b->get_max_health();
                    });

                if (!lane_minions.empty())
                {
                    if (q->is_ready() && laneclear::use_q->get_bool())
                    {
                        if (lane_minions.front()->is_under_ally_turret())
                        {
                            if (myhero->count_enemies_in_range(900) == 0)
                            {
                                if (q->cast(lane_minions.front()))
                                {
                                    return;
                                }
                            }
                        }
                        if (q->cast(lane_minions.front()))
                            return;
                    }

                    if (w->is_ready() && laneclear::use_w->get_bool())
                    {
                        if (lane_minions.front()->is_under_ally_turret())
                        {
                            if (myhero->count_enemies_in_range(900) == 0)
                            {
                                if (w->cast(lane_minions.front()))
                                {
                                    return;
                                }
                            }
                        }
                        if (w->cast(lane_minions.front()))
                            return;
                    }
                }


                if (!monsters.empty())
                {
                    // Logic responsible for monsters
                    if (q->is_ready() && jungleclear::use_q->get_bool())
                    {
                        if (q->cast(monsters.front()))
                            return;
                    }

                    if (w->is_ready() && jungleclear::use_w->get_bool())
                    {
                        if (w->cast(monsters.front()))
                            return;
                    }
                }
            }
        }
    }

#pragma region q_logic
    void q_logic()
    {
        // Get a target from a given range
        auto target = target_selector->get_target(q->range(), damage_type::magical);

        // Always check an object is not a nullptr!
        if (target != nullptr)
        {
            q->cast(target, get_hitchance(hitchance::q_hitchance));
        }
    }
#pragma endregion

#pragma region w_logic
    void w_logic()
    {
        // Get a target from a given range
        auto target = target_selector->get_target(w->range(), damage_type::magical);

        // Always check an object is not a nullptr!
        if (target != nullptr)
        {
            w->cast(target, get_hitchance(hitchance::w_hitchance));
        }
    }
#pragma endregion

#pragma region e_logic
    void e_logic()
    {
        // Get a target from a given range
        auto target = target_selector->get_target(myhero->get_attack_range(), damage_type::magical);

        // Always check an object is not a nullptr!
        if (target != nullptr)
        {
            // Checking if the target will die from E damage
            if (e->get_damage(target) >= target->get_health())
            {
                if (e->cast())
                    return;
            }
        }
    }
#pragma endregion

#pragma region r_logic
    void r_logic()
    {

        // Get a target from a given range
        auto target = target_selector->get_target(r->range(), damage_type::true_dmg);

        // Always check an object is not a nullptr!
        if (target != nullptr)
        {
            if (r->get_damage(target) > target->get_health() && can_use_r_on(target))
            {
                r->cast(target);
            }
        }
        else
        {
            if (flash && flash->is_ready() && combo::r_use_flash_r->get_bool())
            {
                auto target = target_selector->get_target(r->range() + flash->range(), damage_type::magical);
                if (target != nullptr && can_use_r_on(target) && myhero->get_distance(target) > r->range() && r->get_damage(target) > target->get_health())
                {
                    flash->cast(target);
                    r->cast(target);
                    return;
                }
            }
        }

        if (combo::r_use_on_epic_monsters->get_bool())
        {
            // Gets jugnle mobs from the entitylist
            auto monsters = entitylist->get_jugnle_mobs_minions();

            // You can use this function to delete monsters that aren't in the specified range
            monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](game_object_script x)
                {
                    return !x->is_valid_target(r->range());
                }), monsters.end());

            // Delete non epic monsters
            monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](game_object_script x)
                {
                    return !x->is_epic_monster();
                }), monsters.end());

            //std::sort -> sort monsters by max health
            std::sort(monsters.begin(), monsters.end(), [](game_object_script a, game_object_script b)
                {
                    return a->get_max_health() > b->get_max_health();
                });

            if (!monsters.empty())
            {
                if (r->get_damage(monsters.front()) > monsters.front()->get_health())
                {
                    r->cast(monsters.front());
                }
            }
        }
    }
#pragma endregion

#pragma region can_use_r_on
    bool can_use_r_on(game_object_script target)
    {
        auto it = combo::r_use_on.find(target->get_network_id());
        if (it == combo::r_use_on.end())
            return false;

        return it->second->get_bool();
    }
#pragma endregion

#pragma region get_hitchance
    hit_chance get_hitchance(TreeEntry* entry)
    {
        switch (entry->get_int())
        {
            case 0:
                return hit_chance::low;
                break;
            case 1:
                return hit_chance::medium;
                break;
            case 2:
                return hit_chance::high;
                break;
            case 3:
                return hit_chance::very_high;
                break;
            }
        return hit_chance::medium;
    }
#pragma endregion

    void on_before_attack(game_object_script target, bool* process)
    {
        if (e->is_ready())
        {
            // Using e before autoattack on enemies
            if (target->is_ai_hero() && ((orbwalker->combo_mode() && combo::use_e->get_bool()) || (orbwalker->harass() && harass::use_e->get_bool())))
            {
                if (e->cast())
                {
                    return;
                }
            }

            // Using e before autoattack on turrets
            if (orbwalker->lane_clear_mode() && myhero->is_under_enemy_turret() && laneclear::use_e_on_turret->get_bool() && target->is_ai_turret())
            {
                if (e->cast())
                {
                    return;
                }
            }

            // Using e before autoattack on minions
            if (orbwalker->lane_clear_mode() && laneclear::use_e->get_bool() && target->is_ai_minion())
            {
                if (e->cast())
                {
                    return;
                }
            }

            // Using e before autoattack on monsters
            if (orbwalker->lane_clear_mode() && jungleclear::use_e->get_bool() && target->is_monster())
            {
                if (e->cast())
                {
                    return;
                }
            }
        }
    }

    void on_draw()
    {

        if (myhero->is_dead())
        {
            return;
        }

        // Draw Q range
        if (q->is_ready() && draw_settings::draw_range_q->get_bool())
            draw_manager->add_circle(myhero->get_position(), q->range(), draw_settings::q_color->get_color());

        // Draw W range
        if (w->is_ready() && draw_settings::draw_range_w->get_bool())
            draw_manager->add_circle(myhero->get_position(), w->range(), draw_settings::w_color->get_color());

        // Draw R range
        if (e->is_ready() && draw_settings::draw_range_r->get_bool())
            draw_manager->add_circle(myhero->get_position(), r->range(), draw_settings::r_color->get_color());

        auto pos = myhero->get_position();
        renderer->world_to_screen(pos, pos);
        auto spellfarm = laneclear::spell_farm->get_bool();
        draw_manager->add_text_on_screen(pos + vector(0, 40), (spellfarm ? 0xFF00FF00 : 0xFF0000FF), 14, "FARM %s", (spellfarm ? "ON" : "OFF"));
    }
};