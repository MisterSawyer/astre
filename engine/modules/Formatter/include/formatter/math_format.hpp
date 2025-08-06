#pragma once

#include <string>
#include <format>

#include "math/math.hpp"


template <>
struct std::formatter<astre::math::Mat4> : std::formatter<std::string>
{
    auto format(const astre::math::Mat4& mat4, format_context& ctx) const
    {
        std::array<std::array<std::string, 4>, 4> elements;
        std::array<std::size_t, 4> max_width = {0, 0, 0, 0};

        // Format all elements and determine max column widths
        for (std::uint8_t row = 0; row < 4; ++row)
        {
            for (std::uint8_t col = 0; col < 4; ++col)
            {
                std::ostringstream oss;
                oss << std::setprecision(4) << std::fixed << mat4[row][col];
                elements[row][col] = oss.str();
                max_width[col] = std::max(max_width[col], elements[row][col].size());
            }
        }

        // Build formatted string
        std::ostringstream out;
        out << '\n';
        for (std::uint8_t row = 0; row < 4; ++row)
        {
            out << "|";
            for (std::uint8_t col = 0; col < 4; ++col)
            {
                out << std::setw(static_cast<int>(max_width[col])) << elements[row][col];
                if (col != 3) out << " ";
            }
            out << "|\n";
        }

        return std::formatter<std::string>::format(out.str(), ctx);
    }
};