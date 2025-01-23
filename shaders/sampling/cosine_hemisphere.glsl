#include "uniform_disk.glsl"

float cosineHemispherePDF(float cosTheta){
    return cosTheta / PI;
}

vec3 sampleCosineHemisphere(vec2 xis){
    vec2 d = sampleUniformDiskPolar(xis);
    
    float x = d.x;
    float y = d.y;
    float z = sqrt(1 - (x * x) - (y * y));

    return vec3(x, y, z);
}

vec2 invertCosineHemishereSample(vec3 w){
    return invertUniformDiskPolarSample(w.xy);
}