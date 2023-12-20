#version 330 core
uniform lowp vec4 uniformColor;
out vec4 FragColor;
void main()
{
   gl_FragColor = uniformColor;
}
