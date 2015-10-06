#version 150

in  vec4 fColor;
in  vec4 fSpecular;
in  vec2 texCoord;

out vec4 color;

uniform float texScale;
uniform sampler2D texture;

void main()
{
  color = fColor * texture2D( texture, texCoord) + fSpecular;
}
