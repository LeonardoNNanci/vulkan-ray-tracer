float uniformSpherePDF(vec3 w){
    return 1 / (4 * PI);
}

vec3 sampleUniformSphere(vec2 u){
    float xi1 = u.x;
    float xi2 = u.y;

    float z = 1 - 2 * xi1;
    float r = sqrt(1 - (z * z));
    float phi = 2 * PI * xi2;

    float x = r * cos(phi);
    float y = r * sin(phi);

    return vec3(x, y, z);
}

vec2 invertUniformSphereSample(vec3 w){
    // TODO
    return vec2(0.);
}