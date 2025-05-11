#version 330 core

in vec3 normals;
in vec2 coordinates;
in vec3 fragPosition;

uniform sampler2D materDiffuse;
uniform sampler2D materSpecular;
uniform float materShininess;
uniform vec3 lightDiffuse;
uniform vec3 lightAmbient;
uniform vec3 lightSpecular;
uniform vec3 lightPosition;
uniform float lightConstant;
uniform float lightLinear;
uniform float lightQuadratic;
uniform vec3 viewPosition;
uniform vec3 viewDirection;
uniform bool spotToggle;
uniform float spotCutOff;
uniform float spotOuterCutOff;

layout (location=0) out vec4 fragColor;
layout (location=1) out vec4 brightColor;

void main() {
    /* ambient */
    vec3 ambient = lightAmbient * vec3(texture(materDiffuse, coordinates));

    /* diffuse */
    vec3 norm = normalize(normals);
    vec3 lightDirection = normalize(lightPosition - fragPosition);
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = lightDiffuse * diff * vec3(texture(materDiffuse, coordinates));

    /* specular */
    vec3 fragDirection = normalize(viewPosition - fragPosition);
    vec3 reflectDirection = reflect(-lightDirection, norm);
    float spec = pow(max(dot(fragDirection, reflectDirection), 0.0), materShininess);
    vec3 specular = lightSpecular * spec * vec3(texture(materSpecular, coordinates).rrr);

    /* spot light */
    vec3 spotDiffuse = vec3(0.0);
    vec3 spotSpecular = vec3(0.0);
    if (spotToggle) {
        diff = max(dot(norm, fragDirection), 0.0);
        spotDiffuse = lightSpecular * diff * vec3(texture(materDiffuse, coordinates));

        reflectDirection = reflect(-fragDirection, norm);
        spec = pow(max(dot(fragDirection, reflectDirection), 0.0), materShininess);
        spotSpecular = lightSpecular * spec * vec3(texture(materSpecular, coordinates).rrr);

        float theta = dot(fragDirection, normalize(-viewDirection));
        float epsilon = spotCutOff - spotOuterCutOff;
        float intensity = clamp((theta - spotOuterCutOff) / epsilon, 0.0, 1.0);
        spotDiffuse *= intensity;
        spotSpecular *= intensity;
    }

    /* attenuation */
    float sourceDistance = length(lightPosition - fragPosition);
    float attenuation = 1.0 / (lightConstant + sourceDistance * lightLinear + pow(sourceDistance, 2) * lightQuadratic);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    spotDiffuse *= attenuation;
    spotSpecular *= attenuation;

    /* output */
    fragColor = vec4(ambient + diffuse + specular + spotDiffuse + spotSpecular, 1.0);

    /* highlights */
    float brightness = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (0.8 < brightness)
        brightColor = fragColor;
    else
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);
}