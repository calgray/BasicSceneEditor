#version 150

//max bones per mesh = 64
//max bones per vertex = 4
const int MAX_BONES = 64;

in  vec4 vPosition;
in  vec3 vNormal;
in  vec2 vTexCoord;

in ivec4 vBoneIndices;
in vec4 vBoneWeights;   //bone weights should be normalized
uniform mat4 boneTransforms[MAX_BONES];

out vec3 fPositionMV;
out vec3 fNormalMV;
out vec2 fTexCoord;

//camera
uniform mat4 Projection;

//object
uniform mat4 ModelView;

uniform float texScale;

void main()
{
    mat4 Bone = vBoneWeights[0] * boneTransforms[vBoneIndices[0]] +
                vBoneWeights[1] * boneTransforms[vBoneIndices[1]] +
                vBoneWeights[2] * boneTransforms[vBoneIndices[2]] +
                vBoneWeights[3] * boneTransforms[vBoneIndices[3]];
        

    // Transform vertex position into view coordinates
    fPositionMV = (ModelView * vPosition).xyz;
    gl_Position = Projection * vec4(fPositionMV, 1.0f);
	
	fNormalMV = mat3(ModelView) * vNormal;
    fTexCoord = vTexCoord * texScale;
}