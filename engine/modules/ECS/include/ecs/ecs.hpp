#pragma once

#include "ecs/registry.hpp"

#include "generated/ECS/proto/entity_definition.pb.h"

#include "ecs/system/transform_system.hpp"
#include "ecs/system/camera_system.hpp"
#include "ecs/system/visual_system.hpp"
#include "ecs/system/light_system.hpp"
#include "ecs/system/script_system.hpp"

namespace astre::ecs
{
    struct Systems
    {
        system::TransformSystem transform;
        system::CameraSystem camera;
        system::VisualSystem visual;
        system::LightSystem light;
        system::ScriptSystem script;
    };
}