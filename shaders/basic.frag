#version 460 core
#define MAX_HEADL_AMOUNT 20

out vec4 FragColor;

in vec3 vPos;
in vec3 wPos;
in vec4 wCoordLS;
in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vColor;
in vec4 vProjTexCoord[10];
in vec4 wCoordHeadLS[MAX_HEADL_AMOUNT];

uniform mat4 uModel;
uniform vec3 uColor;
uniform sampler2D uTexture;
uniform sampler2D uTest;

uniform int uSunShadowsEnable;
uniform int uCarHeadlightEnable;

uniform vec3 uSunDirection;
uniform sampler2D uShadowMap;
uniform vec2 uShadowMapSize;
uniform float uBias;

#define MAX_LAMPS_AMOUNT 18
uniform int uLampsAmount;
uniform vec3 LampTest;
uniform vec3 uLampLights[MAX_LAMPS_AMOUNT];

uniform vec3 uLampLigthColor;
uniform float uLampC1; // Costante
uniform float uLampC2; // Lineare
uniform float uLampC3; // Quadratico

// texture per projective texture
uniform sampler2D uHeadlightsTexture;
uniform vec3 uProjectorPos[10];
uniform vec3 uProjectorDir[10];

uniform int uHeadlightAmount;
uniform vec3 uHeadlights[MAX_HEADL_AMOUNT];

uniform int uHeadlightNAmount;
uniform vec3 uHeadlightN[MAX_HEADL_AMOUNT];

// Direzione e posizione dei fari
uniform vec3 uHeadlightPos;
uniform vec3 uHeadlightDir;
uniform vec3 uHeadlightColor;

// Attenuazione e cutoff
uniform float uHeadlightC1, uHeadlightC2, uHeadlightC3;
uniform float uHeadlightCutOff, uHeadlightOuterCutOff;

//Spotlight shadows
uniform sampler2D uHeadShadowMap[MAX_HEADL_AMOUNT] ;
uniform vec2 uHeadMapSize;

vec3 LambertDiffuse(vec3 L, vec3 N)
{
    float LN = max(0, dot(L, N));
    vec3 textureColor = texture(uTexture, vTexCoord).rgb;
    return uColor * textureColor * LN;
}

vec3 CalcPointLight(vec3 lightPos, vec3 N)
{
    vec3 lightDir = normalize(lightPos - wPos);
    vec3 diffuse = LambertDiffuse(lightDir, N);

    float distance = length(lightPos - wPos);
    float attenuation = min(1.0 / (uLampC1 + (uLampC2 * distance) + (uLampC3 * distance * distance)), 1.0);

    return uLampLigthColor * diffuse * attenuation;
}

float ShadowCalculation(vec4 CoordLS)
{
    if(uSunShadowsEnable == 0)
        return 1;

    float lit = 1.0;
    vec3 projCoords = wCoordLS.xyz / wCoordLS.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(uShadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // Calcolo della bias dell'ombra
    float bias = clamp(uBias * tan(acos(dot(normalize(vNormal), -uSunDirection))), uBias, 0.05);
    for (float x = -1.5; x <= 1.5; x += 1.0)
        for (float y = -1.5; y <= 1.5; y += 1.0)
        {
            float storedDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) / uShadowMapSize).r;
            if (storedDepth + bias < currentDepth)
                lit -= 1.0 / 16.0;
        }
    return lit;
}


float SpotShadowCalculation(int spotId, vec4 CoordLS)
{
    if(uSunShadowsEnable == 0)
        return 1;

    float lit = 0.0;

    //Proietto le coordinate in lightSpace sullo schermo per ottenere la posizione del frammento nella shadowMap
    vec3 projCoords = CoordLS.xyz / CoordLS.w;
    projCoords = projCoords * 0.5 + 0.5; //Converto da [-1, 1] (range di coord. del clip space) a [0, 1] (range di coordinate texture)

    float currentDepth = projCoords.z;

    int sampleRadius = 2;
    vec2 pixelSize = 1.0/textureSize(uTest, 0);

    vec3 lightDirection = normalize(uHeadlights[spotId] - wPos);
    float bias = max(0.00025f * (1.0f - dot(normalize(vNormal), lightDirection)), 0.000005f);

    for(int y = -sampleRadius; y <= sampleRadius; y++)
    {
        for(int x = -sampleRadius; x <= sampleRadius; x++)
        {
            float closestDepth = texture(uHeadShadowMap[spotId], projCoords.xy + vec2(x, y) * pixelSize).r;
            if(currentDepth > closestDepth + bias)
            lit += 1.0f;
        }
    }

    lit /= pow((sampleRadius * 2 + 1), 2); //Con il Percentage Closest Filtering vogliamo fare la media dei campionamenti effettauti. I campionamenti sono sampleRadius * sampleRadius.
    return  1.0 - lit;
}

