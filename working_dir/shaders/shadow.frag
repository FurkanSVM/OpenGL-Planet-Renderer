#version 430
/*
    Shadow Pass Fragment Shader
    Writes depth value to shadow map
*/

// Input
in float fDepth;

// Output
layout(location = 0) out float fragDepth;

void main(void)
{
    // NDC z değerini [0,1] aralığına normalize et
    fragDepth = fDepth * 0.5 + 0.5;
}
