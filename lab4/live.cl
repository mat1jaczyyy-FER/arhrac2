__kernel void oneDconv(__global int* v, __global int* k, __global int* o, int len)
{
    int i = get_global_id(0);
    int y = 0;
    if (i > 0) y += v[i - 1] * k[2];
    y += v[i] * k[1];
    if (i < len - 1) y += v[i + 1] * k[0];
    o[i] = y;
}
