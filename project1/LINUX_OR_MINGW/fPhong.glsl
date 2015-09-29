#version 150

in vec3 fPositionMV;
in vec3 fNormalMV;
in vec2 fTexCoord;

out vec4 color;

//light
//viewspace
uniform vec4 LightPosition[2];

//material
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;

uniform sampler2D texture;

void main()
{
	vec3 ambient = AmbientProduct + vec3(0.1, 0.1, 0.1);
	vec3 diffuse = vec3(0, 0, 0)
	vec3 specular = vec3(0, 0, 0);
	
	// Unit direction vectors for Phong shading calculation
	vec3 N = normalize(fNormalMV);
	vec3 E = normalize( -fPositionMV ); // Direction to the eye/camera
	
	for(int i = 0; i < 2 ; i++){
		// The vector to the lights from the vertex    
		vec3 Lvec = LightPosition[i].xyz - fPositionMV;
		
		// Unit direction vectors for Phong shading calculation
		vec3 L = normalize( Lvec );   		// Direction to the light source
		vec3 R = reflect(-L, N);				//Perfect reflector

		//reduce intensity with distance from light
		float dist = length(Lvec);
		float attenuation = 1.0f / dist / dist;
		
		float Kd = max( dot(L, N), 0.0 ) * attenuation;
		diffuse += Kd * DiffuseProduct;

		float Ks = pow( max(dot(R, E), 0.0), Shininess ) * attenuation;
		specular += Ks * SpecularProduct;		
	}
	color = texture2D(texture, fTexCoord) * vec4((ambient + diffuse), 1) + vec4(specular, 1);
}
