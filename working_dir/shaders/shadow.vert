#version 430
/*
    Shadow Pass Vertex Shader
    Renders scene from light's perspective for shadow mapping
*/

#define IN_POS          layout(location = 0)

#define U_TRANSFORM_MODEL   layout(location = 0)
#define U_TRANSFORM_VIEW    layout(location = 1)
#define U_TRANSFORM_PROJ    layout(location = 2)

// Input
in IN_POS vec3 vPos;

// Output
out gl_PerVertex {vec4 gl_Position;};
out float fDepth;

// Uniforms
U_TRANSFORM_MODEL   uniform mat4 uModel;
U_TRANSFORM_VIEW    uniform mat4 uView;
U_TRANSFORM_PROJ    uniform mat4 uProjection;

void main(void)
{
    vec4 clipPos = uProjection * uView * uModel * vec4(vPos, 1.0);
    gl_Position = clipPos;
    // NDC z değerini fragment shader'a gönder
    fDepth = clipPos.z / clipPos.w;
}
