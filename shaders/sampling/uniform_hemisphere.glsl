float uniformHemispherePDF(vec3 w){
    return 1/(2 * PI);
}

vec3 sampleUniformHemisphere(vec2 xis){
    float xi1 = xis.x;
    float xi2 = xis.y;

    float r = sqrt(1 - (xi1 * xi1));
    float phi = 2 * PI * xi2;

    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = xi1;

    return vec3(x, y, z);
}

vec2 invertUniformHemisphereSample(vec3 w){
    float phi = atan(w.y, w.x);
    if(phi < 0)
        phi += 2 * PI;

    float xi1 = w.z;
    float xi2 = phi / (2 * PI);

    return vec2(xi1, xi2);
}
