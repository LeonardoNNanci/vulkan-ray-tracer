float uniformDiskPolarPDF(vec2 w){
    return 1. / PI;
}

vec2 sampleUniformDiskPolar(vec2 xis){
    float xi1 = xis.x;
    float xi2 = xis.y;

    float r = sqrt(xi1);
    float theta = 2. * PI * xi2;

    float x = r * cos(theta);
    float y = r * sin(theta);

    return vec2(x, y);
}

vec2 invertUniformDiskPolarSample(vec2 w){
    // not implemented, dummy return
    return vec2(1.);
}