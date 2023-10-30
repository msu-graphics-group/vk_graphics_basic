#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D colorTex;

layout(location = 0) in VS_OUT {
    vec2 texCoord;
} surf;

const int arraySize = 9;

vec4 findMedian(vec4 arr[arraySize], int max_size) {
    for (int c = 0; c < 4; c++) {
        for (int i = 0; i < max_size; i++) {
            for (int j = i + 1; j < max_size; j++) {
                if (arr[i][c] > arr[j][c]) {
                    vec4 tmp = arr[j];
                    arr[j] = arr[i];
                    arr[i] = tmp;
                }
            }
        }
    }
    return arr[int(max_size / 2)];
}

void main() {
    ivec2 textureSize2d = textureSize(colorTex, 0);

    vec4 arr[arraySize];
    int el = 0;
    int x_coord = int(surf.texCoord.x * textureSize2d.x);
    int y_coord = int(surf.texCoord.y * textureSize2d.y);

    for (int i = -1; i <= 1; i++) {
        int x = clamp(x_coord + i, 0, textureSize2d.x - 1);
        for (int j = -1; j <= 1; j++) {
            int y = clamp(y_coord + j, 0, textureSize2d.y - 1);
            arr[el] = textureLod(colorTex, surf.texCoord + vec2(i, j) / textureSize2d, 0);
            el++;
        }
    }

    color = findMedian(arr, el);
}
