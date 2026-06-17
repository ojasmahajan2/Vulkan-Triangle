struct VSInput
{
    float2 position : POSITION;
    float3 color : COLOR;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float3 color : COLOR;
    float pointSize : SV_PointSize;
};

struct FSInput
{
    float4 pos : SV_Position;
    float3 color : COLOR;
};

struct PushConstants
{
    float4x4 mvp;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    
    float4 localPosition = float4(input.position, 0.0, 1.0);
    
    output.pos = mul(pc.mvp, localPosition);
    output.color = input.color;
    output.pointSize = 8.0;
    
    return output;
}

[shader("fragment")]
float4 fragmentMain(FSInput input) : SV_Target
{
    return float4(input.color, 1.0);
}