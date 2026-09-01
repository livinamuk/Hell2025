
vec3 Tonemap_ACES(const vec3 x) { // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

int Modi(int x, int y) {
    return x - y * (x / y);
}

int And(int a, int b) {
    int result = 0;
    int n = 1;
	const int BIT_COUNT = 32;
    for(int i = 0; i < BIT_COUNT; i++) {
        if ((Modi(a, 2) == 1) && (Modi(b, 2) == 1)) {
            result += n;
        }
        a >>= 1;
        b >>= 1;
        n <<= 1;
        if (!(a > 0 && b > 0))
            break;
    }
    return result;
}

vec4 Vibrance(vec4 inCol, float vibrance) {
 	vec4 outCol;
    if (vibrance <= 1.0)
    {
        float avg = dot(inCol.rgb, vec3(0.3, 0.6, 0.1));
        outCol.rgb = mix(vec3(avg), inCol.rgb, vibrance); 
    }
    else // vibrance > 1.0
    {
        float hue_a, a, f, p1, p2, p3, i, h, s, v, amt, _max, _min, dlt;
        float br1, br2, br3, br4, br5, br2_or_br1, br3_or_br1, br4_or_br1, br5_or_br1;
        int use;

        _min = min(min(inCol.r, inCol.g), inCol.b);
        _max = max(max(inCol.r, inCol.g), inCol.b);
        dlt = _max - _min + 0.00001 /*Hack to fix divide zero infinities*/;
        h = 0.0;
        v = _max;

		br1 = step(_max, 0.0);
        s = (dlt / _max) * (1.0 - br1);
        h = -1.0 * br1;

		br2 = 1.0 - step(_max - inCol.r, 0.0); 
        br2_or_br1 = max(br2, br1);
        h = ((inCol.g - inCol.b) / dlt) * (1.0 - br2_or_br1) + (h*br2_or_br1);

		br3 = 1.0 - step(_max - inCol.g, 0.0); 
        
        br3_or_br1 = max(br3, br1);
        h = (2.0 + (inCol.b - inCol.r) / dlt) * (1.0 - br3_or_br1) + (h*br3_or_br1);

        br4 = 1.0 - br2*br3;
        br4_or_br1 = max(br4, br1);
        h = (4.0 + (inCol.r - inCol.g) / dlt) * (1.0 - br4_or_br1) + (h*br4_or_br1);

        h = h*(1.0 - br1);

        hue_a = abs(h); // between h of -1 and 1 are skin tones
        a = dlt;      // Reducing enhancements on small rgb differences

        // Reduce the enhancements on skin tones.    
        a = step(1.0, hue_a) * a * (hue_a * 0.67 + 0.33) + step(hue_a, 1.0) * a;                                    
        a *= (vibrance - 1.0);
        s = (1.0 - a) * s + a * pow(s, 0.25);

        i = floor(h);
        f = h - i;

        p1 = v * (1.0 - s);
        p2 = v * (1.0 - (s * f));
        p3 = v * (1.0 - (s * (1.0 - f)));

        inCol.rgb = vec3(0.0); 
        i += 6.0;
        //use = 1 << ((int)i % 6);
        use = int(pow(2.0,mod(i,6.0)));
        a = float(And(use , 1)); // i == 0;
        use >>= 1;
        inCol.rgb += a * vec3(v, p3, p1);
 
        a = float(And(use , 1)); // i == 1;
        use >>= 1;
        inCol.rgb += a * vec3(p2, v, p1); 

        a = float( And(use,1)); // i == 2;
        use >>= 1;
        inCol.rgb += a * vec3(p1, v, p3);

        a = float(And(use, 1)); // i == 3;
        use >>= 1;
        inCol.rgb += a * vec3(p1, p2, v);

        a = float(And(use, 1)); // i == 4;
        use >>= 1;
        inCol.rgb += a * vec3(p3, p1, v);

        a = float(And(use, 1)); // i == 5;
        use >>= 1;
        inCol.rgb += a * vec3(v, p1, p2);

        outCol = inCol;
    }
    return outCol;
}

vec3 AdjustSaturation(vec3 color, float amount) {
    float gray = dot(color, vec3(0.299, 0.587, 0.114)); // Luminance formula
    return mix(vec3(gray), color, 1.0 + amount);
}

vec3 Saturate(vec3 rgb, float adjustment) {
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    vec3 intensity = vec3(dot(rgb, W));
    return mix(intensity, rgb, adjustment);
}

vec3 AdjustHue(vec3 color, float hueShift) {
    const mat3 toYIQ = mat3(
        0.299,  0.587,  0.114,
        0.595, -0.274, -0.321,
        0.211, -0.523,  0.311
    );
    const mat3 toRGB = mat3(
        1.0,  0.956,  0.621,
        1.0, -0.272, -0.647,
        1.0, -1.106,  1.703
    );

    vec3 yiq = toYIQ * color;
    float angle = radians(hueShift);
    float cosA = cos(angle);
    float sinA = sin(angle);

    mat3 hueRotation = mat3(
        1.0,      0.0,     0.0,
        0.0,   cosA,   -sinA,
        0.0,   sinA,    cosA
    );

    yiq = hueRotation * yiq;
    return toRGB * yiq;
}

vec3 AdjustLightness(vec3 color, float lightness) {
    float factor = 1.0 + lightness;
    return clamp(color * factor, 0.0, 1.0);
}

vec3 FilmPixel(vec2 uv, float time) {
    mat2x3 uvs = mat2x3(uv.xxx, uv.yyy) + mat2x3(vec3(0, 0.1, 0.2), vec3(0, 0.3, 0.4));
    return fract(sin(uvs * vec2(12.9898, 78.233) * time) * 43758.5453);
}

vec3 GammaCorrect(vec3 color) {
    return pow(color, vec3(1.0/2.2));
}

vec3 AdjustContrast(vec3 color, float contrast) {
    return (color - 0.5) * contrast + 0.5;
}

// Overload for a single channel
float AdjustContrast(float c, float contrast) {
    return (c - 0.5) * contrast + 0.5;
}