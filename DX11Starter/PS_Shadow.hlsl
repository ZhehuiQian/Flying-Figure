
// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage

// --------assignment5--------

struct DirectionalLight
{
	float4 AmbientColor;
	float4 DiffuseColor;
	float3 Direction;
};
/*
struct PointLight
{
	float4 Color;
	float3 Position;
	float3 CameraPos;
};
*/
cbuffer lightData : register(b0)
{
	//DirectionalLight directionalLight;
	DirectionalLight directionalLight2;
	//PointLight pointLight;
	//float4 PointLightColor;
	//float3 PointLightPosition;

	//float3 CameraPosition;
};

// ----assignment 6----
Texture2D Texture		: register(t0);
SamplerState Sampler	: register(s0);
// --------------------

struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float4 position		: SV_POSITION;
	float3 normal		: NORMAL;
	float3 worldPos		: WORLDPOS;
	float2 uv			: TEXCOORD;      // uv coordination

};

float4 directionallightcalculation(float3 n, DirectionalLight light)
{
	float3 ToLight = -light.Direction;
	float lightAmount = saturate(dot(n, ToLight));
	float4 result = light.DiffuseColor * lightAmount + light.AmbientColor;
	return result;
}
/*
float4 pointlightcalculation(PointLight light, float3 wpos, float3 n )
{
	float3 ToPointLight = normalize(light.Position - wpos);
	float lightAmount = saturate(dot(n, ToPointLight));
	return light.Color * lightAmount;
}

float specularcalculation(PointLight light, float3 wpos, float3 n)
{
	float3 ToCamera = normalize(light.CameraPos - wpos);
	float3 ToPointLight = normalize(light.Position - wpos);
	float3 refl = reflect(-ToPointLight, n);
	float specular = pow(saturate(dot(refl, ToCamera)), 8);
	return specular;
}
*/
// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
	input.normal = normalize(input.normal);

	// ----assignment 6----
	float4 surfaceColor = Texture.Sample(Sampler, input.uv);
	// .Sample() method takes a sampler and a float2, which returns to a color from the texture
	// --------------------

	// Directional light calculation ---------------
	//float4 first_directional = directionallightcalculation(input.normal, directionalLight);
	
	 float4 second_directional = directionallightcalculation(input.normal, directionalLight2);

	// Point light calculation ----------------
	//float4 pointl = pointlightcalculation(pointLight, input.worldPos, input.normal);

	//float specular = specularcalculation(pointLight, input.worldPos, input.normal);

	// float3 dirToPointLight = normalize(PointLightPosition - input.worldPos);
	// float pointLightAmount = saturate(dot(input.normal, dirToPointLight));

	// Specular (for point light) ------------------
	// float3 toCamera = normalize(CameraPosition - input.worldPos);
	// float3 refl = reflect(-dirToPointLight, input.normal);
	// float spec = pow(max(dot(refl, toCamera), 0), 128);

	return (second_directional /*+ second_directional + PointLightColor * pointLightAmount*/)* surfaceColor;
		// + spec;
			//pointl * surfaceColor+
			//specular;


}

