#version 450 core

layout(location=0) out uvec2 outID;   // (lo, hi)

uniform uint uID_lo;
uniform uint uID_hi;

void main()
{
    outID = uvec2(uID_lo, uID_hi);
}