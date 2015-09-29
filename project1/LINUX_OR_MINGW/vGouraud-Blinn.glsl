#version 150

in  vec4 vPosition;
in  vec3 vNormal;
in  vec2 vTexCoord;

out vec2 texCoord;
out vec4 color;
out vec4 specular;

//light
//viewspace
uniform vec4 LightPosition[2];

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
	color = AmbientProduct;// + vec4(0.1, 0.1, 0.1, 1);
	
	// Transform vertex position into eye coordinates
	vec4 pos = ModelView * vPosition;
	
	// Transform vertex normal into eye coordinates (assumes scaling is uniform across dimensions)
	vec3 N = normalize(mat3(ModelView) * vNormal);
	vec3 E = normalize( -pos.xyz );   // Direction to the eye/camera
	
	for(int i = 0; i < 2; i++){
		// The vector to the light from the vertex    
		vec3 Lvec = LightPosition[i].xyz - pos.xyz;
		
		// Unit direction vectors for Blinn-Phong shading calculation
		vec3 L = normalize( Lvec );   // Direction to the light source
		vec3 H = normalize( L + E );  // Halfway vector
		
		//ADDED
		//reduce intensity with distance from light
		float dist = length(Lvec);
		float attenuation = 1.0f / dist / dist;
		
		float Kd = max( dot(L, N), 0.0 );
		vec3  diffuse = Kd * DiffuseProduct;

		float Ks = pow( max(dot(N, H), 0.0), Shininess );
		vec3  specular = Ks * SpecularProduct;

		color.rgb += diffuse * attenuation;
		specular.rgb += specular * attenuation;
	}
	
	color.a = 1.0;
	gl_Position = Projection * pos;
	texCoord = vTexCoord * texScale;
}
