#version 150

const int MAX_LIGHTS = 3;

in vec3 fPositionMV;
in vec3 fNormalMV;
in vec2 fTexCoord;

out vec4 color;

//camera
uniform vec4 Origin; //faster than multiplying an inversetranspose

//light * material (viewspace)
uniform int LightType[MAX_LIGHTS];
uniform vec4 LightPosition[MAX_LIGHTS];
uniform vec4 LightDirection[MAX_LIGHTS];



uniform vec3 AmbientProduct;
uniform vec3 DiffuseProduct[MAX_LIGHTS], SpecularProduct[MAX_LIGHTS];

//material
uniform float Shininess;

uniform sampler2D texture;

void main()
{
	// Compute terms in the illumination equation
    vec3 ambient = AmbientProduct;
	vec3 diffuse = vec3(0, 0, 0);
	vec3 specular = vec3(0, 0, 0);
	
	vec3 N = normalize(fNormalMV);
	vec3 E = normalize(-fPositionMV);	// Direction to the eye/camera
	
	for(int i = 0; i < MAX_LIGHTS; i++) {
		 
		 //Directional Light
        if(LightType[i] == 0) {
            //vec3 Lvec = (LightPosition[i] - Origin).xyz;	// The vector to the light from the vertex   
			vec3 Lvec = LightDirection[i].xyz;
			
			vec3 L = normalize(Lvec);           // Direction to the light source
			vec3 H = normalize( L + E );        // Halfway vector

			float Kd = max( dot(L, N), 0.0 );
			diffuse += Kd * DiffuseProduct[i];

			float Ks = pow( max(dot(N, H), 0.0), Shininess * 3);
			specular += Ks * SpecularProduct[i];
		
        }
		//Point Light
        else if(LightType[i] == 1) {
			// The vector to the light from the vertex   
			vec3 Lvec = LightPosition[i].xyz - fPositionMV;
		
			vec3 L = normalize(Lvec);           // Direction to the light source
			vec3 H = normalize( L + E );        // Halfway vector
	
			//reduce intensity with distance from light
			float dist = length(Lvec) + 1.0f;
			float attenuation = 1.0f / dist / dist;
			
			float Kd = max( dot(L, N), 0.0 ) * attenuation;
			diffuse += Kd * DiffuseProduct[i];
			
			float Ks = pow( max(dot(N, H), 0.0), Shininess * 3 ) * attenuation;
			specular += Ks * SpecularProduct[i];
        }
		//Spot Light
        else if(LightType[i] == 2) {
			// The vector to the light from the vertex   
			vec3 Lvec = LightPosition[i].xyz - fPositionMV;
			
			//angle between the spotlight centre and the fragment being shaded.
			float coneAngle = acos(dot(-Lvec, normalize(LightDirection[i].xyz)));
		
			vec3 L = normalize(Lvec);           // Direction to the light source
			vec3 H = normalize( L + E );        // Halfway vector
	
			//reduce intensity with distance from light and increasing angle
			float dist = length(Lvec) + 1.0f;
			float attenuation;
			if(coneAngle > 0.5) attenuation = 0.0f ;
			else attenuation = 1.0f / dist / dist * atan(2 * tan(1.0f) * coneAngle);
			
			float Kd = max( dot(L, N), 0.0 ) * attenuation;
			diffuse += Kd * DiffuseProduct[i];
			
			float Ks = pow( max(dot(N, H), 0.0), Shininess * 3 ) * attenuation;
			specular += Ks * SpecularProduct[i];
        }
	}
	
	color = texture2D(texture, fTexCoord) * vec4((ambient + diffuse), 1) + vec4(specular, 0);
}