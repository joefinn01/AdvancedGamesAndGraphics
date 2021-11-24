struct PS_INPUT
{
    float4 PosH : SV_POSITION;
    float2 TexCoords : TEXCOORD;
};

cbuffer SSAOCB : register(b0)
{
    float4 RandomRotations[16];
    float4 HemisphereSamples[64];
};

cbuffer PerFrameCB : register(b1)
{
    float4x4 View;
    float4x4 InvProjection;
    float4x4 ProjTex;
    
    int WindowWidth;
    int WindowHeight;
    int NumSamples;
    float Radius;
};

#include "../Common/Samplers.hlsl"

Texture2D NormalTex : register(t0);
Texture2D DepthTex : register(t1);

float4 main(PS_INPUT input) : SV_TARGET
{    
    float depth = DepthTex.Sample(SamLinearClamp, input.TexCoords);
    
    //Reconstruct position in screen space
    float4 positionV = float4(input.TexCoords.x * 2.0f - 1.0f, (1.0f - input.TexCoords.y) * 2.0f - 1.0f, depth, 1.0f);
    positionV = mul(positionV, InvProjection);
    positionV.xyz /= positionV.w;
    
    //Map UV coords to random rotation index
    int2 index2D = (input.TexCoords * float2(WindowWidth, WindowHeight)) % 4;
    
    //Recreate normal in view space
    float3 normalV = mul(normalize((NormalTex.Sample(SamLinearClamp, input.TexCoords).xyz * 2.0f) - float3(1.0f, 1.0f, 1.0f)), (float3x3) View);
    
    //Create tangent vector using normal and random vector
    float3 randomVec = float3(RandomRotations[4 * index2D.x + index2D.y].xy, 0.0f);
    float3 tangentV = normalize(randomVec - normalV * dot(randomVec, normalV));

    //Calculate TBN matrix
    float3x3 tbn = float3x3(tangentV, cross(normalV, tangentV), normalV);
    
    //calculate the occlusion factor
    float occlusion = 0;
    
    for (int i = 0; i < NumSamples; ++i)
    {
        float3 sample = mul(HemisphereSamples[i].xyz, tbn);
        sample = sample * Radius + positionV.xyz;
        
        float4 offset = mul(float4(sample, 1.0f), ProjTex);
        offset /= offset.w;
        
        float sampleDepth = DepthTex.Sample(SamLinearClamp, offset.xy).x;

        float4 tempDepth = mul(float4(0, 0, sampleDepth, 1), InvProjection);
        
        float depthV = tempDepth.z / tempDepth.w;
        
        //Cast bools instead of using if statements to help increase occupancy and stop thread divergence
        occlusion += 1.0f * (int)(abs(positionV.z - depthV) < Radius) * (int) (depthV <= sample.z);
    }
    
    occlusion = 1.0f - (occlusion / (float) NumSamples);
    
    return float4(occlusion, occlusion, occlusion, 1.0f);
}