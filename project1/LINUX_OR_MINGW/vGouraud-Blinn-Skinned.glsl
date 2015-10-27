#version 150

const int MAX_LIGHTS = 3;
const int MAX_BONES = 64;

in  vec4 vPosition;
in  vec3 vNormal;
in  vec2 vTexCoord;

in ivec4 vBoneIDs;
in vec4 vBoneWeights;

out vec2 texCoord;
out vec4 fColor;
out vec4 fSpecular;

//camera
uniform mat4 Projection;
uniform vec4 Origin; //faster than multiplying an inversetranspose

uniform mat4 BoneTransforms[MAX_BONES];

//object
uniform mat4 ModelView;

//light * material (modelview space)
uniform int LightType[MAX_LIGHTS];
uniform vec4 LightPosition[MAX_LIGHTS];
uniform vec4 LightDirection[MAX_LIGHTS];
uniform vec3 SpotLightVars[MAX_LIGHTS]; //phi theta falloff

uniform vec3 AmbientProduct;
uniform vec3 DiffuseProduct[MAX_LIGHTS], SpecularProduct[MAX_LIGHTS];

//material
uniform float Shininess;
uniform float texScale;

void main()
{
	mat4 Bone = vBoneWeights.x * BoneTransforms[vBoneIDs.x]
              + vBoneWeights.y * BoneTransforms[vBoneIDs.y]
              + vBoneWeights.z * BoneTransforms[vBoneIDs.z]
              + vBoneWeights.w * BoneTransforms[vBoneIDs.w];

    // Transform vertex position into eye coordinates
    vec4 pos = ModelView * Bone * vPosition;
    gl_Position = Projection * pos;
    texCoord = vTexCoord * texScale;

    vec3 ambient = AmbientProduct;
    vec3 diffuse = vec3(0, 0, 0);
    vec3 specular = vec3(0, 0, 0);
       
    // Transform vertex normal into eye coordinates (assumes scaling is uniform across dimensions)
    vec3 N = normalize(mat3(ModelView * Bone) * vNormal);
    vec3 E = normalize( -pos.xyz );   // Direction to the eye/camera

    for(int i = 0; i < MAX_LIGHTS; i++) {
      
		if(LightType[i] == 0) {
			vec3 Lvec = (LightPosition[i] - Origin).xyz; // The vector to the light from the vertex    
			 
			// Unit direction vectors for Blinn-Phong shading calculation
			vec3 L = normalize( Lvec );   // Direction to the light source
			vec3 H = normalize( L + E );  // Halfway vector

				
			float Kd = max( dot(L, N), 0.0 );
			diffuse += Kd * DiffuseProduct[i];
				
			float Ks = pow( max(dot(N, H), 0.0), Shininess * 4 );
			specular += Ks * SpecularProduct[i];
		}
		//Point Light
		else if(LightType[i] == 1) {
			vec3 Lvec = LightPosition[i].xyz - pos.xyz; // The vector to the light from the vertex    
			  
			// Unit direction vectors for Blinn-Phong shading calculation
			vec3 L = normalize( Lvec );   // Direction to the light source
			vec3 H = normalize( L + E );  // Halfway vector

			//reduce intensity with distance from light
			float dist = length(Lvec) + 1.0f;
			float attenuation = 1.0f / dist / dist;
				
			float Kd = max( dot(L, N), 0.0 ) * attenuation;
			diffuse += Kd * DiffuseProduct[i];
				
			float Ks = pow( max(dot(N, H), 0.0), Shininess * 4 ) * attenuation;
			specular += Ks * SpecularProduct[i];
		}
		//Spot Light
		else if(LightType[i] == 2) {
			// The vector to the light from the vertex   
			vec3 Lvec = LightPosition[i].xyz - pos.xyz;
			
			vec3 L = normalize(Lvec);           // Direction to the light source
			vec3 H = normalize( L + E );        // Halfway vector
			
			//angle between the spotlight centre and the fragment being shaded.
			float coneAngle = acos(dot(L, normalize(LightDirection[i].xyz)));
				
			if(coneAngle < SpotLightVars[i].x) {
				
				float illumination;
				if(coneAngle < SpotLightVars[i].y) illumination = 1;
				else {
					illumination =  pow((SpotLightVars[i].x - coneAngle) /
									(SpotLightVars[i].x - SpotLightVars[i].y),
									SpotLightVars[i].z);
				}
				
				float dist = length(Lvec) + 1.0f;
				float attenuation = illumination / dist / dist;
				
				float Kd = max( dot(L, N), 0.0 ) * attenuation;
				diffuse += Kd * DiffuseProduct[i];
				
				float Ks = pow( max(dot(N, H), 0.0), Shininess * 3 ) * attenuation;
				specular += Ks * SpecularProduct[i];
			}
        }
    }

	
    fColor = vec4(ambient + diffuse, 1);
    fSpecular = vec4(specular, 1);
}
