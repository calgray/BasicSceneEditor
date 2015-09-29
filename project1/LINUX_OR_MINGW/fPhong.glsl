#version 150

in vec3 fPositionMV;
in vec3 fNormalMV;
in vec2 fTexCoord;

out vec4 color;

//light
//viewspace
uniform vec4 LightPosition;

//material
uniform vec3 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;

uniform sampler2D texture;

void main()
{
	// The vector to the light from the vertex    
    vec3 Lvec = LightPosition.xyz - fPositionMV;
	
    // Unit direction vectors for Phong shading calculation
	vec3 N = normalize(fNormalMV);
    vec3 L = normalize( Lvec );   		// Direction to the light source
    vec3 E = normalize( -fPositionMV ); // Direction to the eye/camera
    vec3 R = reflect(-L, N);				//Perfect reflector


    // Compute terms in the illumination equation
    vec3 ambient = AmbientProduct;// + vec3(0.1, 0.1, 0.1);

	//reduce intensity with distance from light
	float dist = length(Lvec);
	float attenuation = 1.0f / dist / dist / dist;
	
    float Kd = max( dot(L, N), 0.0 ) * attenuation;
    vec3  diffuse = Kd * DiffuseProduct * 100;

    float Ks = pow( max(dot(R, E), 0.0), Shininess ) * attenuation;
    vec3  specular = Ks * SpecularProduct;
	
	//if( dot(L, N) < 0.0 ) {
    //  specular = vec3(0.0, 0.0, 0.0);
    //} 
	
	color = texture2D(texture, fTexCoord) * vec4((ambient + diffuse), 1) + vec4(specular, 1);
}
