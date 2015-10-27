#version 150

//max bones per mesh = 64
//max bones per vertex = 4
const int MAX_BONES = 64;

in vec4 vPosition;
in vec3 vNormal;
in vec2 vTexCoord;

in ivec4 vBoneIDs;
in vec4 vBoneWeights;

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
	//p1 normalize
	
    mat4 Bone = vBoneWeights.x * BoneTransforms[vBoneIDs.x]
              + vBoneWeights.y * BoneTransforms[vBoneIDs.y]
              + vBoneWeights.z * BoneTransforms[vBoneIDs.z]
              + vBoneWeights.w * BoneTransforms[vBoneIDs.w];

	
    // Transform vertex position into view coordinates
    fPositionMV = (ModelView * Bone * vPosition).xyz;
    gl_Position = Projection * vec4(fPositionMV, 1.0f);
	
	fNormalMV = mat3(ModelView * Bone) * vNormal;
    fTexCoord = vTexCoord * texScale;
}