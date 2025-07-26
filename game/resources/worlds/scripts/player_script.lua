-- Delta time (in seconds), assumed injected or global
dt = delta or 0.016  -- e.g. 60 FPS fallback

-- Rotation speeds (degrees per second)
pitch_speed = 10.0
yaw_speed   = 20.0
roll_speed  = 30.0
move_speed  = 10

local transform = get_transform(entity)

if initial_angles_extracted == nil then
    local quat = transform:get_rotation()
    local euler_deg = glm.degrees(glm.eulerAngles(quat))
    pitch = euler_deg.x
    yaw   = euler_deg.y
    roll  = euler_deg.z
    initial_angles_extracted = true
    print(pitch, yaw, roll) 
end


pitch = pitch + pitch_speed * dt
yaw   = yaw + yaw_speed * dt
roll  = roll + roll_speed * dt

pitch = pitch % 360
yaw = yaw % 360
roll = roll % 360

-- Compute new quaternion rotation
local euler_rad = glm.radians(glm.vec3(pitch, yaw, roll))
local rotation = glm.quat(euler_rad)

-- Apply to entity transform
transform:set_rotation(rotation.w, rotation.x, rotation.y, rotation.z)

local input = get_input(entity)
if input then
    if input:is_pressed("KEY_UP") then
        if transform then
            transform:set_z(transform:get_z() - (move_speed * dt))
        end
    end
    if input:is_pressed("KEY_DOWN") then
        if transform then
            transform:set_z(transform:get_z() + (move_speed * dt))
        end
    end
    
    if input:is_pressed("KEY_RIGHT") then
        if transform then
            transform:set_x(transform:get_x() + (move_speed * dt))
        end
    end
    if input:is_pressed("KEY_LEFT") then
        if transform then
            transform:set_x(transform:get_x() - (move_speed * dt))
        end
    end
end