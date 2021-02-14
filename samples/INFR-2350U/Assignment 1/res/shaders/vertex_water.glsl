#version 410

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec2 outUV;

uniform mat4 u_ModelViewProjection;
uniform mat4 u_View;
uniform mat4 u_Model;
uniform mat3 u_NormalMatrix;
uniform vec3 u_LightPos;

//wave
//uniform float time;

void main() {

	gl_Position = u_ModelViewProjection * vec4(inPosition, 1.0);

	// Lecture 5
	// Pass vertex pos in world space to frag shader
	outPos = (u_Model * vec4(inPosition, 1.0)).xyz;
	//wave
	//outPos.y = sin(vert.x * 25.0 + 0.0004) * 0.004;

	// Normals
	outNormal = u_NormalMatrix * inNormal;

	// Pass our UV coords to the fragment shader
	outUV = inUV;

	//wave
	// Lecture 10b sine
	//vec3 vert = vertex_pos;

	//vert.y = sin(vert.x * 25.0 + time) * 0.004;
	//horizontal wave amounts and amplitude
	
	//sine
	//heightmap
	
	//gl_Position = MVP * vec4(vert, 1.0);



	///////////
	outColor = inColor;

}

