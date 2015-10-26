#version 150

//max bones per mesh = 64
//max bones per vertex = 4
const int MAX_BONES = 64;

in  vec4 vPosition;
in  vec3 vNormal;
in  vec2 vTexCoord;

in ivec4 vBoneIndices;
in vec4 vBoneWeights;   //bone weights need to be p1 normalized

uniform mat4 BoneTransforms[MAX_BONES];

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
    /*
    mat4 Bone = vBoneWeights[0] * BoneTransforms[vBoneIndices[0]] +
                vBoneWeights[1] * BoneTransforms[vBoneIndices[1]] +
                vBoneWeights[2] * BoneTransforms[vBoneIndices[2]] +
                vBoneWeights[3] * BoneTransforms[vBoneIndices[3]];
    */


    //TODO vBoneIndices and weights aren't being written to
    //mat4 Bone = vBoneWeights[0] * BoneTransforms[0]; 
    mat4 Bone = BoneTransforms[0];  

    // Transform vertex position into view coordinates
    fPositionMV = (ModelView * Bone * vPosition).xyz;
    gl_Position = Projection * vec4(fPositionMV, 1.0f);
	
	fNormalMV = mat3(ModelView) * vNormal;
    fTexCoord = vTexCoord * texScale;
}