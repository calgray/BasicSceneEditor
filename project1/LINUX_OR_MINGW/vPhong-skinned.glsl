#version 330

//max bones per mesh = 64
//max bones per vertex = 4
const int MAX_BONES = 64;

in vec4 vPosition;
in vec3 vNormal;
in vec2 vTexCoord;

in ivec4 vBoneIndices;
in vec4 vBoneWeights;   //bone weights must be p1 normalized

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
	
	mat4 Bone = vBoneWeights.x * BoneTransforms[vBoneIndices.x%12]
              + vBoneWeights.y * BoneTransforms[vBoneIndices.y%12]
              + vBoneWeights.z * BoneTransforms[vBoneIndices.z%12]
              + vBoneWeights.w * BoneTransforms[vBoneIndices.w%12];
	
    //Bone = vBoneWeights.x * BoneTransforms[1] +
    //       vBoneWeights.y * BoneTransforms[1] +
    //       vBoneWeights.z * BoneTransforms[1] +
    //       vBoneWeights.w * BoneTransforms[1];

	
    // Transform vertex position into view coordinates
    fPositionMV = (ModelView * Bone * vPosition).xyz;
    gl_Position = Projection * vec4(fPositionMV, 1.0f);
	
	fNormalMV = mat3(ModelView) * vNormal;
    fTexCoord = vTexCoord * texScale;
}