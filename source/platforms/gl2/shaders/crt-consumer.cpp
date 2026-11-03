const char *c2d_crt_consumer_shader = R"text(
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
    TEX0.xy = TexCoord.xy * 1.0001;
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

// crt-consumer (GPL), ported to GLES2 from the libretro slang-shaders
// single-pass version. All "#pragma parameter" defaults are baked in (the
// scanline/mask strengths become runtime uniforms in the pfbneo
// parameterization step; for now they are fixed at their default values).

// baked defaults
#define BLURX 0.25
#define BLURY -0.1
#define WARPX 0.03
#define WARPY 0.04
#define CORNER 0.03
#define SMOOTHNESS 400.0
#define INTER 0.0
#define DOWNSCALE 2.0
#define SCANLOW 6.0
#define SCANHIGH 8.0
#define BEAMLOW 1.5
#define BEAMHIGH 1.05
#define PRESERVE 0.98
#define BRIGHTBOOST1 1.25
#define BRIGHTBOOST2 1.0
#define SHADOWMASK 0.0
#define MASKSIZE 1.0
#define MASKDARK 0.2
#define MASKLIGHT 1.5
#define SLOTMASK 0.0
#define SLOTWIDTH 2.0
#define DOUBLE_SLOT 1.0
#define SLOTMS 1.0
#define GAMMA_OUT 2.25
#define SAT 1.0
#define CONTRAST 1.0
#define NOIS 0.0
#define WP 0.0
#define VIGNETTE 0.0
#define VPOWER 0.15
#define VSTR 40.0
#define ALLOFF 0.0
#define POSTBR 1.0
#define GLOW 3.0
#define GLOW_STR 0.3
#define GLOW_QUALITY 1.0
#define RG 0.0
#define RB 0.0
#define GR 0.0
#define GB 0.0
#define BR 0.0
#define BG 0.0
#define PALETTE_FIX 0.0
#define PRE_SCALE 1.5

vec2 Warp(vec2 pos)
{
    pos = pos * 2.0 - 1.0;
    pos *= vec2(1.0 + (pos.y * pos.y) * WARPX, 1.0 + (pos.x * pos.x) * WARPY);
    return pos * 0.5 + 0.5;
}

float sw(float y, float l)
{
    float beam = mix(SCANLOW, SCANHIGH, y);
    float scan = mix(BEAMLOW, BEAMHIGH, l);
    float ex = y * scan;
    return exp2(-beam * ex * ex);
}

vec3 mask(vec2 x, vec3 col, float l)
{
    x = floor(x / MASKSIZE);

    // Shadowmask 0: rgb stripes
    float m = fract(x.x * 0.4999);
    if (m < 0.4999) {
        return vec3(1.0, MASKDARK, 1.0);
    }
    return vec3(MASKDARK, 1.0, MASKDARK);
}

float SlotMask(vec2 pos, vec3 c)
{
    if (SLOTMASK == 0.0) return 1.0;

    pos = floor(pos / SLOTMS);
    float mx = pow(max(max(c.r, c.g), c.b), 1.33);
    float mlen = SLOTWIDTH * 2.0;
    float px = fract(pos.x / mlen);
    float py = floor(fract(pos.y / (2.0 * DOUBLE_SLOT)) * 2.0 * DOUBLE_SLOT);
    float slot_dark = mix(1.0 - SLOTMASK, 1.0 - 0.80 * SLOTMASK, mx);
    float slot = 1.0 + 0.7 * SLOTMASK * (1.0 - mx);

    if      (py == 0.0              && px <  0.5) slot = slot_dark;
    else if (py == DOUBLE_SLOT      && px >= 0.5) slot = slot_dark;

    return slot;
}

mat3 vign(float l)
{
    vec2 vpos = vTexCoord;
    vpos *= 1.0 - vpos.xy;

    float vig = vpos.x * vpos.y * VSTR;
    vig = min(pow(vig, VPOWER), 1.0);
    if (VIGNETTE == 0.0) vig = 1.0;

    return mat3(vig, 0.0, 0.0,
                0.0, vig, 0.0,
                0.0, 0.0, vig);
}