vec3 CalcSpotLight(vec3 lightPos, vec3 lightDir, vec3 lightColor, vec3 N)
{
    float outerCutoff = uHeadlightOuterCutOff;
    float cutoff = uHeadlightCutOff;

    float c1 = uHeadlightC1;
    float c2 = uHeadlightC2;
    float c3 = uHeadlightC3;

    vec3 lightVec = lightPos - wPos;
    vec3 L = normalize(lightVec);

    float theta = dot(L, normalize(-lightDir));
    if (theta > outerCutoff)
    {
        float epsilon = cutoff - outerCutoff;

        float intensity = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);

        float distance = length(lightVec);
        float attenuation = 1.0 / (c1 + c2 * distance + c3 * (distance * distance));

        return lightColor * intensity * attenuation;
    }
    return vec3(0.0); // Fuori dal cono, niente luce
}

vec3 CalcSpotProj(int spotId, vec4 CoordLS)
{    //Proietto le coordinate in lightSpace sullo schermo in seguito le uso per campionare la texture
    vec2 projCoords = (CoordLS / CoordLS.w).xy * 0.5+0.5;
    vec4 c = texture2D(uHeadlightsTexture, projCoords);

    vec3 lightDirection = normalize(uHeadlights[spotId] - wPos);
    //Se la faccia del modello non e' rivolta verso la luce non deve ricevere la proiezione della texture
    vec3 cc =  vec3(max(0.0,dot(normalize(lightDirection),normalize(vNormal)))) + c.xyz*c.w;

    return cc;
}

// Funzione per calcolare l'intensità dei fari
float CalculateHeadlightIntensity(vec4 projCoord, vec3 wPos, vec3 lightPos, vec3 lightDir, sampler2D uHeadlightsTexture, float maxDistance) 
{
    if(uCarHeadlightEnable == 0)
        return 0;

    projCoord = projCoord / projCoord.w; 
    projCoord = projCoord * 0.5 + 0.5; 

    if (projCoord.x < 0.0 || projCoord.x > 1.0 || projCoord.y < 0.0 || projCoord.y > 1.0) 
        return 0.0;

    vec3 lightVec = wPos - lightPos;
    float distToLight = length(lightVec);
    if (distToLight >= maxDistance || dot(normalize(lightVec), lightDir) <= 0.0) 
        return 0.0;

    vec2 uv = clamp(projCoord.xy, 0.0, 1.0);
    float headlightsIntensity = texture(uHeadlightsTexture, uv).r;

    float dist = distance(uv, vec2(0.5, 0.1));
    
    float fadeFactor = 1.0 - smoothstep(0.1, 0.3, dist); // Dissolvenza meno brusca

    return fadeFactor * headlightsIntensity;
}

void main(void)
{
    vec3 color = texture(uTexture, vTexCoord).rgb * uColor;

    // Calcolo della luce solare con ombra
    //float shadow = 0;ShadowCalculation(wCoordLS);

    // Calcola luce dei lampioni
    vec3 result = LambertDiffuse(uSunDirection, normalize(vNormal));
    for (int i = 0; i < uLampsAmount; i++)
        result += color * CalcPointLight(uLampLights[i], normalize(vNormal));

    /*
    float maxDistance = 0.3;
    // Calcolo dell'intensità dei fari nel ciclo
    for (int i = 0; i < 10; i++) {
        float intensity = CalculateHeadlightIntensity(vProjTexCoord[i], wPos, uProjectorPos[i], uProjectorDir[i], uHeadlightsTexture, maxDistance) * SpotShadowCalculation(i, uHeadLightMatrix[i] * vec4(wPos, 1));
        result += intensity * 1;
    }
    */

    // Calcola luce dei fari (spotlights)
    for (int i = 0; i < uHeadlightAmount; i++)
    {
        result += color * CalcSpotProj(i, wCoordHeadLS[i]) * CalcSpotLight(uHeadlights[i], uHeadlightN[i], uHeadlightColor, normalize(vNormal)) * SpotShadowCalculation(i, wCoordHeadLS[i]);
    }

    FragColor = vec4(result, 1.0);
}
