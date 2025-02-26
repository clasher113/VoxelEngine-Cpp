function on_hud_open()
    input.add_callback("player.pick", function ()
        if hud.is_paused() or hud.is_inventory_open() then
            return
        end
        local pid = hud.get_player()
        local x, y, z = player.get_selected_block(pid)
        if x == nil then
            return
        end
        local id = block.get_picking_item(block.get(x, y, z))
        local inv, cur_slot = player.get_inventory(pid)
        local slot = inventory.find_by_item(inv, id, 0, 9)
        if slot then
            player.set_selected_slot(pid, slot)
            return
        end
        if not rules.get("allow-content-access") then
            return
        end
        slot = inventory.find_by_item(inv, 0, 0, 9)
        if slot then
            cur_slot = slot
        end
        player.set_selected_slot(pid, cur_slot)
        inventory.set(inv, cur_slot, id, 1)
    end)

    input.add_callback("player.noclip", function ()
        if hud.is_paused() or hud.is_inventory_open() then
            return
        end
        local pid = hud.get_player()
        if player.is_noclip(pid) then
            player.set_flight(pid, false)
            player.set_noclip(pid, false)
        else
            player.set_flight(pid, true)
            player.set_noclip(pid, true)
        end
    end)
    
    input.add_callback("player.flight", function ()
        if hud.is_paused() or hud.is_inventory_open() then
            return
        end
        local pid = hud.get_player()
        if player.is_noclip(pid) then
            return
        end
        if player.is_flight(pid) then
            player.set_flight(pid, false)
        else
            player.set_flight(pid, true)
            player.set_vel(pid, 0, 1, 0)
        end
    end)
end
