struct PS_INPUT
{
	float4 PosH : SV_POSITION;
	float2 TexCoords : TEXCOORD;
};

#include "../Common/Samplers.hlsl"

cbuffer PerFrameCB : register(b0)
{
    int ScreenWidth;
    int ScreenHeight;
    int Enabled;
    int BoxBlurNumber;
};

Texture2D CurrentTex : register(t0);

float4 main(PS_INPUT input) : SV_TARGET
{
    if(Enabled == 1)
    {
        int iNeighbourCount = 0;
        float4 average = float4(0, 0, 0, 0);
    
        float2 pixelSize = float2(1 / (float) ScreenWidth, 1 / (float) ScreenHeight);
    
        float2 adjustedCoords;
    
        for (int i = -BoxBlurNumber; i < BoxBlurNumber + 1; ++i)
        {
            for (int j = -BoxBlurNumber; j < BoxBlurNumber + 1; ++j)
            {
                adjustedCoords = input.TexCoords + float2(pixelSize.x * i, pixelSize.y * j);

                if (adjustedCoords.x >= 0.0f && adjustedCoords.y >= 0.0f && adjustedCoords.x <= 1.0f && adjustedCoords.y <= 1.0f)
                {
                    average += CurrentTex.Sample(SamLinearClamp, adjustedCoords);
                
                    ++iNeighbourCount;
                }
            }
        }
    
        average /= (float) iNeighbourCount;
    
        return average;
    }
    else
    {
        return CurrentTex.Sample(SamLinearClamp, input.TexCoords);
    }
}