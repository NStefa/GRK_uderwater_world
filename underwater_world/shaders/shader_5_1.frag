#version 430 core

uniform vec3 color;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 cameraPos

in vec3 interpNormal;
in vec3 worldPos;

out vec4 outColor;
void main()
{
	vec3 normal = normalize(interpNormal);
	vec3 V = normalize(cameraPos - worldPos);

	float diffuse = dot(normal, -lightDir);
	diffuse = max(diffuse, 0.0);

	vec3 R = reflect(lightDir, normal);
	float specular = dot(V, R);
    specular = max(specular, 0.0);
	specular = pow(specular, 8.0);

	vec3 V = normalize(cameraPos - worldPos);
	float ambient = 0.2

	outColor = vec4(color * lightColor * diffuse + lightColor * specular, 1.0);

}
