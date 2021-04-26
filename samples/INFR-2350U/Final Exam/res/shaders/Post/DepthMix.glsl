#version 420

layout(binding = 0) uniform sampler2D s_screenTex; //Source image
uniform float u_Threshold;

out vec4 frag_color;

layout(location = 0) in vec2 inUV;

/*
uniform float u_TextureMix;
uniform vec3  u_CamPos;

layout (binding = 29) uniform sampler2D s_DepthBuffer;

uniform float u_WindowWidth;
uniform float u_WindowHeight;

uniform float u_maximum;
uniform float u_minimum;

out vec4 frag_color;

//depth eqn
float linearize_depth(float d, float zNear, float zFar)
{
	float z_n = 2.0 * d - 1.0;
	return 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
}

*/
void main() 
{
	//effect
	vec4 colour = texture(s_screenTex, inUV);

    float luminance = (colour.r + colour.g + colour.b) / 3.0;


    /*

    //Read in our depth buffer
	vec2 screenUV = vec2(gl_FragCoord.x / u_WindowWidth, gl_FragCoord.y / u_WindowHeight);
	float expectedDepth = linearize_depth(texture(s_DepthBuffer, screenUV).x, 0.01, 1000.0);

	float actualDepth = linearize_depth(gl_FragCoord.z, 0.01, 1000.0);

	float depthDiff = expectedDepth - actualDepth;
    
    if (depthDiff <= u_minimum)
	{
		//blur mix
		float blur = smoothstep(u_minimum, u_maximum, length(position - focusPoint));
		fragColor  = mix(focusColor, outOfFocusColor, blur);
	}
	else if (depthDiff >= u_maximum)
	{
		//blur mix
		float blur = smoothstep(u_minimum, u_maximum, length(position - focusPoint));
		fragColor  = mix(focusColor, outOfFocusColor, blur);
	}
	else
	{
		//keep clear
		
	}




    */



    
    if (luminance > u_Threshold) 
    {
        frag_color = colour;
    }
    else
    {
        frag_color = vec4(0.0, 0.0, 0.0, 1.0);
    }
}