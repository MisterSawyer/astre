local camera_input = get_input(entity)
local camera_transform = get_transform(entity)

-- Initial angle setup
if camera_pitch == nil then
    local init_rotation = camera_transform:get_rotation()
    local euler_deg = glm.degrees(glm.eulerAngles(init_rotation))
    camera_pitch = euler_deg.x
    camera_yaw = euler_deg.y
end

local sensitivity = 0.3
local pitch_min = -89.0
local pitch_max = 89.0
local speed = 5.0
local dt = delta or 0.016  -- e.g. 60 FPS fallback
local dx = camera_input:get_mouse_dx()
local dy = camera_input:get_mouse_dy()

camera_yaw = camera_yaw - (dx * sensitivity)
camera_pitch = camera_pitch - (dy * sensitivity)

-- Clamp pitch to avoid gimbal lock
camera_pitch = math.max(pitch_min, math.min(pitch_max, camera_pitch))

-- Recompute quaternion from Euler angles
local euler_rad = glm.radians(glm.vec3(camera_pitch, camera_yaw, 0.0))
local rotation = glm.quat(euler_rad)

-- Apply rotation to transform
camera_transform:set_rotation(rotation)

forward = camera_transform:get_forward()
right = camera_transform:get_right()
up = camera_transform:get_up()

local movement = glm.vec3(0.0, 0.0, 0.0)

-- Horizontal movement
if camera_input:is_pressed("KEY_A") or camera_input:just_pressed("KEY_A") then
    movement = movement - right
end
if camera_input:is_pressed("KEY_D") or camera_input:just_pressed("KEY_D") then
    movement = movement + right
end

-- Forward/back
if camera_input:is_pressed("KEY_W") or camera_input:just_pressed("KEY_W") then
    movement = movement + forward
end
if camera_input:is_pressed("KEY_S") or camera_input:just_pressed("KEY_S") then
    movement = movement - forward
end

-- Vertical movement
if camera_input:is_pressed("KEY_Q") or camera_input:just_pressed("KEY_Q") then
    movement = movement + up
end
if camera_input:is_pressed("KEY_E") or camera_input:just_pressed("KEY_E") then
    movement = movement - up
end

if acc_movement == nil then
    acc_movement = movement
end

acc_movement = (0.1 * acc_movement) + (0.9 * movement)

-- Normalize and apply movement
if glm.length(movement) > 0.0 then
    local position = camera_transform:get_position()
    movement = glm.normalize(movement) * speed * dt
    position = position + movement
    -- Apply new position to ECS transform
    camera_transform:set_x(position.x)
    camera_transform:set_y(position.y)
    camera_transform:set_z(position.z)
end