vec3 glow0(vec2 texcoord, vec3 col)
{
    // quality baked to 1.0
    vec2 size = SourceSize.zw / GLOW_QUALITY;

    vec3 c01;
    vec3 sum = vec3(0.0);

    for (int i = -3; i <= 3; i++) {
        float x = float(i);
        float factor = 1.0 / GLOW;
        for (int j = -3; j <= 3; j++) {
            float y = float(j);
            vec2 offset = vec2(x, y) * size;
            c01 = COMPAT_TEXTURE(Source, texcoord + offset).rgb * factor;
            c01 = c01 * c01;
            sum += c01;
        }
    }

    return (GLOW_STR * sum / (GLOW * GLOW));
}

float corner0(vec2 coord)
{
    coord = (coord - vec2(0.5, 0.5)) * 1.0 + vec2(0.5, 0.5);
    coord = min(coord, vec2(1.0, 1.0) - coord) * vec2(1.0, SourceSize.y / SourceSize.x);

    vec2 cdist = vec2(CORNER, CORNER);
    coord = (cdist - min(coord, cdist));
    float dist = sqrt(dot(coord, coord));

    return clamp((cdist.x - dist) * SMOOTHNESS, 0.0, 1.0);
}

void main()
{
    vec2 pos = Warp(vTexCoord.xy);
    vec2 tex_size = SourceSize.xy;

    vec2 pC4 = (pos + 0.5 / tex_size);
    vec2 fp = fract(pos * tex_size);
    if (tex_size.y > 400.0) {
        fp.y = fract(pos.y * tex_size.y * 1.0 / DOWNSCALE);
    }

    vec4 res = vec4(1.0);

    vec2 texel = pos * tex_size;
    vec2 texel_floored = floor(texel);

    float scale = PRE_SCALE;
    float region_range = 0.5 - 0.5 / scale;

    // use the hardware bilinear interpolator to avoid sampling 4 times manually
    vec2 center_dist = fp - 0.5;
    vec2 fpp = (center_dist - clamp(center_dist, -region_range, region_range)) * scale + 0.5;

    vec2 mod_texel = texel_floored + fpp;
    vec2 coords = mod_texel / SourceSize.xy;

    vec3 sample1 = COMPAT_TEXTURE(Source, vec2(coords.x + BLURX * SourceSize.z, coords.y - BLURY * SourceSize.w)).rgb;
    vec3 sample2 = COMPAT_TEXTURE(Source, coords).rgb;
    vec3 sample3 = COMPAT_TEXTURE(Source, vec2(coords.x - BLURX * SourceSize.z, coords.y + BLURY * SourceSize.w)).rgb;

    vec3 color = vec3(sample1.r * 0.5 + sample2.r * 0.5,
                      sample1.g * 0.25 + sample2.g * 0.5 + sample3.g * 0.25,
                      sample2.b * 0.5 + sample3.b * 0.5);

    mat3 hue = mat3(1.0, RG, RB,
                    GR, 1.0, GB,
                    BR, BG, 1.0);

    color = hue * color;

    color = (2.0 * pow(color, vec3(2.8))) - pow(color, vec3(3.6));

    float lum = color.r * 0.3 + color.g * 0.6 + color.b * 0.1;

    float f = fract(fp.y - 0.5);
    color = color * sw(f, lum) + color * sw(1.0 - f, lum);

    float lum1 = color.r * 0.3 + color.g * 0.6 + color.b * 0.1;

    color *= mix(mask((vTexCoord * OutputSize.xy), color, lum1), vec3(1.0), lum1 * PRESERVE);

    color *= mix(BRIGHTBOOST1, BRIGHTBOOST2, max(max(color.r, color.g), color.b));

    color = pow(color, vec3(1.0 / GAMMA_OUT));

    if (GLOW_STR != 0.0) {
        color += glow0(coords, color);
    }

    if (CORNER != 0.0) {
        color *= corner0(pC4);
    }
    res = vec4(color, 1.0);
    res.rgb *= vign(lum);

    FragColor = res;
}
#endif
)text";