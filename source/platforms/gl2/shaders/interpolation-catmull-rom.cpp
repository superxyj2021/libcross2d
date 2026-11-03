const char *c2d_interpolation_catmull_rom_shader = R"text(
#if defined(VERTEX)

#if __VERSION__ >= 130
#define COMPAT_VARYING out
#define COMPAT_ATTRIBUTE in
#define COMPAT_TEXTURE texture
#else
#define COMPAT_VARYING varying
#define COMPAT_ATTRIBUTE attribute
#define COMPAT_TEXTURE texture2D
#endif

#ifdef GL_ES
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif

COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_ATTRIBUTE vec4 COLOR;
COMPAT_ATTRIBUTE vec4 TexCoord;
COMPAT_VARYING vec4 COL0;
COMPAT_VARYING vec4 TEX0;

vec4 _oPosition1;
uniform mat4 MVPMatrix;
uniform COMPAT_PRECISION int FrameDirection;
uniform COMPAT_PRECISION int FrameCount;
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;

void main()
{
    gl_Position = MVPMatrix * VertexCoord;
    COL0 = COLOR;
    TEX0.xy = TexCoord.xy;
}

#elif defined(FRAGMENT)

#if __VERSION__ >= 130
#define COMPAT_VARYING in
#define COMPAT_TEXTURE texture
out vec4 FragColor;
#else
#define COMPAT_VARYING varying
#define FragColor gl_FragColor
#define COMPAT_TEXTURE texture2D
#endif

#ifdef GL_ES
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif

uniform COMPAT_PRECISION int FrameDirection;
uniform COMPAT_PRECISION int FrameCount;
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;
uniform sampler2D Texture;
COMPAT_VARYING vec4 TEX0;

// compatibility #defines
#define Source Texture
#define vTexCoord TEX0.xy

#define SourceSize vec4(TextureSize, 1.0 / TextureSize) //either TextureSize or InputSize
#define OutputSize vec4(OutputSize, 1.0 / OutputSize)

// Bicubic Catmull-Rom 9 taps (Fast), ported by Hyllian 2020, from:
// https://github.com/libretro/slang-shaders interpolation-catmull-rom.slang
// Requires LINEAR texture filtering. mat3x4 expanded manually (GLES2 has no
// non-square matrices).

void main()
{
    vec2 samplePos = vTexCoord * SourceSize.xy;
    vec2 texPos1 = floor(samplePos - 0.5) + 0.5;

    vec2 f = samplePos - texPos1;

    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);

    // bilinear filtering evaluates the middle 2 samples at once
    vec2 w12 = w1 + w2;
    vec2 offset12 = w2 / max(w12, vec2(0.0001));

    vec2 texPos0  = texPos1 - 1.0;
    vec2 texPos3  = texPos1 + 2.0;
    vec2 texPos12 = texPos1 + offset12;

    texPos0  *= SourceSize.zw;
    texPos3  *= SourceSize.zw;
    texPos12 *= SourceSize.zw;

    vec4 c00 = COMPAT_TEXTURE(Source, vec2(texPos0.x, texPos0.y));
    vec4 c10 = COMPAT_TEXTURE(Source, vec2(texPos12.x, texPos0.y));
    vec4 c20 = COMPAT_TEXTURE(Source, vec2(texPos3.x, texPos0.y));

    vec4 c01 = COMPAT_TEXTURE(Source, vec2(texPos0.x, texPos12.y));
    vec4 c11 = COMPAT_TEXTURE(Source, vec2(texPos12.x, texPos12.y));
    vec4 c21 = COMPAT_TEXTURE(Source, vec2(texPos3.x, texPos12.y));

    vec4 c02 = COMPAT_TEXTURE(Source, vec2(texPos0.x, texPos3.y));
    vec4 c12 = COMPAT_TEXTURE(Source, vec2(texPos12.x, texPos3.y));
    vec4 c22 = COMPAT_TEXTURE(Source, vec2(texPos3.x, texPos3.y));

    vec3 wx = vec3(w0.x, w12.x, w3.x);
    vec3 wy = vec3(w0.y, w12.y, w3.y);

    // mul(wx, mat3x4(c00, c10, c20)) expanded for GLES2
    vec4 c1 = wx.x * c00 + wx.y * c10 + wx.z * c20;
    vec4 c2 = wx.x * c01 + wx.y * c11 + wx.z * c21;
    vec4 c3 = wx.x * c02 + wx.y * c12 + wx.z * c22;

    FragColor = wy.x * c1 + wy.y * c2 + wy.z * c3;
}
#endif
)text";