struct VSInput {
    float3 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};

cbuffer CBuff : register(b0) {
    float4x4 u_projview;
    float4x4 u_apply;
}

PSInput VShader(VSInput input) {
    PSInput output;
    output.texCoord = input.texCoord;
    output.color = input.color;
    output.position = mul(float4(input.position, 1.f), mul(u_projview, u_apply));
    return output;
}

Texture2D my_texture;
SamplerState my_sampler;

float4 PShader(PSInput input) : SV_TARGET {
    float4 outColor = input.color * my_texture.Sample(my_sampler, input.texCoord);
    if (outColor.a == 0.f)
        discard;
    return outColor;
}