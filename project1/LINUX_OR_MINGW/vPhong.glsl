#version 150

in  vec4 vPosition;
in  vec3 vNormal;
in  vec2 vTexCoord;

out vec3 fPositionMV;
out vec3 fNormalMV;
out vec2 fTexCoord;

//light
uniform vec4 LightPosition;

//camera
uniform mat4 Projection;

//object
uniform mat4 ModelView;

//material
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;

uniform float texScale;

void main()
{
    // Transform vertex position into eye coordinates
    fPositionMV = (ModelView * vPosition).xyz;
    gl_Position = Projection * vec4(fPositionMV, 1.0f);
	
	fNormalMV = mat3(ModelView) * vNormal;
    fTexCoord = vTexCoord * texScale;
}