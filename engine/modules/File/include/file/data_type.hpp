#pragma once

namespace astre::file
{
    struct use_json_t{};
    constexpr use_json_t use_json;

    struct use_binary_t{};
    constexpr use_binary_t use_binary;

    // Mesh source formats. Only OBJ is wired today; FBX/glTF/... slot in here later
    // (new tag + assimp already handles the parse).
    struct use_obj_t{};
    constexpr use_obj_t use_obj;
}