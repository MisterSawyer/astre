-- Delta time (in seconds), assumed injected or global
dt = delta or 0.016  -- e.g. 60 FPS fallback

-- Rotation speeds (degrees per second)
pitch_speed = 10.0
yaw_speed   = 20.0
roll_speed  = 30.0
move_speed  = 10

if initial_angles_extracted == nil then
    local quat = transform_component:get_rotation()
    local euler_deg = astre.math.degrees(astre.math.eulerAngles(quat))
    pitch = euler_deg.x
    yaw   = euler_deg.y
    roll  = euler_deg.z
    initial_angles_extracted = true
end


pitch = pitch + pitch_speed * dt
yaw   = yaw + yaw_speed * dt
roll  = roll + roll_speed * dt

pitch = pitch % 360
yaw = yaw % 360
roll = roll % 360

-- Compute new quaternion rotation
local euler_rad = astre.math.radians(astre.math.Vec3(pitch, yaw, roll))
local rotation = astre.math.Quat(euler_rad)

-- Apply to entity transform
transform_component:set_rotation(rotation.w, rotation.x, rotation.y, rotation.z)

if input_component then
    if input_component:is_pressed("KEY_UP") then
        print("KEY_UP") 
        if transform_component then
            transform_component:set_z(transform_component:get_z() - (move_speed * dt))
        end
    end
    if input_component:is_pressed("KEY_DOWN") then
        print("KEY_DOWN") 
        if transform_component then
            transform_component:set_z(transform_component:get_z() + (move_speed * dt))
        end
    end
    
    if input_component:is_pressed("KEY_RIGHT") then
        print("KEY_RIGHT") 
        if transform_component then
            transform_component:set_x(transform_component:get_x() + (move_speed * dt))
        end
    end
    if input_component:is_pressed("KEY_LEFT") then
        print("KEY_LEFT") 
        if transform_component then
            transform_component:set_x(transform_component:get_x() - (move_speed * dt))
        end
    end
end