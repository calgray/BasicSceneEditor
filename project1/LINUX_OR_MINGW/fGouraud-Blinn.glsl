#version 150

in  vec4 color;
in 	vec4 specular;
in  vec2 texCoord;  // The third coordinate is always 0.0 and is discarded

out vec4 fColor;

uniform float texScale;
uniform sampler2D texture;

void main()
{
  fColor = color * texture2D( texture, texCoord * texScale ) + specular;
}
