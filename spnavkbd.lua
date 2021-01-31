require "math"

-- Global config
hit_zone      = 100
initial_delay = 0.5
repeat_delay  = 0.1

-- Action specific config
actions = {
    panning = {
        hit_zone = 100,
        repeat_delay  = 0.025,
    },

    pressing = {
        hit_zone = 50,
        initial_delay = 0.75,
        repeat_delay  = 0.3,
    },

    tilting = {
        hit_zone = 50,
    },

    twisting = {
        hit_zone = 200,
        initial_delay = 0.25,
        repeat_delay = 0.01,
    },
}

-- Fill in missing values with defaults
for _, a in pairs(actions) do
    a.hit_zone = a.hit_zone or hit_zone or 100
    a.neg_lim = -350 + a.hit_zone
    a.pos_lim =  350 - a.hit_zone
    a.initial_delay = a.initial_delay or initial_delay or 0.5
    a.repeat_delay = a.repeat_delay or repeat_delay or 0.1
end

-- Axis specific config. Comment out or remove the table for any axis you do
-- not wish to use to disable it. You can also omit the 'neg_keycode' or
-- 'pos_keycode' for any action to disable raising key events for movement in
-- that direction.
axis_conf = {
    -- Panning
    x = {
        neg_keycode = 113, -- pan left
        pos_keycode = 114, -- pan right
        action = actions.panning,
    },
    z = {
        neg_keycode = 116, -- pan forward
        pos_keycode = 111, -- pan backward
        action = actions.panning,
    },

    -- Pressing down / Pulling up
    y = {
        neg_keycode = 36, -- push down
        pos_keycode = 22,  -- pull up
        action = actions.pressing,
    },

    -- Tilting
    rx = {
        neg_keycode = 111, -- tilt forward
        pos_keycode = 116, -- tilt backward
        action = actions.tilting,
    },
    rz = {
        neg_keycode = 113, -- tilt left
        pos_keycode = 114, -- tilt right
        action = actions.tilting,
    },

    -- Twisting
    ry = {
        neg_keycode = 21, -- twist clockwise
        pos_keycode = 20, -- twist anti-clockwise
        action = actions.twisting,
    },
}

function calc_event_delay(action, time, disp)
    if (time - (action.next_event or 0)) > action.initial_delay then
        action.next_event = time + action.initial_delay
    else
        local intensity = math.max(350 - math.abs(disp), 0) / action.hit_zone
        local diff = action.initial_delay - action.repeat_delay
        action.next_event = time + action.repeat_delay + (diff * intensity)
    end
end

function motion_event(x, y, z, rx, ry, rz)
    --print(string.format("motion %4d %4d %4d %4d %4d %4d", x, y, z, rx, ry, rz))
    local actions_hit, time = {}

    for axis, disp in pairs { x = x, y = y, z = z, rx = rx, ry = ry, rz = rz } do
        local axis = axis_conf[axis]
        if axis then -- axis enabled?
            local a = axis.action
            if disp <= a.neg_lim or disp >= a.pos_lim then
                -- Prevent key repeat delay being reset (when multiple axises
                -- share the same action class)
                actions_hit[a] = a

                -- Only call gettime() when we need it
                if not time then time = gettime() end

                if (a.next_event or 0) < time then
                    local sign = disp < 0 and "neg" or "pos"
                    if axis[sign .. "_keycode"] then
                        send_keyev(axis[sign.."_keycode"], axis[sign.."_mask"])
                        calc_event_delay(a, time, disp)
                    end
                end
            end
        end
    end

    -- Reset key repeat delays for all action classes which didn't get
    -- activated in this event callback
    for _, a in pairs(actions) do
        if not actions_hit[a] then
            a.next_event = 0
        end
    end
end


-- Available actions
-- send_keyev -- press and immediate release of key
-- press_keyev -- press of key
-- release_keyev -- release of key

SHIFT = 50
CTRL = 37
ALT = 64

function button_event(action, bnum)
    if action == "press" then
        if bnum == 0 then
            send_keyev(9, 0)
        elseif bnum == 1 then
            send_keyev(23, 0)
        elseif bnum == 6 then
            press_keyev(SHIFT, 0)
        elseif bnum == 7 then
            press_keyev(CTRL, 0)
        elseif bnum == 8 then
            press_keyev(ALT, 0)
        end
    else
        if bnum == 6 then
            release_keyev(SHIFT, 0)
        elseif bnum == 7 then
            release_keyev(CTRL, 0)
        elseif bnum == 8 then
            release_keyev(ALT, 0)
        end
    end
end
