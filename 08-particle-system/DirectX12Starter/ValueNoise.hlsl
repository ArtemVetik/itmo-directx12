#ifndef VALUE_NOISE
#define VALUE_NOISE

float valueNoise(int3 p)
{
    int3 pt = p * 1103515245 + 12345;
    return frac(sin(dot(pt, float3(12.9898, 78.233, 45.164))) * 43758.5453);
}

float smoothValueNoise(float3 p)
{
    int3 ip = int3(floor(p));
    float3 fp = frac(p);

    float v000 = valueNoise(ip + int3(0, 0, 0));
    float v001 = valueNoise(ip + int3(0, 0, 1));
    float v010 = valueNoise(ip + int3(0, 1, 0));
    float v011 = valueNoise(ip + int3(0, 1, 1));
    float v100 = valueNoise(ip + int3(1, 0, 0));
    float v101 = valueNoise(ip + int3(1, 0, 1));
    float v110 = valueNoise(ip + int3(1, 1, 0));
    float v111 = valueNoise(ip + int3(1, 1, 1));

    float3 smooth = fp * fp * (3.0 - 2.0 * fp);

    return lerp(lerp(lerp(v000, v100, smooth.x), lerp(v010, v110, smooth.x), smooth.y),
                lerp(lerp(v001, v101, smooth.x), lerp(v011, v111, smooth.x), smooth.y), smooth.z);
}

#endif